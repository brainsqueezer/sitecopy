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

/* This file implements all the functions required by sitecopy in
 * src/frontend.h and also provides a few convenience functions for fun.
 */

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "dirname.h"
#include "rcfile.h"
#include "gcommon.h"

#include "config.h"

static gint button;

GtkWidget *connection_label = NULL;

extern sem_t *update_semaphore;

extern struct site *selected_site;
extern GtkWidget *error_log_list, *error_log_window;

/* For the 'main' upload window */
extern GtkWidget *main_progressbar, *job_progressbar, *error_button;
extern GtkWidget *status_label, *op_label, *file_label, *dir_label;
extern GtkWidget *begin_button;

extern struct site *all_sites;

/* This value is what makes the entire "job" progress bar possible. */
float upload_total, uploaded_bytes = 0.0;

extern GtkWidget *sitecopy;

/*************************************************/

/* Returns TRUE if *oldString _was_ replaced with replacementString, or
 * FALSE if NULL was substituted.
 */
gboolean xsc_replace_string(char **oldString, char *replacementString)
{

    /* Check the value of replacementString. If it's NULL, or an empty
     * string, then set *oldString to be NULL also.
     */
   if ( (!replacementString) || (strlen(replacementString) < 1) )
     {
	*oldString = NULL;
	return FALSE;
     }

    /* If oldString is valid, then optionally free any memory used by
     * *oldString, and then point oldString to replacementString
     */
   if (*oldString)
     {
	free(*oldString);
     }
   *oldString = replacementString;
   return TRUE;
}

/* Searches for the glade file, and loads a static variable with the location.
 * Once found, the variable is returned straight away.
 */

gchar *get_glade_filename(void)
{
   static gchar *theFilename = NULL;

   if (theFilename)
     return theFilename;

    /* Search for the glade file in $(datadir)/xsitecopy/, ./ and ../ */
   theFilename = gnome_datadir_file("xsitecopy/sitecopy-dialogs.glade");
   if ((!theFilename) || (!g_file_exists(theFilename)))
     {
	g_free(theFilename);
	theFilename = g_strdup("sitecopy-dialogs.glade");
	if (!g_file_exists(theFilename))
	  {
	     g_free(theFilename);
	     theFilename = g_strdup("../sitecopy-dialogs.glade");
	     if (!g_file_exists(theFilename))
	       {
		  g_error("Can not find sitecopy-dialogs.glade, exiting.\n");
		  theFilename = NULL;
	       }
	  }
     }
   return theFilename;
}

int fe_gtk_question(char *question, GnomeReplyCallback yes_action)
{
   GtkWidget *question_window;
   question_window = gnome_app_question(GNOME_APP(sitecopy),
					(const gchar *) question,
					yes_action, NULL);
   gtk_widget_show(question_window);
   return 1;
}

void fe_transfer_progress(off_t progress, off_t total)
{
   float div1, div2;

    /* Update the current file's progress */
   div1 = (float) progress;
   div2 = (float) total;
   gdk_threads_enter();
    /* Let's not do anything silly like divide by zero. */
   if (div2)
     gtk_progress_bar_update(GTK_PROGRESS_BAR(main_progressbar), div1 / div2);

    /* Update total job progress */
   gtk_progress_bar_update(GTK_PROGRESS_BAR(job_progressbar),
			   (uploaded_bytes + (float) progress) / (float) upload_total);
   gdk_threads_leave();

}

void gfe_status(const char *message)
{
   gnome_app_message(GNOME_APP(sitecopy), (const gchar *) message);
}

void fe_warning(const char *description, const char *subject,
		const char *error)
{
/*    gnome_warning_dialog(description);*/
}

/* The user is required to authenticate themselves for given context,
 * in the given realm on the given hostname. This is achieved by forming an
 * appropriate label givne the context, realm and hostname. A dialog is
 * then constructed with libglade. run_and_close() is performed on the
 * dialog, then some sanity checking for what was entered. Finally, the
 * 'out' parameters of the function are set, the gtk+ lock is removed, and
 * the appropriate value returned.
 *
 */

