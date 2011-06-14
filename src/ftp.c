/* 
   sitecopy, for managing remote web sites. Generic(ish) FTP routines.
   Copyright (C) 1998-2005, Joe Orton <joe@manyfish.co.uk>
                                                                     
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  

*/

/* This contains an FTP client implementation.
 * It performs transparent connection management - it it dies,
 * it will be reconnected automagically.
 */

#include <config.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#ifdef HAVE_LIMITS_H
#include <limits.h> 	/* for PATH_MAX */
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif 
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <assert.h>

#include <ne_alloc.h>
#include <ne_string.h>
#include <ne_socket.h>

#include "common.h"
#include "frontend.h"
#include "ftp.h"
#include "protocol.h"
#include "i18n.h"
#include "lsparser.h"

struct ftp_session_s {
    /* User options */
    unsigned int use_passive;
    unsigned int echo_quit;
    unsigned int use_cwd; /* CWD before STOR */

    int connected; /* true when open */

    /* Userdata passed to fe_authenticate call */
    void *feauth_userdata;
    
    const char *hostname; /* server hostname for fe_login */

    /* DTP connection: */
    ne_socket *dtpsock;
    unsigned short dtp_port;
    ne_inet_addr *dtp_addr;
    
    /* PI connection: */
    ne_sock_addr *pi_addr;
    unsigned short pi_port;
    ne_socket *pisock;
    const ne_inet_addr *pi_curaddr; /* currently used iaddr */

    /* Current file transfor mode. */
    enum tran_mode {
	tran_unknown, /* whatever the server picked */
	tran_binary,
	tran_ascii
    } mode;

    enum rfc2428_mode {
        rfc2428_unknown = 0, /* RFC24248 support unknown */
        rfc2428_ok, /* RFC2428 is supported */
        rfc2428_bad  /* RFC2428 is not supported */
    } rfc2428;
	
#ifndef PATH_MAX
#define PATH_MAX 2048
#endif
    /* Stores the current working dir on the remote server */
    char cwd[PATH_MAX];

    /* time from MDTM response... bit crap having this here. */
    time_t get_modtime;
    
    /* remember these... we may have to log in more than once. */
    char username[FE_LBUFSIZ], password[FE_LBUFSIZ];

    unsigned int echo_response:1;

    /* Reply buffer */
    char rbuf[BUFSIZ];

    /* Error string */
    char error[BUFSIZ];
};

#define FTP_ERR(x) do { \
int _ftp_err = (x); if (_ftp_err != FTP_OK) return _ftp_err; } while (0)

/* Sets error string */
static void ftp_seterror(ftp_session *sess, const char *error);

/* Opens the data connection */
static int ftp_data_open(ftp_session *sess, const char *command, ...) 
    ne_attribute((format (printf, 2, 3)));

static int get_modtime(ftp_session *sess, const char *root,
		       const char *filename);

/* Sets session error to system errno value 'errnum'. */
static void set_syserr(ftp_session *sess, const char *error, int errnum)
{
    ne_snprintf(sess->error, sizeof sess->error, "%s: %s", error, 
		strerror(errnum));
    NE_DEBUG(DEBUG_FTP, "FTP Error set: %s\n", sess->error);
}

/* Sets the error string using the error from the given socket. */
static void set_sockerr(ftp_session *sess, const ne_socket *sock, 
			const char *doing, ssize_t errnum)
{
    switch (errnum) {
    case NE_SOCK_CLOSED:
	ne_snprintf(sess->error, BUFSIZ, 
		 _("%s: connection was closed by server."), doing);
	break;
    case NE_SOCK_TIMEOUT:
	ne_snprintf(sess->error, BUFSIZ, 
		    _("%s: connection timed out."), doing);
	break;
    default:
	ne_snprintf(sess->error, BUFSIZ, "%s: %s", doing, ne_sock_error(sock));
	break;
    }
    NE_DEBUG(DEBUG_FTP, "ftp: Set socket error (%" NE_FMT_SSIZE_T "): %s\n",
             errnum, sess->error);
}

/* set_pisockerr must be called to handle any PI socket error to
 * ensure that the connection can be correctly re-opened later.  Pass
 * DOING as the operation which failed, ERRNUM as the error from the
 * ne_sock_* layer. */
static void set_pisockerr(ftp_session *sess, const char *doing, ssize_t errnum)
{
    set_sockerr(sess, sess->pisock, doing, errnum);
    NE_DEBUG(DEBUG_FTP, "PI socket closed: %s\n", sess->error);
    ne_sock_close(sess->pisock);
    sess->pisock = NULL;
    sess->connected = 0;
}

#define FTP_PI_BROKEN(r) \
((r) == NE_SOCK_RESET || (r) == NE_SOCK_CLOSED || (r) == NE_SOCK_TIMEOUT)

