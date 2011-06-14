/* 
 *    XSitecopy, for managing remote web sites with a GNOME interface.
 *    Copyright (C) 2000, Lee Mallabone <lee@fonicmonkey.net>
 *                                                                      
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *   
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *   
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "config.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <gnome.h>

#include "sites.h"
#include "rcfile.h"
#include "common.h"
#include "gcommon.h"
#include "init.h"
#include "misc.h"
/* For thread creation. */
#include "operations.h"
#include "resynch.h"

int generate_pixmaps(void);

/* The main gnome_app struct */
GtkWidget *sitecopy;
sem_t *update_semaphore = NULL, *fetch_semaphore = NULL;
sem_t *update_all_semaphore = NULL, *sync_semaphore = NULL;

pthread_t update_tid = 0, fetch_tid = 0, update_all_tid = 0, sync_tid = 0;

extern struct site *all_sites;

gboolean prompting = false;

/* Pixmaps that are used throughout. */

/* dir is closed, dir2 is open */
GdkPixmap *dir, *dir_new, *dir_changed, *dir_del;
GdkPixmap *dir2, *dir2_new, *dir2_changed, *dir2_del;
GdkPixmap *xsitecopy_file, *xsitecopy_file_new, *xsitecopy_file_changed, *xsitecopy_file_del, *xsitecopy_file_moved;
GdkPixmap *symbolic_link, *symbolic_link_new, *symbolic_link_changed, *symbolic_link_del;

/* Bitmaps */
GdkBitmap *dir_map, *dir_new_map, *dir_changed_map, *dir_del_map;
GdkBitmap *dir2_map, *dir2_new_map, *dir2_changed_map, *dir2_del_map;
GdkBitmap *xsitecopy_file_map, *xsitecopy_file_new_map, *xsitecopy_file_changed_map, *xsitecopy_file_del_map, *xsitecopy_file_moved_map;
GdkBitmap *symbolic_link_map, *symbolic_link_new_map, *symbolic_link_changed_map, *symbolic_link_del_map;


/* Initialises all the pixmaps above */

int generate_pixmaps(void)
{
    GtkStyle *style;
    
#include "dir_changed.xpm"
#include "dir_new.xpm"
#include "dir_del.xpm"
#include "dir.xpm"

#include "dir2_changed.xpm"
#include "dir2_new.xpm"
#include "dir2_del.xpm"
#include "dir2.xpm"
    
#include "file_changed.xpm"
#include "file_new.xpm"
#include "file_del.xpm"
#include "file_moved.xpm"
#include "file.xpm"
    
#include "link_changed.xpm"
#include "link_new.xpm"
#include "link_del.xpm"
#include "link.xpm"

    style = gtk_widget_get_style(sitecopy);

    /* Closed directory */
    dir = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &dir_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) dir_xpm);
    dir_new = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &dir_new_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) dir_new_xpm);
    dir_del = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &dir_del_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) dir_del_xpm);
    dir_changed = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &dir_changed_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) dir_changed_xpm);

    /* Open directory */
    
    dir2 = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &dir2_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) dir2_xpm);
    dir2_new = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &dir2_new_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) dir2_new_xpm);
    dir2_del = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &dir2_del_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) dir2_del_xpm);
    dir2_changed = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &dir2_changed_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) dir2_changed_xpm);

    xsitecopy_file = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &xsitecopy_file_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) file_xpm);
    xsitecopy_file_new = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &xsitecopy_file_new_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) file_new_xpm);
    xsitecopy_file_del = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &xsitecopy_file_del_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) file_del_xpm);
    xsitecopy_file_changed = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &xsitecopy_file_changed_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) file_changed_xpm);
    xsitecopy_file_moved = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &xsitecopy_file_moved_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) file_moved_xpm);

    symbolic_link = gdk_pixmap_create_from_xpm_d(sitecopy->window,
						 &symbolic_link_map,
						 &style->bg[GTK_STATE_NORMAL],
						 (gchar **) link_xpm);
    symbolic_link_new = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &symbolic_link_new_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) link_new_xpm);
    symbolic_link_del = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &symbolic_link_del_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) link_del_xpm);
    symbolic_link_changed = gdk_pixmap_create_from_xpm_d(sitecopy->window,
				       &symbolic_link_changed_map,
				       &style->bg[GTK_STATE_NORMAL],
				       (gchar **) link_changed_xpm);    
    return 1;
}
    

int main(int argc, char *argv[])
{

    static int debug_mask = 0;

    /* Command line options */
    static const struct poptOption options[] =
    {
#ifdef DEBUGGING
	/* joe: These need updating to the correct values, or better yet,
	 * nick the --debug option code out of console_fe.c... or even 
	 * better than that, I'll move it into common.c so we can all use
	 * it and live hapilly ever after. */
	{"debug", 'd', POPT_ARG_INT, &debug_mask, 0, N_("Debugging level (sum of: 1=socket, 2=files, 4=rcfile, 8=WebDAV, 16=FTP, 32=XML, 64=GNOME )"), "LEVEL"},
#endif
	{NULL, '\0', 0, NULL, 0}
    };
    
    /* Initialise threading */
    g_thread_init(NULL);

    /* Do the gnome init stuff, and create command line options */
    gnome_init_with_popt_table("Sitecopy", PACKAGE_VERSION, argc, argv,
			       options, 0, NULL);
    /* Initilize debugging */
    ne_debug_init(stderr, debug_mask);

    glade_gnome_init();
    
    /* Configuration / Gnome initialisation */
    xsitecopy_read_configuration();
    sitecopy = gnome_app_new("xsc", "XSitecopy");
    gtk_signal_connect(GTK_OBJECT(sitecopy), "delete_event",
		       GTK_SIGNAL_FUNC(quit_please), NULL);
    gtk_widget_realize(sitecopy);
    generate_pixmaps();

    /* Create semaphores to signal/wait for updates and fetches to be needed */
    
    update_semaphore = malloc(sizeof(sem_t));
    sem_init(update_semaphore, 0, 0);
    
    fetch_semaphore = malloc(sizeof(sem_t));
    sem_init(fetch_semaphore, 0, 0);

    sync_semaphore = malloc(sizeof(sem_t));
    sem_init(sync_semaphore, 0, 0);
    
    update_all_semaphore = malloc(sizeof(sem_t));
    sem_init(update_all_semaphore, 0, 0);
    
    /* Create a thread to do updates on, and another to do an 'update all' */
    
    if (pthread_create(&update_tid, NULL, update_thread, NULL) != 0)
      {
	  g_error("Could not create a thread - no I/O is possible, exiting...");
	  exit(-1);
      }

    if (pthread_create(&update_all_tid, NULL, update_all_thread, NULL) != 0)
      {
	  g_error("Could not create a thread - no I/O is possible, exiting...");
	  exit(-1);
      }

    /* Create a thread to do fetches on */
    
    if (pthread_create(&fetch_tid, NULL, fetch_thread, NULL) != 0)
      {
	  g_error("Could not create a fetch thread - no I/O is possible, exiting...");
	  exit(-1);
      }

    /* And make one for resync too! */
    
    if (pthread_create(&sync_tid, NULL, sync_thread, NULL) != 0)
      {
	  g_error("Could not create a sync thread - no I/O is possible, exiting...");
	  exit(-1);
      }

    /* Create the GUI and hit the main loop */
    create_main_window();

    gdk_threads_enter();
    gtk_main();
    gdk_threads_leave();
    exit(0);
    return 0;
}

