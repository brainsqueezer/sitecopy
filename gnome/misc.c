
/* 
 *      XSitecopy, for managing remote web sites with a GNOME interface.
 *      Copyright (C) 1999, Lee Mallabone <lee@fonicmonkey.net>
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

#include "misc.h"

int generate_pixmaps(void);
extern struct site *all_sites;

gboolean rcfile_saved = TRUE;
extern char *copypath, *rcfile;

/* Global widgets */
extern GtkWidget *main_area_box, *area_data, *the_tree;
GtkWidget *error_log_list, *error_log_window;

struct site *selected_site;
GtkCTreeNode *current_site_node;

extern struct proto_driver ftp_driver;
#ifdef USE_DAV
extern struct proto_driver dav_driver;
#endif
GList *errors;

/************************************************************************/

/* Copies selected_site's infofile to infofile.bak */

void backup_infofile(void)
{
    gchar *backup_name;
    int ret;

    if (!selected_site) {
	gfe_status("No site selected. Cannot backup anything.");
	return;
    }
    backup_name = malloc(BUFSIZ);
    strcpy(backup_name, selected_site->infofile);
    strcat(backup_name, ".bak");
    NE_DEBUG(DEBUG_GNOME, "selected infofile is %s\n", selected_site->infofile);
    ret = copy_a_file(selected_site->infofile, backup_name);
    free(backup_name);
    switch (ret) {
    case 1:
	gfe_status("Could not open infofile");
	break;
    case 2:
	gfe_status("Could not write backup file");
	break;
    case 0:
	gfe_status("The state of your files has been backed up.\nIt may be restored at a later date from the backup menu.");
	break;
    default:
	gfe_status("Something is probably broken here.");
    }
}

void backup_rcfile(void)
{
    gchar *rcbackup;
    gint ret;
    rcbackup = malloc(BUFSIZ);
    strcpy(rcbackup, rcfile);
    strcat(rcbackup, ".bak");
    ret = copy_a_file(rcfile, rcbackup);
    free(rcbackup);
    switch (ret) {
    case 1:
	gfe_status("Could not open the rcfile. That is not good.");
	break;
    case 2:
	gfe_status("Could not create backup file.");
	break;
    case 0:
	gfe_status("The details of your sites have been backed up.");
	break;
    default:
	break;
    }
}

void restore_rcfile(void)
{
    struct stat *backup_file;
    gchar *question, *backup_name;

    backup_name = malloc(BUFSIZ);
    strcpy(backup_name, rcfile);
    strcat(backup_name, ".bak");
    backup_file = malloc(sizeof(struct stat));

    if (stat(backup_name, backup_file) == -1) {
	gfe_status("Could not find backup rcfile. Site configurations not restored.");
	free(backup_name);
	free(backup_file);
	return;
    }
    question = malloc(BUFSIZ);
    sprintf(question, "The last backup of your rcfile was modified on %s.\nAre you sure you wish to restore the site details from that date?", ctime(&(backup_file->st_ctime)));
    fe_gtk_question(question, (GnomeReplyCallback) restore_rc);
    free(question);
    free(backup_name);
    free(backup_file);
}

void restore_rc(gint button_number)
{
    gint ret;
    gchar *rcfile_backup;

    if (button_number == GNOME_YES) {
	rcfile_backup = malloc(strlen(rcfile) + 5);
	strcpy(rcfile_backup, rcfile);
	strcat(rcfile_backup, ".bak");

	ret = copy_a_file(rcfile_backup, rcfile);
	switch (ret) {
	case 1:
	    gfe_status("Could not open a backup");
	    break;
	case 2:
	    gfe_status("Could not write to your rcfile");
	    break;
	case 0:
	    gfe_status("Backup restored. Currently you will need to restart XSitecopy now.");
	    break;
	default:
	    printf("there was a problem restoring.\n");
	    break;
	}
	free(rcfile_backup);
    }
}