/* Read the reply to an FTP command on the PI connection using given
 * buffer.  Returns FTP_BROKEN if the PI connection has been broken
 * since the last command was executed. (due to a timeout etc) */
static int read_reply(ftp_session *sess, int *code, char *buf, size_t bufsiz)
{
    int multiline = 0;

    *code = 0;

    do {
        ssize_t ret = ne_sock_readline(sess->pisock, buf, bufsiz-1);
	if (ret < 0) {
            set_pisockerr(sess, _("Could not read response line"), ret);
            return FTP_PI_BROKEN(ret) ? FTP_BROKEN : FTP_ERROR;
	}
	
        buf[ret] = '\0';
	NE_DEBUG(DEBUG_FTP, "< %s", buf);
        
        if (ret > 4 && isdigit(buf[0]) && isdigit(buf[1]) && isdigit(buf[2])) {
            *code = atoi(buf);
            if (multiline == 0 && buf[3] == '-') {
                /* Begin multiline response. */
                multiline = *code;
            } else if (multiline == *code && buf[3] == ' ') {
                /* End multiline response. */
                multiline = 0;
            }
        }

    } while (multiline);

    return FTP_OK;
}

/* Parses the 213 response to a MDTM command... on success, returns
 * FTP_MODTIME and sets modtime to the time in the response.  On
 * failute, returns FTP_ERROR. */
static int parse_modtime(ftp_session *sess, char *response, time_t *modtime) 
{
    struct tm t = {0};

    ne_shave(response, "\r\n");
    NE_DEBUG(DEBUG_FTP, "Parsing modtime: %s\n", response);

    if (strlen(response) != 18) {
        ftp_seterror(sess, _("Cannot parse MDTM response; wrong length."));
	return FTP_ERROR;
    }

    if (sscanf(response, "213 %4d%2d%2d" "%2d%2d%2d",
               &t.tm_year, &t.tm_mon, &t.tm_mday,
               &t.tm_hour, &t.tm_min, &t.tm_sec) < 6) {
        ftp_seterror(sess, _("Cannot parse MDTM response."));
	return FTP_ERROR;
    }

    t.tm_year -= 1900; /* years since 1900 */
    t.tm_mon -= 1; /* zero-based */
    t.tm_isdst = -1;

    *modtime = mktime(&t);

    NE_DEBUG(DEBUG_FTP, "Converted to: %s", ctime(modtime));
    return FTP_MODTIME;
}

/* Parses the response to a PASV command.  Sets DTP port and address
 * in session structure. Returns FTP_ERROR on failure. */
static int parse_pasv_reply(ftp_session *sess, char *reply)
{
    unsigned int h1, h2, h3, h4, p1, p2;
    unsigned char addr[4];
    char *start;

    start = strchr(reply, ' ');
    if (start == NULL) {
        ftp_seterror(sess, _("Could not find address in PASV response"));
	return FTP_ERROR;
    }
    while (! isdigit(*start) && (*start != '\0'))
	start++;
    /* get the host + port */
    if (sscanf(start, "%u,%u,%u,%u,%u,%u", &h1, &h2, &h3, &h4, &p1, &p2) < 6) {
        ftp_seterror(sess, _("Could not parse PASV response"));
	return FTP_ERROR;
    }
    addr[0] = h1 & 0xff; addr[1] = h2 & 0xff;
    addr[2] = h3 & 0xff; addr[3] = h4 & 0xff;
    /* Record this for future reference */
    sess->dtp_port = (p1<<8) | p2;
    sess->dtp_addr = ne_iaddr_make(ne_iaddr_ipv4, addr);
    if (sess->dtp_addr == NULL) {
        ftp_seterror(sess, _("Invalid IP address in PASV response"));
	return FTP_ERROR;
    }

    NE_DEBUG(DEBUG_FTP, "ftp: Parsed PASV response, using port %d\n", sess->dtp_port);

    return FTP_PASSIVE;
}

/* parse an EPSV reply */
static int parse_epasv_reply(ftp_session *sess, char *reply)
{
    char *ptr = strchr(reply, '('), *eptr;
    long port;

    /* reply syntax: "229 fFOO (|||port|)" where | can be any character
     * in ASCII from 33-126, obscurely. */
    if (ptr == NULL || ptr[1] != ptr[2] || ptr[1] != ptr[3]
        || (eptr = strchr(ptr + 4, ')')) == NULL
        || eptr == ptr + 4 || eptr[-1] != ptr[1]) {
        ftp_seterror(sess,_("Malformed EPSV response"));
        return FTP_ERROR;
    }

    eptr[-1] = '\0';

    port = strtol(ptr + 4, &eptr, 10);
    if (*eptr != '\0') {
        ftp_seterror(sess, _("Malformed port in EPSV response"));
        return FTP_ERROR;
    }

    sess->dtp_addr = (ne_inet_addr *)sess->pi_curaddr;
    sess->dtp_port = (unsigned short)port;
    
    NE_DEBUG(DEBUG_FTP, "ftp: Parsed EPSV response, using port %ld\n", port);

    return FTP_PASSIVE;
}

