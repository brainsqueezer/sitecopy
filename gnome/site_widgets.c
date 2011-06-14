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

#include "site_widgets.h"

extern struct site *all_sites;
extern GtkWidget *site_list, *status_bar;

extern GtkWidget *main_area_box, *area_data;
extern GtkWidget *the_tree;

extern struct site *selected_site;
extern gboolean rcfile_saved;
struct slist_gui *the_excludes, *the_ignores, *ascii_files;
GtkWidget *main_panel;
int last_notepage = 0;

/* These widgets are global due to the freak deadlock bugs seen when using my
 * `get_widget' macro to retrieve per-instance pointers.
 */

GtkWidget *sym_follow;
GtkWidget *sym_ignore;
GtkWidget *sym_maintain;
GtkWidget *port;
GtkWidget *perms_ignore;
GtkWidget *perms_exec;
GtkWidget *perms_all;
GtkWidget *ftp_mode;
GtkWidget *nooverwrite;

void record_notepage(GtkNotebook * note, GtkNotebookPage * page,
		     gint pagenum, gpointer data)
{
/*    last_notepage = pagenum;*/
    NE_DEBUG(DEBUG_GNOME, "Recording notebook page number, %d.\n", pagenum);
}

GtkWidget *
 make_site_info_area(struct site *the_site)
{
    int files_on_site;
    char *tmp;
    gboolean current_rcfile_saved;
    
    GtkWidget *container;
    GtkWidget *vbox21;
    GtkWidget *frame14;
    GtkWidget *table4;
    GtkWidget *label33;
    GtkWidget *label34;
    GtkWidget *label35;
    GtkWidget *username;
    GtkWidget *combo_entry3;
    GtkWidget *label36;
    GtkWidget *password;
    GtkWidget *hbox15;
    GtkWidget *servername;
    GtkWidget *combo_entry2;
    GtkWidget *label50;
    GtkObject *port_adj;
    GtkWidget *hbox8;
    GSList *protocol_group = NULL;
    GtkWidget *proto_ftp;
    GtkWidget *proto_dav;
    GtkWidget *stats_frame;
    GtkWidget *stats_label;
    GtkWidget *label26;
    GtkWidget *vbox22;
    GtkWidget *frame16;
    GtkWidget *table5;
    GtkWidget *label40;
    GtkWidget *url;
    GtkWidget *remote_dir;
    GtkWidget *combo_entry5;
    GtkWidget *label38;
    GtkWidget *label39;
    GtkWidget *local_dir;
    GtkWidget *combo_entry4;
    GtkWidget *frame17;
    GtkWidget *table6;
    GtkWidget *label41;
    GtkWidget *label42;
    GtkWidget *label43;
    GtkWidget *hbox10;
    GSList *sym_link_group = NULL;
    GtkWidget *hbox9;
    GSList *perms_group = NULL;
    GtkWidget *hbox11;
    GtkWidget *detection_mode;
    GtkWidget *detection_button;
    GtkWidget *label27;
    GtkWidget *frame18;
    GtkWidget *vbox23;
    GtkWidget *nodelete;
    GtkWidget *checkmoved;
    GtkWidget *lowercase;
    GtkWidget *use_safemode;
    GtkWidget *label28;
    GtkWidget *vbox24;
    GtkWidget *scrolledwindow2;

    GtkWidget *label44;
    GtkWidget *hbox12;
    GtkWidget *label45;
    GtkWidget *exclude_gentry;
    GtkWidget *excludes_new;
    GtkWidget *exclude_remove;
    GtkWidget *label30;
    GtkWidget *vbox25;
    GtkWidget *scrolledwindow3;

    GtkWidget *label46;
    GtkWidget *hbox13;
    GtkWidget *label47;

    GtkWidget *ascii_gentry;
    GtkWidget *ascii_new;
    GtkWidget *ascii_remove;
    GtkWidget *label31;
    GtkWidget *vbox26;
    GtkWidget *scrolledwindow4;
    GtkWidget *label48;
    GtkWidget *hbox14;
    GtkWidget *label49;
    GtkWidget *ignores_gentry;
    GtkWidget *ignores_label;
    GtkWidget *ignore_new;
    GtkWidget *ignore_remove;

    
    current_rcfile_saved = rcfile_saved;
    
    main_panel = gtk_hbox_new(FALSE, 0);

    container = gtk_notebook_new();
    gtk_widget_show(container);
    gtk_container_add(GTK_CONTAINER(main_panel), container);

    vbox21 = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox21);
    gtk_container_add(GTK_CONTAINER(container), vbox21);
    gtk_container_set_border_width(GTK_CONTAINER(vbox21), 3);

    frame14 = gtk_frame_new("Server Details");
    gtk_widget_show(frame14);
    gtk_box_pack_start(GTK_BOX(vbox21), frame14, TRUE, TRUE, 0);

    table4 = gtk_table_new(4, 2, FALSE);

    gtk_widget_show(table4);
    gtk_container_add(GTK_CONTAINER(frame14), table4);
    gtk_container_set_border_width(GTK_CONTAINER(table4), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table4), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table4), 2);

    label33 = gtk_label_new("Host Name: ");
    gtk_widget_show(label33);
    gtk_table_attach(GTK_TABLE(table4), label33, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label33), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label33), 0, 0.5);

    label34 = gtk_label_new("Protocol: ");
    gtk_widget_show(label34);
    gtk_table_attach(GTK_TABLE(table4), label34, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label34), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label34), 0, 0.5);

    label35 = gtk_label_new("Username: ");

    gtk_widget_show(label35);
    gtk_table_attach(GTK_TABLE(table4), label35, 0, 1, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label35), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label35), 0, 0.5);

    username = gnome_entry_new("user_history");
    gtk_widget_show(username);
    gtk_table_attach(GTK_TABLE(table4), username, 1, 2, 2, 3,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    combo_entry3 = gnome_entry_gtk_entry(GNOME_ENTRY(username));
    gtk_entry_set_text(GTK_ENTRY(combo_entry3), selected_site->server.username);
    gtk_widget_show(combo_entry3);

    label36 = gtk_label_new("Password: ");
    gtk_widget_show(label36);
    gtk_table_attach(GTK_TABLE(table4), label36, 0, 1, 3, 4,
		     (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label36), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label36), 0, 0.5);

    password = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(password), selected_site->server.password);
    gtk_entry_set_visibility(GTK_ENTRY(password), FALSE);

    gtk_widget_show(password);
    gtk_table_attach(GTK_TABLE(table4), password, 1, 2, 3, 4,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    hbox15 = gtk_hbox_new(FALSE, 3);
    gtk_widget_show(hbox15);
    gtk_table_attach(GTK_TABLE(table4), hbox15, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    servername = gnome_entry_new("host_history");
    gtk_widget_show(servername);
    gtk_box_pack_start(GTK_BOX(hbox15), servername, TRUE, TRUE, 0);

    combo_entry2 = gnome_entry_gtk_entry(GNOME_ENTRY(servername));
    gtk_entry_set_text(GTK_ENTRY(combo_entry2), selected_site->server.hostname);
    gtk_widget_show(combo_entry2);

    label50 = gtk_label_new("Port: ");
    gtk_widget_show(label50);
    gtk_box_pack_start(GTK_BOX(hbox15), label50, FALSE, FALSE, 0);

    port_adj = gtk_adjustment_new(65532, 1, 65536, 1, 10, 10);
    port = gtk_spin_button_new(GTK_ADJUSTMENT(port_adj), 1, 0);

    gtk_widget_show(port);
    gtk_box_pack_start(GTK_BOX(hbox15), port, TRUE, TRUE, 0);
    gtk_widget_set_usize(port, 12, -2);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(port), TRUE);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(port), TRUE);

    hbox8 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox8);
    gtk_table_attach(GTK_TABLE(table4), hbox8, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    proto_ftp = gtk_radio_button_new_with_label(protocol_group, "FTP");
    gtk_signal_connect(GTK_OBJECT(proto_ftp), "toggled",
		       GTK_SIGNAL_FUNC(change_protocol),
		       "ftp");

    protocol_group = gtk_radio_button_group(GTK_RADIO_BUTTON(proto_ftp));
    gtk_widget_show(proto_ftp);
    gtk_box_pack_start(GTK_BOX(hbox8), proto_ftp, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(proto_ftp), TRUE);
#ifdef USE_DAV
    proto_dav = gtk_radio_button_new_with_label(protocol_group, "WebDAV");
    gtk_signal_connect(GTK_OBJECT(proto_dav), "toggled",
		       GTK_SIGNAL_FUNC(change_protocol),
		       "dav");
    protocol_group = gtk_radio_button_group(GTK_RADIO_BUTTON(proto_dav));

    gtk_widget_show(proto_dav);
    gtk_box_pack_start(GTK_BOX(hbox8), proto_dav, FALSE, FALSE, 0);
#endif				/* USE_DAV */
    stats_frame = gtk_frame_new("Site Statistics");
    gtk_widget_show(stats_frame);
    gtk_box_pack_start(GTK_BOX(vbox21), stats_frame, TRUE, TRUE, 0);

    stats_label = gtk_label_new("\n\n\n\n\n");
    files_on_site = selected_site->numnew + selected_site->numchanged + selected_site->numdeleted + selected_site->nummoved + selected_site->numunchanged;

    if (!selected_site->remote_is_different) {
	tmp = g_strdup_printf("\nThe local site contains %d files, none of which\nhave changed since the last update.\n\n", files_on_site);
    } else {
	tmp = g_strdup_printf("The local site has changed since the last update:\n%d files have been added, %d files have changed,\n%d files have been deleted, %d files have been moved,\n%d remain unchanged. There are currently %d files on the local site.",
			      selected_site->numnew,
			      selected_site->numchanged,
			      selected_site->numdeleted,
			      selected_site->nummoved,
			      selected_site->numunchanged,
			      files_on_site);
    }
    gtk_label_set(GTK_LABEL(stats_label), tmp);

    gtk_widget_show(stats_label);
    gtk_container_add(GTK_CONTAINER(stats_frame), stats_label);

    label26 = gtk_label_new("Basic Details");
    gtk_widget_show(label26);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(container), gtk_notebook_get_nth_page(GTK_NOTEBOOK(container), 0), label26);

    vbox22 = gtk_vbox_new(FALSE, 4);
    gtk_widget_show(vbox22);
    gtk_container_add(GTK_CONTAINER(container), vbox22);
    gtk_container_set_border_width(GTK_CONTAINER(vbox22), 3);

    frame16 = gtk_frame_new("Locations");
    gtk_widget_show(frame16);
    gtk_box_pack_start(GTK_BOX(vbox22), frame16, TRUE, TRUE, 0);

    table5 = gtk_table_new(3, 2, FALSE);
    gtk_widget_show(table5);
    gtk_container_add(GTK_CONTAINER(frame16), table5);
    gtk_container_set_border_width(GTK_CONTAINER(table5), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table5), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table5), 2);

    label40 = gtk_label_new("Root URL of the remote site: ");

    gtk_widget_show(label40);
    gtk_table_attach(GTK_TABLE(table5), label40, 0, 1, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label40), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label40), 0, 0.5);

    url = gtk_entry_new();
    if (selected_site->url)
	gtk_entry_set_text(GTK_ENTRY(url), selected_site->url);
    gtk_widget_show(url);
    gtk_table_attach(GTK_TABLE(table5), url, 1, 2, 2, 3,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    remote_dir = gnome_entry_new(NULL);

    gtk_widget_show(remote_dir);
    gtk_table_attach(GTK_TABLE(table5), remote_dir, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    combo_entry5 = gnome_entry_gtk_entry(GNOME_ENTRY(remote_dir));
    if (selected_site->remote_root_user)
	gtk_entry_set_text(GTK_ENTRY(combo_entry5),
			   selected_site->remote_root_user);

    gtk_widget_show(combo_entry5);

    label38 = gtk_label_new("Directory for remote files: ");

    gtk_widget_show(label38);
    gtk_table_attach(GTK_TABLE(table5), label38, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label38), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label38), 0, 0.5);

    label39 = gtk_label_new("Directory for local files: ");
    gtk_widget_show(label39);
    gtk_table_attach(GTK_TABLE(table5), label39, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label39), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label39), 0, 0.5);

    local_dir = gnome_file_entry_new(NULL, NULL);
    gtk_widget_show(local_dir);
    gtk_table_attach(GTK_TABLE(table5), local_dir, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    combo_entry4 = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(local_dir));
    if (selected_site->local_root_user)
	gtk_entry_set_text(GTK_ENTRY(combo_entry4),
			   selected_site->local_root_user);
    gtk_widget_show(combo_entry4);

    frame17 = gtk_frame_new("File Attributes");

    gtk_widget_show(frame17);
    gtk_box_pack_start(GTK_BOX(vbox22), frame17, TRUE, TRUE, 0);

    table6 = gtk_table_new(3, 2, FALSE);
    gtk_widget_show(table6);
    gtk_container_add(GTK_CONTAINER(frame17), table6);
    gtk_container_set_border_width(GTK_CONTAINER(table6), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table6), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table6), 2);

    label41 = gtk_label_new("Permissions mode: ");

    gtk_widget_show(label41);
    gtk_table_attach(GTK_TABLE(table6), label41, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label41), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label41), 0, 0.5);

    label42 = gtk_label_new("Symbolic links: ");

    gtk_widget_show(label42);
    gtk_table_attach(GTK_TABLE(table6), label42, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label42), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label42), 0, 0.5);

    label43 = gtk_label_new("Detect changes using: ");

    gtk_widget_show(label43);
    gtk_table_attach(GTK_TABLE(table6), label43, 0, 1, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label43), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label43), 0, 0.5);

    hbox10 = gtk_hbox_new(FALSE, 0);

    gtk_widget_show(hbox10);
    gtk_table_attach(GTK_TABLE(table6), hbox10, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    sym_follow = gtk_radio_button_new_with_label(sym_link_group, "Follow all");
    sym_link_group = gtk_radio_button_group(GTK_RADIO_BUTTON(sym_follow));

    gtk_widget_show(sym_follow);
    gtk_box_pack_start(GTK_BOX(hbox10), sym_follow, FALSE, FALSE, 0);

    sym_ignore = gtk_radio_button_new_with_label(sym_link_group, "Ignore links");
    sym_link_group = gtk_radio_button_group(GTK_RADIO_BUTTON(sym_ignore));

    gtk_widget_show(sym_ignore);
    gtk_box_pack_start(GTK_BOX(hbox10), sym_ignore, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sym_ignore), TRUE);

    sym_maintain = gtk_radio_button_new_with_label(sym_link_group, "Maintain links");
    sym_link_group = gtk_radio_button_group(GTK_RADIO_BUTTON(sym_maintain));

    gtk_widget_show(sym_maintain);
    gtk_box_pack_start(GTK_BOX(hbox10), sym_maintain, FALSE, FALSE, 0);

    hbox9 = gtk_hbox_new(FALSE, 0);

    gtk_widget_show(hbox9);
    gtk_table_attach(GTK_TABLE(table6), hbox9, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    perms_ignore = gtk_radio_button_new_with_label(perms_group, "Ignore all");
    perms_group = gtk_radio_button_group(GTK_RADIO_BUTTON(perms_ignore));

    gtk_widget_show(perms_ignore);
    gtk_box_pack_start(GTK_BOX(hbox9), perms_ignore, FALSE, FALSE, 0);

    perms_exec = gtk_radio_button_new_with_label(perms_group, "Executables only");
    perms_group = gtk_radio_button_group(GTK_RADIO_BUTTON(perms_exec));

    gtk_widget_show(perms_exec);
    gtk_box_pack_start(GTK_BOX(hbox9), perms_exec, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(perms_exec), TRUE);

    perms_all = gtk_radio_button_new_with_label(perms_group, "Maintain all");
    perms_group = gtk_radio_button_group(GTK_RADIO_BUTTON(perms_all));

    gtk_widget_show(perms_all);

    gtk_box_pack_start(GTK_BOX(hbox9), perms_all, FALSE, FALSE, 0);

    hbox11 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox11);
    gtk_table_attach(GTK_TABLE(table6), hbox11, 1, 2, 2, 3,
		     (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    
    /* Detection mode status */
    detection_mode = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(detection_mode), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(detection_mode), 0, 0.5);

    gtk_box_pack_start(GTK_BOX(hbox11), detection_mode, TRUE, TRUE, 4);
    gtk_widget_show(detection_mode);
    
    /* Change button */
    detection_button = gtk_button_new_with_label("Change");
    gtk_signal_connect(GTK_OBJECT(detection_button), "clicked",
		       GTK_SIGNAL_FUNC(change_detection_mode), NULL);
    gtk_box_pack_start(GTK_BOX(hbox11), detection_button, TRUE, FALSE, 4);
    gtk_widget_show(detection_button);
    
/* Added */
    if (the_site->state_method == state_timesize)
      gtk_label_set(GTK_LABEL(detection_mode), "File size & modification time");
    else
      gtk_label_set(GTK_LABEL(detection_mode), "File checksum");
/*      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(detect_sizetime), TRUE);*/
/* End */
    label27 = gtk_label_new("Locations & Files");

    gtk_widget_show(label27);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(container), gtk_notebook_get_nth_page(GTK_NOTEBOOK(container), 1), label27);

    frame18 = gtk_frame_new("Operations checklist");
    gtk_widget_show(frame18);
    gtk_container_add(GTK_CONTAINER(container), frame18);
    gtk_container_set_border_width(GTK_CONTAINER(frame18), 3);

    vbox23 = gtk_vbox_new(FALSE, 0);

    gtk_widget_show(vbox23);
    gtk_container_add(GTK_CONTAINER(frame18), vbox23);

    nodelete = gtk_check_button_new_with_label("Delete a file from the server if it is deleted locally");

    gtk_widget_show(nodelete);
    gtk_box_pack_start(GTK_BOX(vbox23), nodelete, TRUE, FALSE, 0);

    checkmoved = gtk_check_button_new_with_label("Move a remote file if it is moved locally");

    gtk_widget_show(checkmoved);
    gtk_box_pack_start(GTK_BOX(vbox23), checkmoved, TRUE, FALSE, 0);

    nooverwrite = gtk_check_button_new_with_label("When uploading changed files, first delete them");

    gtk_widget_show(nooverwrite);
    gtk_box_pack_start(GTK_BOX(vbox23), nooverwrite, TRUE, FALSE, 0);

    lowercase = gtk_check_button_new_with_label("Convert all filenames to lowercase when uploading");

    gtk_widget_show(lowercase);
    gtk_box_pack_start(GTK_BOX(vbox23), lowercase, TRUE, FALSE, 0);

    use_safemode = gtk_check_button_new_with_label("Use \"safe mode\"");

    gtk_widget_show(use_safemode);
    gtk_box_pack_start(GTK_BOX(vbox23), use_safemode, TRUE, FALSE, 0);

    ftp_mode = gtk_check_button_new_with_label("Use passive mode FTP");

    gtk_widget_show(ftp_mode);
    gtk_box_pack_start(GTK_BOX(vbox23), ftp_mode, TRUE, FALSE, 0);

    label28 = gtk_label_new("Update Options");

    gtk_widget_show(label28);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(container), gtk_notebook_get_nth_page(GTK_NOTEBOOK(container), 2), label28);

    vbox24 = gtk_vbox_new(FALSE, 4);

    gtk_widget_show(vbox24);
    gtk_container_add(GTK_CONTAINER(container), vbox24);
    gtk_container_set_border_width(GTK_CONTAINER(vbox24), 3);

    scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);

    gtk_widget_show(scrolledwindow2);
    gtk_box_pack_start(GTK_BOX(vbox24), scrolledwindow2, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    the_excludes = malloc(sizeof(struct slist_gui));
    the_excludes->data = the_site->excludes;
    the_excludes->type = list_exclude;
    the_excludes->chosen_row = -1;
    the_excludes->list = gtk_clist_new(1);

    gtk_widget_show(the_excludes->list);
    gtk_container_add(GTK_CONTAINER(scrolledwindow2), the_excludes->list);
    gtk_clist_set_column_width(GTK_CLIST(the_excludes->list), 0, 80);
    gtk_clist_column_titles_show(GTK_CLIST(the_excludes->list));

    label44 = gtk_label_new("Files and regular expressions to exclude from the site");

    gtk_widget_show(label44);
    gtk_clist_set_column_widget(GTK_CLIST(the_excludes->list), 0, label44);

    hbox12 = gtk_hbox_new(FALSE, 2);

    gtk_widget_show(hbox12);
    gtk_box_pack_start(GTK_BOX(vbox24), hbox12, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox12), 1);

    label45 = gtk_label_new("Exclude: ");

    gtk_widget_show(label45);
    gtk_box_pack_start(GTK_BOX(hbox12), label45, FALSE, FALSE, 0);

    exclude_gentry = gnome_entry_new("excludes_history");

    gtk_widget_show(exclude_gentry);
    gtk_box_pack_start(GTK_BOX(hbox12), exclude_gentry, TRUE, TRUE, 0);

    the_excludes->entry = gnome_entry_gtk_entry(GNOME_ENTRY(exclude_gentry));

    gtk_widget_show(the_excludes->entry);

    excludes_new = gtk_button_new_with_label("New");

    gtk_widget_show(excludes_new);
    gtk_box_pack_start(GTK_BOX(hbox12), excludes_new, FALSE, FALSE, 0);

    exclude_remove = gtk_button_new_with_label("Remove");

    gtk_widget_show(exclude_remove);
    gtk_box_pack_start(GTK_BOX(hbox12), exclude_remove, FALSE, FALSE, 0);

    label30 = gtk_label_new("Excludes");

    gtk_widget_show(label30);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(container), gtk_notebook_get_nth_page(GTK_NOTEBOOK(container), 3), label30);
    /* Actually fill in the excludes list */
    populate_minilist(the_excludes);
    vbox25 = gtk_vbox_new(FALSE, 4);

    gtk_widget_show(vbox25);
    gtk_container_add(GTK_CONTAINER(container), vbox25);
    gtk_container_set_border_width(GTK_CONTAINER(vbox25), 3);

    scrolledwindow3 = gtk_scrolled_window_new(NULL, NULL);

    gtk_widget_show(scrolledwindow3);
    gtk_box_pack_start(GTK_BOX(vbox25), scrolledwindow3, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    ascii_files = malloc(sizeof(struct slist_gui));
    ascii_files->data = the_site->asciis;
    ascii_files->type = list_ascii;
    ascii_files->chosen_row = -1;
    ascii_files->list = gtk_clist_new(1);
    gtk_widget_show(ascii_files->list);
    gtk_container_add(GTK_CONTAINER(scrolledwindow3), ascii_files->list);
    gtk_clist_set_column_width(GTK_CLIST(ascii_files->list), 0, 80);
    gtk_clist_column_titles_show(GTK_CLIST(ascii_files->list));

    label46 = gtk_label_new("Files to transfer in 'ASCII' mode");

    gtk_widget_show(label46);
    gtk_clist_set_column_widget(GTK_CLIST(ascii_files->list), 0, label46);

    hbox13 = gtk_hbox_new(FALSE, 2);

    gtk_widget_show(hbox13);
    gtk_box_pack_start(GTK_BOX(vbox25), hbox13, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox13), 1);

    label47 = gtk_label_new("Filename: ");
    gtk_widget_show(label47);
    gtk_box_pack_start(GTK_BOX(hbox13), label47, FALSE, FALSE, 0);

    ascii_gentry = gnome_entry_new(NULL);
    gtk_widget_show(ascii_gentry);
    gtk_box_pack_start(GTK_BOX(hbox13), ascii_gentry, TRUE, TRUE, 0);

    ascii_files->entry = gnome_entry_gtk_entry(GNOME_ENTRY(ascii_gentry));

    gtk_widget_show(ascii_files->entry);

    ascii_new = gtk_button_new_with_label("New");

    gtk_widget_show(ascii_new);
    gtk_box_pack_start(GTK_BOX(hbox13), ascii_new, FALSE, FALSE, 0);

    ascii_remove = gtk_button_new_with_label("Remove");

    gtk_widget_show(ascii_remove);
    gtk_box_pack_start(GTK_BOX(hbox13), ascii_remove, FALSE, FALSE, 0);

    label31 = gtk_label_new("ASCII");

    gtk_widget_show(label31);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(container), gtk_notebook_get_nth_page(GTK_NOTEBOOK(container), 4), label31);

    populate_minilist(ascii_files);

    vbox26 = gtk_vbox_new(FALSE, 4);

    gtk_widget_show(vbox26);
    gtk_container_add(GTK_CONTAINER(container), vbox26);
    gtk_container_set_border_width(GTK_CONTAINER(vbox26), 3);

    scrolledwindow4 = gtk_scrolled_window_new(NULL, NULL);

    gtk_widget_show(scrolledwindow4);
    gtk_box_pack_start(GTK_BOX(vbox26), scrolledwindow4, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow4), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    the_ignores = malloc(sizeof(struct slist_gui));
    the_ignores->data = the_site->ignores;
    the_ignores->type = list_ignore;
    the_ignores->chosen_row = -1;
    the_ignores->list = gtk_clist_new(1);

    gtk_widget_show(the_ignores->list);
    gtk_container_add(GTK_CONTAINER(scrolledwindow4), the_ignores->list);
    gtk_clist_set_column_width(GTK_CLIST(the_ignores->list), 0, 80);
    gtk_clist_column_titles_show(GTK_CLIST(the_ignores->list));

    label48 = gtk_label_new("Files whose *changes* should not be transferred to the remote site");

    gtk_widget_show(label48);
    gtk_clist_set_column_widget(GTK_CLIST(the_ignores->list), 0, label48);

    populate_minilist(the_ignores);

    hbox14 = gtk_hbox_new(FALSE, 2);

    gtk_widget_show(hbox14);
    gtk_box_pack_start(GTK_BOX(vbox26), hbox14, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox14), 1);

    label49 = gtk_label_new("Ignore: ");

    gtk_widget_show(label49);
    gtk_box_pack_start(GTK_BOX(hbox14), label49, FALSE, FALSE, 0);

    ignores_gentry = gnome_entry_new(NULL);
    gtk_widget_show(ignores_gentry);
    gtk_box_pack_start(GTK_BOX(hbox14), ignores_gentry, TRUE, TRUE, 0);

    the_ignores->entry = gnome_entry_gtk_entry(GNOME_ENTRY(ignores_gentry));

    gtk_widget_show(the_ignores->entry);

    ignore_new = gtk_button_new_with_label("New");
    gtk_widget_show(ignore_new);
    gtk_box_pack_start(GTK_BOX(hbox14), ignore_new, FALSE, FALSE, 0);

    ignore_remove = gtk_button_new_with_label("Remove");

    gtk_widget_show(ignore_remove);
    gtk_box_pack_start(GTK_BOX(hbox14), ignore_remove, FALSE, FALSE, 0);

    ignores_label = gtk_label_new("Ignore");

    gtk_widget_show(ignores_label);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(container), gtk_notebook_get_nth_page(GTK_NOTEBOOK(container), 5), ignores_label);

    if (selected_site->protocol == siteproto_ftp)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(proto_ftp), TRUE);
