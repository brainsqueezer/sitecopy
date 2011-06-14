/* 
   sitecopy sftp/ssh protocol driver module (forked from rshdriver.c)
   Copyright (C) 2000-2005, Joe Orton <joe@manyfish.co.uk>
   Copyright (C) 2004, Nobuyuki Tsuchimura <tutimura@nn.iij4u.or.jp>
                                                                     
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

/* TODO: 
 * - handle ssh directly instead of handling sftp....(?)
 */

#include <config.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <errno.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ne_alloc.h>
#include <ne_string.h>
#include <ne_socket.h>

#include "protocol.h"
#include "lsparser.h"

#ifndef HAVE_SOCKETPAIR
#define USE_PIPES
#endif

typedef struct {
    struct site *site;
    const char *sftp_cmd, *ssh_cmd;
    char buf[BUFSIZ];
    int connected, fd_in, fd_out, sftp_pid;
    FILE *fp;  /* popen for sftp client */
} sftp_session;

static int run_sftp(sftp_session *sess, const char *template, ...) 
    ne_attribute((format (printf, 2, 3)));

static int read_sftp(sftp_session *sess) {
    size_t pos = 0;

    do {
        ssize_t ret = read(sess->fd_in, sess->buf+pos, BUFSIZ-pos);
        if (ret < 0) return SITE_FAILED;
        if (ret == 0) break;
        pos += ret;
        sess->buf[pos] = '\0';
    } while (strchr(sess->buf, '>') == NULL && pos < BUFSIZ);
    NE_DEBUG(DEBUG_SFTP, "(%s)", sess->buf);
    return SITE_OK;
}
    
static void exec_sftp(sftp_session *sess)
{
    size_t len;
    char *username;

    username = sess->site->server.username;
    len = ne_snprintf(sess->buf, BUFSIZ, "%s%s%s",
                      (username != NULL ? username : ""),
                      (username != NULL ? "@" : ""),
                      sess->site->server.hostname);
    if (len + 1 >= BUFSIZ) {
        NE_DEBUG(DEBUG_SFTP, "sftp: user/host name too long.\n");
        return;
    }

    execlp(sess->sftp_cmd, sess->sftp_cmd, sess->buf, NULL);
    NE_DEBUG(DEBUG_SFTP, "sftp exec: %s %s: %s\n",
             sess->sftp_cmd, sess->buf, strerror(errno));
}

static int sftp_connect(sftp_session *sess)
{
    int c_in, c_out;
#ifdef USE_PIPES
    int pin[2], pout[2];
    if ((pipe(pin) == -1) || (pipe(pout) == -1)) {
        NE_DEBUG(DEBUG_SFTP, "sftp pipe: %s", strerror(errno));
        return SITE_FAILED;
    }
    sess->fd_in  = pin[0];
    sess->fd_out = pout[1];
    c_in = pout[0];
    c_out = pin[1];
#else /* !USE_PIPES */
    int inout[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, inout) == -1) {
        NE_DEBUG(DEBUG_SFTP, "sftp socketpair: %s", strerror(errno));
        return SITE_FAILED;
    }
    sess->fd_in = sess->fd_out = inout[0];
    c_in = c_out = inout[1];
#endif /* USE_PIPES */

    sess->connected = true;
    sess->sftp_pid = fork();
    switch (sess->sftp_pid) {
    case -1:
        NE_DEBUG(DEBUG_SFTP, "sftp: fork: %s", strerror(errno));
        return SITE_FAILED;
    case 0:
        if ((dup2(c_in, STDIN_FILENO) == -1) ||
            (dup2(c_out, STDOUT_FILENO) == -1)) {
            NE_DEBUG(DEBUG_SFTP, "sftp: dup2: %s\n", strerror(errno));
            return SITE_FAILED;
        }
        close(sess->fd_in); close(sess->fd_out);
        close(c_in);        close(c_out);
        exec_sftp(sess);
        return SITE_FAILED;
    }
    close(c_in);
    close(c_out);
    read_sftp(sess);    /* wait for prompt */
    return SITE_OK;
}

