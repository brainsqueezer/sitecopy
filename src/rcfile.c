/* 
   sitecopy, for managing remote web sites.
   Copyright (C) 1998-2006, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

#include <config.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif 
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <ne_string.h>
#include <ne_alloc.h>

#include "common.h"
#include "netrc.h"
#include "rcfile.h"
#include "sites.h"

/** Global variables **/
char *copypath;
char *rcfile;
char *netrcfile;
char *home;
int havenetrc;

/* These are used for reporting errors back to the calling procedures. */
int rcfile_linenum; 
char *rcfile_err;

/** Not quite so global variables **/

/* These are appended to $HOME */
#define RCNAME "/.sitecopyrc"
#define COPYNAME "/.sitecopy/"
#define NETRCNAME "/.netrc"

/* Stores the list of entries in the ~/.netrc */
netrc_entry *netrc_list;

const char *rc_get_netrc_password(const char *server, const char *username);

/* The driver definitions */
#ifdef USE_FTP
extern const struct proto_driver ftp_driver;
#endif /* USE_FTP */
#ifdef USE_DAV
extern const struct proto_driver dav_driver;
#endif /* USE_DAV */
#ifdef USE_RSH
extern const struct proto_driver rsh_driver;
#endif /* USE_RSH */
#ifdef USE_SFTP
extern const struct proto_driver sftp_driver;
#endif /* USE_SFTP */

/* rcfile_read will read the rcfile and fill given sites list.
 * This returns 0 on success, RC_OPENFILE if the rcfile could not
 * be read, or RC_CORRUPT if the rcfile was corrupt.
 * If it is corrupt, rcfile_linenum and rcfile_line are set to the
 * the corrupt line.
 */
