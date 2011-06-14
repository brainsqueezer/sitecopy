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
#include "config.h"
#include <sys/stat.h>
#include <unistd.h>
#include "basename.h"
#include "sites.h"
#include "file_widgets.h"

GtkWidget *file_info_container;
extern struct site *selected_site;

/* FIXME: This won't work, but `view' is disabled for now anyway. */
static void file_view_clicked(GtkWidget * button, gpointer data)
{

    GtkWidget *base = gtk_object_get_data(GTK_OBJECT(file_info_container),
					  "entry20");
    gchar *base_command = gtk_entry_get_text(GTK_ENTRY(base));
    system(base_command);
}

GtkWidget *make_file_info_area(struct site_file *the_file)
{
    const gchar *mime_info = NULL;
    struct stat *file_data = NULL;
    GtkWidget *table7;
    GtkWidget *label51;
    GtkWidget *entry11;
    GtkWidget *entry14;
    GtkWidget *label52;
    GtkWidget *label53;
    GtkWidget *entry15;
    GtkWidget *entry16;
    GtkWidget *entry17;
    GtkWidget *label54;
    GtkWidget *label55;
    GtkWidget *label56;
    GtkWidget *entry12;
    GtkWidget *entry13;
    GtkWidget *label57;
    GtkWidget *label58;
    GtkWidget *entry18;
    GtkWidget *entry19;
    GtkWidget *label60;
    GtkWidget *entry20;
    GtkWidget *label59;
    GtkWidget *view_button, *label;

    g_assert(the_file != NULL);
    file_info_container = gtk_frame_new(file_name(the_file));
    gtk_widget_show(file_info_container);
    if (!the_file) {
	label = gtk_label_new("Selected file pointed to null!");
	gtk_widget_show(label);
	gtk_container_add(GTK_CONTAINER(file_info_container),
			  label);
	return file_info_container;
    }
    file_data = (struct stat *) malloc(sizeof(struct stat));

    if (the_file->local.exists) {
	if (stat((const char *) file_full_local(&(the_file->local),
						selected_site),
		 file_data) == -1) {
	    label = gtk_label_new("Could not retrieve file information");
	    gtk_widget_show(label);
	    gtk_container_add(GTK_CONTAINER(file_info_container), label);
	    return file_info_container;
	}
    } else {
	label = gtk_label_new("The file does not appear to exist locally any more.\nIt has either been deleted or there is an access problem.");
	gtk_widget_show(label);
	gtk_container_add(GTK_CONTAINER(file_info_container), label);
	return file_info_container;
    }

    gtk_object_set_data(GTK_OBJECT(file_info_container), "file_info_container", file_info_container);

    table7 = gtk_table_new(8, 4, FALSE);
    gtk_widget_show(table7);
    gtk_container_add(GTK_CONTAINER(file_info_container), table7);
    gtk_container_set_border_width(GTK_CONTAINER(table7), 4);
    gtk_table_set_row_spacings(GTK_TABLE(table7), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table7), 6);

    label51 = gtk_label_new("Local filename: ");
    gtk_widget_show(label51);
    gtk_table_attach(GTK_TABLE(table7), label51, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label51), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label51), 0, 0.5);

    entry11 = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry11), base_name(file_name(the_file)));

    gtk_widget_show(entry11);
    gtk_table_attach(GTK_TABLE(table7), entry11, 1, 4, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_entry_set_editable(GTK_ENTRY(entry11), FALSE);

    entry14 = gtk_entry_new();
    if (the_file->local.exists)
	mime_info = gnome_mime_type((const gchar *) file_full_local(&(the_file->local),
							 selected_site));
    if (mime_info && (the_file->type != file_dir))
	gtk_entry_set_text(GTK_ENTRY(entry14), mime_info);
    gtk_widget_show(entry14);
    gtk_table_attach(GTK_TABLE(table7), entry14, 1, 4, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_entry_set_editable(GTK_ENTRY(entry14), FALSE);

    label52 = gtk_label_new("Mime type: ");
    gtk_widget_show(label52);
    gtk_table_attach(GTK_TABLE(table7), label52, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label52), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label52), 0, 0.5);

    label53 = gtk_label_new("Last modified: ");
    gtk_widget_show(label53);
    gtk_table_attach(GTK_TABLE(table7), label53, 0, 1, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label53), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label53), 0, 0.5);

    entry15 = gtk_entry_new();
    if (the_file->local.exists)
	gtk_entry_set_text(GTK_ENTRY(entry15),
			   ctime(&(the_file->local.time)));

    gtk_widget_show(entry15);
    gtk_table_attach(GTK_TABLE(table7), entry15, 1, 4, 2, 3,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_entry_set_editable(GTK_ENTRY(entry15), FALSE);

    entry16 = gtk_entry_new();
    /*changed */
    gtk_entry_set_text(GTK_ENTRY(entry16),
		       ctime(&(file_data->st_ctime)));
    gtk_widget_show(entry16);
    gtk_table_attach(GTK_TABLE(table7), entry16, 1, 4, 3, 4,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_entry_set_editable(GTK_ENTRY(entry16), FALSE);

    entry17 = gtk_entry_new();
    /*accessed */
    gtk_entry_set_text(GTK_ENTRY(entry17),
		       ctime(&(file_data->st_atime)));
    gtk_widget_show(entry17);
    gtk_table_attach(GTK_TABLE(table7), entry17, 1, 4, 4, 5,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_entry_set_editable(GTK_ENTRY(entry17), FALSE);

    label54 = gtk_label_new("Last changed: ");

    gtk_widget_show(label54);
    gtk_table_attach(GTK_TABLE(table7), label54, 0, 1, 3, 4,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_misc_set_alignment(GTK_MISC(label54), 0, 0.5);

    label55 = gtk_label_new("Last accessed: ");
    gtk_widget_show(label55);
    gtk_table_attach(GTK_TABLE(table7), label55, 0, 1, 4, 5,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label55), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label55), 0, 0.5);

    label56 = gtk_label_new("User ID: ");
    gtk_widget_show(label56);
    gtk_table_attach(GTK_TABLE(table7), label56, 0, 1, 5, 6,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label56), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label56), 0, 0.5);

    entry12 = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry12),
		       g_strdup_printf("%d", file_data->st_uid));
    gtk_widget_show(entry12);
    gtk_table_attach(GTK_TABLE(table7), entry12, 1, 2, 5, 6,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_widget_set_usize(entry12, 80, -2);
    gtk_entry_set_editable(GTK_ENTRY(entry12), FALSE);

    entry13 = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry13),
		       g_strdup_printf("%d", file_data->st_gid));
    gtk_widget_show(entry13);
    gtk_table_attach(GTK_TABLE(table7), entry13, 3, 4, 5, 6,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_widget_set_usize(entry13, 80, -2);
    gtk_entry_set_editable(GTK_ENTRY(entry13), FALSE);

    label57 = gtk_label_new("Group ID: ");
    gtk_widget_show(label57);
    gtk_table_attach(GTK_TABLE(table7), label57, 2, 3, 5, 6,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label57), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label57), 0, 0.5);

    label58 = gtk_label_new("Filesize (bytes): ");
    gtk_widget_show(label58);
    gtk_table_attach(GTK_TABLE(table7), label58, 0, 1, 6, 7,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label58), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label58), 0, 0.5);

    entry18 = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry18),
		       g_strdup_printf("%d", (int) file_data->st_size));
    gtk_widget_show(entry18);
    gtk_table_attach(GTK_TABLE(table7), entry18, 1, 2, 6, 7,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_widget_set_usize(entry18, 80, -2);
    gtk_entry_set_editable(GTK_ENTRY(entry18), FALSE);

    entry19 = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry19),
		       g_strdup_printf("%d", file_data->st_mode));
    gtk_widget_show(entry19);
    gtk_table_attach(GTK_TABLE(table7), entry19, 3, 4, 6, 7,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_widget_set_usize(entry19, 98, -2);

    label60 = gtk_label_new("Viewer command: ");
    gtk_widget_show(label60);
    gtk_table_attach(GTK_TABLE(table7), label60, 0, 1, 7, 8,
		     (GtkAttachOptions) (0),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    entry20 = gtk_entry_new();
/*viewer command */
    if (gnome_mime_program(mime_info))
	gtk_entry_set_text(GTK_ENTRY(entry20),
			   gnome_mime_program(mime_info));
    gtk_widget_show(entry20);
    gtk_table_attach(GTK_TABLE(table7), entry20, 1, 3, 7, 8,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    label59 = gtk_label_new("Protection: ");
    gtk_widget_show(label59);
    gtk_table_attach(GTK_TABLE(table7), label59, 2, 3, 6, 7,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label59), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label59), 0, 0.5);

    view_button = gtk_button_new_with_label("View");
    if (the_file->type == file_dir)
	gtk_widget_set_sensitive(view_button, FALSE);
    gtk_widget_show(view_button);
    gtk_table_attach(GTK_TABLE(table7), view_button, 3, 4, 7, 8,
		     (GtkAttachOptions) (0),
		     (GtkAttachOptions) (GTK_EXPAND), 0, 0);

    gtk_signal_connect(GTK_OBJECT(view_button), "clicked",
		       GTK_SIGNAL_FUNC(file_view_clicked),
		       NULL);

    return file_info_container;
}
