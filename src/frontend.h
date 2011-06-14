/* 
   sitecopy, manage remote web sites.
   Copyright (C) 1998-2006, Joe Orton <joe@manyfish.co.uk>.
                                                                     
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

/* You MUST implement all of the below functions in the front end.
 * After a site_update call, for each file which is being updated
 * the following calls are made:
 *   if (fe_can_update(file))  {
 *       fe_updating(file);
 *       fe_updated(file, true, NULL) if the update was successful, or
 *       fe_updated(file, false, "error message") otherwise.
 *   }
 * Similarly for site_synch calls.
 * If fe_can_updated(file) returns false, the update of that file
 * will not continue.
 *
 * During a file transfer, whenever a block is written to the remote
 * site, fe_transfer_progress(bytes_transferred, bytes_total) is 
 * called. In other words, for a 10k file, the calls might be:
 *  fe_t_p(1024, 10240), fe_t_p(2048, 10240),
 *  fe_t_p(3072, 10240) ... fe_t_p(10240, 10240)
 * File transfers occur in site_update and site_synch, when uploading
 * and downloading files, respectively.
 */

#ifndef FRONTEND_H
#define FRONTEND_H

#include <ne_ssl.h>
#include <ne_auth.h>

#include "common.h"
#include "sites.h"

typedef enum {
    fe_namelookup,
    fe_connecting,
    fe_connected
} fe_status;

/* Connection Status API.
 *
 * fe_connection() is called to indicate what state the connection is
 * in. Note, the status may bounce between fe_connected and
 * fe_connecting many times during an operation - especially if
 * connected to a WebDAV server which doesn't implement HTTP/1.1
 * persistent connections.
 * info will indicate the hostname being looked up for fe_namelookup,
 * and will be NULL otherwise.
 */
void fe_connection(fe_status status, const char *info);

/* The user is required to authenticate themselves for given context,
 * in the given realm on the given hostname.
 * (The Netscape UI for this is: "Enter username for REALM at HOSTNAME:")
 * realm will be NULL for non-HTTP protocols, so the UI might be better as:
 *  "Enter username for HOSTNAME:" or whatever...
 * Must return:
 *    0:  Success: 
 *       *username must be non-NULL and *password must be non-NULL,
 *       malloc()-allocated memory. The FE MUST NOT free them ever.
 * non-zero: User cancelled operation. *username and *password ignored,
 * and never free()'d if non-NULL.
 */

typedef enum {
    fe_login_server,
    fe_login_proxy
} fe_login_context;

#define FE_LBUFSIZ NE_ABUFSIZ

/* Enter username/password; username and password are fixed-size
 * buffers of size FE_LBUFSIZ. The username field may be pre-filled
 * by the username from the rcfile; otherwise username[0] == '\0'.
 * Contents of password are undefined. */
int fe_login(fe_login_context ctx, const char *realm, const char *hostname,
	     char *username, char *password);

/* Return zero if the given server certificate 'cert', which had a
 * failures mask of 'failures' (NE_SSL_*), should be accepted. */
int fe_accept_cert(const ne_ssl_certificate *cert, int failures);

/* Enter password needed to decrypt given client certificate.  Returns
 * zero on success, non-zero on failure.  'password' is of size
 * FE_LBUFSIZ. */
int fe_decrypt_clicert(const ne_ssl_client_cert *cert, char *password);

int fe_can_update(const struct site_file *file);
void fe_updating(const struct site_file *file);
void fe_updated(const struct site_file *file, int success, const char *error);

/* Also called during updates: */
void fe_setting_perms(const struct site_file *file);
void fe_set_perms(const struct site_file *file, int success, const char *error);

/* For synch mode */
void fe_synching(const struct site_file *file);
void fe_synched(const struct site_file *file, int success, const char *error);

/* For synch and update modes... */
void fe_transfer_progress(off_t progress, off_t total);

/* Called while checksumming remote files, in fetch mode.
 * Note, these are just filenames not site_file *'s, because at the
 * checksumming state, we haven't yet modified the files list. */
void fe_checksumming(const char *filename);
void fe_checksummed(const char *filename, int success, const char *error);


/* For fetch mode - called for each file found remotely */
void fe_fetch_found(const struct site_file *file);

/* To display a non-fatal warning message to the user. 
 * description is the user-friendly description of the warning,
 * subject is typically the filename 
 * reason is the low-level error message.
 * description may contain newlines (\n), but will not have a trailing 
 * new-line.
 * subject and reason will not contain newlines.
 * reason and/or reason may be NULL.
 */
void fe_warning(const char *description, const char *subject,
		       const char *error);

/* Verified - whether the remote file matches or not... 
 match will be one of file_new, file_deleted, file_changed, file_unchanged.
 (NOT file_moved yet)
*/

void fe_verified(const char *fname, enum file_diff match);

#endif /* FRONTEND_H */