/* Thanks to David Knight. This function was copied almost verbatim from the screem upload wizard.
 */

int
  fe_login( fe_login_context ctx, const char *realm, const char *hostname,
	   char *username, char *password )
{
   GtkWidget *widget;
   gchar *tmp;
   gchar const *server_type;
   GladeXML *login_xml;

   g_print("fe_login");
   gdk_threads_enter();

   switch (ctx)
     {
      case fe_login_server:
	server_type = "remote";
	break;
      case fe_login_proxy:
	server_type = "proxy";
	break;
#ifndef NDEBUG
      default:
	fprintf(stderr, "fe_login: unknown ctx %d.  Aborting.\n", (int) ctx);
	abort();
#endif /* !NDEBUG */
     }

    /* Form a sane prompt for details regardless of whether a realm is
     * provided or not.
     */
   tmp = g_strdup_printf("The %s server requires authentication.\n\n"
			 "Please enter details for %s%s%s.",
			 server_type,
			 realm?realm:"",
			 realm?" at ":"",
			 hostname);

   login_xml = glade_xml_new((const char *)get_glade_filename(),
			     "user_pass_dialog");
   gtk_label_set(GTK_LABEL(glade_widget(login_xml, "user_pass_prompt_label")),
		 tmp);

   g_free(tmp);

   if( username[0] != '\0' )
     {
	widget = glade_xml_get_widget( login_xml, "user_entry" );
	gtk_entry_set_text( GTK_ENTRY( widget ), username );
     }

   widget = glade_xml_get_widget(login_xml, "user_pass_dialog");
   gtk_window_set_modal(GTK_WINDOW(widget), TRUE);
   gtk_widget_show_all(widget);
   glade_xml_signal_autoconnect(login_xml);

   button = -1;
   gdk_threads_leave();

   sem_wait(update_semaphore);

   if( button != 0 )
     {
        /* canceled */
	gtk_widget_destroy( widget );
	return -1;
     }
   gdk_threads_enter();
   gtk_window_set_modal(GTK_WINDOW(widget), FALSE);
   widget = glade_xml_get_widget( login_xml, "user_entry" );
   strncpy(username, gtk_entry_get_text( GTK_ENTRY( widget ) ),
	   FE_LBUFSIZ);

   widget = glade_xml_get_widget( login_xml, "pass_entry" );
   strncpy(password, gtk_entry_get_text( GTK_ENTRY( widget ) ),
	   FE_LBUFSIZ);

   widget = glade_xml_get_widget( login_xml, "user_pass_dialog" );
   gtk_widget_destroy( widget );

   gdk_threads_leave();

   return 0;
}


void fe_login_clicked( GnomeDialog *dialog, gint number )
{
           button = number;
           sem_post(update_semaphore);
}

void fe_login_close( GnomeDialog *dialog )
{
           sem_post(update_semaphore);
}


void fe_connection( fe_status status, const char *info)
{
   char *tmp;

   gdk_threads_enter();
   switch (status)
     {
      case (fe_namelookup):
	if (info)
	  {
	     tmp = g_strdup_printf("Looking up %s...", info);
	     gtk_label_set(GTK_LABEL(connection_label), tmp);
	     g_free(tmp);
	  }
	else
	  {
	     gtk_label_set(GTK_LABEL(connection_label), "Looking up hostname...");
	  }
	break;
      case (fe_connecting):
	gtk_label_set(GTK_LABEL(connection_label), "Attempting to connect...");
	break;
      case (fe_connected):
	gtk_label_set(GTK_LABEL(connection_label), "Connected.");
	break;
     }
   gdk_threads_leave();
}

int fe_can_update(const struct site_file *file)
{
   g_print("Confirmation given to upload file %s.\n",
	   file->local.filename);
   return TRUE;
}

