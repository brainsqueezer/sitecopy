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
 *  
 */
#include <ne_utils.h>

#include "changes.h"

/* Throughout this file, strdup is used, and not the preferable g_strdup.
 * This is because many of the fields are allocated in the core of
 * sitecopy, and g_free and free are not compatible. I didn't fancy my
 * chances of getting the core of sitecopy dependant upon glib, so only
 * strdup & free are used in this file.
 */

extern struct site *selected_site;
extern gboolean rcfile_saved;
extern GtkWidget *sitecopy;

extern GtkWidget *nooverwrite, *sym_maintain, *perms_exec, *perms_all;
extern GtkWidget *perms_ignore, *sym_ignore, *port, *ftp_mode;

void change_detection_mode(GtkWidget *button, gpointer data)
{
    if ( selected_site->remote_is_different)
      {
	  gnome_error_dialog_parented("Your local site must be in sync with the remote site\nbefore change-detection mode can be changed.", 
				      GTK_WINDOW(sitecopy));
    	  return;
      }

    if (selected_site->state_method == state_checksum)
      selected_site->state_method = state_timesize;
    else
      selected_site->state_method = state_checksum;
    
    /* Restore file info */
    site_readfiles(selected_site);
    NE_DEBUG (DEBUG_GNOME, "**************\nReadfiles done.\n*************");
    site_catchup(selected_site);
    NE_DEBUG (DEBUG_GNOME, "**************\nCatchup done.\n*************");
    site_write_stored_state(selected_site);
    NE_DEBUG (DEBUG_GNOME, "**************\nStored state written.\n*************");
    rescan_selected(TRUE);
}

void change_port(GtkWidget * spinbutton, gpointer data)
{
    int p = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinbutton));
    selected_site->server.port = p;
}

void change_lowercase(GtkWidget * checkbutton, gpointer data)
{
    selected_site->lowercase = GTK_TOGGLE_BUTTON(checkbutton)->active;
}

void change_safemode(GtkWidget * checkbutton, gpointer data)
{
    /* This makes the nooverwrite checkbutton insensitive,
       * as it can't be used with safe mode.
     */
    gboolean active = GTK_TOGGLE_BUTTON(checkbutton)->active;
    g_assert(nooverwrite != NULL);

    if (active)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nooverwrite), FALSE);

    gtk_widget_set_sensitive(nooverwrite, !active);
    selected_site->safemode = active;
}

void change_host_name(GtkWidget * entry, gpointer data)
{
    char *new_hostname = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));

    xsc_replace_string(&(selected_site->server.hostname), new_hostname);
    
    rcfile_saved = FALSE;
}

void change_protocol(GtkWidget * menu_item, gpointer proto)
{
    gchar *protoc;
    extern struct proto_driver ftp_driver;
#ifdef USE_DAV
    extern struct proto_driver dav_driver;
#endif

    protoc = (gchar *) proto;
    if (strcmp(protoc, "ftp") == 0) {
	selected_site->protocol = siteproto_ftp;
	/* Change protocol to ftp */
	selected_site->driver = &ftp_driver;
	/* We can't do sym links with ftp */
	gtk_widget_set_sensitive(sym_maintain, FALSE);
	/* So make sure we ignore them */
	if (selected_site->symlinks == sitesym_maintain)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sym_ignore), TRUE);
	/* Let's set an appropriate default port */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 21);
#ifdef USE_DAV
	/* If using DAV caused the passive FTP checkbox to get de-sensitived,
	 * then make sure it's now sensitive.
	 */

	g_assert(ftp_mode != NULL);
	g_assert(perms_exec != NULL);
	g_assert(perms_all != NULL);
	g_assert(perms_ignore != NULL);
	g_assert(sym_maintain != NULL);
	
	gtk_widget_set_sensitive(ftp_mode, TRUE);
	gtk_widget_set_sensitive(perms_exec, TRUE);
	gtk_widget_set_sensitive(perms_all, TRUE);

    } else if (strcmp(protoc, "dav") == 0) {
	selected_site->protocol = siteproto_dav;
	/* DAV */
	selected_site->driver = &dav_driver;
	gtk_widget_set_sensitive(GTK_WIDGET(ftp_mode),
				 FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ftp_mode),
				     FALSE);
	/* Set an appropriate port */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(port), 80);

	gtk_widget_set_sensitive(sym_maintain, TRUE);
	gtk_widget_set_sensitive(perms_exec, FALSE);
	gtk_widget_set_sensitive(perms_all, FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(perms_ignore), TRUE);
