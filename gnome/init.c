/*
 *      XSitecopy, for managing remote web sites with a GNOME interface.
 *      Copyright (C) 2000, 2005, Lee Mallabone <lee@fonicmonkey.net>
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
#include "init.h"

GtkWidget *site_tree, *app_bar;
GtkWidget *main_area_box, *area_data;
GtkWidget *popup_win, *popup_label, *popup_frame;
int site_keepgoing = FALSE;
extern GtkWidget *sitecopy;
extern struct site *all_sites;

struct xsitecopy_prefs *main_prefs, *edited_prefs;

gboolean fatal_error_encountered = FALSE;

int create_about(void)
{
   GtkWidget *about_window;
   const gchar *people[] =
     {"Lee Mallabone <lee@fonicmonkey.net>", "Joe Orton <joe@manyfish.co.uk>", NULL};
   about_window = gnome_about_new("XSitecopy", PACKAGE_VERSION, "Copyright (C) 1999,2000 Lee Mallabone", people,
				  "XSitecopy is the official GNOME front end to sitecopy.\nSitecopy is for copying locally stored websites to remote web servers.", NULL);
   gtk_widget_show(about_window);
   return 1;
}

int create_main_window(void)
{
   GtkWidget *ctree_scroller, *app_contents, *panes;

    /* MENUS */
   static GnomeUIInfo file_menu[] =
     {
	  {GNOME_APP_UI_ITEM, N_("_New site..."), N_("Create a new site definition"), start_new_site_wizard, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW, 'n', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("_Open..."), N_("Open a new rc file. (EXPERIMENTAL)."), open_new_request, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN, 'o', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("_Save Sites"), N_("Save the current site definitions"), save_default, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 's', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Sav_e Sites As..."), N_("Save the current site definitions to a file."), saveas_request, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 'e', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("_Delete this site"), N_("Delete the selected site entry"), delete_a_site, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_TRASH, 'w', GDK_CONTROL_MASK, NULL
	  },

	GNOMEUIINFO_SEPARATOR,

	  {GNOME_APP_UI_ITEM, N_("E_xit"), N_("Exit the program"), quit_please, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'q', GDK_CONTROL_MASK, NULL
	  },

	GNOMEUIINFO_END
     };

   static GnomeUIInfo ops_menu[] =
     {
	  {GNOME_APP_UI_ITEM, N_("_Initialise site"), N_("All files will be uploaded if a site is initialised"), fe_init_site, NULL, NULL,
	       GNOME_APP_PIXMAP_NONE, NULL, 'i', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("'_Catchup' site"), N_("Records the site as already updated."), fe_catchup_site, NULL, NULL,
	       GNOME_APP_PIXMAP_NONE, NULL, 'c', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("_Fetch site listing"), N_("Figures out which files are new/changed from the actual remote site."), fetch_site_list_please, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REVERT, 'f', GDK_CONTROL_MASK, NULL
	  },
	GNOMEUIINFO_SEPARATOR,
	  {GNOME_APP_UI_ITEM, N_("_Resynchronize site"), N_("Copies newer files from the remote site to the local drive."), NULL, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REFRESH, 'y', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("_Update site..."), N_("Apply the local site's changes to the remote site."), main_update_please, "single", NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REDO, 'u', GDK_CONTROL_MASK, NULL
	  },
	  { GNOME_APP_UI_ITEM, N_("Update _ALL sites"), N_("Performs the required operations on all site definitions. (TODO)."), main_update_please, "all", NULL,
	       GNOME_APP_PIXMAP_NONE, NULL, 'e', GDK_CONTROL_MASK, NULL
	  },
	GNOMEUIINFO_SEPARATOR,
	  {GNOME_APP_UI_ITEM, N_("Rescan local directory"), N_("Re-reads the local directory of the selected site."), rescan_selected, NULL, NULL,
	       GNOME_APP_PIXMAP_NONE, NULL, 'r', GDK_CONTROL_MASK, NULL
	  },
	GNOMEUIINFO_END
     };

   static GnomeUIInfo report_menu[] =
     {
	  {GNOME_APP_UI_ITEM, N_("_Required updates"), N_("Displays a brief report of any required updates."), site_report, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW, 'r', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Site web-r_eport"), N_("Generates a web page detailing the changes required on the selected site."), site_web_report, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW, 'e', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Prin_t site info..."), N_("Print information about the selected site"), NULL, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT, 't', GDK_CONTROL_MASK, NULL
	  },
	GNOMEUIINFO_END
     };
   static GnomeUIInfo backup_menu[] =
     {
	  {GNOME_APP_UI_ITEM, N_("_Backup files status"), N_("Save the 'state' of your local files, in case of accident."), backup_infofile, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 'b', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Restore files status..."), N_("If a backup has previously been made, it will be restored."), restore_infofile, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 'r', GDK_CONTROL_MASK, NULL
	  },
	GNOMEUIINFO_SEPARATOR,
	  {GNOME_APP_UI_ITEM, N_("_Backup site definitions"), N_("Backup your site definitions, in case you delete your sites."), backup_rcfile, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 't', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Restore site definitions..."), N_("If a backup has previously been made, it will be restored."), restore_rcfile, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 'r', GDK_CONTROL_MASK, NULL
	  },
	GNOMEUIINFO_END
     };

   static GnomeUIInfo prefs_menu[] =
     {
	  {GNOME_APP_UI_ITEM, N_("Preferences..."), N_("Performs the required operations on all site definitions. (TODO)."), NULL, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF, 'p', GDK_CONTROL_MASK, NULL
	  },

	GNOMEUIINFO_END
     };

   static GnomeUIInfo help_menu[] =
     {
	  {GNOME_APP_UI_ITEM, N_("_About XSitecopy"), N_("Information about XSitecopy"),
	     create_about, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL
	  },

	GNOMEUIINFO_SEPARATOR,
	  GNOMEUIINFO_HELP("xsitecopy"),
	GNOMEUIINFO_END
     };

   static GnomeUIInfo main_menu[] =
     {
	GNOMEUIINFO_SUBTREE(N_("_File"), file_menu),
	GNOMEUIINFO_SUBTREE(N_("_Operations"), ops_menu),
	GNOMEUIINFO_SUBTREE(N_("_Reports"), report_menu),
	GNOMEUIINFO_SUBTREE(N_("_Settings"), prefs_menu),
	GNOMEUIINFO_SUBTREE(N_("_Backup"), backup_menu),
	GNOMEUIINFO_SUBTREE(N_("_Help"), help_menu),
	GNOMEUIINFO_END
     };

    /* TOOLBAR */

   static GnomeUIInfo main_toolbar[] =
     {
	  {GNOME_APP_UI_ITEM, N_("New site"), N_("Create a new site definition"), start_new_site_wizard, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW, 'n', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Open"), N_("Open a new site definitions file."), open_new_request, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN, 'o', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Save"), N_("Save the current site definitions"), save_default, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 's', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Delete site"), N_("Delete the selected site"), delete_a_site, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_CUT, 'w', GDK_CONTROL_MASK, NULL
	  },

	GNOMEUIINFO_SEPARATOR,
	  {GNOME_APP_UI_ITEM, N_("Initialise"), N_("Initialise the site (mark all files as needing to be updated)."), fe_init_site, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_ABOUT, 'i', GDK_CONTROL_MASK, NULL
	  },
	  {GNOME_APP_UI_ITEM, N_("Catch-up"), N_("Mark all of the selected site's files as updated."), fe_catchup_site, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_ABOUT, 'c', GDK_CONTROL_MASK, NULL
	  },

	GNOMEUIINFO_SEPARATOR,

	  {GNOME_APP_UI_ITEM, N_("Download"), N_("Download newer files from the remote site. (Use with care)"), GTK_SIGNAL_FUNC(main_update_please), NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_UNDO, 'u', GDK_CONTROL_MASK, NULL
	  },

	  {GNOME_APP_UI_ITEM, N_("Update"), N_("Updates the selected site"), GTK_SIGNAL_FUNC(main_update_please), "single", NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_REDO, 'u', GDK_CONTROL_MASK, NULL
	  },

	  {GNOME_APP_UI_ITEM, N_("Quit"), N_("Exit the program"), quit_please, NULL, NULL,
	       GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_EXIT, 'q', GDK_CONTROL_MASK, NULL
	  },

	GNOMEUIINFO_END
     };
   extern GList *errors;
   GtkWidget *errs;

    /* MENUS */
   gnome_app_create_menus(GNOME_APP(sitecopy), main_menu);
    /* TOOLBARS */
   gnome_app_create_toolbar(GNOME_APP(sitecopy),
			    main_toolbar);

    /* PANES */
   site_tree = ctree_create_sites();
   panes = gtk_hpaned_new();
   gtk_paned_set_gutter_size(GTK_PANED(panes), 12);
   gtk_widget_show(panes);

    /* Arbitrary widget that holds all the site/file detailed info. */
   main_area_box = gtk_hbox_new(FALSE, 2);

    /* Widget is practically arbitrary */
   area_data = create_initial_main_area();
    /* FIXME: Add some default data to area_data */
   gtk_container_add(GTK_CONTAINER(main_area_box), area_data);
   gtk_widget_show(main_area_box);

   app_contents = gtk_vbox_new(FALSE, 0);
   ctree_scroller = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ctree_scroller),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
   gtk_widget_show(ctree_scroller);
   gtk_container_add(GTK_CONTAINER(ctree_scroller), site_tree);

   gtk_paned_pack1(GTK_PANED(panes), ctree_scroller, false, false);
   gtk_paned_pack2(GTK_PANED(panes), main_area_box, false, false);
    /*gtk_container_set_border_width (GTK_CONTAINER (GTK_PANED(panes)->child2), 5); */

    /* APP BAR */
   app_bar = gnome_appbar_new(FALSE, TRUE, GNOME_PREFERENCES_USER);
   gtk_widget_show(app_bar);

    /* Add some hints and stuff */
   gnome_app_install_appbar_menu_hints(GNOME_APPBAR(app_bar),
				       main_menu);

    /* Pack it all in */
   gtk_box_pack_start(GTK_BOX(app_contents), panes, TRUE, TRUE, 0);
   gtk_box_pack_start(GTK_BOX(app_contents), app_bar, FALSE, FALSE, 0);

   gnome_app_set_contents(GNOME_APP(sitecopy), app_contents);

    /* FIXME: remove all this once it gets implemented */
   gtk_widget_set_sensitive(ops_menu[2].widget, FALSE);     /* fetch */
   gtk_widget_set_sensitive(ops_menu[4].widget, FALSE);	/* resynch */
   gtk_widget_set_sensitive(ops_menu[6].widget, FALSE);	/* Update all */
   gtk_widget_set_sensitive(report_menu[1].widget, FALSE);	/* web report */
   gtk_widget_set_sensitive(report_menu[2].widget, FALSE);	/* print */
   gtk_widget_set_sensitive(prefs_menu[0].widget, FALSE);	/* prefs */
   gtk_widget_set_sensitive(main_toolbar[8].widget, FALSE);	/* resynch */

    /* SHOW IT ALL */
   /*   gtk_window_set_default_size (GTK_WINDOW (sitecopy), 610, 376); */
   gtk_widget_show(sitecopy);
   if (errors)
     {
	errs = create_site_errors_dialog();
	gtk_widget_show(errs);
     }
   return 1;
}