void fe_verified(const char *fname, enum file_diff match)
{
    /* TODO */
}

void fe_updating(const struct site_file *file)
{
   char *file_status;

   gdk_threads_enter();
   file_status = g_strdup_printf("Commiting updates to %s...",
				 selected_site->server.hostname);
   gtk_label_set(GTK_LABEL(status_label), file_status);
    /* Check this */
   g_free(file_status);
   if (file->type == file_dir)
     {
	if (file->diff == file_new)
	  {
	     gtk_label_set(GTK_LABEL(op_label), "Creating directory...");
	     gtk_label_set(GTK_LABEL(file_label), file_name(file));
	    /*         gtk_label_set (GTK_LABEL (dir_label), file->directory); */
	     gtk_label_set(GTK_LABEL(dir_label), "");
	  }
	else
	  {
	    /* can we move dirs yet? */
	     gtk_label_set(GTK_LABEL(op_label), "Deleting directory...");
	     gtk_label_set(GTK_LABEL(dir_label), "");
	  }
     }
   else
     {
	switch (file->diff)
	  {
	   case file_changed:
	     gtk_label_set(GTK_LABEL(op_label), "Uploading...");
	     gtk_label_set(GTK_LABEL(file_label), file_name(file));
	     gtk_label_set(GTK_LABEL(dir_label), dir_name(file_name(file)));
	     break;
	   case file_new:
	     gtk_label_set(GTK_LABEL(op_label), "Uploading...");
	     gtk_label_set(GTK_LABEL(file_label), file_name(file));
	     gtk_label_set(GTK_LABEL(dir_label), dir_name(file_name(file)));
	     break;
	   case file_deleted:
	     gtk_label_set(GTK_LABEL(op_label), "Deleting...");
	     gtk_label_set(GTK_LABEL(file_label), file_name(file));
	     gtk_label_set(GTK_LABEL(dir_label), "");
	     break;
	   case file_moved:
	     gtk_label_set(GTK_LABEL(op_label), "Moving...");
	     gtk_label_set(GTK_LABEL(file_label), file_name(file));
	    /* FIXME: Check this, I think it's dodgy. */
	     gtk_label_set(GTK_LABEL(dir_label), dir_name(file_name(file)));
	     break;
	   case file_unchanged:
	     gtk_label_set(GTK_LABEL(op_label), "ARG! The file hasn't changed, we shouldn't be doing anything!");
	  }
     }
   gdk_threads_leave();
}

/* Once a file has been updated, any errors with it are recorded in the
 * error list, and the progress bar is reset.
 */
void fe_updated(const struct site_file *file, int success,
		const char *error)
{
   gchar *error_item[2];

   gdk_threads_enter();

   if (!success)
     {

        g_assert(error!=NULL);

        error_item[0] = file_name(file);
	error_item[1] = g_strdup(error);

	gtk_clist_append(GTK_CLIST(error_log_list), error_item);

        NE_DEBUG(DEBUG_GNOME, "Error \"%s\" with file: %s\n",
	      error, file_name(file));
	g_free(error_item[1]);
     }
   gtk_progress_bar_update(GTK_PROGRESS_BAR(main_progressbar), 0.0);

   /* If the file exists locally, update how many bytes have globally
    * been updated */
   if (file->local.filename)
     uploaded_bytes += (float) file->local.size;

   gdk_threads_leave();
}