static int sftp_disconnect(sftp_session *sess)
{
#ifndef USE_PIPES
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
    shutdown(sess->fd_in,  SHUT_RDWR);
    shutdown(sess->fd_out, SHUT_RDWR);
#endif /* USE_PIPES */
    close(sess->fd_in);
    close(sess->fd_out);

    if (waitpid(sess->sftp_pid, NULL, 0) == -1) {
        NE_DEBUG(DEBUG_SFTP, "sftp: Couldn't wait for sftp process: %s",
                 strerror(errno));
        return SITE_FAILED;
    }
    return SITE_OK;
}


static int run_sftp(sftp_session *sess, const char *template, ...) 
{
    va_list params;
    if (!sess->connected) sftp_connect(sess);

    va_start(params, template);
    ne_vsnprintf(sess->buf, BUFSIZ, template, params);
    va_end(params);

    if (strlen(sess->buf) + 2 >= BUFSIZ) { /* will be added '\n' */
        NE_DEBUG(DEBUG_SFTP, "sftp: sftp: too long command '%s'.\n", sess->buf);
        return SITE_FAILED;  /* buff over flow */
    }
    NE_DEBUG(DEBUG_SFTP, "SFTP: '%s'\n", sess->buf);

    strcat(sess->buf, "\n");
    if (write(sess->fd_out, sess->buf, strlen(sess->buf)) == -1) {
	return SITE_FAILED;
    }
    read_sftp(sess);    /* wait for prompt */
    return SITE_OK;
}


static int run_ssh(sftp_session *sess, const char *template, ...) 
    ne_attribute((format (printf, 2, 3)));

static int run_ssh(sftp_session *sess, const char *template, ...) 
{
    va_list params;
    char *buf = sess->buf;
    size_t len;
    char *username = sess->site->server.username;

    len = ne_snprintf(buf, BUFSIZ, "%s%s%s %s \\\\", 
                      sess->ssh_cmd,
                      (username != NULL ? " -l " : ""),
                      (username != NULL ? username : ""),
                      sess->site->server.hostname);

    va_start(params, template);
    len += ne_vsnprintf(buf + len, BUFSIZ - len, template, params);
    va_end(params);

    len += ne_snprintf(buf + len, BUFSIZ - len, " 2> /dev/null" );
    if (len + 1 >= BUFSIZ) return SITE_FAILED;  /* buff over flow */
    NE_DEBUG(DEBUG_SFTP, "rcmd: %s\n", buf);

    sess->fp = popen(buf, "r");
    if (sess->fp != NULL) {
        return SITE_OK;
    } else {
        return SITE_FAILED;
    }
    /* why not free(cmd)? */
}

static int ssh_finish(sftp_session *sess)
{
    if (pclose(sess->fp) == 0) {
        sess->fp = NULL;
	return SITE_OK;
    } else {
	return SITE_FAILED;
    }
}

static int init(void **session, struct site *site)
{
    sftp_session *sess = ne_calloc(sizeof *sess);
    *session = sess;
    if (site->rcp_cmd != NULL) {
	sess->sftp_cmd = site->rcp_cmd;
    } else {
	sess->sftp_cmd = "sftp";
    }
    if (site->rsh_cmd != NULL) {
	sess->ssh_cmd = site->rsh_cmd;
    } else {
	sess->ssh_cmd = "ssh";
    }
    sess->site = site;
    sess->connected = false;
    return SITE_OK;
}    

static void finish(void *session)
{
    sftp_session *sess = session;

    if (sess->connected) sftp_disconnect(sess);
    if (sess->fp != NULL) ssh_finish(sess);
    free(sess);
}

static int file_move(void *session, const char *from, const char *to)
{
    sftp_session *sess = session;
    return run_sftp(sess, "rename %s %s", from, to);
}

