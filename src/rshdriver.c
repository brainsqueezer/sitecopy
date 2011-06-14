/* 
   sitecopy rsh/rcp protocol driver module
   Copyright (C) 2000-2005, Joe Orton <joe@manyfish.co.uk>
   Copyright (C) 2003, Nobuyuki Tsuchimura <tutimura@nn.iij4u.or.jp>

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
 * - At least fork/exec to save a sh process
 * - Do something with stdout
 * - Lee M says ssh will prompt for passwords on stdout, so we'd have to 
 *   parse the output and frig appropriately.
 * - Dave K: actually it prompts on stderr
 */

#include <config.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <ne_alloc.h>
#include <ne_string.h>
#include <ne_socket.h>

#include "protocol.h"
#include "lsparser.h"

typedef struct {
    struct site *site;
    const char *rsh_cmd, *rcp_cmd;
    char buf[BUFSIZ];
    char err[BUFSIZ];
    FILE *fp;
} rsh_session;

enum rcommand { RCP, RSH, RSH_PIPE_READ, RSH_PIPE_WRITE };

static int run_rcmd(enum rcommand rcmd,
                    rsh_session *sess, const char *template, ...) 
    ne_attribute((format (printf, 3, 4)));

static int run_rcmd(enum rcommand rcmd,
                    rsh_session *sess, const char *template, ...) 
{
    va_list params;
    char *cmd;
    size_t len;
    char *username = sess->site->server.username;

    va_start(params, template);
    len = ne_vsnprintf(sess->buf, BUFSIZ, template, params);
    va_end(params);

    if (rcmd == RCP) {
        cmd = ne_concat(sess->rcp_cmd, "  2>/dev/null ", sess->buf, NULL);
    } else if (username) {
        cmd = ne_concat(sess->rsh_cmd, " -l ", username, "  2>/dev/null ", 
                        sess->site->server.hostname, " ", sess->buf, NULL);
    } else {
        cmd = ne_concat(sess->rsh_cmd, " 2>/dev/null ", sess->site->server.hostname, 
                        " ", sess->buf, NULL);
    }

    NE_DEBUG(DEBUG_RSH, "rcmd: %s\n", cmd);
    
    if (rcmd == RSH_PIPE_READ || rcmd == RSH_PIPE_WRITE) {
        sess->fp = popen(cmd, rcmd == RSH_PIPE_READ ? "r" : "w");
        return sess->fp != NULL ? SITE_OK : SITE_FAILED;
    } else {
        return system(cmd) == 0 ? SITE_OK : SITE_FAILED;
    }
    
}

static int run_finish(rsh_session *sess)
{
    return pclose(sess->fp) == 0 ? SITE_OK : SITE_FAILED;
}

static int init(void **session, struct site *site)
{
    rsh_session *sess = ne_calloc(sizeof *sess);
    *session = sess;
    if (site->rsh_cmd) {
	sess->rsh_cmd = site->rsh_cmd;
    } else {
	sess->rsh_cmd = "rsh";
    }
    if (site->rcp_cmd) {
	sess->rcp_cmd = site->rcp_cmd;
    } else {
	sess->rcp_cmd = "rcp";
    }
    sess->site = site;
    return SITE_OK;
}    

static void finish(void *session) {
    rsh_session *sess = session;
    free(sess);
}

static int file_move(void *session, const char *from, const char *to) {
    rsh_session *sess = session;
    return run_rcmd(RSH, sess, "mv '%s' '%s'", from, to);
}

static int file_upload(void *session, const char *local, const char *remote, 
			int ascii) {
    rsh_session *sess = session;

    if (sess->site->server.username) {
	return run_rcmd(RCP, sess, "'%s' '%s@%s:%s'",
                        local, sess->site->server.username,
                        sess->site->server.hostname, remote);
    }
    else {
	return run_rcmd(RCP, sess, "'%s' '%s:%s'", local, 
                        sess->site->server.hostname, remote);
    }
}

static int file_upload_cond(void *session,
			    const char *local, const char *remote,
			    int ascii, time_t t) {
    return SITE_UNSUPPORTED;
}

static int file_get_modtime(void *sess, const char *remote, time_t *modtime)
{
    return SITE_UNSUPPORTED;
}
    
static int file_download(void *sess, const char *local, const char *remote,
			 int ascii)
{
    return SITE_UNSUPPORTED;
}


static int file_read(void *session, const char *remote,
                     ne_block_reader reader, void *userdata) {
    rsh_session *sess = session;
    size_t len;

    if (run_rcmd(RSH_PIPE_READ, sess, "cat '%s'", remote) != SITE_OK)
        return SITE_FAILED;
    
    while ((len = fread(sess->buf, 1, BUFSIZ, sess->fp)) > 0) {
        reader(userdata, sess->buf, len);
    }
     
    return run_finish(sess);
}

static int file_delete(void *session, const char *filename)
{
    rsh_session *sess = session;
    return run_rcmd(RSH, sess, "rm '%s'", filename);
}

static int file_chmod(void *session, const char *fname, const mode_t mode)
{
    rsh_session *sess = session;
    return run_rcmd(RSH, sess, "chmod %03o '%s'", mode, fname);
}

static int dir_create(void *session, const char *dirname)
{
    rsh_session *sess = session;
    return run_rcmd(RSH, sess, "mkdir '%s'", dirname);
}

static int dir_remove(void *session, const char *dirname)
{
    rsh_session *sess = session;
    return run_rcmd(RSH, sess, "rmdir '%s'", dirname);
}

static const char *error(void *session) {
    return "An error occurred.";
}

static int get_dummy_port(struct site *site)
{
    return 0;
}

static int rsh_fetch(rsh_session *sess, const char *startdir, 
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
            NE_DEBUG(DEBUG_RSH, "Could not parse line.\n");
            success = SITE_ERRORS;
            break;

        default:
            ls_pflist_add(list, &tail, &lfile, res);
            break;
	}
    }

    ls_destroy(lsctx);

    NE_DEBUG(DEBUG_RSH, "Fetch finished successfully.\n");
    return run_finish(sess);
}

static int fetch_list(void *session, const char *dirname, int need_modtimes,
                     struct proto_file **files) {
    rsh_session *sess = session;
    int ret;

    ret = run_rcmd(RSH_PIPE_READ, sess, "ls -la '%s'", dirname);
    if (ret == SITE_OK) {
        ret = rsh_fetch(sess, dirname, files);
    }

    return ret;
}

/* The protocol drivers */
const struct proto_driver rsh_driver = {
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
    "rsh/rcp"
};