int copy_a_file(gchar * input_name, gchar * output_name)
{
    FILE *input, *output;
    gchar *buffer;

    if ((input = fopen(input_name, "r")) == NULL) {
	return 1;
    } else if ((output = fopen(output_name, "w")) == NULL) {
	return 2;
    } else {
	buffer = malloc(BUFSIZ);
	while (fgets(buffer, BUFSIZ, input) != NULL) {
	    fputs(buffer, output);
	    memset(buffer, 0, BUFSIZ);
	}
	free(buffer);
	fclose(input);
	fclose(output);
	return 0;
    }
}

void restore_infofile(void)
{
    struct stat *backup_file;
    gchar *question, *backup_name;

    if (!selected_site) {
	gfe_status("You must select a site to restore information about.");
	return;
    }
    backup_name = malloc(BUFSIZ);
    strcpy(backup_name, selected_site->infofile);
    strcat(backup_name, ".bak");
    backup_file = malloc(sizeof(struct stat));
    NE_DEBUG(DEBUG_GNOME, "backup is %s\n", strcat(selected_site->infofile, ".bak"));
    if (stat(backup_name, backup_file) == -1) {
	gfe_status("Could not find backup info file. File state not restored.");
	free(backup_name);
	free(backup_file);
	return;
    }
    question = malloc(BUFSIZ);
    sprintf(question, "The last backup of your files' state was modified on %s.\nAre you sure you wish to restore the file states from that date?", ctime(&(backup_file->st_ctime)));
    fe_gtk_question(question, (GnomeReplyCallback) actual_restoration);
    free(question);
    free(backup_name);
    free(backup_file);
}

void actual_restoration(gint button_number)
{
    gint ret;
    gchar *backup_name;

    if (button_number == 0) {
	backup_name = malloc(BUFSIZ);
	strcpy(backup_name, selected_site->infofile);
	strcat(backup_name, ".bak");
	ret = copy_a_file(backup_name, selected_site->infofile);
	switch (ret) {
	case 1:
	    gfe_status("Could not open the backup.");
	    break;
	case 2:
	    gfe_status("Could not open the infofile.");
	    break;
	case 0:
	    gfe_status("Backup restored.");
	    rescan_selected(1);
	    break;
	default:
	    break;
	}
	free(backup_name);
    }
}

/* If mode is 0, the buttons will be save, quit, cancel.
 * otherwise just quit & cancel.
 */
GtkWidget *create_quit_save_question(const char *question, int mode)
{
    GtkWidget *quit_save_question;
    GtkWidget *dialog_vbox6;
    GtkWidget *button12;
    GtkWidget *button13;
    GtkWidget *button14;
    GtkWidget *dialog_action_area6;

    quit_save_question = gnome_message_box_new(question,
					   GNOME_MESSAGE_BOX_INFO, NULL);
    gtk_window_set_policy(GTK_WINDOW(quit_save_question), FALSE, FALSE, FALSE);
    gnome_dialog_set_close(GNOME_DIALOG(quit_save_question), TRUE);

    dialog_vbox6 = GNOME_DIALOG(quit_save_question)->vbox;
    gtk_widget_show(dialog_vbox6);

    if (!mode) {
	gnome_dialog_append_button_with_pixmap(GNOME_DIALOG(quit_save_question),
			   "Save then quit", GNOME_STOCK_PIXMAP_SAVE_AS);
	button12 = g_list_last(GNOME_DIALOG(quit_save_question)->buttons)->data;
	gtk_widget_show(button12);
	GTK_WIDGET_SET_FLAGS(button12, GTK_CAN_DEFAULT);
    }
    gnome_dialog_append_button_with_pixmap(GNOME_DIALOG(quit_save_question),
				   "Just quit", GNOME_STOCK_PIXMAP_EXIT);
    button13 = g_list_last(GNOME_DIALOG(quit_save_question)->buttons)->data;
    gtk_widget_show(button13);
    GTK_WIDGET_SET_FLAGS(button13, GTK_CAN_DEFAULT);

    gnome_dialog_append_button(GNOME_DIALOG(quit_save_question), GNOME_STOCK_BUTTON_CANCEL);
    button14 = g_list_last(GNOME_DIALOG(quit_save_question)->buttons)->data;
    gtk_widget_show(button14);
    GTK_WIDGET_SET_FLAGS(button14, GTK_CAN_DEFAULT);

    dialog_action_area6 = GNOME_DIALOG(quit_save_question)->action_area;

    gtk_widget_grab_default(button14);
    return quit_save_question;
}