#define LINESIZE 128
int rcfile_read(struct site **sites) 
{
    FILE *fp;
    int state, last_state=8, ret=0;
    int alpha, hash;
    char buf[LINESIZE];
    char *ch;
    char *ptr, key[LINESIZE], val[LINESIZE], val2[LINESIZE];
    /* Holders for the site info, and default site settings */
    struct site *this_site, *last_site, default_site = {0};
    
    if ((fp = fopen(rcfile, "r")) == NULL) {
	rcfile_err = strerror(errno);
	return RC_OPENFILE;
    } 
    
    default_site.perms = sitep_ignore;
    default_site.symlinks = sitesym_follow;
    default_site.protocol = siteproto_ftp;
    default_site.proto_string = ne_strdup("ftp");

    default_site.ftp_pasv_mode = true;
    default_site.ftp_use_cwd = false;
    
    last_site = this_site = NULL;
    rcfile_linenum = 0;
    rcfile_err = NULL;

    while ((ret==0) && (fgets(buf, sizeof(buf), fp) != NULL)) {
	rcfile_linenum++;
	/* Put the line without the LF into the error buffer */
	if (rcfile_err != NULL) free(rcfile_err);
	rcfile_err = ne_strdup(buf);
	ptr = strchr(rcfile_err, '\n');
	if (ptr != NULL) *ptr = '\0';
	state = 0;
	ptr = key;
	memset(key, 0, LINESIZE);
	memset(val, 0, LINESIZE);
	memset(val2, 0, LINESIZE);
	for (ch=buf; *ch!='\0'; ch++) {
	    alpha = !isspace((unsigned)*ch); /* well, alphaish */
	    hash = (*ch == '#');
	    switch (state) {
	    case 0: /* whitespace at beginning of line */
		if (hash) {
		    state = 8;
		} else if (alpha) {
		    *(ptr++) = *ch;
		    state = 1;
		}
		break;
	    case 1: /* key */
		if (hash) {
		    state = 8;
		} else if (!alpha) {
		    ptr = val;
		    state = 2;
		} else {
		    *(ptr++) = *ch;
		}
		break;
	    case 2: /* whitespace after key */
		if (hash) {
		    state = 8;
		} else if (*ch == '"') {
		    state = 4; /* begin quoted value */
		} else if (alpha) {
		    *(ptr++) = *ch;
		    state = 3;
		} 
		break;
	    case 3: /* unquoted value 1 */
		if (hash) {
		    state = 8;
		} else if (!alpha) {
		    ptr = val2;
		    state = 5;
		} else {
		    *(ptr++) = *ch;
		}
		break;
	    case 4: /* quoted value 1 */
		if (*ch == '"') {
		    ptr = val2;
		    state = 5;
		} else if (*ch == '\\') {
		    last_state = 4;
		    state = 9;
		} else {
		    *(ptr++) = *ch;
		}
		break;
	    case 5: /* whitespace after value 1 */
		if (hash) {
		    state = 8;
		} else if (*ch == '"') {
		    state = 6; /* begin quoted value 2 */
		} else if (alpha) {
		    *(ptr++) = *ch;
		    state = 7; /* begin unquoted value 2 */
		} 
		break;
	    case 6: /* quoted value 2 */
		if (*ch == '"') {
		    state = 8;
		} else if (*ch == '\\') {
		    last_state = 4;
		    state = 9;
		} else {
		    *(ptr++) = *ch;
		}
		break;
	    case 7: /* unquoted value 2 */
		if (hash) {
		    state = 8;
		} else if (!alpha) {
		    state = 8;
		} else {
		    *(ptr++) = *ch;
		}
		break;
	    case 8: /* ignore till end of line */
		break;
	    case 9: /* a literal (\-slashed) in a value */
		*(ptr++) = *ch;
		state = last_state;
		break;
	    }
	}
	
	NE_DEBUG(DEBUG_RCFILE, "Key [%s] Value: [%s] Value2: [%s]\n", key, val, val2);
	
	if (strlen(key) == 0) {
	    continue;
	}
	if (strlen(val) == 0) {
	    /* A key with no value. */
	    if (this_site == NULL) {
		if (strcmp(key, "default") == 0) {
		    /* Setting up the default site */
		    NE_DEBUG(DEBUG_RCFILE, "Default site entry:\n");
		    this_site = &default_site;
		} else {
		    /* Need to be in a site! */
		    ret = RC_CORRUPT;
		}
	    } else if (strcmp(key, "nodelete") == 0) {
		this_site->nodelete = true;
	    } else if (strcmp(key, "checkmoved") == 0) {
		this_site->checkmoved = true;
	    } else if (strcmp(key, "nooverwrite") == 0) {
		this_site->nooverwrite = true;
	    } else if (strcmp(key, "lowercase") == 0) {
		this_site->lowercase = true;
	    } else if (strcmp(key, "safe") == 0) {
		this_site->safemode = true;
	    } else if (strcmp(key, "tempupload") == 0) {
		this_site->tempupload = true;
	    } else {
		ret = RC_CORRUPT;
	    }
	} else if (strlen(val2) == 0) {
	    /* A key with a single value. */
	    if (strcmp(key, "site") == 0) {
		/* Beginning of a new Site */
		if (this_site != &default_site)
		    last_site = this_site;
		/* Allocate new item */
		this_site = ne_malloc(sizeof(struct site));
		/* Copy over the defaults */
		memcpy(this_site, &default_site, sizeof(struct site));
		/* Deep-copy the string lists */
		this_site->excludes = fnlist_deep_copy(default_site.excludes);
		this_site->ignores = fnlist_deep_copy(default_site.ignores);
		this_site->asciis = fnlist_deep_copy(default_site.asciis);
		this_site->prev = last_site;
		if (last_site != NULL) { /* next site */
		    last_site->next = this_site;
		} else { /* First site */
		    *sites = this_site;
		}		
		this_site->name = ne_strdup(val);
		this_site->files = NULL;
		this_site->proto_string = ne_strdup(default_site.proto_string);
		/* Now work out the info filename */
		this_site->infofile = ne_concat(copypath, val, NULL);
                this_site->certfile = ne_concat(copypath, val, ".crt", NULL);
	    } else if (this_site == NULL) {
		ret = RC_CORRUPT;
	    } else if (strcmp(key, "username") == 0) {
		/* username */
		this_site->server.username = ne_strdup(val);
	    } else if (strcmp(key, "server") == 0) {
		this_site->server.hostname = ne_strdup(val);
	    } else if (strcmp(key, "port") == 0) {
		this_site->server.port = atoi(val);
	    } else if (strcmp(key, "proxy-server") == 0) {
		this_site->proxy.hostname = ne_strdup(val);
       printf("proxy-server: %s\n", this_site->proxy.hostname);
	    } else if (strcmp(key, "proxy-port") == 0) {
		this_site->proxy.port = atoi(val);
       printf("proxy-port: %d\n", this_site->proxy.port);
	    } else if (strcmp(key, "proxy-password") == 0) {
		this_site->proxy.password = ne_strdup(val);
	    } else if (strcmp(key, "proxy-username") == 0) {
		this_site->proxy.username = ne_strdup(val);
	    } else if (strcmp(key, "password") == 0) {
		this_site->server.password = ne_strdup(val);
	    } else if (strcmp(key, "url") == 0) {
	        this_site->url = ne_strdup(val);
	    } else if (strcmp(key, "remote") == 0) {
		/* Relative filenames must start with "~/" */
		if (val[0] == '~') {
		    if (val[1] == '/') {
			this_site->remote_isrel = true;
		    } else {
			ret = RC_CORRUPT;
		    }
		} else {
		    /* Dirname doesn't begin with "~/" */
		    this_site->remote_isrel = false;
		}
		if (val[strlen(val)-1] != '/')
		    strcat(val, "/");
		this_site->remote_root_user = ne_strdup(val);
	    } else if (strcmp(key, "local") == 0) {
		/* Relative filenames must start with "~/" */
		if (val[0] == '~') {
		    if (val[1] == '/') {
			this_site->local_isrel = true;
		    } else {
			ret = RC_CORRUPT;
		    }
		} else { 
		    /* Dirname doesn't begin with a "~/" */
		    this_site->local_isrel = false;
		}
		if (val[strlen(val)-1] != '/')
		    strcat(val, "/");
		this_site->local_root_user = ne_strdup(val);
	    } else if (strcmp(key, "permissions") == 0) {
		if (strcmp(val, "ignore") == 0) {
		    this_site->perms = sitep_ignore;
                    this_site->dirperms = 0;
		} else if (strcmp(val, "exec") == 0) {
		    this_site->perms = sitep_exec;
		} else if (strcmp(val, "all") == 0) {
		    this_site->perms = sitep_all;
                } else if (strcmp(val, "dir") == 0) {
                    this_site->dirperms = 1;
		} else {
		    ret = RC_CORRUPT;
		}
	    } else if (strcmp(key, "symlinks") == 0) {
		if (strcmp(val, "follow") == 0) {
		    this_site->symlinks = sitesym_follow;
		} else if (strcmp(val, "maintain") == 0) {
		    this_site->symlinks = sitesym_maintain;
		} else if (strcmp(val, "ignore") == 0) {
		    this_site->symlinks = sitesym_ignore;
		} else {
		    ret = RC_CORRUPT;
		}
	    } else if (strcmp(key, "exclude") == 0) {
		struct fnlist *f = fnlist_prepend(&this_site->excludes);
		if (val[0] == '/') {
		    f->pattern = ne_strdup(val+1);
		    f->haspath = true;
		} else {
		    f->pattern = ne_strdup(val);
		    f->haspath = false;
		}
	    } else if (strcmp(key, "ignore") == 0) {
		struct fnlist *f = fnlist_prepend(&this_site->ignores);
		if (val[0] == '/') {
		    f->pattern = ne_strdup(val+1);
		    f->haspath = true;
		} else {
		    f->pattern = ne_strdup(val);
		    f->haspath = false;
		}
	    } else if (strcmp(key, "ascii") == 0) {
		struct fnlist *f = fnlist_prepend(&this_site->asciis);
		if (val[0] == '/') {
		    f->pattern = ne_strdup(val+1);
		    f->haspath = true;
		} else {
		    f->pattern = ne_strdup(val);
		    f->haspath = false;
		}
	    } else if (strcmp(key, "protocol") == 0) {
		if (strcasecmp(val, "ftp") == 0) {
		    this_site->protocol = siteproto_ftp;
		} else if (strcasecmp(val, "http") == 0 || 
			   strcasecmp(val, "dav") == 0 ||
			   strcasecmp(val, "webdav") == 0) {
		    this_site->protocol = siteproto_dav;
		} else if (strcasecmp(val, "rsh") == 0) {
		    this_site->protocol = siteproto_rsh;
		} else if (strcasecmp(val, "ssh") == 0) {
                    this_site->protocol = siteproto_rsh;
                    if (this_site->rsh_cmd == NULL) 
                        this_site->rsh_cmd = ne_strdup("ssh");
                    if (this_site->rcp_cmd == NULL) 
                        this_site->rcp_cmd = ne_strdup("scp");
		} else if (strcasecmp(val, "sftp") == 0) {
		    this_site->protocol = siteproto_sftp;
                } else {
		    this_site->protocol = siteproto_unknown;
		}
		free(this_site->proto_string);
		this_site->proto_string = ne_strdup(val);
	    } else if (strcmp(key, "ftp") == 0) {
		if (strcmp(val, "nopasv") == 0) {
		    this_site->ftp_pasv_mode = false;
		} else if (strcmp(val, "showquit") == 0) {
		    this_site->ftp_echo_quit = true;		    
		} else if (strcmp(val, "usecwd") == 0) {
		    this_site->ftp_use_cwd = true;		    
		} else if (strcmp(val, "nousecwd") == 0) {
		    this_site->ftp_use_cwd = false;		    
		} else {
		    ret = RC_CORRUPT;
		}
	    } else if (strcmp(key, "http") == 0) {
		if (strcmp(val, "expect") == 0) {
		    this_site->http_use_expect = true;
		} else if (strcmp(val, "limit") == 0) {
		    this_site->http_limit = true;
		} else if (strcmp(val, "secure") == 0) {
		    this_site->http_secure = true;
		} else if (strcmp(val, "tolerant") == 0) {
		    this_site->http_tolerant = true;
		} else {		    
		    ret = RC_CORRUPT;
		}
            } else if (strcmp(key, "client-cert") == 0) {
                this_site->client_cert = ne_strdup(val);
	    } else if (strcmp(key, "rsh") == 0) {
		this_site->rsh_cmd = ne_strdup(val);
	    } else if (strcmp(key, "rcp") == 0) {
		this_site->rcp_cmd = ne_strdup(val);
	    } else if (strcmp(key, "state") == 0) {
		if (strcmp(val, "checksum") == 0) {
		    this_site->state_method = state_checksum;
		} else if (strcmp(val, "timesize") == 0) {
		    this_site->state_method = state_timesize;
		} else {
		    ret = RC_CORRUPT;
		}
	    } else if (strcmp(key, "checkmoved") == 0) {
		if (strcmp(val, "renames") == 0) {
		    this_site->checkrenames = true;
		    this_site->checkmoved = true;
		} else {
		    ret = RC_CORRUPT;
		}
	    } else if (strcmp(key, "charset") == 0) {
                NE_DEBUG(DEBUG_RCFILE, "Ignored key %s\n", key);
            } else {
		/* Unknown key! */
		ret = RC_CORRUPT;
	    }
	} else {
	    {
		ret = RC_CORRUPT;
	    }
	}
    }

    fclose(fp);
    return ret;
}
#undef LINESIZE

