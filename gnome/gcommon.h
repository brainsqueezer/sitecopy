/* 
 *      XSitecopy, for managing remote web sites with a GNOME interface.
 *      Copyright (C) 2000, Lee Mallabone <lee@fonicmonkey.net>
 *                                                                        
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *     
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *     
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GNOME_FE_COMMON_H
#define GNOME_FE_COMMON_H

#include <gnome.h>
#include <glade/glade.h>

#include "common.h"
#include "sites.h"
#include "frontend.h"

#define glade_widget(xml,string) glade_xml_get_widget(xml,string)

enum view_type {
    gtk_slim, gtk_full
};

gboolean xsc_replace_string(char **oldString, char *replacementString);
/* For synch and update modes */
gchar *get_glade_filename(void);
int fe_gtk_question(char *question, GnomeReplyCallback yes_action);
void gfe_status(const char *message);
/*
void fe_transfer_progress(size_t progress, size_t total);
void fe_warning(const char *description, const char *subject,
		const char *error);
*/
/*
void fe_connection( sock_status status, const char *info );
int  fe_login( fe_login_context ctx, const char *realm, const char *hostname,
	      char **username, char **password );
int fe_can_update(const struct site_file *file);
void fe_updating(const struct site_file *file);
void fe_updated(const struct site_file *file, int success,
		const char *error);
*/

int verifysite_gnome(struct site *a_site);

/* These have no yet been implemented.
 * joe: don't need these
void fe_synching(const struct site_file *file);
void fe_synched(const struct site_file *file, int success,
		const char *error);
*/

/* joe: don't need these.
void fe_checksumming(const char *filename);
void fe_checksummed(const char *filename, int success,
		    const char *error);
void fe_setting_perms(const struct site_file *file);
void fe_set_perms(const struct site_file *file, int success,
		  const char *error);
*/

void set_status_after_operation(int return_code,
				GtkLabel *infoLabel);
#endif