void quit_please(void)
{
    GtkWidget *qs;
    int button;

    /* FIXME: If user cares about unsaved definitions */
    if (!rcfile_saved) {
	qs = create_quit_save_question("Some of your site definitions may not be saved.\nWhat would you like to do?", 0);
	button = gnome_dialog_run_and_close(GNOME_DIALOG(qs));
	switch (button) {
	case -1:
	    break;
	case 0:
	    /* How should we handle this if it fails? */
	    rcfile_write(rcfile, all_sites);
	    gtk_main_quit();
	case 1:
	    gtk_main_quit();
	case 2:
	    /* Dialog is already closed by default. */
	    return;
	default:
	    return;
	}
    } else {
	/* FIXME: if user wants quit question */
	qs = create_quit_save_question("Are you sure you want to quit?", 1);
	button = gnome_dialog_run_and_close(GNOME_DIALOG(qs));
	switch (button) {
	case -1:
	    break;
	case 0:
	    gtk_main_quit();
	case 1:
	    return;
	    break;
	default:
	    ;
	}
    }
}

void open_new_request(void)
{
    filename_request(OPENNEW);
}

/* Reads the filename from the GtkFileSelection passed to it, and takes 
 * appropriate action depending upon the filename.
 *
 * Success: the global variable all_sites now contains the sites held within
 * the filename selected in the FileSelection. The main ctree is also rebuilt.
 */

void open_newrc(GtkWidget * ok, GtkFileSelection * fileb)
{
    struct site *new_sites, *old_sites;
    gchar *old_rcfile, *new_rcfile;
    gint new_status;
    struct stat *stat_tmp;

    if ((new_rcfile = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileb))) == NULL)
	return;

    stat_tmp = malloc(sizeof(struct stat));

    if (stat((const char *) new_rcfile, stat_tmp) != 0) {
	gfe_status("File does not exist!!");
	free(stat_tmp);
	return;
    }
    if (!S_ISREG(stat_tmp->st_mode)) {
	gfe_status("You must select an actual sitecopy configuration file, (rcfile).");
	free(stat_tmp);
	return;
    }
    NE_DEBUG(DEBUG_GNOME, "new rcfile is %s.\n", new_rcfile);
    old_rcfile = strdup(rcfile);
    rcfile = new_rcfile;
    NE_DEBUG(DEBUG_GNOME, "About to parse new rcfile...\n");
    new_status = rcfile_read(&new_sites);

    switch (new_status) {
    case RC_OPENFILE:
	gfe_status("Could not open file");
	return;
    case RC_DIROPEN:
	gfe_status("Could not open directory.");
	return;
    case RC_PERMS:
	gfe_status("Configuration file is not valid:\nPermissions are insecure.");
	return;
    case 0:
	NE_DEBUG(DEBUG_GNOME, "New rcfile successfully loaded into memory.\n");
	break;
    case RC_CORRUPT:
	gfe_status("Not a valid sitecopy configuration file.\n");
	rcfile = old_rcfile;
	free(new_rcfile);
	return;
    }
    if (new_sites == NULL) {
	gfe_status("Creation of new sites failed - not a valid sitecopy configuration file?");
	/* Free up some memory */
	return;
    }
    old_sites = all_sites;
    all_sites = new_sites;
    free(old_sites);
    gtk_clist_clear(GTK_CLIST(the_tree));
    fill_tree_from_all_sites(the_tree);
}

void saveas_request(void)
{
    filename_request(SAVEAS);
}

void save_sites_as(GtkWidget * ok, GtkFileSelection * fileb)
{
    gchar *file_to_saveas;

    file_to_saveas = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileb));
    NE_DEBUG(DEBUG_GNOME, "saving as file: %s\n", file_to_saveas);
    if (rcfile_write(file_to_saveas, all_sites) != 0) {
	gfe_status("There was a problem saving your sites.");
    } else {
	gfe_status("Site definitions saved.");
    }
    gtk_widget_destroy(GTK_WIDGET(fileb));
}