const char *rc_get_netrc_password(const char *server, const char *username) {
    netrc_entry *found;
    found = search_netrc(netrc_list, server);
    if (found == NULL) {
	return NULL;
    }
    if (strcmp(found->account, username) == 0) {
	return found->password;
    } else {
	return NULL;
    }
}

/* Returns zero if site is properly defined, else non-zero */
int rcfile_verify(struct site *any_site) 
{
    struct stat localst;
    char *temp;
    int ret;

    /* Protocol-specific checks first, since if a new protocol driver is used,
     * any of the other checks may be irrelevant. */
    switch (any_site->protocol) {
    case siteproto_ftp:
#ifdef USE_FTP
	any_site->driver = &ftp_driver;
	/* FTP checks */
	if (any_site->symlinks == sitesym_maintain) {
	    return SITE_NOMAINTAIN;
	}
	break;
#else /* !USE_FTP */
	return SITE_UNSUPPORTED;
#endif /* USE_FTP */
    case siteproto_dav:
#ifdef USE_DAV
	any_site->driver = &dav_driver;
	/* HTTP checks */
	if (any_site->remote_isrel) { 
	    return SITE_NOREMOTEREL;
	}
	if (any_site->perms == sitep_all || any_site->dirperms) {
	    return SITE_NOPERMS;
	}
	if (any_site->symlinks == sitesym_maintain) {
	    return SITE_NOMAINTAIN;
	}
	break;
#else /* !USE_DAV */
	return SITE_UNSUPPORTED;
#endif /* USE_DAV */
    case siteproto_rsh:
#ifdef USE_RSH
	any_site->driver = &rsh_driver;
	/* FIXME: rsh checks? */
	break;
#else /* !USE_RSH */
	return SITE_UNSUPPORTED;
#endif /* USE_RSH */
    case siteproto_sftp:
#ifdef USE_SFTP
	any_site->driver = &sftp_driver;
	/* FIXME: sftp checks? */
	break;
#else /* !USE_SFTP */
	return SITE_UNSUPPORTED;
#endif /* USE_SFTP */
    case siteproto_unknown:
	return SITE_UNSUPPORTED;
    }

    /* Valid options check */
    if (any_site->checkrenames && (any_site->state_method != state_checksum)) {
	return SITE_NORENAMES;
    }

    /* Check they specified everything in the rcfile */
    if (any_site->server.hostname == NULL) {
	return SITE_NOSERVER;
    } 

    if (any_site->server.username != NULL && any_site->server.password == NULL) {
	if (havenetrc) {
	    const char *pass;
	    NE_DEBUG(DEBUG_RCFILE, "Checking netrc for password for %s@%s...",
		   any_site->server.username, any_site->server.hostname);
	    pass = rc_get_netrc_password(any_site->server.hostname, 
					  any_site->server.username);
	    if (pass != NULL) {
		NE_DEBUG(DEBUG_RCFILE, "found!\n");
		any_site->server.password = (char *) pass;
	    } else {
		NE_DEBUG(DEBUG_RCFILE, "none found.\n");
	    }
	}
    }
    /* TODO: lookup proxy username/password in netrc too */

    if (any_site->remote_root_user == NULL) {
	return SITE_NOREMOTEDIR;
    } else if (any_site->local_root_user == NULL) {
	return SITE_NOLOCALDIR;
    }
    
    /* Need a home directory if we're using relative local root */
    if (home == NULL && any_site->local_root)
	return SITE_NOLOCALREL;

    /* Can't use safe mode and nooverwrite mode */
    if (any_site->safemode && any_site->nooverwrite)
	return SITE_NOSAFEOVER;

    if (any_site->safemode && any_site->tempupload)
	return SITE_NOSAFETEMPUP;

    if (any_site->remote_isrel) {
	any_site->remote_root = ne_strdup(any_site->remote_root_user + 2);
    } else {
	any_site->remote_root = ne_strdup(any_site->remote_root_user);
    }
    if (any_site->local_isrel) {
	/* We skip the first char ('~') of l_r_u */
	any_site->local_root = ne_concat(home, any_site->local_root_user + 1,
					 NULL);
    } else {
	any_site->local_root = any_site->local_root_user;
    }

    /* Now check the local directory actually exists.
     * To do this, stat `/the/local/root/.', which will fail if the
     * can't read the directory or if it's a file not a directory */
    temp = ne_concat(any_site->local_root, ".", NULL);
    ret = stat(temp, &localst);
    free(temp);
    if (ret != 0) {
	return SITE_ACCESSLOCALDIR;
    }

    if (any_site->client_cert && strncmp(any_site->client_cert, "~/", 2) == 0) {
        temp = ne_concat(home, any_site->client_cert + 1, NULL);
        ne_free(any_site->client_cert);
        any_site->client_cert = temp;
    }

    /* Assign default ports if they didn't bother to */
    if (any_site->server.port == 0) {
	NE_DEBUG(DEBUG_RCFILE, "Lookup up default port:\n");
	any_site->server.port = (*any_site->driver->get_server_port)(any_site);
	NE_DEBUG(DEBUG_RCFILE, "Using port: %d\n", any_site->server.port);
    }

    if (any_site->proxy.port == 0) {
	NE_DEBUG(DEBUG_RCFILE, "Lookup default proxy port...\n");
	any_site->proxy.port = (*any_site->driver->get_proxy_port)(any_site);
	NE_DEBUG(DEBUG_RCFILE, "Using port %d\n", any_site->proxy.port);
    }

    /* TODO: ditto for proxy server */
    return 0;
}

