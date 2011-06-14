/* 
   FTP client implementation
   Copyright (C) 1998-2002, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

#ifndef FTP_H
#define FTP_H

#include <netinet/in.h>

#include "protocol.h"

/* Reply codes - these are returned by internal functions,
 * ftp_exec and ftp_open mainly. */
#define FTP_OK 0
#define FTP_NEEDPASSWORD 1
#define FTP_PASSIVE 2
#define FTP_READY 3
#define FTP_FILEMORE 4
#define FTP_MODTIME 5
#define FTP_SENT 6

#define FTP_CLOSED 101
#define FTP_FILEBAD 102
#define FTP_FAILED 103
#define FTP_LOOKUP 991
#define FTP_CONNECT 992
#define FTP_HELLO 993
#define FTP_LOGIN 994
#define FTP_BROKEN 995
#define FTP_DENIED 996
#define FTP_UNSUPPORTED 997
#define FTP_NOPASSIVE 998
#define FTP_ERROR 999

/* This module contains an FTP client implementation.
 * All commands return FTP_OK on success.
 */

typedef struct ftp_session_s ftp_session;

/* Connection functions */
ftp_session *ftp_init(void);

/* Opens the control connection if necessary.
 * Returns:
 *   FTP_OK on success
 *   FTP_CONNECT on failed socket connect
 *   FTP_HELLO if the greeting message couldn't be read
 *   FTP_LOGIN on failed login
 */
int ftp_open(ftp_session *sess); /* Performs the login procedure */

int ftp_set_server(ftp_session *sess, struct site_host *server);
void ftp_set_passive(ftp_session *sess, int use_passive);
void ftp_set_usecwd(ftp_session *sess, int use_cwd);

const char *ftp_get_error(ftp_session *sess);
int ftp_finish(ftp_session *sess);

/* The commands available */
int ftp_put(ftp_session *sess, 
	     const char *localfile, const char *remotefile, int ascii);
int ftp_put_cond(ftp_session *sess,
		  const char *local, const char *remote, 
		  int ascii, const time_t t);
int ftp_get(ftp_session *sess, const char *localfile, const char *remotefile,
	    int ascii);
int ftp_delete(ftp_session *sess, const char *filename);
int ftp_rmdir(ftp_session *sess, const char *dir);
int ftp_mkdir(ftp_session *sess, const char *dir);
int ftp_chmod(ftp_session *sess, const char *filename, const mode_t mode);
int ftp_move(ftp_session *sess, const char *from, const char *to);
int ftp_fetch(ftp_session *sess, const char *startdir, struct proto_file **files);
int ftp_fetch_modtimes(ftp_session *sess,
		       const char *rotodir, struct proto_file *files);

int ftp_get_modtime(ftp_session *sess, const char *filename, time_t *modtime);

int ftp_read_file(ftp_session *sess, const char *remotefile,
		  ne_block_reader reader, void *userdata);

#endif /* FTP_H */