void filename_request(gint mode_num)
{
    GtkWidget *filebox = NULL;

    if (mode_num == OPENNEW) {
	filebox = gtk_file_selection_new("Choose a new sitecopy rcfile to load.");
    } else if (mode_num == SAVEAS) {
	filebox = gtk_file_selection_new("Save your site definitions to a file.");
    } else {
	gfe_status("error with dialog creation.");
    }
    gtk_signal_connect_object(GTK_OBJECT(filebox), "destroy",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(filebox));
    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filebox)->cancel_button), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(filebox));
    if (mode_num == OPENNEW) {
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filebox)->ok_button),
			"clicked", GTK_SIGNAL_FUNC(open_newrc), filebox);
    } else if (mode_num == SAVEAS) {
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filebox)->ok_button),
		     "clicked", GTK_SIGNAL_FUNC(save_sites_as), filebox);
    } else {
	gfe_status("there was an error");
    }
    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filebox)->ok_button), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),
			      GTK_OBJECT(filebox));
    gtk_widget_show(filebox);
}

GtkWidget *create_default_main_area(void)
{
    GtkWidget *vbox, *label;

    vbox = gtk_vbox_new(FALSE, 5);
    label = gtk_label_new("No site is currently selected.\n\nPlease choose one from the tree view\non the left before proceeding with any more operations.");
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 10);
    gtk_widget_show(label);

    gtk_widget_show(vbox);
    return vbox;
}

GtkWidget *create_initial_main_area(void)
{
    GtkWidget *vbox, *label;
    gchar *summary;
    gint num_that_need_updates = 0;
    struct site *tmp_site;

    vbox = gtk_vbox_new(FALSE, 10);
    label = gtk_label_new("\nWelcome to XSitecopy!\n\n");
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 10);
    gtk_widget_show(label);

    for (tmp_site = all_sites; tmp_site != NULL; tmp_site = tmp_site->next) {
	if (tmp_site->remote_is_different) {
	    summary = (char *) malloc(strlen(tmp_site->name) + 32);
	    sprintf(summary, "The site '%s' requires an update.", tmp_site->name);
	    label = gtk_label_new(summary);
	    gtk_widget_show(label);
	    free(summary);
	    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 1);
	    num_that_need_updates++;
	}
    }
    if (all_sites == NULL) {
	label = gtk_label_new("Click on \"New site\" to begin using the program.");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

    } else if (num_that_need_updates == 0) {
	label = gtk_label_new("All local sites are fully synchronized with the remote sites.");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);
    }
    gtk_widget_show(vbox);
    return vbox;
}

void redraw_main_area(void)
{
    gtk_container_remove(GTK_CONTAINER(main_area_box), area_data);
    area_data = make_site_info_area(selected_site);
    gtk_container_add(GTK_CONTAINER(main_area_box), area_data);
}

void clear_main_area(void)
{
    selected_site = NULL;
    gtk_container_remove(GTK_CONTAINER(main_area_box), area_data);
    area_data = create_default_main_area();
    gtk_container_add(GTK_CONTAINER(main_area_box), area_data);
}


/** If I start making any decent reports, move the report functions into reports.c **/

void site_report(void)
{
    gtk_container_remove(GTK_CONTAINER(main_area_box), area_data);
    area_data = create_initial_main_area();
    gtk_container_add(GTK_CONTAINER(main_area_box), area_data);
}

void site_web_report(void)
{
    gchar *lee;
    gchar *command_to_exec;

    if (selected_site == NULL) {
	gfe_status("For a report to be generated, a site must be selected.");
	return;
    }
    /* FIXME: path */
    lee = g_strdup_printf("sitecopy -ll %s | awk -f changes.awk > /tmp/xsitecopy_report.html",
			  selected_site->name);
    system(lee);
    system("gnome-moz-remote --newwin /tmp/xsitecopy_report.html");
}