/* Parse a reply of given 'code' (which is pre-parsed). */
static int parse_reply(ftp_session *sess, int code, char *reply)
{
    /* Special case reply parsing: */
    switch (code) {
    case 200: /* misc OK codes */
    case 220:
    case 230:
    case 250: /* completed file action */
    case 253: /* delete successful */
    case 257: /* mkdir success */
	return FTP_OK;
    case 226: /* received file okay */
	return FTP_SENT;
    case 150: /* file transfer... ready for data */
    case 125:
	return FTP_READY;
    case 550: /* couldn't complete file action */
	return FTP_FILEBAD;
    case 331: /* got username, want password */
	return FTP_NEEDPASSWORD;
    case 350: /* file action pending further info - RNFR */
	return FTP_FILEMORE;
    case 221:
	/* succesful QUIT reseponse. */
	return FTP_CLOSED;
    case 421: /* service denied; PI connection closure */
        ne_sock_close(sess->pisock);
        sess->pisock = NULL;
        sess->connected = 0;
        return FTP_BROKEN;
    case 553: /* couldn't create directory */
	return FTP_ERROR;
    case 213: /* MDTM response, hopefully */
	return parse_modtime(sess, reply, &sess->get_modtime);
    case 227: /* PASV response, hopefully */
	return parse_pasv_reply(sess, reply); 
    case 229: /* EPSV response */
        return parse_epasv_reply(sess, reply);
    default:
        return FTP_ERROR;
    }
}

/* Runs FTP command 'cmd', and reads the response.  Returns FTP_BROKEN
 * if the PI connection broke due to timeout etc.  execute() should be
 * used instead of this function for normal FTP commands. */
static int run_command(ftp_session *sess, const char *cmd)
{
    char *line = ne_concat(cmd, "\r\n", NULL);
    int code, ret;

#ifdef NE_DEBUGGING
    if (strncmp(cmd, "PASS ", 4) == 0
        && (ne_debug_mask & NE_DBG_HTTPPLAIN) == 0) {
        NE_DEBUG(DEBUG_FTP, "> PASS ...\n");
    } else {
        NE_DEBUG(DEBUG_FTP, "> %s\n", cmd);
    }
#endif

    /* Send the command. */
    ret = ne_sock_fullwrite(sess->pisock, line, strlen(line));
    free(line);

    if (ret < 0) {
        set_pisockerr(sess, "Could not send command", ret);
        return FTP_PI_BROKEN(ret) ? FTP_BROKEN : FTP_ERROR;
    }
    
    /* Read the response. */
    ret = read_reply(sess, &code, sess->rbuf, sizeof sess->rbuf);
    if (ret != FTP_OK) return ret;

    /* Set reply as default error string. */
    ftp_seterror(sess, sess->rbuf);

    /* Parse the reply. */
    return parse_reply(sess, code, sess->rbuf);
}    

/* Wrapper for run_command: runs an FTP command using printf template
 * and arguments, handling PI connection timeouts as necessary. */
static int execute(ftp_session *sess, const char *template, ...) 
{
    va_list params;
    int tries = 0, ret;
    char buf[1024];

    va_start(params, template);
    ne_vsnprintf(buf, sizeof buf, template, params);
    va_end(params);

    do {
        ret = ftp_open(sess);
        if (ret != FTP_OK) return ret;

        ret = run_command(sess, buf);
    } while (ret == FTP_BROKEN && ++tries < 3);

    /* Don't let FTP_BROKEN get out */
    if (ret == FTP_BROKEN) 
	ret = FTP_ERROR;
    return ret;
}

/* Dump the given filename down the DTP socket, performing ASCII line
 * ending translation. Returns zero on success or non-zero on error
 * (in which case, session error string is set). */
static int send_file_ascii(ftp_session *sess, FILE *f, off_t fsize)
{
    char buffer[BUFSIZ];
    off_t total = 0, lasttotal = 0;

    while (fgets(buffer, BUFSIZ - 1, f) != NULL) {
        size_t buflen;
        char *pnt;
        int ret;

        pnt = strchr(buffer, '\r');
        if (pnt == NULL)
            pnt = strchr(buffer, '\n');
        if (pnt) {
            *pnt++ = '\r';
            *pnt++ = '\n';
            buflen = pnt - buffer;
        } else {
            /* line with no CRLF. */
            buflen = strlen(buffer);
        }
        
	ret = ne_sock_fullwrite(sess->dtpsock, buffer, buflen);
	if (ret) {
	    set_sockerr(sess, sess->dtpsock, _("Error sending file"), ret);
	    return -1;
	}
	total += (pnt - buffer) + 2;
	/* Only call progress every 4K otherwise it is way too 
	 * chatty. */
	if (total > lasttotal + 4096) {
	    lasttotal = total;
	    fe_transfer_progress(total, fsize);
	}
    }

    if (ferror(f)) {
	int errnum = errno;
	set_syserr(sess, _("Error reading file"), errnum);
	return -1;
    }

    return 0;
}

