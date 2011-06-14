/* 
   sitecopy, for managing remote web sites.
   Copyright (C) 1998-2002, 2005, Joe Orton <joe@manyfish.co.uk>
                                                                     
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

/* Protocol driver interface, second attempt.
 * The actual driver handles the interface to the remote site.
 */

#ifndef PROTO_H
#define PROTO_H

#include <sys/types.h>

#include <ne_request.h>

#include "sites.h"
#include "common.h"

enum proto_filetype {
    proto_file,
    proto_link,
    proto_dir
};

struct proto_file {
    char *filename;
    enum proto_filetype type;
    off_t size;
    time_t modtime;
    mode_t mode;
    unsigned char checksum[16];
    struct proto_file *next;
};

struct site;

struct proto_driver {

    /* Protocol driver initialization.
     * Returns SITE_* return code.
     *  If you don't return SITE_OK, ->error will be called
     *  to retrieve the error string.  So be sure to initialize
     *  *session to something meaningful in that case too.
     *  finish() will still be called in this case too, to allow
     *  you to clean up properly.
     */
    int (*init)(void **session, struct site *site);

    /* Called when the driver has been finished with */
    void (*finish)(void *session);

    /* Perform the file operations - these should return one of 
     * the PROTO_ codes */
    int (*file_move)(void *session, const char *from, const char *to);
    int (*file_upload)(void *session, const char *local, const char *remote, 
		       int ascii);
    
    /* Conditional file upload: upload given file under the
     * condition that the remote file has the given time and size
     * Returns:
     *   PROTO_OK      if upload was okay
     *   PROTO_ERROR   if upload failed
     *   PROTO_FAILED  if condition is not met.
     */
    int (*file_upload_cond)(void *session,
	const char *local, const char *remote,
	int ascii, time_t t);
    /* Retrieve the remote file modification time and file size */
    int (*file_get_modtime)(void *sess, const char *remote, time_t *modtime);
    int (*file_download)(void *sess, const char *local, const char *remote,
			 int ascii);
    int (*file_read)(void *sess, const char *remote, 
		     ne_block_reader reader, void *userdata);

    int (*file_delete)(void *sess, const char *filename);
    int (*file_chmod)(void *sess, const char *filename, mode_t mode);
    /* Perform the directory operations */
    int (*dir_create)(void *sess, const char *dirname);
    int (*dir_remove)(void *sess, const char *dirname);

    /* Creates a link with given target */
    int (*link_create)(void *session, const char *fn, const char *target);
    /* Changes a link to point to a different target */
    int (*link_change)(void *session, const char *fn, const char *target);
    /* Deletes a link */
    int (*link_delete)(void *session, const char *fn);
    
    /* Fetches the list of files.
     * Returns:
     *   PROTO_OK     if the fetch was successful
     *   PROTO_ERROR  if the fetch failed
     * The files list must be returned in dynamically allocated 
     * memory, which will be freed by the caller.
     */
    int (*fetch_list)(void *sess, const char *dirname, 
		      int need_modtimes, struct proto_file **files);
    /* Returns the last error string used. */
    const char *(*error)(void *sess);
    
    /* Return the default port to use for the server on the given site */
    int (*get_server_port)(struct site *site);

    /* Return the default port to use for the proxy on the given site */
    int (*get_proxy_port)(struct site *site);

    const char *protocol_name; /* The user-visible name for this protocol */
};

/* Callback of type 'sock_progress'. */
void site_sock_progress_cb(void *userdata, off_t progress, off_t total);

#endif /* PROTO_H */
