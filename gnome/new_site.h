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

#ifndef _NEW_SITE_H_
#define _NEW_SITE_H_

#include <gnome.h>
#include "sites.h"
#include "misc.h"

struct create_site_widget_set {
    GtkWidget *name;
    GtkWidget *hostname_choice;
    GtkWidget *username_choice;

    GtkWidget *checkdelete_choice;
    GtkWidget *checkmove_choice;
    GtkWidget *passiveftp_choice;
    GtkWidget *deletefirst_choice;

    GtkWidget *protocol_choice;
    GtkWidget *protocol_choice_menu;
    GtkWidget *password_choice;
    GtkWidget *local_choice;
    GtkWidget *remotedir_choice;
    GtkWidget *url_choice;

    GtkWidget *init_choice;
    GtkWidget *catchup_choice;

    /* private stuff */
    GtkWidget *sym_choice;
    GtkWidget *sym_choice_menu;
    GtkWidget *perms_choice;
    GtkWidget *perms_choice_menu;
};

struct site *default_site(void);
void set_proto(GtkWidget * menu_item, gpointer data);
void set_perms(GtkWidget * menu_item, gpointer data);
void set_sym(GtkWidget * menu_item, gpointer data);
GtkWidget *create_new_wiz_book(void);
void start_new_site_wizard(void);

gchar *grab_druid_entry_into_string(gchar *widget_name);

/* Callbacks */
gboolean
on_site_druid_delete_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
verify_name_and_url                    (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
verify_server_details                  (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

void
set_new_port                           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
verify_directories                     (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
verify_file_attributes                 (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
verify_update_options                  (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
should_we_fetch                        (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
set_back_insensitive                   (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
make_site_from_druid                   (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
check_fetch_worked                     (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

void
begin_first_time_fetch                 (GtkButton       *button,
                                        gpointer         user_data);

void
druid_finished                         (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
on_general_info_cancel                 (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
on_server_info_cancel                  (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
on_directories_cancel                  (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
on_file_handling_cancel                (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
on_update_options_cancel               (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
on_server_state_cancel                 (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);

gboolean
on_fetch_a_list_cancel                 (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);
gboolean druid_back(GnomeDruidPage * gnomedruidpage,
		    gpointer arg1,
		    gpointer user_data);
gboolean
on_finish_page_cancel                  (GnomeDruidPage  *gnomedruidpage,
                                        gpointer         arg1,
                                        gpointer         user_data);


#endif				/* _NEW_SITE_H_ */