#ifdef USE_DAV
    if (selected_site->protocol == siteproto_dav)
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(proto_dav), TRUE);
#endif /* USE_DAV */

    gtk_signal_connect(GTK_OBJECT(combo_entry3), "changed",
		       GTK_SIGNAL_FUNC(change_username),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(password), "changed",
		       GTK_SIGNAL_FUNC(change_password),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(combo_entry2), "changed",
		       GTK_SIGNAL_FUNC(change_host_name),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(port), "changed",
		       GTK_SIGNAL_FUNC(change_port),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(url), "changed",
		       GTK_SIGNAL_FUNC(change_url),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(combo_entry5), "changed",
		       GTK_SIGNAL_FUNC(change_remote_dir),
		       NULL);
    /* Sets the local dir, but doesn't do a rescan. */
    gtk_signal_connect(GTK_OBJECT(combo_entry4), "changed",
		       GTK_SIGNAL_FUNC(set_local_dir),
		       NULL);
    /* If activated do a rescan. focus out can get kinda icky if the dir
     * is not readable.
     */
    gtk_signal_connect(GTK_OBJECT(combo_entry4), "activate",
		       GTK_SIGNAL_FUNC(change_local_dir),
		       NULL);
/*    gtk_signal_connect(GTK_OBJECT(combo_entry4), "focus_out_event",
		       GTK_SIGNAL_FUNC(change_local_dir),
		       NULL);*/
    gtk_signal_connect(GTK_OBJECT(sym_follow), "toggled",
		       GTK_SIGNAL_FUNC(change_sym_mode),
		       "follow");
    gtk_signal_connect(GTK_OBJECT(sym_ignore), "toggled",
		       GTK_SIGNAL_FUNC(change_sym_mode),
		       "ignore");
    gtk_signal_connect(GTK_OBJECT(sym_maintain), "toggled",
		       GTK_SIGNAL_FUNC(change_sym_mode),
		       "maintain");
    gtk_signal_connect(GTK_OBJECT(perms_ignore), "toggled",
		       GTK_SIGNAL_FUNC(change_perms),
		       "ignore");
    gtk_signal_connect(GTK_OBJECT(perms_exec), "toggled",
		       GTK_SIGNAL_FUNC(change_perms),
		       "exec");
    gtk_signal_connect(GTK_OBJECT(perms_all), "toggled",
		       GTK_SIGNAL_FUNC(change_perms),
		       "all");
    gtk_signal_connect(GTK_OBJECT(nodelete), "toggled",
		       GTK_SIGNAL_FUNC(change_delete),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(checkmoved), "toggled",
		       GTK_SIGNAL_FUNC(change_move_status),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(nooverwrite), "toggled",
		       GTK_SIGNAL_FUNC(change_nooverwrite),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(lowercase), "toggled",
		       GTK_SIGNAL_FUNC(change_lowercase),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(use_safemode), "toggled",
		       GTK_SIGNAL_FUNC(change_safemode),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(ftp_mode), "toggled",
		       GTK_SIGNAL_FUNC(change_passive_ftp),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(the_excludes->list), "select_row",
		       GTK_SIGNAL_FUNC(select_minilist_item),
		       the_excludes);
    gtk_signal_connect(GTK_OBJECT(the_excludes->entry), "changed",
		       GTK_SIGNAL_FUNC(change_minilist_entry),
		       the_excludes);
    gtk_signal_connect(GTK_OBJECT(excludes_new), "clicked",
		       GTK_SIGNAL_FUNC(add_minilist_item),
		       the_excludes);
    gtk_signal_connect(GTK_OBJECT(exclude_remove), "clicked",
		       GTK_SIGNAL_FUNC(remove_minilist_item),
		       the_excludes);
    gtk_signal_connect(GTK_OBJECT(ascii_files->list), "select_row",
		       GTK_SIGNAL_FUNC(select_minilist_item),
		       ascii_files);
    gtk_signal_connect(GTK_OBJECT(ascii_files->entry), "changed",
		       GTK_SIGNAL_FUNC(change_minilist_entry),
		       ascii_files);
    gtk_signal_connect(GTK_OBJECT(ascii_new), "clicked",
		       GTK_SIGNAL_FUNC(add_minilist_item),
		       ascii_files);
    gtk_signal_connect(GTK_OBJECT(ascii_remove), "clicked",
		       GTK_SIGNAL_FUNC(remove_minilist_item),
		       ascii_files);
    gtk_signal_connect(GTK_OBJECT(the_ignores->list), "select_row",
		       GTK_SIGNAL_FUNC(select_minilist_item),
		       the_ignores);
    gtk_signal_connect(GTK_OBJECT(the_ignores->entry), "changed",
		       GTK_SIGNAL_FUNC(change_minilist_entry),
		       the_ignores);
    gtk_signal_connect(GTK_OBJECT(ignore_new), "clicked",
		       GTK_SIGNAL_FUNC(add_minilist_item),
		       the_ignores);
    gtk_signal_connect(GTK_OBJECT(ignore_remove), "clicked",
		       GTK_SIGNAL_FUNC(remove_minilist_item),
		       the_ignores);
    gtk_widget_show(main_panel);
    /* if (main_prefs->remember_notebook_pos) */
    /* Why doesn't this work? */
    NE_DEBUG(DEBUG_GNOME, "Setting notebook page to page %d.\n", last_notepage);
    gtk_notebook_set_page(GTK_NOTEBOOK(container), last_notepage);
    gtk_signal_connect(GTK_OBJECT(container), "switch-page",
		       GTK_SIGNAL_FUNC(record_notepage), NULL);

