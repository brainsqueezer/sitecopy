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

#ifndef MINILIST_H
#define MINILIST_H

#include "common.h"
#include "misc.h"
#include "gcommon.h"
#include "sites.h"

enum minilists {
    list_exclude, list_ascii, list_ignore
};

struct slist_gui {
    enum minilists type;
    /* Will point to one of: selected_site->ignores, excludes, ascii */
    struct fnlist *data;
    GtkWidget *list;
    GtkWidget *entry;
    int chosen_row;
};

void populate_minilist(struct slist_gui *the_gui);
void select_minilist_item(GtkWidget * list, gint row, gint column,
			  GdkEventButton * event, gpointer data);
void change_minilist_entry(GtkWidget * entry, gpointer data);
void add_minilist_item(GtkWidget * button, gpointer data);
void remove_minilist_item(GtkWidget * button, gpointer data);

#endif				/* MINILIST_H */
