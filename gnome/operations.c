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

#include <stdlib.h>
#include "operations.h"

extern struct site *selected_site;
extern GtkWidget *error_log_list, *error_log_window;

extern struct site *all_sites;
extern float uploaded_bytes, upload_total;

/* For the 'main' upload window */
GtkWidget *upload_window, *main_progressbar, *job_progressbar, *error_button;
GtkWidget *status_label, *op_label, *file_label, *dir_label;
GtkWidget *begin_button, *close_button, *kill_button, *keep_going_button;
pthread_t update_thread_id = 0;

volatile int in_critical_section = 0, want_abort = 0;
sigjmp_buf abort_buf;

/* This sets up a signal handler and provides a wrapper around
 * site_ transfer operations.
 * It uses setjmp and longjmp to do backward error recovery. 
 * This allows the aborting of operations without having to 
 * destroy threads, which is A Good Thing, because all 
 * site_updates use the same (single) thread. :)
 */


/* Signal handler */
void handle_abort( int sig ) {
   if( in_critical_section ) {
      /* Can't abort now, so remember we want to for later */
      want_abort = 1;
   } else {
      do_abort();
   }
}

/* Actually abort the update */
void do_abort(void) {
    printf("performing longjmp...\n");
    want_abort = 0;
    siglongjmp( abort_buf, 1 );
}

/* Enter critical section */
void fe_disable_abort( struct site *site ) {
  in_critical_section = 1;
}

/* Leave critical section */
void fe_enable_abort( struct site *site ) {
  in_critical_section = 0;
  /* Carry out the abort if we were aborted while in the
   * critical section */
  if( want_abort ) {
     do_abort();
  }
}

int my_abortable_transfer_wrapper(struct site *site,
				  enum site_op operation ) 
{
   int ret;
   signal( SIGUSR1, handle_abort );
   if( !sigsetjmp( abort_buf, 1 ) ) {
       /* Normal execution */
       switch (operation)
	 {
	  case site_op_update:
	     ret = site_update(site);
	     break;
	  case site_op_fetch:
	     ret = site_fetch(site);
	     break;
#if 0 /* Can't happen in current code. */
	  case site_op_resync:
	     ret = site_synch(site);
	     break;
#endif
#ifndef NDEBUG
	  default:
	     fprintf(stderr, "my_abortable_transfer_wrapper: unknown operation %d.  Aborting.\n", operation);
	     abort();
#endif /* !NDEBUG */
	 }
   } else {
       /* The update was aborted */
       ret = SITE_ABORTED;
   }
   signal( SIGUSR1, SIG_IGN );
   return ret;
}


void close_main_update_window(GtkButton * button, gpointer data)
{
    extern GtkWidget *connection_label;
    gtk_window_set_modal(GTK_WINDOW(upload_window), FALSE);
    gtk_widget_destroy(upload_window);
    connection_label = NULL;
    return;
}

void abort_site_update(GtkWidget * button, gpointer data)
{
    extern pthread_t update_tid;
    gtk_label_set(GTK_LABEL(status_label), "Aborting...");
    pthread_kill(update_tid, SIGUSR1);
}

/* This is just a skel until the next release */

void *update_all_thread(void *no_data)
{
    extern sem_t *update_all_semaphore;
    pthread_detach(pthread_self());
    NE_DEBUG(DEBUG_GNOME, "update_all_thread: detached.\n");

    for(;;)
      {
	  struct site *working_site;
	  NE_DEBUG(DEBUG_GNOME, "update_all_thread: sleeping...\n");
	  /* sleep straight away */
	  sem_wait(update_all_semaphore);

	  for (working_site = all_sites;
	       working_site;
	       working_site = working_site->next)
	    {
		int ret;
		ret = 
		  my_abortable_transfer_wrapper(working_site, site_op_update);
		set_status_after_operation(ret, GTK_LABEL(status_label));
		if (ret == SITE_ABORTED)
		  break;
	    }
      }
}