/*******************************************/
/**** Setup the permissions GUI widgets ****/
/*******************************************/

    switch (selected_site->perms) {
    case sitep_ignore:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(perms_ignore), TRUE);
	break;
    case sitep_exec:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(perms_exec), TRUE);
	break;
    case sitep_all:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(perms_all), TRUE);
	break;
    }

#ifdef USE_DAV
    if (selected_site->protocol == siteproto_dav) {
	gtk_widget_set_sensitive(perms_all, FALSE);
	gtk_widget_set_sensitive(perms_exec, FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(perms_ignore), TRUE);
    }
#endif				/* USE_DAV */

/*****************************************/
/**** Setup the sym links GUI widgets ****/
/*****************************************/

    switch (selected_site->symlinks) {
    case sitesym_ignore:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sym_ignore), TRUE);
	break;
    case sitesym_maintain:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sym_maintain), TRUE);
	break;
    case sitesym_follow:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sym_follow), TRUE);
	break;
    }
    if (selected_site->protocol == siteproto_ftp) {
	gtk_widget_set_sensitive(sym_maintain, FALSE);
	if (selected_site->symlinks == sitesym_maintain)
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sym_ignore), TRUE);
    }
/************/
/*** Port ***/
/************/
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(port),
			      (float) selected_site->server.port);

/***************/
/*** Options ***/
/***************/

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nodelete),
				 !(selected_site->nodelete));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(nooverwrite),
				 selected_site->nooverwrite);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkmoved),
				 selected_site->checkmoved);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lowercase),
				 selected_site->lowercase);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_safemode),
				 selected_site->safemode);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ftp_mode),
				 selected_site->ftp_pasv_mode);

    /* Restore rcfile_saved status after bastardization by the various signal
     * handlers that will have gone off.
     */
    rcfile_saved = current_rcfile_saved;
    return main_panel;
}

   /* TODO: Add a config option for more stats, like how much space the new
    * files take up, how many excludes, etc. */