static int file_upload(void *session, const char *local, const char *remote, 
                       int ascii)
{
    sftp_session *sess = session;
    return run_sftp(sess, "put %s %s", local, remote);
}

static int file_upload_cond(void *session,
			    const char *local, const char *remote,
			    int ascii, time_t t)
{
    return SITE_UNSUPPORTED;
}

static int file_get_modtime(void *sess, const char *remote, time_t *modtime)
{
    return SITE_UNSUPPORTED;
}
    
static int file_download(void *session, const char *local, const char *remote,
			 int ascii)
{
    sftp_session *sess = session;
    return run_sftp(sess, "get %s %s", remote, local);
}


static int file_read(void *session, const char *remote,
                     ne_block_reader reader, void *userdata)
{
    sftp_session *sess = session;
    size_t len;

    if (run_ssh(sess, "cat '%s'", remote) != SITE_OK)
        return SITE_FAILED;
    
    while ((len = fread(sess->buf, 1, BUFSIZ, sess->fp)) > 0) {
        reader(userdata, sess->buf, len);
    }

    return ssh_finish(sess);
}

static int file_delete(void *session, const char *filename)
{
    sftp_session *sess = session;
    return run_sftp(sess, "rm %s", filename);
}

static int file_chmod(void *session, const char *fname, const mode_t mode)
{
    sftp_session *sess = session;
    return run_sftp(sess, "chmod %03o %s", mode, fname);
}

static int dir_create(void *session, const char *dirname)
{
    sftp_session *sess = session;
    return run_sftp(sess, "mkdir %s", dirname);
}

static int dir_remove(void *session, const char *dirname)
{
    sftp_session *sess = session;
    return run_sftp(sess, "rmdir %s", dirname);
}

static const char *error(void *session)
{
    return "An error occurred.";
}

static int get_dummy_port(struct site *site)
{
    return 0;
}

static int ssh_fetch(sftp_session *sess, const char *startdir, 
                     struct proto_file **list)
{
    struct proto_file *tail = NULL;
    int success = SITE_OK;
    ls_context_t *lsctx = ls_init(startdir);

    memset(sess->buf, 0, BUFSIZ);
    
    while (fgets(sess->buf, BUFSIZ, sess->fp) != NULL) {
        enum ls_result res;
        struct ls_file lfile;
        
        res = ls_parse(lsctx, sess->buf, &lfile);

        switch (res) {
        case ls_error:
            NE_DEBUG(DEBUG_SFTP, "Could not parse line.\n");
            success = SITE_ERRORS;
            break;

        default:
            ls_pflist_add(list, &tail, &lfile, res);
            break;
	}
    }

    ls_destroy(lsctx);

    NE_DEBUG(DEBUG_SFTP, "Fetch finished successfully.\n");
    return success;
}

static int fetch_list(void *session, const char *dirname, int need_modtimes,
                     struct proto_file **files)
{
    sftp_session *sess = session;
    int ret;

    ret = run_ssh(sess, "ls -la '%s'", dirname);
    if (ret == SITE_OK) {
        ssh_fetch(sess, dirname, files);
        ret = ssh_finish(sess);
    }

    /*
    if (ret == SITE_OK && need_modtimes) {
	ret = sftp_fetch_modtimes(sess, dirname, *files);
    }
    */

    return ret;
}

/* The protocol driver */
const struct proto_driver sftp_driver = {
    init,
    finish,
    file_move,
    file_upload,
    file_upload_cond,
    file_get_modtime,
    file_download,
    file_read,
    file_delete,
    file_chmod,
    dir_create,
    dir_remove,
    NULL, /* create link */
    NULL, /* change link target */
    NULL, /* delete link */
    fetch_list, /* fetch list. */
    error,
    get_dummy_port,
    get_dummy_port,
    "sftp/ssh"
};
