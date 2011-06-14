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

#ifndef MISC_H
#define MISC_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnome.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "sites.h"
#include "common.h"
#include "rcfile.h"
#include "protocol.h"
#include "dirname.h"

#include "new_site.h"
#include "gcommon.h"
#include "tree.h"
#include "changes.h"
#include "site_widgets.h"
#include "file_widgets.h"
#include "operations.h"

#define IS_A_FILE 1
#define IS_A_SITE 2

#define SAVEAS 1
#define OPENNEW 2

#define get_widget(obj, string) gtk_object_get_data(GTK_OBJECT(obj), string)

/* Possible things that can be displayed next to a site's name in the tree */
typedef enum {
    local_dir, remote_dir, remote_host, url, none
} site_tree_info;

struct xsitecopy_prefs {
    /* "visuals" */
    site_tree_info label;
    gboolean should_sort;
    long site_needs_update_col;
    long site_updated_col;
    GdkColor *needs_update;
    GdkColor *in_sync;

    /* "transfer" */
    gboolean backup_state_before_fetch;
    gboolean always_keep_going;
    gboolean log_errors;
    gchar *error_log_filename;

    /* general */
    gboolean quit_confirmation;
    gboolean save_confirmation;
};

struct ctree_attachment {
    int file_or_site;
    int sorted_yet;
    void *info_struct;
};

void backup_infofile(void);
void backup_rcfile(void);
void restore_rcfile(void);
void restore_rc(gint button_number);
int copy_a_file(gchar * input_name, gchar * output_name);
void restore_infofile(void);
void actual_restoration(gint button_number);

/* Exit functions */
GtkWidget *create_quit_save_question(const char *question, int mode);
void quit_please(void);

/* Load & Save rcfile functions */
void open_new_request(void);
void open_newrc(GtkWidget * ok, GtkFileSelection * fileb);
void saveas_request(void);
void save_sites_as(GtkWidget * ok, GtkFileSelection * fileb);
void filename_request(gint mode_num);

GtkWidget *create_default_main_area(void);
GtkWidget *create_initial_main_area(void);
void redraw_main_area(void);
void clear_main_area(void);

void site_report(void);
void site_web_report(void);

void rescan_selected(int shouldRedraw);

int check_site_and_record_errors(struct site *current);
GtkWidget *ctree_create_sites(void);

void make_error_window(void);
GtkWidget *create_site_errors_dialog(void);
GtkWidget *make_transfer_anim(gint direction);

#endif 	/* MISC_H */