/* Send file 'f' (of size 'size') down DTP socket. */
static int send_file_binary(ftp_session *sess, FILE *f, off_t size)
{
    char buffer[BUFSIZ];
    size_t ret;
    off_t total = 0;
    
    while ((ret = fread(buffer, 1, sizeof buffer, f)) > 0) {
	int rv = ne_sock_fullwrite(sess->dtpsock, buffer, ret);
	if (rv) {
	    set_sockerr(sess, sess->dtpsock, _("Could not send file"), rv);
	    return -1;
	}
	total += ret;
	fe_transfer_progress(total, size);
    }

    if (ferror(f)) {
	int errnum = errno;
	set_syserr(sess, _("Error reading file"), errnum);
	return -1;
    }
    
    return 0;
}

/* Dump from given socket into given file.  Returns number of bytes
 * written on success, or -1 on error (session error string is
 * set). */
static int receive_file(ftp_session *sess, FILE *f)
{
    ssize_t bytes;
    off_t count = 0;
    char buffer[BUFSIZ];

    while ((bytes = ne_sock_read(sess->dtpsock, buffer, BUFSIZ)) > 0) {
	count += bytes;
	fe_transfer_progress(count, -1);
	if (fwrite(buffer, 1, bytes, f) < (size_t)bytes) {
	    int errnum = errno;
	    set_syserr(sess, _("Error writing to file"), errnum);
	    return -1;
	}
    }

    if (bytes < 0 && bytes != NE_SOCK_CLOSED) {
	set_sockerr(sess, sess->dtpsock, _("Receiving file"), bytes);
	return -1;
    }

    return 0;
}

/* Passively (client-connects) open the DTP socket ; return non-zero
 * on success. */
static int dtp_open_passive(ftp_session *sess) 
{
    int ret;
    sess->dtpsock = ne_sock_create();
    ret = ne_sock_connect(sess->dtpsock, sess->dtp_addr, sess->dtp_port);
    if (ret) {
	set_sockerr(sess, sess->dtpsock, 
                    _("Could not connect passive data socket"), ret);
        ne_sock_close(sess->dtpsock);
	return 0;
    } else {
	return 1;
    }
}

/* Initializes the driver + connection.
 * Returns FTP_OK on success, or FTP_LOOKUP if the hostname
 * could not be resolved.
 */
ftp_session *ftp_init(void)
{
    ftp_session *sess = ne_calloc(sizeof(ftp_session));
    sess->connected = 0;
    sess->mode = tran_unknown;
    return sess;
}

void ftp_set_passive(ftp_session *sess, int use_passive) 
{
    sess->use_passive = use_passive;
}

void ftp_set_usecwd(ftp_session *sess, int use_cwd) 
{
    sess->use_cwd = use_cwd;
}

int ftp_set_server(ftp_session *sess, struct site_host *server)
{
    if (server->username) {
	strcpy(sess->username, server->username);
    }
    if (server->password) {
	strcpy(sess->password, server->password);
    }
    sess->hostname = server->hostname;
    sess->pi_port = server->port;
    fe_connection(fe_namelookup, server->hostname);
    sess->pi_addr = ne_addr_resolve(server->hostname, 0);
    if (ne_addr_result(sess->pi_addr)) {
	char buf[256];
	ne_snprintf(sess->error, sizeof sess->error,
		    "Could not resolve server `%s': %s", server->hostname,
		    ne_addr_error(sess->pi_addr, buf, sizeof buf));
	return FTP_LOOKUP;
    }
    return FTP_OK;
}

/* Cleans up and closes the control connection.
 * Returns FTP_OK if the connection is closed, else FTP_ERROR;
 */
int ftp_finish(ftp_session *sess)
{
    int ret = FTP_OK;
    int old_er = sess->echo_response;
    if (sess->connected) {
	sess->echo_response = sess->echo_quit;
	if (run_command(sess, "QUIT") != FTP_CLOSED) {
	    ret = FTP_ERROR;
	}
        if (sess->pisock) ne_sock_close(sess->pisock);
        sess->connected = 0;
	sess->echo_response = old_er;
    }
    return ret;
}

/* Creates the given directory
 * FTP state response */
int ftp_mkdir(ftp_session *sess, const char *dir)
{
    return execute(sess, "MKD %s", dir);
}
 
