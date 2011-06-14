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
#include "minilist.h"

extern struct site *selected_site;

/** the_gui should have been initialised to have a pointer to the data
 *  it represents, pointers to the GtkCList and GtkEntry widgets, and
 *  the chosen row should be -1.
 * If a GUI component hasn't been initialised, then the fact you can't 
 * populate data is probably the least of our worries.
 */

void populate_minilist(struct slist_gui *the_gui)
{
    struct fnlist *current;
    gchar *to_add[1];

    if (!the_gui) {
	NE_DEBUG(DEBUG_GNOME, "WARNING! gui component was null that should not have been.\n");
	return;
    }
    for (current = the_gui->data; current != NULL; current = current->next) {
	/* If the path is absolute, we actually want to display it without
	 * modifying the original text.
	 */
	if (current->haspath)
	  /* FIXME: This will barf when 'free'd. */
	    to_add[0] = g_strjoin(NULL, "/", current->pattern, NULL);
	else
	    to_add[0] = current->pattern;
	the_gui->chosen_row = gtk_clist_append(GTK_CLIST(the_gui->list), to_add);
	gtk_clist_set_row_data(GTK_CLIST(the_gui->list), the_gui->chosen_row,
			       (gpointer) current);
    }
}

void select_minilist_item(GtkWidget * list, gint row, gint column,
			  GdkEventButton * event, gpointer data)
{
    struct slist_gui *the_gui = (struct slist_gui *) data;
    gchar *to_change;
    the_gui->chosen_row = row;

    gtk_clist_get_text(GTK_CLIST(the_gui->list), row, 0, &to_change);
    gtk_entry_set_text(GTK_ENTRY(the_gui->entry), to_change);
    gtk_entry_select_region(GTK_ENTRY(the_gui->entry), 0, -1);
}

void change_minilist_entry(GtkWidget * entry, gpointer data)
{
    gchar *text;
    extern gboolean rcfile_saved;
    struct slist_gui *the_gui = (struct slist_gui *) data;
    struct fnlist *selected_data_struct;

    if (the_gui->chosen_row < 0)
	return;
    
    selected_data_struct = (struct fnlist *) gtk_clist_get_row_data(GTK_CLIST(the_gui->list),
								    the_gui->chosen_row);
    
    g_assert (selected_data_struct != NULL);
    g_assert (the_gui != NULL);
    
    text = strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    if (! xsc_replace_string(&(selected_data_struct->pattern), text))
      return;

    gtk_clist_set_text(GTK_CLIST(the_gui->list), the_gui->chosen_row, 0, text);
    
    /* TODO: Find out how Joe allocates these things, and put in routines
     * to make sure any existing memory is always freed.
     */
    
    selected_data_struct->haspath = FALSE;
    if (*text == '/') {
	selected_data_struct->haspath = TRUE;
	/* Strip that leading slash. */
	selected_data_struct->pattern++;
    }
    gtk_widget_grab_focus(GTK_WIDGET(the_gui->entry));
    rcfile_saved = FALSE;
}

void add_minilist_item(GtkWidget * button, gpointer data)
{
    int added_row;
    gchar *to_add[1];
    struct slist_gui *the_gui = (struct slist_gui *) data;
    struct fnlist *current, *new_one;

    if (!the_gui) {
	NE_DEBUG(DEBUG_GNOME, "WARNING: gui component was null.\n");
	return;
    }
    to_add[0] = strdup("New filename or expression");
    added_row = gtk_clist_append(GTK_CLIST(the_gui->list), to_add);

    current = the_gui->data;

    if (current != NULL)
	while (current->next != NULL)
	    current = current->next;

    new_one = malloc(sizeof(struct fnlist));
    new_one->pattern = to_add[0];
    new_one->next = NULL;
    if (!current) {
	new_one->prev = NULL;
	the_gui->data = new_one;
	switch (the_gui->type) {
	case list_exclude:
	    selected_site->excludes = the_gui->data;
	    the_gui->data = selected_site->excludes;
	    break;
	case list_ascii:
	    selected_site->asciis = the_gui->data;
	    the_gui->data = selected_site->asciis;
	    break;
	case list_ignore:
	    selected_site->ignores = the_gui->data;
	    the_gui->data = selected_site->ignores;
	    break;
	}
    } else {
	new_one->prev = current;
	current->next = new_one;
    }

    gtk_clist_set_row_data(GTK_CLIST(the_gui->list), added_row,
			   (gpointer) new_one);
    gtk_clist_select_row(GTK_CLIST(the_gui->list), added_row, 0);
}

void remove_minilist_item(GtkWidget * button, gpointer data)
{
    struct fnlist *current, *to_free;
    struct slist_gui *the_gui = (struct slist_gui *) data;

    if (!the_gui) {
	NE_DEBUG(DEBUG_GNOME, "Warning! gui thingy was null.\n");
	return;
    }
    if (the_gui->chosen_row < 0)
	return;
    current = gtk_clist_get_row_data(GTK_CLIST(the_gui->list),
				     the_gui->chosen_row);
    gtk_clist_remove(GTK_CLIST(the_gui->list), the_gui->chosen_row);

    to_free = current;
    if (current->prev) {
	current->prev->next = current->next;
	if (current->next) {
	    current->next->prev = current->next->prev->prev;
	}
    } else {
	/* ick */
	/* Small memory leak alert */
	switch (the_gui->type) {
	case list_exclude:
	    selected_site->excludes = the_gui->data->next;
	    the_gui->data = selected_site->excludes;
	    NE_DEBUG(DEBUG_GNOME, "Removing an item from the excludes list.\n");
	    break;
	case list_ascii:
	    selected_site->asciis = the_gui->data->next;
	    the_gui->data = selected_site->asciis;
	    NE_DEBUG(DEBUG_GNOME, "Removing an item from the ASCIIs list.\n");
	    break;
	case list_ignore:
	    selected_site->ignores = the_gui->data->next;
	    the_gui->data = selected_site->ignores;
	    NE_DEBUG(DEBUG_GNOME, "Removing an item from the ignores list.\n");
	    break;
	default:
	    ;
	}
	if (the_gui->data)
	    the_gui->data->prev = NULL;
    }

    the_gui->chosen_row = -1;
    gtk_entry_set_text(GTK_ENTRY(the_gui->entry), "");
    free(to_free);
}