void *update_thread(void *no_data)
{
    int ret;
    extern sem_t *update_semaphore;
    extern gboolean site_keepgoing;
    
    pthread_detach(pthread_self());
    NE_DEBUG(DEBUG_GNOME, "update_thread: detached.\n");

    for(;;)
      {
	  NE_DEBUG(DEBUG_GNOME, "update_thread: sleeping...\n");
	  /* sleep straight away */
	  sem_wait(update_semaphore);
	  NE_DEBUG(DEBUG_GNOME, "update_thread: Okay, who woke me up!?\n");
	  
	  gdk_threads_enter();
	  gtk_widget_set_sensitive(begin_button, FALSE);
	  gtk_widget_set_sensitive(keep_going_button, FALSE);
	  
	  NE_DEBUG(DEBUG_GNOME, "update_thread: Acquired gtk+ lock\n");
	  
	  if (verifysite_gnome(selected_site)) {
	      close_main_update_window(NULL, NULL);
	      NE_DEBUG(DEBUG_GNOME, "update_thread: The site was wrong, skipping.\n");
	      continue;
	  }
	  
	  NE_DEBUG(DEBUG_GNOME, "update_thread: Verified site okay, updating...");
	  
	  /* Perform the actual update */
	  if (GTK_TOGGLE_BUTTON(keep_going_button)->active) {
	      site_keepgoing = TRUE;
	  } else {
	      site_keepgoing = FALSE;
	  }
	  gdk_threads_leave();	  

	  
	  /* site_update blocks until finished */
	  NE_DEBUG(DEBUG_GNOME, "update_thread: Entering site_update.\n");
	  /* we might want to give this a 3rd argument sometime to allow
	   * updating of single files at a time */
	  /* This calls site_update with a wrapper to facilitate good
	   * abort behaviour */
	  ret = my_abortable_transfer_wrapper(selected_site, site_op_update);
	  NE_DEBUG(DEBUG_GNOME, 
		"update_thread: site_update returned value of %d.\n", ret);
	  
	  gdk_threads_enter();
	  switch (ret) {
	   case SITE_CONNECT:
	      gtk_label_set(GTK_LABEL(status_label), "Unable to establish connection.");
	      gtk_label_set(GTK_LABEL(op_label), " ");
	      gtk_label_set(GTK_LABEL(file_label), " ");
	      gtk_label_set(GTK_LABEL(dir_label), " ");
	      break;
	   case SITE_AUTH:
	      gtk_label_set(GTK_LABEL(status_label), "Authentication with the remote server failed..");
	      gtk_label_set(GTK_LABEL(op_label), " ");
	      gtk_label_set(GTK_LABEL(file_label), " ");
	      gtk_label_set(GTK_LABEL(dir_label), " ");
	      break;
	   case SITE_ERRORS:
	      gtk_label_set(GTK_LABEL(status_label), "There was a problem with the file/directory transfer.");
	      gtk_label_set(GTK_LABEL(op_label), " ");
	      gtk_label_set(GTK_LABEL(file_label), " ");
	      gtk_label_set(GTK_LABEL(dir_label), " ");
	      site_write_stored_state(selected_site);
	      rescan_selected(1);
	      gtk_widget_set_sensitive(error_button, TRUE);
	      break;
	   case SITE_LOOKUP:
	      gtk_label_set(GTK_LABEL(status_label), "Unable to connect: Host name look-up failed.");
	      gtk_label_set(GTK_LABEL(op_label), " ");
	      gtk_label_set(GTK_LABEL(file_label), " ");
	      gtk_label_set(GTK_LABEL(dir_label), " ");
	      break;
	   case SITE_OK:
	      gtk_label_set(GTK_LABEL(status_label), "Update complete. (No errors)");
	      gtk_label_set(GTK_LABEL(op_label), " ");
	      gtk_label_set(GTK_LABEL(file_label), " ");
	      gtk_label_set(GTK_LABEL(dir_label), " ");
	      site_write_stored_state(selected_site);
	      rescan_selected(1);
	      break;
	   case SITE_FAILED:
	      gtk_label_set(GTK_LABEL(status_label), "Update failed. (Authentication problems)");
	      gtk_label_set(GTK_LABEL(op_label), " ");
	      gtk_label_set(GTK_LABEL(file_label), " ");
	      gtk_label_set(GTK_LABEL(dir_label), " ");
	      break;
	   case SITE_ABORTED:
	      gtk_label_set(GTK_LABEL(status_label), "Update was aborted.");
	      gtk_label_set(GTK_LABEL(op_label), " ");
	      gtk_label_set(GTK_LABEL(file_label), " ");
	      gtk_label_set(GTK_LABEL(dir_label), " ");
	      site_write_stored_state(selected_site);
	      /*rescan_selected(1);*/
	      break;
	   default:
	      gtk_label_set(GTK_LABEL(status_label), "Unexpected Update Return Value! Contact Maintainer.");
	      NE_DEBUG(DEBUG_GNOME, "ARG! site_update returned %d.\n", ret);
	      break;
	  }
	  NE_DEBUG(DEBUG_GNOME, "Dealt with site_update's return code. Changing sensitivities.\n");
	  gtk_widget_hide (kill_button);
	  gtk_widget_show (close_button); 
	  gtk_widget_set_sensitive(close_button, TRUE);
	  gtk_window_set_modal(GTK_WINDOW(upload_window), FALSE);
	  gdk_threads_leave();
	  NE_DEBUG(DEBUG_GNOME, "update_thread: Reported update status okay, looping...\n");
      }
}


