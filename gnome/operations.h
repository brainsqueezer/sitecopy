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

#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <setjmp.h>
#include <signal.h>

#include <gnome.h>
#include "gcommon.h"
#include "sites.h"
#include "common.h"
#include "rcfile.h"
#include "gcommon.h"
#include "misc.h"

#include "config.h"
#include "tree.h"

enum site_op
{
    site_op_update, site_op_fetch, site_op_resync
};

/* Abort handling */
void handle_abort( int sig );
void do_abort(void);
void fe_disable_abort( struct site *site );
void fe_enable_abort( struct site *site );
int my_abortable_transfer_wrapper( struct site *site, 
				  enum site_op operation );

/* Updates */
void *update_all_thread(void *no_data);
void *update_thread(void *no_data);
void close_main_update_window(GtkButton * button, gpointer data);
void abort_site_update(GtkWidget * button, gpointer data);
int start_main_update(GtkWidget *button, gpointer single_or_all);
int main_update_please(GtkWidget * update_button, gpointer data);

/* File List changes */
int fe_catchup_site(void);
void catchup_selected(gint button_number, gpointer data);
int fe_init_site(void);
void initialize_selected(gint button_number, gpointer data);

/* Disk accesses */
void save_default(void);	/* Saves site definitions to current value of 'rcfile' */

/* Site List changes */
void delete_a_site(GtkWidget * button_or_menu, gpointer data);
struct site *find_prev_site(struct site *a_site);	/* Used for delete_a_site */
void delete_selected(gint button_number);

#endif				/* OPERATIONS_H */