/* Renames or moves a file */
int ftp_move(ftp_session *sess, const char *from, const char *to)
{
    if (execute(sess, "RNFR %s", from) == FTP_FILEMORE) {
	return execute(sess, "RNTO %s", to);
    }
    return FTP_ERROR;
}

int ftp_delete(ftp_session *sess, const char *filename)
{
    return execute(sess, "DELE %s", filename);
}

int ftp_rmdir(ftp_session *sess, const char *filename)
{
    return execute(sess, "RMD %s", filename);
}

/* Actively open the data connection, running command COMMAND; the
 * client then listens for and accepts the connection from the server.
 * On successful return, the DTP connection is open. */
static int dtp_open_active(ftp_session *sess, const char *command)
{
    char *a, *p;
    int ret;
    int listener;
    ksize_t alen;
    struct sockaddr_in addr;

    ret = ftp_open(sess);
    if (ret != FTP_OK) return ret;
    
    alen = sizeof(addr);
    if (getsockname(ne_sock_fd(sess->pisock), 
		    (struct sockaddr *)&addr, &alen) < 0) {
	int errnum = errno;
	set_syserr(sess, _("Active open failed: could not determine "
			   "address of control socket"), errnum);
        return FTP_ERROR;
    }

    /* Let the kernel choose a port */
    addr.sin_port = 0;
 
    /* Create a local socket to accept the connection on */
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
	int errnum = errno;
	set_syserr(sess, _("Active open failed: could not create socket"),
		   errnum);
	return FTP_ERROR;
    }

    /* Bind it to an address. */
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	int errnum = errno;
	set_syserr(sess, _("Active open failed: could not bind to address"),
		   errnum);
	(void) close(listener);
	return FTP_ERROR;
    }

    /* Retrieve the address again; determine which port was chosen. */
    alen = sizeof(addr);
    if (getsockname(listener, (struct sockaddr *)&addr, &alen) < 0) {
	int errnum = errno;
	set_syserr(sess, _("Active open failed: could not determine address "
			   "of data socket"), errnum);
	(void) close(listener);
	return FTP_ERROR;
    }

    if (addr.sin_port == 0) {
        ftp_seterror(sess, _("Could not determine bound port number for "
                             "data socket"));
        close(listener);
        return FTP_ERROR;
    }

    if (listen(listener, 1) < 0) {
	int errnum = errno;
	set_syserr(sess, ("Active open failed: could not listen for "
			  "connection"), errnum);
	(void) close(listener);
	return FTP_ERROR;
    }

#define	UC(b)	(((int)b)&0xff)
    a = (char *)&addr.sin_addr.s_addr;
    p = (char *)&addr.sin_port;

    /* Execute the PORT command */
    ret = execute(sess, "PORT %d,%d,%d,%d,%d,%d",
		    UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
		    UC(p[0]), UC(p[1]));
    if (ret != FTP_OK) {
	/* Failed to execute the PORT command - close the socket */
	NE_DEBUG(DEBUG_FTP, "PORT command failed.\n");
	close(listener);
	return ret;
    }

    /* Send the command.  This will make the remote end
     * initiate the connection.
     */
    ret = execute(sess, "%s", command);
    
    /* Do they want it? */
    if (ret != FTP_READY) {
	NE_DEBUG(DEBUG_FTP, "Command failed.\n");
    } else {
	/* Now wait for a connection from the remote end. */
	sess->dtpsock = ne_sock_create();
	if (ne_sock_accept(sess->dtpsock, listener)) {
	    int errnum = errno;
	    set_syserr(sess,
		       _("Active open failed: could not accept connection"),
		       errnum);
            ne_sock_close(sess->dtpsock);
	    ret = FTP_ERROR;
	}
    }

    (void) close(listener);
    return ret;
}

int ftp_chmod(ftp_session *sess, const char *filename, const mode_t mode)
{
    return execute(sess, "SITE CHMOD %03o %s", mode & 0777, filename);
}

/* default to always trying EPSV if using neon 0.24, which shouldn't
 * hurt too much since it falls back on PASV on failure anyway. */
#if NE_VERSION_MINOR == 24
#define ne_iaddr_typeof(ia) (ne_iaddr_ipv6)
#endif

/* Open the DATA connection using whichever means appropriate, running
 * FTP command COMMAND with printf-style arguments. */