void rescan_selected(int shouldRedraw)
{
    extern GtkWidget *sitecopy;
    int ret; char *newLabel;
    
    if (!selected_site) {
	gnome_error_dialog("You must select a site to rescan.");
	return;
    }

    ret = verifysite_gnome(selected_site);
    if ((ret == SITE_NOLOCALDIR) || (ret == SITE_ACCESSLOCALDIR))
      return;
    
    gnome_app_flash(GNOME_APP(sitecopy), "Scanning files...");
    if (site_readfiles(selected_site)) {
	gnome_error_dialog("Could not read file information for the local site.");
	return;
    }
    rebuild_node_files(current_site_node);
    
    newLabel = getAppropriateTreeLabel(selected_site);
    
    gtk_ctree_node_set_text(GTK_CTREE(the_tree),
			    GTK_CTREE_NODE(current_site_node),
			    0, newLabel);
    g_free(newLabel);
    if (shouldRedraw)
      redraw_main_area();
}


int check_site_and_record_errors(struct site *current)
{
    char *errstr;
    int ret = rcfile_verify(current);
    if (!ret) {
	/* joe: is this right? */
	errors = g_list_append(errors, "rcfile was corrupt");
	return 1;
    }

    errstr = (char *) malloc(BUFSIZ);
    switch (ret) {
	/* Everything was fine */
    case 0:
	return 0;
    case SITE_NOSERVER:
	sprintf(errstr, _("Server not specified in site `%s'.\n"),
		current->name);
	break;
    case SITE_NOREMOTEDIR:
	sprintf(errstr, _("Remote directory not specified in site `%s'.\n"),
		current->name);
	break;
    case SITE_NOLOCALDIR:
	sprintf(errstr, _("Local directory not specified in site `%s'.\n"),
		current->name);
	break;
    case SITE_ACCESSLOCALDIR:
	sprintf(errstr, _("Could not read directory for `%s':\n\t%s\n"),
		current->name, current->local_root);
	break;
    case SITE_INVALIDPORT:
	sprintf(errstr, _("Invalid port used in site `%s'.\n"),
		current->name);
	break;
    case SITE_NOMAINTAIN:
	sprintf(errstr, _("Protocol '%s' cannot maintain symbolic links (site `%s').\n"),
		current->driver->protocol_name, current->name);
	break;
    case SITE_NOREMOTEREL:
	sprintf(errstr, _("Cannot use a relative remote directory protocol '%s' (site `%s').\n"), current->driver->protocol_name, current->name);
	break;
    case SITE_NOPERMS:
	sprintf(errstr, _("%s's protocol does not currently support maintaining permissions."),
		current->name);
	break;
    case SITE_NOLOCALREL:
	sprintf(errstr, _("Could not find 'relative' local directory"));
    }
    errstr = realloc(errstr, strlen(errstr) + 1);
    errors = g_list_append(errors, errstr);
    return ret;
}

GtkWidget *ctree_create_sites(void)
{
    the_tree = gtk_ctree_new(1, 0);
    gtk_ctree_set_indent(GTK_CTREE(the_tree), 14);
    gtk_ctree_set_spacing(GTK_CTREE(the_tree), 4);
    gtk_ctree_set_line_style(GTK_CTREE(the_tree), GTK_CTREE_LINES_DOTTED);

    gtk_signal_connect(GTK_OBJECT(the_tree), "tree-select-row",
		       GTK_SIGNAL_FUNC(select_ctree_cb), NULL);
    fill_tree_from_all_sites(the_tree);
    gtk_widget_set_usize(the_tree, 150, -1);
    gtk_widget_show(the_tree);
    gtk_clist_set_column_auto_resize(GTK_CLIST(the_tree), 0, TRUE);
    return the_tree;
}

/* GUI making functions.
 * Most of these were made using Glade.
 */

