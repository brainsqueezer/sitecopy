/* 
   sitecopy FTP protocol driver module
   Copyright (C) 2000-2005, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

#include <ne_socket.h>

#include "protocol.h"
#include "ftp.h"

static int f2s(int errcode)
{
    switch (errcode) {
    case FTP_OK:
	return SITE_OK;
    case FTP_LOOKUP:
	return SITE_LOOKUP;
    case FTP_CONNECT:
	return SITE_CONNECT;
    case FTP_FAILED:
	return SITE_FAILED;
    case FTP_LOGIN:
	return SITE_AUTH;
    default:
	return SITE_ERRORS;
    }
}    

static int get_server_port(struct site *site)
{
    return 21;
}

static int init(void **session, struct site *site) 
{
    int ret;
    ftp_session *sess = ftp_init();
    ret = ftp_set_server(sess, &site->server);
    if (ret == FTP_OK) {
	if (site->ftp_pasv_mode) {
	    ftp_set_passive(sess, 1);
	}
	if (site->ftp_use_cwd) {
	    ftp_set_usecwd(sess, 1);
	}
	ret = ftp_open(sess);
    }
    *session = sess;
    /* map it to a SITE_* code. */
    ret = f2s(ret);
    if (ret == SITE_ERRORS)
	ret = SITE_FAILED;
    return ret;
}

static void finish(void *session)
{
    ftp_session *sess = session;
    ftp_finish(sess);
}

static int file_move(void *session, const char *from, const char *to)
{
    ftp_session *sess = session;
    return f2s(ftp_move(sess, from, to));
}

static int file_upload(void *session, const char *local, const char *remote, int ascii)
{
    ftp_session *sess = session;
    return f2s(ftp_put(sess, local, remote, ascii));
}

static int file_upload_cond(void *session,
			    const char *local, const char *remote,
			    int ascii, time_t t)
{
    ftp_session *sess = session;
    return f2s(ftp_put_cond(sess, local, remote, ascii, t));
}

static int file_get_modtime(void *session, const char *remote, time_t *modtime)
{
    ftp_session *sess = session;
    return f2s(ftp_get_modtime(sess, remote, modtime));
}
    
static int file_download(void *session, const char *local, const char *remote,
			 int ascii)
{
    ftp_session *sess = session;
    return f2s(ftp_get(sess, local, remote, ascii));
}

static int file_read(void *session, const char *remote, 
		     ne_block_reader reader, void *userdata)
{
    ftp_session *sess = session;
    return f2s(ftp_read_file(sess, remote, reader, userdata));
}

static int file_delete(void *session, const char *filename)
{
    ftp_session *sess = session;
    return f2s(ftp_delete(sess, filename));
}

static int file_chmod(void *session, const char *filename, mode_t mode)
{
    ftp_session *sess = session;
    return f2s(ftp_chmod(sess, filename, mode));
}

static int dir_create(void *session, const char *dirname)
{
    ftp_session *sess = session;
    return f2s(ftp_mkdir(sess, dirname));
}

static int dir_remove(void *session, const char *dirname)
{
    ftp_session *sess = session;
    return f2s(ftp_rmdir(sess, dirname));
}

static int fetch_list(void *session, const char *dirname, int need_modtimes,
		      struct proto_file **files) 
{
    ftp_session *sess = session;
    int ret;
    
    ret = ftp_fetch(sess, dirname, files);

    if (ret == FTP_OK && need_modtimes) {
	ret = ftp_fetch_modtimes(sess, dirname, *files);
    }

    return f2s(ret);
}

static const char *error(void *session)
{
    ftp_session *sess = session;
    return ftp_get_error(sess);
}

/* The protocol drivers */
const struct proto_driver ftp_driver = {
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
    fetch_list,
    error,
    get_server_port,
    get_server_port,
    "FTP"
};