int start_main_update(GtkWidget *button, gpointer single_or_all)
{
    extern sem_t *update_semaphore, *update_all_semaphore;

    gtk_widget_set_sensitive(begin_button, FALSE);
/*    gtk_widget_set_sensitive(close_button, FALSE);*/
    gtk_window_set_modal(GTK_WINDOW(upload_window), TRUE);
    gtk_widget_hide(close_button);
    gnome_dialog_append_button_with_pixmap (GNOME_DIALOG (upload_window),
					    "Stop!", GNOME_STOCK_PIXMAP_STOP);
    kill_button = g_list_last (GNOME_DIALOG (upload_window)->buttons)->data;
    gtk_signal_connect (GTK_OBJECT(kill_button), "clicked",
			GTK_SIGNAL_FUNC(abort_site_update), NULL);
    gtk_widget_show(kill_button);
   if (strcmp((gchar *)single_or_all, "single") == 0) {
      printf("updating single site.\n");
      sem_post(update_semaphore);
   } else if (strcmp((gchar *)single_or_all, "all") == 0) {
      printf("Updating ALL sites that need it.\n");
      sem_post(update_all_semaphore);
   } else {
      g_assert_not_reached();
   }
    return 1;
}

int main_update_please(GtkWidget * update_button, gpointer data)
{
    extern GtkWidget *connection_label;
    GtkWidget *dialog_vbox1;
    GtkWidget *vbox1;
    GtkWidget *hseparator3;
    GtkWidget *table1;
    GtkWidget *label3;
    GtkWidget *label6;
    GtkWidget *hseparator1;
    GtkWidget *label2;
    GtkWidget *label1;
    GtkWidget *dialog_action_area1;

    uploaded_bytes = 0.0;

    if (selected_site == NULL) {
	gnome_error_dialog("You must select a site if you want to upload the changes!");
	return 0;
    }
    if (!selected_site->remote_is_different) {
	gfe_status("Remote and local sites are already synchronised.");
	return 1;
    }
   
    upload_total = selected_site->totalnew + selected_site->totalchanged;
    make_error_window();
    upload_window = gnome_dialog_new("Update Progress", NULL);
    gtk_widget_set_usize(upload_window, 480, -2);

    dialog_vbox1 = GNOME_DIALOG(upload_window)->vbox;
    gtk_widget_show(dialog_vbox1);

    vbox1 = gtk_vbox_new(FALSE, 1);
    gtk_widget_ref(vbox1);
    gtk_widget_show(vbox1);
    gtk_box_pack_start(GTK_BOX(dialog_vbox1), vbox1, TRUE, TRUE, 0);

    hseparator3 = gtk_hseparator_new();
    gtk_widget_ref(hseparator3);
    gtk_widget_show(hseparator3);
    gtk_box_pack_start(GTK_BOX(vbox1), hseparator3, TRUE, TRUE, 3);

    table1 = gtk_table_new(4, 2, FALSE);
    gtk_widget_show(table1);
    gtk_box_pack_start(GTK_BOX(vbox1), table1, TRUE, TRUE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table1), 1);
    gtk_table_set_col_spacings(GTK_TABLE(table1), 2);

    status_label = gtk_label_new("Click Upload to begin.");
    gtk_widget_show(status_label);
    connection_label = status_label;
    gtk_table_attach(GTK_TABLE(table1), status_label, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(status_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(status_label), 7.45058e-09, 0.5);

    op_label = gtk_label_new(" ");
    gtk_widget_show(op_label);
    gtk_table_attach(GTK_TABLE(table1), op_label, 0, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(op_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(op_label), 7.45058e-09, 0.5);

    file_label = gtk_label_new("");
    gtk_widget_show(file_label);
    gtk_table_attach(GTK_TABLE(table1), file_label, 0, 2, 2, 3,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(file_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(file_label), 7.45058e-09, 0.5);

    label3 = gtk_label_new("Status: ");
    gtk_widget_show(label3);
    gtk_table_attach(GTK_TABLE(table1), label3, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label3), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label3), 7.45058e-09, 0.5);

    label6 = gtk_label_new("To: ");
    gtk_widget_show(label6);
    gtk_table_attach(GTK_TABLE(table1), label6, 0, 1, 3, 4,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label6), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label6), 7.45058e-09, 0.5);

    dir_label = gtk_label_new(" ");
    gtk_widget_show(dir_label);
    gtk_table_attach(GTK_TABLE(table1), dir_label, 1, 2, 3, 4,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(dir_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(dir_label), 7.45058e-09, 0.5);

    hseparator1 = gtk_hseparator_new();
    gtk_widget_show(hseparator1);
    gtk_box_pack_start(GTK_BOX(vbox1), hseparator1, TRUE, TRUE, 5);

    label2 = gtk_label_new("Current Progress");
    gtk_widget_show(label2);
    gtk_box_pack_start(GTK_BOX(vbox1), label2, TRUE, FALSE, 2);
    gtk_misc_set_alignment(GTK_MISC(label2), 0.5, 1);

    main_progressbar = gtk_progress_bar_new();
    gtk_widget_show(main_progressbar);
    gtk_box_pack_start(GTK_BOX(vbox1), main_progressbar, TRUE, FALSE, 0);
    gtk_progress_set_show_text(GTK_PROGRESS(main_progressbar), TRUE);

    label1 = gtk_label_new("Total Progress");
    gtk_widget_show(label1);
    gtk_box_pack_start(GTK_BOX(vbox1), label1, TRUE, FALSE, 2);
    gtk_misc_set_alignment(GTK_MISC(label1), 0.5, 1);

    job_progressbar = gtk_progress_bar_new();
    gtk_widget_show(job_progressbar);
    gtk_box_pack_start(GTK_BOX(vbox1), job_progressbar, TRUE, FALSE, 0);
    gtk_progress_set_show_text(GTK_PROGRESS(job_progressbar), TRUE);

    keep_going_button = gtk_check_button_new_with_label("Ignore any errors and always keep going.");
    gtk_widget_show(keep_going_button);
    gtk_box_pack_start(GTK_BOX(vbox1), keep_going_button, TRUE, TRUE, 0);

    dialog_action_area1 = GNOME_DIALOG(upload_window)->action_area;
    gtk_widget_show(dialog_action_area1);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area1), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(dialog_action_area1), 8);

    gnome_dialog_append_button_with_pixmap(GNOME_DIALOG(upload_window),
				   "Upload", GNOME_STOCK_PIXMAP_CONVERT);
    begin_button = g_list_last(GNOME_DIALOG(upload_window)->buttons)->data;
    gtk_widget_show(begin_button);
    GTK_WIDGET_SET_FLAGS(begin_button, GTK_CAN_DEFAULT);

    gnome_dialog_append_button_with_pixmap(GNOME_DIALOG(upload_window),
			       "View Errors", GNOME_STOCK_PIXMAP_SEARCH);
    error_button = g_list_last(GNOME_DIALOG(upload_window)->buttons)->data;
    gtk_signal_connect_object(GTK_OBJECT(error_button), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_show),
			      GTK_OBJECT(error_log_window));
    gtk_widget_show(error_button);
    gtk_widget_set_sensitive(error_button, FALSE);
    GTK_WIDGET_SET_FLAGS(error_button, GTK_CAN_DEFAULT);

    gnome_dialog_append_button(GNOME_DIALOG(upload_window), GNOME_STOCK_BUTTON_CLOSE);
    close_button = g_list_last(GNOME_DIALOG(upload_window)->buttons)->data;
    gtk_signal_connect(GTK_OBJECT(close_button), "clicked",
		       GTK_SIGNAL_FUNC(close_main_update_window), NULL);
