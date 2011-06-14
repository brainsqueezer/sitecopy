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
#include <gtk/gtk.h>

#include "common.h"
#include "sites.h"
#include "gcommon.h"
#include "misc.h"
#include <stdio.h>
#include "config.h"
#include <malloc.h>

void change_detection_mode(GtkWidget *button, gpointer data);
void change_detect_mode(GtkWidget * togglebutton, gpointer data);
void change_port(GtkWidget * spinbutton, gpointer data);
void change_lowercase(GtkWidget * checkbutton, gpointer data);
void change_safemode(GtkWidget * checkbutton, gpointer data);
void change_detectmode(GtkWidget * checkbutton, gpointer data);

void change_host_name(GtkWidget * entry, gpointer data);
void change_protocol(GtkWidget * menu_item, gpointer proto);
void change_username(GtkWidget * entry, gpointer data);
void change_password(GtkWidget * entry, gpointer data);
void change_passive_ftp(GtkWidget * toggle, gpointer data);
void change_nooverwrite(GtkWidget * toggle, gpointer data);
void change_delete(GtkWidget * toggle, gpointer data);
void change_move_status(GtkWidget * toggle, gpointer data);
void change_http_expect(GtkWidget * toggle, gpointer data);
void change_http_limit(GtkWidget * toggle, gpointer data);
void change_perms(GtkWidget * menu_item, gpointer perm_data);
void change_sym_mode(GtkWidget * menu_item, gpointer sym_data);
void set_local_dir(GtkWidget * entry, gpointer data);
void change_local_dir(GtkWidget * entry, gpointer data);
void change_remote_dir(GtkWidget * entry);
void change_url(GtkWidget * entry, gpointer data);