/* Initialisation. Attempts to read configuration info and/or set sensible
 * defaults where appropriate.
 */

int xsitecopy_read_configuration(void)
{
   extern GList *errors;
    /* TODO:
     * Use the gnome_config functions to
     * setup preferences for different aspects of the UI/default file
     * names. Put all the gnome_config calls in this function and rename.
     */
   int ret;
   extern gchar *rcfile;
   struct stat *rc_stat, *info_dir_stat;
   gchar *rcfilename, *infodirname;
   FILE *rc;

    /* Allocate and set default preferences */

   main_prefs = malloc(sizeof(struct xsitecopy_prefs));
   main_prefs->label = local_dir;
   main_prefs->should_sort = TRUE;
   main_prefs->site_needs_update_col = 0;
   main_prefs->site_updated_col = 0;
   main_prefs->needs_update = NULL;
   main_prefs->in_sync = NULL;
   main_prefs->backup_state_before_fetch = FALSE;
   main_prefs->always_keep_going = FALSE;
   main_prefs->log_errors = FALSE;
   main_prefs->error_log_filename = FALSE;
   main_prefs->quit_confirmation = TRUE;
   main_prefs->save_confirmation = TRUE;

   rcfilename = g_strdup_printf("%s/.sitecopyrc",
				g_get_home_dir());
   infodirname = g_strdup_printf("%s/.sitecopy/",
				 g_get_home_dir());

    /* FIXME: Read configuration data using gnome-config */

   rc_stat = malloc(sizeof(struct stat));
   info_dir_stat = malloc(sizeof(struct stat));

   if (stat((const char *) rcfilename, rc_stat) != 0)
     {
	NE_DEBUG(DEBUG_GNOME, "creating rc...\n");
	rc = fopen(rcfilename, "a");
	fclose(rc);
	chmod(rcfilename, 00600);
     }
   if (stat((const char *) infodirname, info_dir_stat) != 0)
     {
	if (mkdir(infodirname, 00700) != 0)
	  {
	     errors = g_list_append(errors, "FATAL ERROR: Could not create required config directory.\n");
	     fatal_error_encountered = TRUE;
	  }
	else
	  {
	     chmod(infodirname, 00700);
	  }
     }
   if (init_env())
     {
	errors = g_list_append(errors, "FATAL ERROR: Could not setup the environment correctly!\n");
	fatal_error_encountered = TRUE;
     }
   ret = init_paths();
   switch (ret)
     {
      case RC_OPENFILE:
	errors = g_list_append(errors,
			       "FATAL ERROR: Could not read the site definitions.\n");
	fatal_error_encountered = TRUE;
	break;
      case RC_DIROPEN:
	errors = g_list_append(errors,
			       "FATAL ERROR: Could not get information about the file data storage directory.\n");
	fatal_error_encountered = TRUE;
	break;
      case RC_PERMS:
	errors = g_list_append(errors,
			       "WARNING: Your sitecopy configuration file has dangerous permissions.\nOther users may be able to read your passwords unless this is corrected.\n");
	break;
      case RC_DIRPERMS:
	errors = g_list_append(errors,
			       "WARNING: Your sitecopy storage directory (~/.sitecopy) has invalid or insecure permissions.\n");
	break;
      case RC_NETRCPERMS:
	errors = g_list_append(errors,
			       "WARNING: Your network configuration file (~/.netrc) has invalid or insecure permissions.\n");
	break;
      case 0:
	break;
      default:
	errors = g_list_append(errors, "Could not setup the environment correctly!\n");
	fatal_error_encountered = TRUE;
     }

   if (init_netrc())
     errors = g_list_append(errors,
			    "There was a problem parsing your ~/.netrc file.\n");

    /* If this fails, we're really screwed... hmmm.. Need to have a flag to
     * tell the errors dialog to just die when ok is pressed, if this has
     * failed.
     */
   /*if (rcfile_read(&all_sites))
     {
	printf("Error: Could not read rcfile: %s.\n",
	       rcfile);
	errors = g_list_append(errors, "Could not read your site definitions file! (usually ~/.sitecopyrc)\n");
	fatal_error_encountered = TRUE;
    */
   ret = rcfile_read(&all_sites);
   if (ret)
     {
        /* So we can use the same gettext strings as sitecopy. */
	char const *progname = "xsitecopy";

	switch (ret)
	  {
	   case RC_OPENFILE:
	     printf( _("%s: Error: Could not open rcfile: %s.\n"),
		    progname, rcfile );
	     break;

	   case RC_CORRUPT:
	     printf( _("%s: rcfile corrupt at line %d:\n%s\n"),
		    progname, rcfile_linenum, rcfile_err );
	     break;

	   default:
	     printf( _("%s: Error: Could not read rcfile: %s.\n"),
		    progname, rcfile );
	     break;
	  }
	errors = g_list_append(errors, "Could not read your site definitions file! (usually ~/.sitecopyrc)\n");
	fatal_error_encountered = TRUE;

     }
   return 1;
}