/*    gtk_signal_connect_object(GTK_OBJECT(close_button), "clicked",
			      GTK_SIGNAL_FUNC(gnome_dialog_close),
			      (gpointer) upload_window);
*/  
    gtk_widget_show(close_button);
    GTK_WIDGET_SET_FLAGS(close_button, GTK_CAN_DEFAULT);

   if (strcmp((gchar *)data, "single") == 0) {
      gtk_signal_connect(GTK_OBJECT(begin_button), "clicked",
			 GTK_SIGNAL_FUNC(start_main_update),
			 "single");
   } else if (strcmp((gchar *)data, "all") == 0) {
      gtk_signal_connect(GTK_OBJECT(begin_button), "clicked",
			 GTK_SIGNAL_FUNC(start_main_update),
			 "all");
   } else {
      g_assert_not_reached();
   }
    gtk_widget_show(upload_window);
    return 2;
}

int fe_catchup_site(void)
{
    if (selected_site == NULL) {
	gfe_status("No site has been selected to catch-up.");
	return 0;
    }
    fe_gtk_question("This will mark ALL files as already updated.\nAre you sure you want to do this?",
		    (GnomeReplyCallback) catchup_selected);
    return 0;
}

void catchup_selected(gint button_number, gpointer data)
{
    gchar *result;
    extern GtkCTreeNode *current_site_node;
    if (button_number == GNOME_YES) {
	site_catchup(selected_site);
	result = g_strdup_printf("All files in %s of site %s\nhave been marked as updated.",
			 selected_site->local_root, selected_site->name);
	gfe_status(result);
	g_free(result);
	/* On huge sites, this may take ages? Progress bars/locking of ctree
	 * selection may be advisable... */
	site_write_stored_state(selected_site);
	rebuild_node_files(current_site_node);
    }
}