int init_netrc() {
    if (!havenetrc) return 0;
    netrc_list = parse_netrc(netrcfile);
    if (netrc_list == NULL) {
	/* Couldn't parse it */
	return 1;
    } else {
	/* Could parse it */
	return 0;
    }
}

/* Checks the perms of the rcfile and site storage directory. */
int init_paths()
{
    struct stat st;
    if (stat(rcfile, &st) < 0) {
	NE_DEBUG(DEBUG_RCFILE, "stat failed on %s: %s\n", 
	       rcfile, strerror(errno));
	return RC_OPENFILE;
    }
#if !defined (__EMX__) && !defined(__CYGWIN__)
    if (!S_ISREG(st.st_mode)) {
        return RC_OPENFILE;
    }
    if ((st.st_mode & ~(S_IFREG | S_IREAD | S_IWRITE)) > 0) {
	return RC_PERMS;
    }
#endif
    if ((netrcfile == 0) || (stat(netrcfile, &st) < 0)) {
	havenetrc = false;
#if !defined (__EMX__) && !defined(__CYGWIN__)
    } else if ((st.st_mode & ~(S_IFREG | S_IREAD | S_IWRITE)) > 0) {
	return RC_NETRCPERMS;
#endif
    } else {
	havenetrc = true;
    }
    if (stat(copypath, &st) < 0) {
	NE_DEBUG(DEBUG_RCFILE, "stat failed on %s: %s\n", 
	       copypath, strerror(errno));
	return RC_DIROPEN;
    }
#if !defined (__EMX__) && !defined(__CYGWIN__)
    if (st.st_mode & (S_IRWXG | S_IRWXO)) {
	return RC_DIRPERMS;
    }
#endif
    return 0;
}