static int ftp_data_open(ftp_session *sess, const char *command, ...) 
{
    int ret;
    va_list params;
    char buf[BUFSIZ];

    va_start(params, command);
    ne_vsnprintf(buf, BUFSIZ, command, params);
    va_end(params);

    if (sess->use_passive) {
        ret = FTP_ERROR;

        if (sess->rfc2428 != rfc2428_bad
            && ne_iaddr_typeof(sess->pi_curaddr) == ne_iaddr_ipv6) {
            ret = execute(sess, "EPSV");
            if (ret == FTP_PASSIVE) sess->rfc2428 = rfc2428_ok;
        }
        if ((sess->rfc2428 == rfc2428_unknown && ret != FTP_PASSIVE)
            || sess->rfc2428 == rfc2428_bad) {
            ret = execute(sess, "PASV");
        }
	if (ret == FTP_PASSIVE) {
	    if (dtp_open_passive(sess)) {
		return execute(sess, "%s", buf);
	    } else {
		return FTP_ERROR;
	    }
	} else {
	    return FTP_NOPASSIVE;
	}
    } else {
	/* we are not using passive mode. */
	return dtp_open_active(sess, buf);
    }
}


/* Closes the data connection.  If 'discard' is non-zero any error
 * string is discarded; otherwise the session error is set. */
static int dtp_close(ftp_session *sess, int discard)
{
    /* Read the response line */
    int ret;
    
    NE_DEBUG(DEBUG_FTP, "Closing DTP connection...\n");
    ret = ne_sock_close(sess->dtpsock);
    if (ret < 0) {
	int errnum = errno;
	if (!discard) set_syserr(sess, _("Error closing data socket"), errnum);
	return FTP_ERROR;
    } else {
        int code;

	ret = read_reply(sess, &code, sess->rbuf, sizeof sess->rbuf);
        if (ret != FTP_OK) return ret;
        
        ret = parse_reply(sess, code, sess->rbuf);
	if (ret == FTP_OK || ret == FTP_SENT)
	    return FTP_SENT;
	else {
            if (!discard) ftp_seterror(sess, sess->rbuf);
	    return FTP_ERROR;
        }
    }
}

/* Set the transfer type appropriately.
 * Only set it if it *needs* setting, though.
 * Returns FTP_OK on success, or something else otherwise. */
static int set_mode(ftp_session *sess, enum tran_mode mode)
{
    int ret;

    if (sess->mode == mode)
	return FTP_OK;

    ret = execute(sess, mode==tran_ascii?"TYPE A":"TYPE I");
    if (ret == FTP_OK)
	sess->mode = mode;
    else
	sess->mode = tran_unknown;

    return ret;
}

/* Change dir before uploading if necessary; some servers (notably
 * some version of ProFTPD) will reject a STOR if it is not in the
 * CWD.  *remotefile is updated to be the filename relative to the
 * working directory.
 *
 * 5, 7 Apr 2002, Volker Kuhlmann <VolkerKuhlmann@GMX.de> */
static int maybe_chdir(ftp_session *sess, const char **remotefile)
{
    int ret;
    const char *slash, *fn = *remotefile;
    char dir[PATH_MAX];

    if (!sess->use_cwd || fn[0] != '/' || strlen(fn) > PATH_MAX)
	return FTP_OK;

    slash = strrchr(fn, '/'); /* can't be NULL since fn[0] == '/'. */
    *remotefile = slash + 1;

    ne_strnzcpy(dir, fn, 1 + slash - fn);

    if (strcmp(dir, sess->cwd)) {
	ret = execute(sess, "CWD %s", dir);
	if (ret == FTP_OK) {
	    NE_DEBUG(DEBUG_FTP, "Stored new CWD as %s\n", dir);
	    strcpy(sess->cwd, dir);
	}
    } else {
	NE_DEBUG(DEBUG_FTP, "CWD not needed.\n");
	ret = FTP_OK;
    }

    return ret;
}


/* upload the given file */
int ftp_put(ftp_session *sess, 
	    const char *localfile, const char *remotefile, int ascii) 
{
    int ret;
    struct stat st;
    FILE *f;
    int tries = 0, dret;

    /* Set the transfer type correctly */
    if (set_mode(sess, ascii?tran_ascii:tran_binary))
	return FTP_ERROR;
    
    f = fopen(localfile, "r" FOPEN_BINARY_FLAGS);
    if (f == NULL) {
	int errnum = errno;
	set_syserr(sess, _("Could not open file"), errnum);
	return FTP_ERROR;
    }

    if (fstat(fileno(f), &st) < 0) {
	int errnum = errno;
	set_syserr(sess, _("Could not determine length of file"),
			 errnum);
	fclose(f);
	return FTP_ERROR;
    }
    
    do {
        /* Rewind the file pointer for attempts other than the
         * first: */
        if (tries && fseek(f, 0, SEEK_SET) < 0) {
            int errnum = errno;
            set_syserr(sess, _("Could not rewind to beginning of file"),
                       errnum);
            break;
        }

        ret = maybe_chdir(sess, &remotefile);
        if (ret != FTP_OK) {
            fclose(f);
            return ret;
        }

        ret = ftp_data_open(sess, "STOR %s", remotefile);
        if (ret == FTP_READY) {
            if (ascii)
                ret = send_file_ascii(sess, f, st.st_size);
            else
                ret = send_file_binary(sess, f, st.st_size);

            /* Now close the DTP connection and read the response. */
            dret = dtp_close(sess, 0);

            if (dret == FTP_SENT && ret == 0) {
                fclose(f);
                return FTP_OK;
            }
        } else {
            /* ftp_data_open() will already have retried as necessary
             * so don't retry it again here if that was all that
             * failed. */
            dret = FTP_ERROR;
        }
    } while (dret == FTP_BROKEN && ++tries < 3);

    fclose(f);
    return FTP_ERROR;
}