#endif
    }
    rcfile_saved = FALSE;
}

void change_username(GtkWidget * entry, gpointer data)
{
    gchar *username;

    username = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    xsc_replace_string(&(selected_site->server.username), username);

    rcfile_saved = FALSE;
}

void change_password(GtkWidget * entry, gpointer data)
{
    gchar *new_password = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    xsc_replace_string(&(selected_site->server.password), new_password);
    
    rcfile_saved = FALSE;
}

void change_passive_ftp(GtkWidget * toggle, gpointer data)
{
    selected_site->ftp_pasv_mode = GTK_TOGGLE_BUTTON(toggle)->active;
    rcfile_saved = FALSE;
}

void change_nooverwrite(GtkWidget * toggle, gpointer data)
{
    selected_site->nooverwrite = GTK_TOGGLE_BUTTON(toggle)->active;
    rcfile_saved = FALSE;
}

void change_delete(GtkWidget * toggle, gpointer data)
{
    if (GTK_TOGGLE_BUTTON(toggle)->active) {
	selected_site->nodelete = FALSE;
    } else {
	selected_site->nodelete = TRUE;
    }
    rcfile_saved = FALSE;
}

void change_move_status(GtkWidget * toggle, gpointer data)
{
    selected_site->checkmoved = GTK_TOGGLE_BUTTON(toggle)->active;
    rcfile_saved = FALSE;
}

void change_http_expect(GtkWidget * toggle, gpointer data)
{
    selected_site->http_use_expect = GTK_TOGGLE_BUTTON(toggle)->active;
    rcfile_saved = FALSE;
}

void change_http_limit(GtkWidget * toggle, gpointer data)
{
    selected_site->http_limit = GTK_TOGGLE_BUTTON(toggle)->active;
    rcfile_saved = FALSE;
}

void change_perms(GtkWidget * menu_item, gpointer perm_data)
{
    gchar *perm_set;

    perm_set = (gchar *) perm_data;
    if (strcmp(perm_set, "ignore") == 0) {
	selected_site->perms = sitep_ignore;
    } else if (strcmp(perm_set, "exec") == 0) {
	selected_site->perms = sitep_exec;
    } else if (strcmp(perm_set, "all") == 0) {
	selected_site->perms = sitep_all;
    }
    rcfile_saved = FALSE;
}

void change_sym_mode(GtkWidget * menu_item, gpointer sym_data)
{
    gchar *sym_info;

    sym_info = (gchar *) sym_data;
    if (strcmp(sym_info, "ignore") == 0) {
	selected_site->symlinks = sitesym_ignore;
    } else if (strcmp(sym_info, "follow") == 0) {
	selected_site->symlinks = sitesym_follow;
    } else if (strcmp(sym_info, "maintain") == 0) {
	selected_site->symlinks = sitesym_maintain;
    }
    rcfile_saved = FALSE;
}

/* This is for the 'change' event. */

void set_local_dir(GtkWidget * entry, gpointer data)
{
    gchar *new_local_dir = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    
    xsc_replace_string(&(selected_site->local_root_user), new_local_dir);
    
    if (*selected_site->local_root_user == '~')
	selected_site->local_isrel = TRUE;
    else
	selected_site->local_isrel = FALSE;

    rcfile_saved = FALSE;
}


/** FIXME: Sort this, and make sure the event it's connected to is right.
 * Currently connected to the focus-out-event.
 */
void change_local_dir(GtkWidget * entry, gpointer data)
{
    set_local_dir(entry, data);
    rescan_selected(FALSE);
}

void change_remote_dir(GtkWidget * entry)
{
    gchar *new_remote_dir = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    
    xsc_replace_string(&(selected_site->remote_root_user), new_remote_dir);

    if (*new_remote_dir == '~') {
	selected_site->remote_isrel = TRUE;
    } else {
	selected_site->remote_isrel = FALSE;
    }

    if (verifysite_gnome(selected_site) != 0) {
	NE_DEBUG(DEBUG_GNOME, "site did not verify correctly.");
	return;
    }
    if ((*new_remote_dir != '~') && (*new_remote_dir != '/')) {
	gfe_status("Warning! The remote directory must begin with either a '/' or a '~/'.\n Use ~/ to denote a directory relative to your logon directory");
	return;
    }
    /* Do we want to enquire about a site_fetch_list if this changes? */
    rcfile_saved = FALSE;
}

void change_url(GtkWidget * entry, gpointer data)
{
    char *new_url= strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    
    xsc_replace_string(&(selected_site->url), new_url);
    rcfile_saved = FALSE;
}