int init_env() {
    /* Assign default filenames if they didn't give us any */
    home = getenv("HOME");
    if (home == NULL) {
	if ((rcfile == NULL) || (copypath == NULL)) {
	    /* We need a $HOME or both rcfile and info dir path */
	    return 1;
	} else {
	    /* No $HOME, but we've got the rcfile and info dir path */
	    return 0;
	}
    }
    if (rcfile == NULL) {
	rcfile = ne_concat(home, RCNAME, NULL);
    }
    if (copypath == NULL) {
	copypath = ne_concat(home, COPYNAME, NULL);
    }
    netrcfile = ne_concat(home, NETRCNAME, NULL);
    return 0;
}

/* rcfile_write() by Lee Mallabone, cleaned by JO.
 * Write the contents of list_of_sites to the specified 'filename'
 * in the standard sitecopy rc format.
 *
 * Any data already in 'filename' is over-written.
 */
int rcfile_write (char *filename, struct site *list_of_sites) 
{
    struct site *current;
    struct fnlist *item;
    FILE *fp;
    
    fp = fopen (filename, "w");
    if (fp == NULL) {
	printf ("There was a problem writing to the sitecopy configuration file.\n\nCheck permissions on %s.", filename);
	return RC_OPENFILE;
   }

    /* Set rcfile permissions properly */
#if !defined (__EMX__) && !defined(__CYGWIN__)
    if (fchmod (fileno(fp), 00600) == -1) {
	return RC_PERMS;
    }
#endif
    
    for (current=list_of_sites; current!=NULL; current=current->next) {
	/* Okay so this maybe isn't the most intuitive thing to look at.
	 * With any luck though, the rcfile's it produces will be. :) */
	if (fprintf (fp, "site %s\n", current->name) == -1) {
	    return RC_CORRUPT;
	}
	if (fprintf (fp, "  server %s\n", current->server.hostname) == -1) {
	    return RC_CORRUPT;
	}
       
	if ((current->server.username != NULL) && 
	    (strlen(current->server.username) > 0))
	    if (fprintf(fp, "  username %s\n", 
			current->server.username) == -1) {
		return RC_CORRUPT;
	    }
	
	if ((current->server.password != NULL) 
	    && (strlen(current->server.password) > 0))
	    if (fprintf(fp, "  password %s\n", 
			current->server.password) == -1) {
		return RC_CORRUPT;
	    }
	
        if (fprintf(fp, "  remote %s\n  local %s\n",
		    current->remote_root_user,
		    current->local_root_user) == -1) {
	    return RC_CORRUPT;
	}
	
	if (fprintf (fp, "  protocol %s\n", current->proto_string) == -1) {
	    return RC_CORRUPT;
	}
	
	/* Makes sense to have protocol (ish) options after we specify
	 * the protocol.  Warning, if the http declarations in site_t
	 * are ever surrounded by an ifdef USE_DAV, then this will need
	 * to be changed.  */
	
	/* Write out the boolean fields */
	
#define RCWRITEBOOL(field,name) \
      if ((field) && (fprintf(fp, "  %s\n", name) == -1)) return RC_CORRUPT;
	
	RCWRITEBOOL(current->nodelete, "nodelete");
	if (current->checkmoved) {
	    if (current->checkrenames) {
		if (fprintf(fp, "  checkmoved renames\n") == -1)
		    return RC_CORRUPT;
	    } else {
		if (fprintf(fp, "  checkmoved\n") == -1)
		    return RC_CORRUPT;
	    }
	}
	
	RCWRITEBOOL(current->nooverwrite, "nooverwrite");
	RCWRITEBOOL(current->safemode, "safe");
	RCWRITEBOOL(current->lowercase, "lowercase");
	RCWRITEBOOL(current->tempupload, "tempupload");
	
	RCWRITEBOOL(!current->ftp_pasv_mode, "ftp nopasv");
	RCWRITEBOOL(current->ftp_echo_quit, "ftp showquit");
	RCWRITEBOOL(current->ftp_use_cwd, "ftp usecwd");
	RCWRITEBOOL(current->http_limit, "http limit");
	RCWRITEBOOL(current->http_use_expect, "http expect");
	
#undef RCWRITEBOOL
	
	if (current->server.port > 0) { /* Sanity check */
	    if (fprintf (fp, "  port %d\n", current->server.port) == -1) {
		return RC_CORRUPT;
	    }
	}
	
	/* Add the site's URL if one has been supplied. */
	if (current->url) {
	    if (fprintf (fp, "  url %s\n", current->url) == -1) {
		return RC_CORRUPT;
	    }
	}
      
	/* State method */
	switch (current->state_method) {
	case (state_timesize):
	    if (fprintf (fp, "  state timesize\n") == -1) {
		return RC_CORRUPT;
	    }   
	    break;
	case (state_checksum):
	    if (fprintf (fp, "  state checksum\n") == -1) {
		return RC_CORRUPT;
	    }
	    break;
	}
	
	/* Permissions now */
	switch (current->perms) {
	case (sitep_ignore): 
	    if (fprintf (fp, "  permissions ignore\n") == -1) {
		return RC_CORRUPT;
	    }
	    break;
	case (sitep_exec):
	    if (fprintf (fp, "  permissions exec\n") == -1) {
		return RC_CORRUPT;
	    }
	    break;
	case (sitep_all):
	    if (fprintf (fp, "  permissions all\n") == -1) {
		return RC_CORRUPT;
	    }
	    break;
	}
	
        if (current->dirperms) {
            if (fprintf(fp, "  permissions dir\n") == -1) {
                return RC_CORRUPT;
            }
        }

	/* Sym link mode */
	switch (current->symlinks) {
	case (sitesym_ignore): 
	    if (fprintf (fp, "  symlinks ignore\n") == -1) {
		return RC_CORRUPT;
	    }
	    break;
	case (sitesym_follow):
	    if (fprintf (fp, "  symlinks follow\n") == -1) {
		return RC_CORRUPT;
	    }
	    break;
	case (sitesym_maintain):
	    if (fprintf (fp, "  symlinks maintain\n") == -1) {
		return RC_CORRUPT;
	    }
	    break;
	}
	
#define DUMP_FNLIST(list, name) 					  \
do {\
for (item = list; item != NULL; item = item->next)		  	  \
    if (fprintf(fp, "  " name " \"%s%s\"\n", item->haspath?"/":"",	  \
	item->pattern) == -1)						  \
    return RC_CORRUPT;					     \
} while(0)
	
	DUMP_FNLIST(current->excludes, "exclude");
	DUMP_FNLIST(current->asciis, "ascii");
	DUMP_FNLIST(current->ignores, "ignore");
	
#undef DUMP_FNLIST
	
    }
    if (fclose (fp) != 0)
	return RC_CORRUPT;

    return 0;
}