void make_error_window(void)
{
    GtkWidget *dialog_vbox2;
    GtkWidget *label9;
    GtkWidget *sc_win;
    GtkWidget *label7;
    GtkWidget *label8;
    GtkWidget *dialog_action_area2;
    GtkWidget *cancel;

    error_log_window = gnome_dialog_new("Errors during the recent update", NULL);
    gtk_widget_set_usize(error_log_window, 567, 248);
    gtk_window_set_policy(GTK_WINDOW(error_log_window), TRUE, TRUE, FALSE);

    dialog_vbox2 = GNOME_DIALOG(error_log_window)->vbox;
    gtk_widget_show(dialog_vbox2);

    label9 = gtk_label_new("There were errors with the following files and/or directories:");

    gtk_widget_show(label9);
    gtk_box_pack_start(GTK_BOX(dialog_vbox2), label9, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(label9), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label9), 7.45058e-09, 0.5);

    sc_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(sc_win);
    gtk_box_pack_start(GTK_BOX(dialog_vbox2), sc_win, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc_win), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    error_log_list = gtk_clist_new(2);
    gtk_widget_show(error_log_list);
    gtk_container_add(GTK_CONTAINER(sc_win), error_log_list);
    gtk_clist_set_column_width(GTK_CLIST(error_log_list), 0, 265);
    gtk_clist_set_column_width(GTK_CLIST(error_log_list), 1, 80);
    gtk_clist_column_titles_show(GTK_CLIST(error_log_list));

    label7 = gtk_label_new("File/Directory Name");

    gtk_widget_show(label7);
    gtk_clist_set_column_widget(GTK_CLIST(error_log_list), 0, label7);

    label8 = gtk_label_new("Error code/message");

    gtk_widget_show(label8);
    gtk_clist_set_column_widget(GTK_CLIST(error_log_list), 1, label8);

    dialog_action_area2 = GNOME_DIALOG(error_log_window)->action_area;
    gtk_widget_show(dialog_action_area2);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area2), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(dialog_action_area2), 8);

    gnome_dialog_append_button(GNOME_DIALOG(error_log_window), GNOME_STOCK_BUTTON_CLOSE);
    cancel = g_list_last(GNOME_DIALOG(error_log_window)->buttons)->data;
    gtk_widget_show(cancel);
    GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);

    gtk_signal_connect_object(GTK_OBJECT(error_log_window), "delete_event",
			      GTK_SIGNAL_FUNC(gtk_widget_hide),
			      GTK_OBJECT(error_log_window));
    gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_hide),
			      GTK_OBJECT(error_log_window));
}

/* Enumerates any errors with the default rcfile, on startup. */

GtkWidget *
 create_site_errors_dialog()
{
    GtkWidget *site_errors_dialog;
    GtkWidget *dialog_vbox2;
    GtkWidget *vbox1;
    GtkWidget *label1;
    GtkWidget *hseparator1;
    GtkWidget *error_output;
    GtkWidget *sw;
    GtkWidget *dialog_action_area2;
    GtkWidget *button2;
    GList *current;
    extern gboolean fatal_error_encountered;

    site_errors_dialog = gnome_dialog_new(NULL, NULL);
    GTK_WINDOW(site_errors_dialog)->type = GTK_WINDOW_DIALOG;
    gtk_window_set_position(GTK_WINDOW(site_errors_dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_policy(GTK_WINDOW(site_errors_dialog), FALSE, FALSE, FALSE);
    gnome_dialog_set_close(GNOME_DIALOG(site_errors_dialog), TRUE);

    dialog_vbox2 = GNOME_DIALOG(site_errors_dialog)->vbox;
    gtk_widget_show(dialog_vbox2);

    vbox1 = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox1);
    gtk_box_pack_start(GTK_BOX(dialog_vbox2), vbox1, TRUE, TRUE, 0);

    if (fatal_error_encountered) {
	label1 = gtk_label_new("WARNING!\n\nThe following FATAL errors were encountered while examining\nyour site definitions. They must be corrected manually.\nXsitecopy will terminate when you press the close button.");
    } else {
	label1 = gtk_label_new("WARNING!\n\nThe following errors were encountered while examining\nyour site definitions. Unless corrected, Xsitecopy may fail\nto function properly.");
    }
    gtk_widget_show(label1);
    gtk_box_pack_start(GTK_BOX(vbox1), label1, FALSE, FALSE, 0);

    hseparator1 = gtk_hseparator_new();
    gtk_widget_show(hseparator1);
    gtk_box_pack_start(GTK_BOX(vbox1), hseparator1, TRUE, TRUE, 8);

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
				   GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);
    gtk_widget_show(sw);
    error_output = gtk_text_new(NULL, NULL);
    gtk_widget_show(error_output);
    gtk_container_add(GTK_CONTAINER(sw), error_output);
    gtk_box_pack_start(GTK_BOX(vbox1), sw, TRUE, TRUE, 0);

    dialog_action_area2 = GNOME_DIALOG(site_errors_dialog)->action_area;

    gtk_widget_show(dialog_action_area2);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area2), GTK_BUTTONBOX_SPREAD);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(dialog_action_area2), 8);

    gnome_dialog_append_button(GNOME_DIALOG(site_errors_dialog), GNOME_STOCK_BUTTON_OK);
    button2 = g_list_last(GNOME_DIALOG(site_errors_dialog)->buttons)->data;
    gtk_widget_show(button2);
    GTK_WIDGET_SET_FLAGS(button2, GTK_CAN_DEFAULT);

    for (current = errors; current; current = current->next) {
	gtk_text_insert(GTK_TEXT(error_output),
			NULL, NULL, NULL,
			(const char *) current->data, -1);
	/* Can't do this anymore, as I can't guarantee the data was in fact
	 * malloc'ed.
	 * g_free (current->data);
	 */
    }
    if (fatal_error_encountered) {
	gtk_signal_connect(GTK_OBJECT(button2), "clicked",
			   GTK_SIGNAL_FUNC(gtk_main_quit),
			   NULL);
    } else {
	gtk_signal_connect_object(GTK_OBJECT(button2), "clicked",
				  GTK_SIGNAL_FUNC(gtk_widget_destroy),
				  GTK_OBJECT(site_errors_dialog));
    }
    errors = NULL;
    gtk_widget_grab_default(button2);
    return site_errors_dialog;
}