/* Conditionally upload the given file */
int ftp_put_cond(ftp_session *sess, const char *localfile, 
		 const char *remotefile, int ascii, const time_t mtime)
{
    int ret;
    
    ret = get_modtime(sess, remotefile, "");
    if (ret != FTP_OK) {
	return ret;
    }
    
    /* Is the retrieved time different from the given time? */
    if (sess->get_modtime != mtime) {
	return FTP_FAILED;
    }

    /* Do a normal upload */
    return ftp_put(sess, localfile, remotefile, ascii);
}

/* Slightly dodgy ftp_get in that you need to know the remote file
 * size. Works, but not very generic. */
int ftp_get(ftp_session *sess, const char *localfile, const char *remotefile, 
	    int ascii) 
{
    int ret;
    FILE *f = fopen(localfile, "wb");

    if (f == NULL) {
	int errnum = errno;
	set_syserr(sess, _("Could not open file"), errnum);
	return FTP_ERROR;
    }

    if (set_mode(sess, ascii?tran_ascii:tran_binary))
	return FTP_ERROR;

    if (ftp_data_open(sess, "RETR %s", remotefile) == FTP_READY) {
        int clo, errnum = 0;

	/* Receive the file */
	ret = receive_file(sess, f);
	clo = fclose(f);
        if (clo) errnum = errno;

	if (dtp_close(sess, 0) == FTP_SENT && ret == 0 && clo == 0) {
	    /* Success! */
	    return FTP_OK;
	} else if (clo) {
            set_syserr(sess, _("Error writing to file"), errnum);
        }
            
    }

    return FTP_ERROR;
}

int 
ftp_read_file(ftp_session *sess, const char *remotefile,
	      ne_block_reader reader, void *userdata) 
{
    ssize_t ret;

    /* Always use binary mode */
    if (set_mode(sess, tran_binary))
	return FTP_ERROR;

    if (ftp_data_open(sess, "RETR %s", remotefile) == FTP_READY) {
	char buffer[BUFSIZ];
	while ((ret = ne_sock_read(sess->dtpsock, buffer, sizeof buffer))
	       > 0)
	    reader(userdata, buffer, ret);
	if ((dtp_close(sess, 0) == FTP_SENT) && (ret == NE_SOCK_CLOSED)) {
	    return FTP_OK;
	}
    }
    return FTP_ERROR;
}

/* Log in to the server.  This routine *cannot* use execute() to
 * prevent recursion; if the connection times out during
 * authentication then that is a fatal error. */
static int authenticate(ftp_session *sess)
{
    int ret;
    char buf[1024];
    
    /* Fetch creds from user if necessary. */
    if (strlen(sess->username) == 0 || strlen(sess->password) == 0) {
	if (fe_login(fe_login_server, NULL, sess->hostname,
		     sess->username, sess->password))
	    return FTP_ERROR;
    }
    NE_DEBUG(DEBUG_FTP,  "FTP: Sending 'USER %s':\n", sess->username);

    ne_snprintf(buf, sizeof buf, "USER %s", sess->username);
    ret = run_command(sess, buf);
    if (ret == FTP_NEEDPASSWORD) {
	NE_DEBUG(DEBUG_FTP,  "FTP: Sending PASS command...\n");
        ne_snprintf(buf, sizeof buf, "PASS %s", sess->password);
	ret = run_command(sess, buf);
    }

    return ret;
}