int verifysite_gnome(struct site *a_site)
{
   int ret = rcfile_verify(a_site);
   if (!ret)
     return 0;

   switch (ret)
     {
      case SITE_NOSERVER:
	gnome_error_dialog("Server not specified.");
	break;
      case SITE_NOREMOTEDIR:
	gnome_error_dialog("Remote directory not specified.");
	break;
      case SITE_NOLOCALDIR:
	gnome_error_dialog("Local directory not specified.");
	break;
      case SITE_ACCESSLOCALDIR:
	gnome_error_dialog("Could not read local directory.");
	break;
      case SITE_INVALIDPORT:
	gnome_error_dialog("Invalid port.");
	break;
      case SITE_NOMAINTAIN:
	gnome_error_dialog("The chosen protocol cannot maintain symbolic links");
	break;
      case SITE_NOREMOTEREL:
	gnome_error_dialog("The chosen protocol cannot use a relative remote directory.");
	break;
      case SITE_NOPERMS:
	gnome_error_dialog("The protocol you are attempting to use does\nnot currently support maintaining permissions.");
	break;
      case SITE_NOLOCALREL:
	gnome_error_dialog("Could not use a 'relative' local directory");
      default:
	gnome_error_dialog("There was an undetermined problem verifying the correctness of your site definition. Please report this to the maintainer.");
	break;
     }
   return ret;
}

/* Coming soon... */
void fe_synching(const struct site_file *file)
{
}

void fe_synched(const struct site_file *file, int success,
		const char *error)
{
}

void fe_setting_perms(const struct site_file *file)
{
}
void fe_set_perms(const struct site_file *file, int success,
		  const char *error)
{
}

/* This function keeps the handling of return codes from site_update,
 * site_fetch and site_resync consistant, as they all return the same
 * potential values.
 */

void set_status_after_operation(int return_code,
				GtkLabel *infoLabel)
{
   g_assert(infoLabel != NULL);

   switch (return_code)
     {
      case SITE_OK:
	gtk_label_set(GTK_LABEL(infoLabel), "Operation succesfully completed.");
	site_write_stored_state(selected_site);
	NE_DEBUG (DEBUG_GNOME, "Site fetch was successful.\n");
	break;
      case SITE_LOOKUP:
	gtk_label_set(GTK_LABEL(infoLabel), "Host name lookup failed.");
	NE_DEBUG (DEBUG_GNOME, "lookup failed\n");
	break;
      case SITE_PROXYLOOKUP:
	gtk_label_set(GTK_LABEL(infoLabel), "Host name lookup for the proxy server failed.");
	NE_DEBUG (DEBUG_GNOME, "lookup failed\n");
	break;
      case SITE_CONNECT:
	gtk_label_set(GTK_LABEL(infoLabel), "Could not connect to remote site.");
	NE_DEBUG (DEBUG_GNOME, "no connection.\n");
	break;
      case SITE_ERRORS:
	gtk_label_set(GTK_LABEL(infoLabel), "There were errors during the transfer.");
	NE_DEBUG (DEBUG_GNOME, "no connection.\n");
	break;
      case SITE_AUTH:
	gtk_label_set(GTK_LABEL(infoLabel), "Could not authenticate you with the remote server.");
	NE_DEBUG (DEBUG_GNOME, "Could not authenticate\n");
	break;
      case SITE_PROXYAUTH:
	gtk_label_set(GTK_LABEL(infoLabel), "Could not authenticate you with the proxy server.");
	NE_DEBUG (DEBUG_GNOME, "Could not do proxy authenticate\n");
	break;
      case SITE_FAILED:
	gtk_label_set(GTK_LABEL(infoLabel), "The operation failed. No reason was given.");
	NE_DEBUG (DEBUG_GNOME, "Fetch failed.\n");
	break;
      case SITE_UNSUPPORTED:
	gtk_label_set(GTK_LABEL(infoLabel), "This operation is unsupported for the selected protocol.");
	NE_DEBUG (DEBUG_GNOME, "NOt implemented!\n");
	break;
      case SITE_ABORTED:
	gtk_label_set(GTK_LABEL(infoLabel), "Operation aborted.");
	NE_DEBUG (DEBUG_GNOME, "Um. We got aborted HERE?!\n");
	break;
      default:
	gtk_label_set(GTK_LABEL(infoLabel), "This should never appear. Please contact the maintainer.");
	NE_DEBUG (DEBUG_GNOME, "default.\n");
     }
}