/* Direction should be 1 for forwards, or 0 for backwards. */

GtkWidget *make_transfer_anim(gint direction)
{
    gchar *pixmap_dir_str;
    GnomeAnimator *anim;

    /* Create the animator. */
    anim = (GnomeAnimator *) gnome_animator_new_with_size(42, 43);
    
    /* Append the animation frames */

    pixmap_dir_str = gnome_unconditional_pixmap_file("xsitecopy/anim_base.png");
    if (pixmap_dir_str)
      gnome_animator_append_frame_from_file(anim, pixmap_dir_str, 0, 0, 900);
    g_free(pixmap_dir_str);

    pixmap_dir_str = gnome_unconditional_pixmap_file("xsitecopy/anim_yel0.png");
    if (pixmap_dir_str)
      gnome_animator_append_frame_from_file(anim, pixmap_dir_str, 0, 0, 900);
    g_free(pixmap_dir_str);
    
    pixmap_dir_str = gnome_unconditional_pixmap_file("xsitecopy/anim_yel1.png");
    if (pixmap_dir_str)
      gnome_animator_append_frame_from_file(anim, pixmap_dir_str, 0, 0, 900);
    g_free(pixmap_dir_str);
    
    pixmap_dir_str = gnome_unconditional_pixmap_file("xsitecopy/anim_yel2.png");
    if (pixmap_dir_str)
      gnome_animator_append_frame_from_file(anim, pixmap_dir_str, 0, 0, 900);
    g_free(pixmap_dir_str);
    
    pixmap_dir_str = gnome_unconditional_pixmap_file("xsitecopy/anim_yel3.png");
    if (pixmap_dir_str)
      gnome_animator_append_frame_from_file(anim, pixmap_dir_str, 0, 0, 900);
    g_free(pixmap_dir_str);
    
    pixmap_dir_str = gnome_unconditional_pixmap_file("xsitecopy/anim_yel4.png");
    if (pixmap_dir_str)
      gnome_animator_append_frame_from_file(anim, pixmap_dir_str, 0, 0, 900);
    g_free(pixmap_dir_str);
    
    pixmap_dir_str = gnome_unconditional_pixmap_file("xsitecopy/anim_yel5.png");
    if (pixmap_dir_str)
      gnome_animator_append_frame_from_file(anim, pixmap_dir_str, 0, 0, 900);
    g_free(pixmap_dir_str);
    
    /* Set speed, direction and loop */
    gnome_animator_set_playback_speed(anim, 20);
    gnome_animator_set_playback_direction(anim, direction);
    gnome_animator_set_loop_type(anim, GNOME_ANIMATOR_LOOP_RESTART);
    
    return GTK_WIDGET(anim);
}