int fe_init_site(void)
{
    if (selected_site == NULL) {
	gfe_status("No site has been selected to initialise.");
	return 0;
    }
    fe_gtk_question("This will mark all files as *NOT* updated on the remote site.\nAre you sure you want to do this?",
		    (GnomeReplyCallback) initialize_selected);
    return 0;
}

void initialize_selected(gint button_number,
			 gpointer data)
{
    gchar *result;
    extern GtkCTreeNode *current_site_node;

    switch (button_number) {
    case GNOME_YES:
	site_initialize(selected_site);
	result = g_strdup_printf("All files in %s of site '%s'\nhave been marked as *NOT* updated.",
			 selected_site->local_root, selected_site->name);
	gfe_status(result);
	g_free(result);
	/* On huge sites, this may take ages? Progress bars/locking of ctree
	 * selection may be advisable... */
	site_write_stored_state(selected_site);
	redraw_main_area();
	rebuild_node_files(current_site_node);
	break;
    case GNOME_NO:
	break;
    }
}

void save_default(void)
{
    extern char *rcfile;
    extern gboolean rcfile_saved;
    extern GtkWidget *sitecopy;

    if (rcfile == NULL) {
	gnome_app_error(GNOME_APP(sitecopy), "The current rcfile is set to NULL. Sites will not be saved. Contact the maintainer.");
	return;
    }
    if (rcfile_write(rcfile, all_sites) == 0) {
	gchar *tmp;
	tmp = g_strdup_printf("Site definitions saved to %s.", rcfile);
	gfe_status(tmp);
	g_free(tmp);
    } else {
	gnome_error_dialog("There was an error writing the site definitions.\n They may not have saved correctly.");
    }
    rcfile_saved = true;
}

void delete_a_site(GtkWidget * button_or_menu, gpointer data)
{
    gchar *tmp;

    if (selected_site == NULL) {
	gnome_error_dialog("Cannot perform deletion - no site appears to be selected");
    } else {
	tmp = g_strdup_printf("Are you sure you wish to permanently delete the record of '%s'?",
			      selected_site->name);
	fe_gtk_question(tmp, (GnomeReplyCallback) (delete_selected));
    }
}

struct site *find_prev_site(struct site *a_site)
{
    struct site *tmp;
    tmp = all_sites;
    while (tmp->next != NULL) {
	if (strcmp(tmp->next->name, selected_site->name) == 0) {
	    return tmp;
	} else {
	    tmp = tmp->next;
	}
    }
    return NULL;		/* Bad if this ever happens */
}

void delete_selected(gint button_number)
{
    struct site *tmp, *tmp2;
    extern GtkCTree *the_tree;
    extern GtkCTreeNode *current_site_node;
    extern gboolean rcfile_saved;

    if (button_number != GNOME_YES)
	return;
    if ((selected_site != NULL) && (all_sites != NULL)) {
	if (strcmp(all_sites->name, selected_site->name) == 0) {
	    all_sites = all_sites->next;
	} else {
	    tmp = find_prev_site(selected_site);
	    if (tmp->next != NULL) {
		tmp2 = tmp->next;
		tmp->next = tmp2->next;
	    }
	}
	gtk_ctree_remove_node(GTK_CTREE(the_tree),
			      GTK_CTREE_NODE(current_site_node));
	selected_site = NULL;
	current_site_node = NULL;
    } else {
	gnome_error_dialog("I've detected no site is selected, but one should be selected in order to get here. Oh dear.");
    }
    clear_main_area();
    rcfile_saved = false;
}