int ftp_open(ftp_session *sess) 
{
    int ret, code, success;
    const ne_inet_addr *ia;

    if (sess->connected) return FTP_OK;
    NE_DEBUG(DEBUG_FTP, "Opening socket to port %d\n", sess->pi_port);

    /* Invalidate cwd, so a CWD is always used if needed (valid cwds
     * must begin with a slash. */
    strcpy(sess->cwd, "x");
    
    /* Open TCP connection */
    fe_connection(fe_connecting, NULL);
    sess->pisock = ne_sock_create();
    for (ia = ne_addr_first(sess->pi_addr), success = 0;
	 !success && ia != NULL; 
	 ia = ne_addr_next(sess->pi_addr)) {
	success = ne_sock_connect(sess->pisock, ia, sess->pi_port) == 0;
        sess->pi_curaddr = ia;
    }

    if (!success) {
        ne_sock_close(sess->pisock);
	return FTP_CONNECT;
    }

    fe_connection(fe_connected, NULL);

    /* Read the hello message */
    ret = read_reply(sess, &code, sess->rbuf, sizeof sess->rbuf);
    if (ret != FTP_OK) return FTP_HELLO;

    ret = parse_reply(sess, code, sess->rbuf);
    if (ret != FTP_OK) {
        ftp_seterror(sess, sess->rbuf);
        return FTP_HELLO;
    }

    if (authenticate(sess) != FTP_OK) {
        if (sess->connected) {
            ne_sock_close(sess->pisock);
            sess->pisock = NULL;
        }
	return FTP_LOGIN;
    }

    /* PI connection OK. */
    sess->connected = 1;

    /* Restore previous transfer mode; set mode to unknown for the
     * duration to avoid infinite recusion (since set_mode calls
     * execute_command calls ftp_open) . */
    if (sess->mode != tran_unknown) {
	enum tran_mode mode = sess->mode;
	sess->mode = tran_unknown;
	return set_mode(sess, mode);
    }

    return FTP_OK;
}

void ftp_seterror(ftp_session *sess, const char *error)
{
    memset(sess->error, 0, BUFSIZ);
    strncpy(sess->error, error, BUFSIZ);
}

const char *ftp_get_error(ftp_session *sess)
{
    return sess->error;
}    

int ftp_fetch(ftp_session *sess, const char *startdir, struct proto_file **list)
{
    struct proto_file *tail = NULL;
    struct ls_context *lsctx;
    int ret;

    if ((ret = ftp_data_open(sess, "LIST -la %s", startdir)) != FTP_READY) {
        return FTP_ERROR;
    }

    lsctx = ls_init(startdir);

    ret = FTP_OK;

    do {
        enum ls_result lsrv;
        struct ls_file lfile;
        ssize_t len;

        len = ne_sock_readline(sess->dtpsock, sess->rbuf, BUFSIZ);
        if (len == NE_SOCK_CLOSED) {
            NE_DEBUG(DEBUG_FTP, "ftp: EOF from DTP connection.\n");
            break;
        }
        if (len < 0) {
            set_sockerr(sess, sess->dtpsock,
                        _("Could not read 'LIST' response."), len);
            ret = FTP_ERROR;
            break;
        }

        lsrv = ls_parse(lsctx, sess->rbuf, &lfile);
        if (lsrv == ls_error) {
            char err[512];

            ne_snprintf(err, sizeof err, _("Parse error in LIST response: %s"),
                        ls_geterror(lsctx));
            NE_DEBUG(DEBUG_FTP, "ftp: ls_parse error, aborting.\n");
            ftp_seterror(sess, err);
            ret = FTP_ERROR;
        } else {
            ls_pflist_add(list, &tail, &lfile, lsrv);
        }
    } while (ret == FTP_OK);

    ls_destroy(lsctx);

    NE_DEBUG(DEBUG_FTP, "ftp: Fetch finished with %d.\n", ret);
    if (ret == FTP_OK) {
        return dtp_close(sess, 0) == FTP_SENT ? FTP_OK : FTP_ERROR;
    } else {
        dtp_close(sess, 1); /* ignore retval */
        return ret;
    }
}

static int 
get_modtime(ftp_session *sess, const char *root, const char *filename) 
{
    NE_DEBUG(DEBUG_FTP, "Getting modtime.\n");
    if (execute(sess, "MDTM %s%s", root, filename) == FTP_MODTIME) {
	NE_DEBUG(DEBUG_FTP, "Got modtime.\n");
	return FTP_OK;
    } else {
	return FTP_ERROR;
    }
}

int ftp_get_modtime(ftp_session *sess, const char *filename, time_t *modtime) 
{
    if (get_modtime(sess, filename, "") == FTP_OK) {
	*modtime = sess->get_modtime;
	return FTP_OK;
    } else {
	*modtime = -1;
	return FTP_ERROR;
    }
}

/* Sorts out the modtimes for all the files in the list.
 * Returns FTP_OK on success, else FTP_ERROR. */
int 
ftp_fetch_modtimes(ftp_session *sess, const char *rootdir, 
		   struct proto_file *files) 
{
    struct proto_file *this_file;
 
    for (this_file=files; this_file!=NULL; this_file=this_file->next) {
	if (this_file->type != proto_file) {
	    continue;
	}
	NE_DEBUG(DEBUG_FTP, "File: %s%s\n", rootdir, this_file->filename);
	if (get_modtime(sess, rootdir, this_file->filename) == FTP_OK) {
	    this_file->modtime = sess->get_modtime;
	} else {
	    NE_DEBUG(DEBUG_FTP, "Didn't get modtime.\n");
	    return FTP_ERROR;
	}
    }
    NE_DEBUG(DEBUG_FTP, "Walk finished ok.\n");

    return FTP_OK;
}
