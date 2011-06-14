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

#ifndef GNOME_RESYNC_H
#define GNOME_RESYNC_H

#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <gnome.h>
#include "sites.h"
#include "rcfile.h"
#include "gcommon.h"
#include "misc.h"
#include "operations.h"

gboolean delete_fetch(GtkWindow *fnar, gpointer data);
int fetch_site_list_please(GtkWidget * update_button, gpointer data);
void close_fetch_window(void);
int start_fetch_list(void);
void *fetch_thread(void *no_data);
void *actual_fetch_list(void *nothing);
void fe_fetch_found(const struct site_file *file);
void *sync_thread(void *no_data);

#endif				/* GNOME_RESYNC_H */
