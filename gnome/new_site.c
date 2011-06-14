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

#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "new_site.h"

GtkWidget *site_druid = NULL;
struct site *site_to_add = NULL;

extern GtkWidget *the_tree;

/* This function taken from Glade's generated 'support.c' file. */

GdkImlibImage*
create_image                           (const gchar     *filename)
{
  GdkImlibImage *image;
  gchar *pathname;

  pathname = gnome_pixmap_file (filename);
  if (!pathname)
    {
      g_warning ("Couldn't find pixmap file: %s", filename);
      return NULL;
    }

  image = gdk_imlib_load_image (pathname);
  g_free (pathname);
  return image;
}

/* After using this function, the info file must still be set before 
 * the site can be used. This only initialises *core* stuff. More should
 * be done before adding the site to the sites list and/or verifying it.
 */

struct site *default_site(void)
{
    struct site *new_site = g_malloc0(sizeof(struct site));
    new_site->protocol = siteproto_unknown;
/*    new_site->driver = &ftp_driver;*/
    new_site->server.hostname = g_strdup("remote.site.com");
    new_site->server.port = 21;
    new_site->server.username = g_strdup("joe.bloggs");
    new_site->server.password = g_strdup("password");

    new_site->local_root_user = g_get_home_dir();
    new_site->remote_root_user = g_strdup("~/");

    /* Let's be conservative. If the user wants more exotic options
     * they have plenty of time to set them for themselves.
     */
    new_site->perms = sitep_ignore;
    new_site->symlinks = sitesym_ignore;
    new_site->state_method = state_timesize;
    return new_site;
}

void add_site(gpointer user_data)
{
    struct site *this_site, *last_site;
    int mode = 0;
    
    this_site = g_malloc0( sizeof( struct site ) );
    memcpy( this_site, site_to_add, sizeof( struct site ) );

    if (verifysite_gnome(this_site) != 0) 
      {
	 g_assert_not_reached();
	  return;
      }
    
    /* Simple case: we have no sites */
    if (!all_sites)
      {
	  all_sites = this_site;
      }
   else
     {
   
	/* Find the end of the sites list */
	for(last_site = all_sites;
	    last_site->next != NULL;
	    last_site = last_site->next)
	  ;
	
	this_site->prev = last_site;    
	this_site->next = NULL;
	
	if (this_site->prev)
	  this_site->prev->next = this_site;
     }
   
    if (GTK_TOGGLE_BUTTON(get_widget(site_druid, "new_init"))->active)
      {
	  printf("Will init the site...\n");
	  mode = 1;
      }
    
    else
      {
	  mode = 2;
	  printf("will catchup the site...\n");
      }
    site_write_stored_state(this_site);
    if (add_a_site_to_the_tree(this_site, mode) < 0)
      gnome_error_dialog("There was a problem creating the site");
    memset(site_to_add, 0, sizeof(struct site));
    return;
}

/** PRE: site_to_add should already have been allocated with g_malloc **/

void close_druid(void)
{

    if (site_to_add) {
	g_free(site_to_add);
	site_to_add = NULL;
    }
    gtk_widget_destroy(site_druid);
    site_druid = NULL;
}

GtkWidget *
 create_site_druid()
{
/**** Start of glade generated code ****/
    GtkWidget *new_site_druid;
    GtkWidget *start_page;
    GdkColor start_page_logo_bg_color =
    {0, 6400, 6400, 28672};
    GtkWidget *general_info;
    GdkColor general_info_logo_bg_color =
    {0, 6400, 6400, 28672};
    GtkWidget *druid_vbox1;
    GtkWidget *vbox28;
    GtkWidget *label67;
    GtkWidget *hseparator5;
    GtkWidget *table12;
    GtkWidget *new_url;
    GtkWidget *label86;
    GtkWidget *label85;
    GtkWidget *new_name;
    GtkWidget *server_info;
    GdkColor server_info_logo_bg_color =
    {0, 6400, 6400, 28672};
    GtkWidget *druid_vbox2;
    GtkWidget *vbox27;
    GtkWidget *label66;
    GtkWidget *hseparator4;
    GtkWidget *table8;
    GtkWidget *new_hostname;
    GtkWidget *hbox16;
    GSList *new_proto_group = NULL;
    GtkWidget *new_ftp;
    GtkWidget *new_http;
    GtkObject *new_port_adj;
    GtkWidget *new_port;
    GtkWidget *new_username;
    GtkWidget *new_password;
    GtkWidget *label64;
    GtkWidget *label63;
    GtkWidget *label65;
    GtkWidget *label62;
    GtkWidget *label61;
    GtkWidget *directories;
    GdkColor directories_logo_bg_color =
    {0, 6400, 6400, 28672};
    GtkWidget *druid_vbox6;
    GtkWidget *vbox29;
    GtkWidget *label68;
    GtkWidget *hseparator9;
    GtkWidget *table11;
    GtkWidget *new_local_fentry;
    GtkWidget *new_local_entry;
    GtkWidget *label82;
    GtkWidget *label81;
    GtkWidget *new_remote_entry;
    GtkWidget *file_handling;
    GdkColor file_handling_logo_bg_color =
    {0, 6400, 6400, 28672};
    GtkWidget *druid_vbox3;
    GtkWidget *vbox30;
    GtkWidget *label69;
    GtkWidget *hseparator6;
    GtkWidget *table9;
    GtkWidget *label71;
    GtkWidget *hbox18;
    GSList *perms_group = NULL;
    GtkWidget *new_perms_ignore;
    GtkWidget *new_perms_exec;
    GtkWidget *new_perms_all;
    GtkWidget *label72;
    GtkWidget *hbox17;
    GSList *sym_link_group = NULL;
    GtkWidget *new_sym_follow;
    GtkWidget *new_sym_ignore;
    GtkWidget *new_sym_maintain;
    GtkWidget *label73;
    GtkWidget *hbox19;
    GSList *state_detector_group = NULL;
    GtkWidget *new_timesize;
    GtkWidget *new_checksum;
    GtkWidget *update_options;
    GdkColor update_options_logo_bg_color =
    {0, 6400, 6400, 28672};
    GtkWidget *druid_vbox4;
    GtkWidget *vbox31;
    GtkWidget *label70;
    GtkWidget *hseparator7;
    GtkWidget *vbox32;
    GtkWidget *new_nodelete;
    GtkWidget *new_checkmoved;
    GtkWidget *new_nooverwrite;
    GtkWidget *new_lowercase;
    GtkWidget *new_safe;
    GtkWidget *new_pasv;
    GtkWidget *server_state;
    GdkColor server_state_logo_bg_color =
    {0, 6400, 6400, 28672};
    GtkWidget *druid_vbox5;
    GtkWidget *vbox33;
    GtkWidget *label74;
    GtkWidget *hseparator8;
    GtkWidget *vbox34;
    GSList *vbox34_group = NULL;
    GtkWidget *new_init;
    GtkWidget *new_catchup;
    GtkWidget *new_fetch_please;
    GtkWidget *fetch_a_list;
    GdkColor fetch_a_list_logo_bg_color =
    {0, 6400, 6400, 28672};
    GtkWidget *druid_vbox10;
    GtkWidget *label91;
    GtkWidget *hseparator10;
    GtkWidget *vbox36;
    GtkWidget *hbox23;
    GtkWidget *label88;
    GtkWidget *new_fetch_status;
    GtkWidget *hbox24;
    GtkWidget *label90;
    GtkWidget *fetch_num_files_got;
    GtkWidget *new_fetch_begin;
    GtkWidget *hbuttonbox2;
    GtkWidget *finish_page;
    GdkColor finish_page_logo_bg_color =
    {0, 6400, 6400, 28672};

    site_druid = gtk_window_new(GTK_WINDOW_DIALOG);
    gtk_object_set_data(GTK_OBJECT(site_druid), "site_druid", site_druid);
    gtk_window_set_title(GTK_WINDOW(site_druid), "Create a new site definition");
    gtk_window_set_default_size(GTK_WINDOW(site_druid), 510, 340);
    gtk_window_set_policy(GTK_WINDOW(site_druid), TRUE, TRUE, FALSE);

    new_site_druid = gnome_druid_new();
    gtk_widget_ref(new_site_druid);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_site_druid", new_site_druid,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_site_druid);
    gtk_container_add(GTK_CONTAINER(site_druid), new_site_druid);

    start_page = gnome_druid_page_start_new();
    gtk_widget_ref(start_page);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "start_page", start_page,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(start_page);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(start_page));
    gnome_druid_set_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(start_page));
    gnome_druid_page_start_set_logo_bg_color(GNOME_DRUID_PAGE_START(start_page), &start_page_logo_bg_color);
    gnome_druid_page_start_set_title(GNOME_DRUID_PAGE_START(start_page), "Site Creation Druid");
    gnome_druid_page_start_set_text(GNOME_DRUID_PAGE_START(start_page), "This Druid will help you to create a new web site definition,\nthat Xsitecopy can then keep track of to ensure your remote\nsite remains in sync with the local copy.\n\nIn order to do that, you need to answer the questions on the\nfollowing pages. If you get stuck at all, be sure to consult the\ninformation available from Xsitecopy's help menu.\n\nNot all the details are mandatory, but you should enter as\nmuch as possible to gain the best performance.");
    gnome_druid_page_start_set_logo(GNOME_DRUID_PAGE_START(start_page),
				    create_image("gnome-networktool.png"));
/*    gnome_druid_page_start_set_watermark(GNOME_DRUID_PAGE_START(start_page),
				 create_image("xsitecopy/xsc_side.xpm"));
*/
    general_info = gnome_druid_page_standard_new_with_vals("", NULL);
    gtk_widget_ref(general_info);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "general_info", general_info,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show_all(general_info);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(general_info));
    gnome_druid_page_standard_set_logo_bg_color(GNOME_DRUID_PAGE_STANDARD(general_info), &general_info_logo_bg_color);
    gnome_druid_page_standard_set_title(GNOME_DRUID_PAGE_STANDARD(general_info), "General Information");
    gnome_druid_page_standard_set_logo(GNOME_DRUID_PAGE_STANDARD(general_info),
			create_image("gnome-networktool.png"));

    druid_vbox1 = GNOME_DRUID_PAGE_STANDARD(general_info)->vbox;
    gtk_widget_ref(druid_vbox1);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "druid_vbox1", druid_vbox1,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(druid_vbox1);

    vbox28 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox28);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox28", vbox28,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox28);
    gtk_box_pack_start(GTK_BOX(druid_vbox1), vbox28, TRUE, TRUE, 0);

    label67 = gtk_label_new("First, you should give this site a 'friendly' name to refer to it by. This name is also used by sitecopy (the command line utility) to reference sites.\n\nYou should also enter the root URL of your site. For example, http://www.geocities.com/~xsitecopy-user/\nThis will aid in the generation of web-based reports.");
    gtk_widget_ref(label67);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label67", label67,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label67);
    gtk_box_pack_start(GTK_BOX(vbox28), label67, TRUE, TRUE, 0);
    gtk_widget_set_usize(label67, 370, -2);
    gtk_label_set_justify(GTK_LABEL(label67), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap(GTK_LABEL(label67), TRUE);

    hseparator5 = gtk_hseparator_new();
    gtk_widget_ref(hseparator5);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hseparator5", hseparator5,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hseparator5);
    gtk_box_pack_start(GTK_BOX(vbox28), hseparator5, TRUE, TRUE, 0);

    table12 = gtk_table_new(2, 2, FALSE);
    gtk_widget_ref(table12);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "table12", table12,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(table12);
    gtk_box_pack_start(GTK_BOX(vbox28), table12, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(table12), 5);

    new_url = gtk_entry_new();
    gtk_widget_ref(new_url);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_url", new_url,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_url);
    gtk_table_attach(GTK_TABLE(table12), new_url, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    label86 = gtk_label_new("Root URL: ");
    gtk_widget_ref(label86);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label86", label86,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label86);
    gtk_table_attach(GTK_TABLE(table12), label86, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label86), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label86), 0, 0.5);

    label85 = gtk_label_new("Friendly name: ");
    gtk_widget_ref(label85);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label85", label85,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label85);
    gtk_table_attach(GTK_TABLE(table12), label85, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label85), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label85), 0, 0.5);

    new_name = gtk_entry_new();
    gtk_widget_ref(new_name);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_name", new_name,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_name);
    gtk_table_attach(GTK_TABLE(table12), new_name, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    server_info = gnome_druid_page_standard_new_with_vals("", NULL);
    gtk_widget_ref(server_info);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "server_info", server_info,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show_all(server_info);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(server_info));
    gnome_druid_page_standard_set_logo_bg_color(GNOME_DRUID_PAGE_STANDARD(server_info), &server_info_logo_bg_color);
    gnome_druid_page_standard_set_title(GNOME_DRUID_PAGE_STANDARD(server_info), "Server Information");
    gnome_druid_page_standard_set_logo(GNOME_DRUID_PAGE_STANDARD(server_info),
				       create_image("mc/i-sock.png"));

    druid_vbox2 = GNOME_DRUID_PAGE_STANDARD(server_info)->vbox;
    gtk_widget_ref(druid_vbox2);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "druid_vbox2", druid_vbox2,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(druid_vbox2);

    vbox27 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox27);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox27", vbox27,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox27);
    gtk_box_pack_start(GTK_BOX(druid_vbox2), vbox27, TRUE, TRUE, 0);

    label66 = gtk_label_new("Now enter details about the remote server that your website is stored on. When selecting the protocol, an appropriate default port number will be provided.\nYour password will not appear on screen.");
    gtk_widget_ref(label66);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label66", label66,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label66);
    gtk_box_pack_start(GTK_BOX(vbox27), label66, TRUE, TRUE, 0);
    gtk_widget_set_usize(label66, 370, -2);
    gtk_label_set_justify(GTK_LABEL(label66), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap(GTK_LABEL(label66), TRUE);

    hseparator4 = gtk_hseparator_new();
    gtk_widget_ref(hseparator4);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hseparator4", hseparator4,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hseparator4);
    gtk_box_pack_start(GTK_BOX(vbox27), hseparator4, TRUE, TRUE, 0);

    table8 = gtk_table_new(5, 2, FALSE);
    gtk_widget_ref(table8);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "table8", table8,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(table8);
    gtk_box_pack_start(GTK_BOX(vbox27), table8, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(table8), 5);

    new_hostname = gtk_entry_new();
    gtk_widget_ref(new_hostname);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_hostname", new_hostname,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_hostname);
    gtk_table_attach(GTK_TABLE(table8), new_hostname, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    hbox16 = gtk_hbox_new(FALSE, 0);
    gtk_widget_ref(hbox16);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hbox16", hbox16,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hbox16);
    gtk_table_attach(GTK_TABLE(table8), hbox16, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    new_ftp = gtk_radio_button_new_with_label(new_proto_group, "FTP");
    new_proto_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_ftp));
    gtk_widget_ref(new_ftp);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_ftp", new_ftp,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_ftp);
    gtk_box_pack_start(GTK_BOX(hbox16), new_ftp, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(new_ftp), TRUE);

#ifdef USE_DAV
    new_http = gtk_radio_button_new_with_label(new_proto_group, "WebDAV");
    new_proto_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_http));
    gtk_widget_ref(new_http);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_http", new_http,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_http);
    gtk_box_pack_start(GTK_BOX(hbox16), new_http, FALSE, FALSE, 0);
#endif				/* USE_DAV */

    new_port_adj = gtk_adjustment_new(21, 1, 65536, 1, 10, 10);
    new_port = gtk_spin_button_new(GTK_ADJUSTMENT(new_port_adj), 1, 0);
    gtk_widget_ref(new_port);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_port", new_port,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_port);
    gtk_table_attach(GTK_TABLE(table8), new_port, 1, 2, 2, 3,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    new_username = gtk_entry_new();
    gtk_widget_ref(new_username);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_username", new_username,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_username);
    gtk_table_attach(GTK_TABLE(table8), new_username, 1, 2, 3, 4,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    new_password = gtk_entry_new();
    gtk_widget_ref(new_password);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_password", new_password,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_password);
    gtk_table_attach(GTK_TABLE(table8), new_password, 1, 2, 4, 5,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_visibility(GTK_ENTRY(new_password), FALSE);

    label64 = gtk_label_new("Username: ");
    gtk_widget_ref(label64);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label64", label64,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label64);
    gtk_table_attach(GTK_TABLE(table8), label64, 0, 1, 3, 4,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label64), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label64), 0, 0.5);

    label63 = gtk_label_new("Port Number: ");
    gtk_widget_ref(label63);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label63", label63,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label63);
    gtk_table_attach(GTK_TABLE(table8), label63, 0, 1, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    label65 = gtk_label_new("Password: ");
    gtk_widget_ref(label65);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label65", label65,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label65);
    gtk_table_attach(GTK_TABLE(table8), label65, 0, 1, 4, 5,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label65), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label65), 0, 0.5);

    label62 = gtk_label_new("Protocol: ");
    gtk_widget_ref(label62);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label62", label62,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label62);
    gtk_table_attach(GTK_TABLE(table8), label62, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label62), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label62), 0, 0.5);

    label61 = gtk_label_new("Host name: ");
    gtk_widget_ref(label61);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label61", label61,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label61);
    gtk_table_attach(GTK_TABLE(table8), label61, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label61), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label61), 0, 0.5);

    directories = gnome_druid_page_standard_new_with_vals("", NULL);
    gtk_widget_ref(directories);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "directories", directories,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show_all(directories);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(directories));
    gnome_druid_page_standard_set_logo_bg_color(GNOME_DRUID_PAGE_STANDARD(directories), &directories_logo_bg_color);
    gnome_druid_page_standard_set_title(GNOME_DRUID_PAGE_STANDARD(directories), "Directories");
    gnome_druid_page_standard_set_logo(GNOME_DRUID_PAGE_STANDARD(directories),
			      create_image("mc/i-directory.png"));

    druid_vbox6 = GNOME_DRUID_PAGE_STANDARD(directories)->vbox;
    gtk_widget_ref(druid_vbox6);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "druid_vbox6", druid_vbox6,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(druid_vbox6);

    vbox29 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox29);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox29", vbox29,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox29);
    gtk_box_pack_start(GTK_BOX(druid_vbox6), vbox29, TRUE, TRUE, 0);

    label68 = gtk_label_new("I now need to know where you store the files that make up your web site; both locally, and on the remote site.\nDirectories must either be absolute (eg. /home/lee/web/), or relative to your login directory, (eg. ~/web/).\n\nAll directories should end with a trailing slash character.");
    gtk_widget_ref(label68);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label68", label68,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label68);
    gtk_box_pack_start(GTK_BOX(vbox29), label68, TRUE, TRUE, 0);
    gtk_widget_set_usize(label68, 370, -2);
    gtk_label_set_justify(GTK_LABEL(label68), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap(GTK_LABEL(label68), TRUE);

    hseparator9 = gtk_hseparator_new();
    gtk_widget_ref(hseparator9);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hseparator9", hseparator9,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hseparator9);
    gtk_box_pack_start(GTK_BOX(vbox29), hseparator9, TRUE, TRUE, 0);

    table11 = gtk_table_new(2, 2, FALSE);
    gtk_widget_ref(table11);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "table11", table11,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(table11);
    gtk_box_pack_start(GTK_BOX(vbox29), table11, TRUE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(table11), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table11), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table11), 2);

    new_local_fentry = gnome_file_entry_new(NULL, NULL);
    gnome_file_entry_set_directory(GNOME_FILE_ENTRY(new_local_fentry), TRUE);
    gtk_widget_ref(new_local_fentry);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_local_fentry", new_local_fentry,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_local_fentry);
    gtk_table_attach(GTK_TABLE(table11), new_local_fentry, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    new_local_entry = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(new_local_fentry));
    gtk_widget_ref(new_local_entry);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_local_entry", new_local_entry,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_local_entry);

    label82 = gtk_label_new("Directory for local files: ");
    gtk_widget_ref(label82);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label82", label82,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label82);
    gtk_table_attach(GTK_TABLE(table11), label82, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label82), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label82), 0, 0.5);

    label81 = gtk_label_new("Directory for remote files: ");
    gtk_widget_ref(label81);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label81", label81,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label81);
    gtk_table_attach(GTK_TABLE(table11), label81, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label81), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label81), 0, 0.5);

    new_remote_entry = gtk_entry_new();
    gtk_widget_ref(new_remote_entry);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_remote_entry", new_remote_entry,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_remote_entry);
    gtk_table_attach(GTK_TABLE(table11), new_remote_entry, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    file_handling = gnome_druid_page_standard_new_with_vals("", NULL);
    gtk_widget_ref(file_handling);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "file_handling", file_handling,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show_all(file_handling);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(file_handling));
    gnome_druid_page_standard_set_logo_bg_color(GNOME_DRUID_PAGE_STANDARD(file_handling), &file_handling_logo_bg_color);
    gnome_druid_page_standard_set_title(GNOME_DRUID_PAGE_STANDARD(file_handling), "File Handling");
    gnome_druid_page_standard_set_logo(GNOME_DRUID_PAGE_STANDARD(file_handling),
				 create_image("mc/i-floppy.png"));

    druid_vbox3 = GNOME_DRUID_PAGE_STANDARD(file_handling)->vbox;
    gtk_widget_ref(druid_vbox3);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "druid_vbox3", druid_vbox3,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(druid_vbox3);

    vbox30 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox30);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox30", vbox30,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox30);
    gtk_box_pack_start(GTK_BOX(druid_vbox3), vbox30, TRUE, TRUE, 0);

    label69 = gtk_label_new("Make a choice from the selections below to denote whether you want any file permissiosn to be maintained. You can also choose the way you would like symbolic links to be treated.\n\nFinally, Xsitecopy can determine local changes by 2 different methods. Checksum is the most accurate, but file size and modification time is quicker for large sites.");
    gtk_widget_ref(label69);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label69", label69,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label69);
    gtk_box_pack_start(GTK_BOX(vbox30), label69, TRUE, TRUE, 0);
    gtk_widget_set_usize(label69, 370, -2);
    gtk_label_set_justify(GTK_LABEL(label69), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap(GTK_LABEL(label69), TRUE);

    hseparator6 = gtk_hseparator_new();
    gtk_widget_ref(hseparator6);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hseparator6", hseparator6,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hseparator6);
    gtk_box_pack_start(GTK_BOX(vbox30), hseparator6, TRUE, TRUE, 0);

    table9 = gtk_table_new(3, 2, FALSE);
    gtk_widget_ref(table9);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "table9", table9,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(table9);
    gtk_box_pack_start(GTK_BOX(vbox30), table9, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(table9), 5);
    gtk_table_set_row_spacings(GTK_TABLE(table9), 2);
    gtk_table_set_col_spacings(GTK_TABLE(table9), 2);

    label71 = gtk_label_new("Permissions mode: ");
    gtk_widget_ref(label71);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label71", label71,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label71);
    gtk_table_attach(GTK_TABLE(table9), label71, 0, 1, 0, 1,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label71), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label71), 0, 0.5);

    hbox18 = gtk_hbox_new(FALSE, 0);
    gtk_widget_ref(hbox18);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hbox18", hbox18,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hbox18);
    gtk_table_attach(GTK_TABLE(table9), hbox18, 1, 2, 0, 1,
		     (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    new_perms_ignore = gtk_radio_button_new_with_label(perms_group, "Ignore all");
    perms_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_perms_ignore));
    gtk_widget_ref(new_perms_ignore);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_perms_ignore", new_perms_ignore,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_perms_ignore);
    gtk_box_pack_start(GTK_BOX(hbox18), new_perms_ignore, FALSE, FALSE, 0);

    new_perms_exec = gtk_radio_button_new_with_label(perms_group, "Maintain for Executables only");
    perms_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_perms_exec));
    gtk_widget_ref(new_perms_exec);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_perms_exec", new_perms_exec,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_perms_exec);
    gtk_box_pack_start(GTK_BOX(hbox18), new_perms_exec, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(new_perms_ignore), TRUE);

    new_perms_all = gtk_radio_button_new_with_label(perms_group, "Maintain all");
    perms_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_perms_all));
    gtk_widget_ref(new_perms_all);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_perms_all", new_perms_all,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_perms_all);
    gtk_box_pack_start(GTK_BOX(hbox18), new_perms_all, FALSE, FALSE, 0);

    label72 = gtk_label_new("Symbolic links: ");
    gtk_widget_ref(label72);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label72", label72,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label72);
    gtk_table_attach(GTK_TABLE(table9), label72, 0, 1, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label72), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label72), 0, 0.5);

    hbox17 = gtk_hbox_new(FALSE, 0);
    gtk_widget_ref(hbox17);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hbox17", hbox17,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hbox17);
    gtk_table_attach(GTK_TABLE(table9), hbox17, 1, 2, 1, 2,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    new_sym_follow = gtk_radio_button_new_with_label(sym_link_group, "Follow all");
    sym_link_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_sym_follow));
    gtk_widget_ref(new_sym_follow);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_sym_follow", new_sym_follow,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_sym_follow);
    gtk_box_pack_start(GTK_BOX(hbox17), new_sym_follow, FALSE, FALSE, 0);

    new_sym_ignore = gtk_radio_button_new_with_label(sym_link_group, "Ignore links");
    sym_link_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_sym_ignore));
    gtk_widget_ref(new_sym_ignore);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_sym_ignore", new_sym_ignore,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_sym_ignore);
    gtk_box_pack_start(GTK_BOX(hbox17), new_sym_ignore, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(new_sym_ignore), TRUE);

    new_sym_maintain = gtk_radio_button_new_with_label(sym_link_group, "Maintain links");
    sym_link_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_sym_maintain));
    gtk_widget_ref(new_sym_maintain);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_sym_maintain", new_sym_maintain,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_sym_maintain);
    gtk_box_pack_start(GTK_BOX(hbox17), new_sym_maintain, FALSE, FALSE, 0);

    label73 = gtk_label_new("Detect changes by: ");
    gtk_widget_ref(label73);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label73", label73,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label73);
    gtk_table_attach(GTK_TABLE(table9), label73, 0, 1, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);
    gtk_label_set_justify(GTK_LABEL(label73), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label73), 0, 0.5);

    hbox19 = gtk_hbox_new(FALSE, 0);
    gtk_widget_ref(hbox19);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hbox19", hbox19,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hbox19);
    gtk_table_attach(GTK_TABLE(table9), hbox19, 1, 2, 2, 3,
		     (GtkAttachOptions) (GTK_FILL),
		     (GtkAttachOptions) (0), 0, 0);

    new_timesize = gtk_radio_button_new_with_label(state_detector_group, "Filesize & modification time");
    state_detector_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_timesize));
    gtk_widget_ref(new_timesize);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_timesize", new_timesize,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_timesize);
    gtk_box_pack_start(GTK_BOX(hbox19), new_timesize, FALSE, FALSE, 0);

    new_checksum = gtk_radio_button_new_with_label(state_detector_group, "Checksum");
    state_detector_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_checksum));
    gtk_widget_ref(new_checksum);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_checksum", new_checksum,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_checksum);
    gtk_box_pack_start(GTK_BOX(hbox19), new_checksum, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(new_checksum), TRUE);

    update_options = gnome_druid_page_standard_new_with_vals("", NULL);
    gtk_widget_ref(update_options);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "update_options", update_options,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show_all(update_options);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(update_options));
    gnome_druid_page_standard_set_logo_bg_color(GNOME_DRUID_PAGE_STANDARD(update_options), &update_options_logo_bg_color);
    gnome_druid_page_standard_set_title(GNOME_DRUID_PAGE_STANDARD(update_options), "Update Attributes");
    gnome_druid_page_standard_set_logo(GNOME_DRUID_PAGE_STANDARD(update_options),
			create_image("gnome-networktool.png"));

    druid_vbox4 = GNOME_DRUID_PAGE_STANDARD(update_options)->vbox;
    gtk_widget_ref(druid_vbox4);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "druid_vbox4", druid_vbox4,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(druid_vbox4);

    vbox31 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox31);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox31", vbox31,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox31);
    gtk_box_pack_start(GTK_BOX(druid_vbox4), vbox31, TRUE, TRUE, 0);

    label70 = gtk_label_new("When mirroring your local changes onto the remote site, there are various options available to Xsitecopy. You can control most of them with the check boxes below.");
    gtk_widget_ref(label70);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label70", label70,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label70);
    gtk_box_pack_start(GTK_BOX(vbox31), label70, TRUE, TRUE, 0);
    gtk_widget_set_usize(label70, 370, -2);
    gtk_label_set_justify(GTK_LABEL(label70), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap(GTK_LABEL(label70), TRUE);

    hseparator7 = gtk_hseparator_new();
    gtk_widget_ref(hseparator7);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hseparator7", hseparator7,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hseparator7);
    gtk_box_pack_start(GTK_BOX(vbox31), hseparator7, TRUE, TRUE, 0);

    vbox32 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox32);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox32", vbox32,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox32);
    gtk_box_pack_start(GTK_BOX(vbox31), vbox32, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox32), 5);

    new_nodelete = gtk_check_button_new_with_label("Delete a file from the server if it is deleted locally");
    gtk_widget_ref(new_nodelete);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_nodelete", new_nodelete,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_nodelete);
    gtk_box_pack_start(GTK_BOX(vbox32), new_nodelete, FALSE, FALSE, 0);

    new_checkmoved = gtk_check_button_new_with_label("Move a remote file if it is moved locally");
    gtk_widget_ref(new_checkmoved);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_checkmoved", new_checkmoved,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_checkmoved);
    gtk_box_pack_start(GTK_BOX(vbox32), new_checkmoved, FALSE, FALSE, 0);

    new_nooverwrite = gtk_check_button_new_with_label("When uploading changed files, first delete them");
    gtk_widget_ref(new_nooverwrite);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_nooverwrite", new_nooverwrite,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_nooverwrite);
    gtk_box_pack_start(GTK_BOX(vbox32), new_nooverwrite, FALSE, FALSE, 0);

    new_lowercase = gtk_check_button_new_with_label("Convert all filenames to lowercase when uploading");
    gtk_widget_ref(new_lowercase);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_lowercase", new_lowercase,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_lowercase);
    gtk_box_pack_start(GTK_BOX(vbox32), new_lowercase, FALSE, FALSE, 0);

    new_safe = gtk_check_button_new_with_label("Use \"safe mode\"");
    gtk_widget_ref(new_safe);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_safe", new_safe,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_safe);
    gtk_box_pack_start(GTK_BOX(vbox32), new_safe, FALSE, FALSE, 0);

    new_pasv = gtk_check_button_new_with_label("Use passive mode FTP");
    gtk_widget_ref(new_pasv);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_pasv", new_pasv,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_pasv);
    gtk_box_pack_start(GTK_BOX(vbox32), new_pasv, FALSE, FALSE, 0);

    server_state = gnome_druid_page_standard_new_with_vals("", NULL);
    gtk_widget_ref(server_state);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "server_state", server_state,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show_all(server_state);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(server_state));
    gnome_druid_page_standard_set_logo_bg_color(GNOME_DRUID_PAGE_STANDARD(server_state), &server_state_logo_bg_color);
    gnome_druid_page_standard_set_title(GNOME_DRUID_PAGE_STANDARD(server_state), "Current State of The Remote Site");
    gnome_druid_page_standard_set_logo(GNOME_DRUID_PAGE_STANDARD(server_state),
			create_image("gnome-networktool.png"));

    druid_vbox5 = GNOME_DRUID_PAGE_STANDARD(server_state)->vbox;
    gtk_widget_ref(druid_vbox5);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "druid_vbox5", druid_vbox5,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(druid_vbox5);

    vbox33 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox33);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox33", vbox33,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox33);
    gtk_box_pack_start(GTK_BOX(druid_vbox5), vbox33, TRUE, TRUE, 0);

    label74 = gtk_label_new("Xsitecopy now needs to know the current state of the remote site. If the site is empty or up to date, that is great. If not, it can attempt to connect to the remote server and figure out what is there.");
    gtk_widget_ref(label74);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label74", label74,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label74);
    gtk_box_pack_start(GTK_BOX(vbox33), label74, TRUE, TRUE, 0);
    gtk_widget_set_usize(label74, 370, -2);
    gtk_label_set_justify(GTK_LABEL(label74), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap(GTK_LABEL(label74), TRUE);

    hseparator8 = gtk_hseparator_new();
    gtk_widget_ref(hseparator8);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hseparator8", hseparator8,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hseparator8);
    gtk_box_pack_start(GTK_BOX(vbox33), hseparator8, TRUE, TRUE, 0);

    vbox34 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox34);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox34", vbox34,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox34);
    gtk_box_pack_start(GTK_BOX(vbox33), vbox34, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox34), 4);

    new_init = gtk_radio_button_new_with_label(vbox34_group, "The remote site is empty.");
    vbox34_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_init));
    gtk_widget_ref(new_init);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_init", new_init,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_init);
    gtk_box_pack_start(GTK_BOX(vbox34), new_init, FALSE, FALSE, 0);

    new_catchup = gtk_radio_button_new_with_label(vbox34_group, "The remote site has been uploaded.");
    vbox34_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_catchup));
    gtk_widget_ref(new_catchup);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_catchup", new_catchup,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_catchup);
    gtk_box_pack_start(GTK_BOX(vbox34), new_catchup, FALSE, FALSE, 0);

    new_fetch_please = gtk_radio_button_new_with_label(vbox34_group, "Connect to the remote site and figure out the state automatically.");
    vbox34_group = gtk_radio_button_group(GTK_RADIO_BUTTON(new_fetch_please));
    gtk_widget_ref(new_fetch_please);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_fetch_please", new_fetch_please,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_fetch_please);
    /** Added **/
    gtk_widget_set_sensitive(new_fetch_please, FALSE);
    /** End **/
    gtk_box_pack_start(GTK_BOX(vbox34), new_fetch_please, FALSE, FALSE, 0);

    fetch_a_list = gnome_druid_page_standard_new_with_vals("", NULL);
    gtk_widget_ref(fetch_a_list);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "fetch_a_list", fetch_a_list,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show_all(fetch_a_list);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(fetch_a_list));
    gnome_druid_page_standard_set_logo_bg_color(GNOME_DRUID_PAGE_STANDARD(fetch_a_list), &fetch_a_list_logo_bg_color);
    gnome_druid_page_standard_set_title(GNOME_DRUID_PAGE_STANDARD(fetch_a_list), "Ready to download file information");
    gnome_druid_page_standard_set_logo(GNOME_DRUID_PAGE_STANDARD(fetch_a_list),
			create_image("gnome-networktool.png"));

    druid_vbox10 = GNOME_DRUID_PAGE_STANDARD(fetch_a_list)->vbox;
    gtk_widget_ref(druid_vbox10);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "druid_vbox10", druid_vbox10,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(druid_vbox10);

    vbox36 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox36);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox36", vbox36,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox36);
    gtk_box_pack_start(GTK_BOX(druid_vbox10), vbox36, TRUE, TRUE, 0);

    label91 = gtk_label_new("When you click on the button below to begin, Xsitecopy will try and establish a connection to the remote server you specified earlier.\n\nIf a connection is made, a file listing will then be obtained. If this fails then you will be asked to initialise or catchup the site for now, and you can attempt another 'fetch' at a later date.");
    gtk_widget_ref(label91);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label91", label91,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label91);
    gtk_box_pack_start(GTK_BOX(vbox36), label91, TRUE, TRUE, 0);
    gtk_widget_set_usize(label91, 370, -2);
    gtk_label_set_justify(GTK_LABEL(label91), GTK_JUSTIFY_FILL);
    gtk_label_set_line_wrap(GTK_LABEL(label91), TRUE);

    hseparator10 = gtk_hseparator_new();
    gtk_widget_ref(hseparator10);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hseparator10", hseparator10,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hseparator10);
    gtk_box_pack_start(GTK_BOX(vbox36), hseparator10, TRUE, FALSE, 0);

    vbox36 = gtk_vbox_new(FALSE, 0);
    gtk_widget_ref(vbox36);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "vbox36", vbox36,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(vbox36);
    gtk_box_pack_start(GTK_BOX(vbox36), vbox36, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox36), 5);

    hbox23 = gtk_hbox_new(FALSE, 0);
    gtk_widget_ref(hbox23);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hbox23", hbox23,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hbox23);
    gtk_box_pack_start(GTK_BOX(vbox36), hbox23, FALSE, TRUE, 0);

    label88 = gtk_label_new("Status: ");
    gtk_widget_ref(label88);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label88", label88,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label88);
    gtk_box_pack_start(GTK_BOX(hbox23), label88, FALSE, FALSE, 0);
    gtk_label_set_justify(GTK_LABEL(label88), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment(GTK_MISC(label88), 0, 0.5);

    new_fetch_status = gtk_label_new("Awaiting user input. ");
    gtk_widget_ref(new_fetch_status);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_fetch_status", new_fetch_status,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_fetch_status);
    gtk_box_pack_start(GTK_BOX(hbox23), new_fetch_status, FALSE, FALSE, 0);

    hbox24 = gtk_hbox_new(FALSE, 0);
    gtk_widget_ref(hbox24);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hbox24", hbox24,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hbox24);
    gtk_box_pack_start(GTK_BOX(vbox36), hbox24, TRUE, FALSE, 0);

    label90 = gtk_label_new("Found information about ");
    gtk_widget_ref(label90);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "label90", label90,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(label90);
    gtk_box_pack_start(GTK_BOX(hbox24), label90, FALSE, FALSE, 0);

    fetch_num_files_got = gtk_label_new("0 files.");
    gtk_widget_ref(fetch_num_files_got);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "fetch_num_files_got", fetch_num_files_got,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(fetch_num_files_got);
    gtk_box_pack_start(GTK_BOX(hbox24), fetch_num_files_got, FALSE, FALSE, 0);

    new_fetch_begin = gtk_button_new_with_label("Fetch file state");
    gtk_widget_ref(new_fetch_begin);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "new_fetch_begin", new_fetch_begin,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(new_fetch_begin);
    gtk_box_pack_start(GTK_BOX(vbox36), new_fetch_begin, FALSE, FALSE, 0);
    GTK_WIDGET_SET_FLAGS(new_fetch_begin, GTK_CAN_DEFAULT);

    hbuttonbox2 = gtk_hbutton_box_new();
    gtk_widget_ref(hbuttonbox2);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "hbuttonbox2", hbuttonbox2,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(hbuttonbox2);
    gtk_box_pack_start(GTK_BOX(vbox36), hbuttonbox2, FALSE, FALSE, 0);

    finish_page = gnome_druid_page_finish_new();
    gtk_widget_ref(finish_page);
    gtk_object_set_data_full(GTK_OBJECT(site_druid), "finish_page", finish_page,
			     (GtkDestroyNotify) gtk_widget_unref);
    gtk_widget_show(finish_page);
    gnome_druid_append_page(GNOME_DRUID(new_site_druid), GNOME_DRUID_PAGE(finish_page));
    gnome_druid_page_finish_set_logo_bg_color(GNOME_DRUID_PAGE_FINISH(finish_page), &finish_page_logo_bg_color);
    gnome_druid_page_finish_set_title(GNOME_DRUID_PAGE_FINISH(finish_page), "Creation successful!");
    gnome_druid_page_finish_set_text(GNOME_DRUID_PAGE_FINISH(finish_page), "Enough information has now been gathered,\nand a suitable site definition has been created.\nClick finish to close.");
    gnome_druid_page_finish_set_logo(GNOME_DRUID_PAGE_FINISH(finish_page),
				     create_image("gnome-networktool.png"));
/*    gnome_druid_page_finish_set_watermark(GNOME_DRUID_PAGE_FINISH(finish_page),
					  create_image("xsitecopy/xsc_side.xpm"));
*/
    gtk_signal_connect(GTK_OBJECT(site_druid), "delete_event",
		       GTK_SIGNAL_FUNC(on_site_druid_delete_event),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(start_page), "cancel",
		       GTK_SIGNAL_FUNC(on_general_info_cancel),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(general_info), "next",
		       GTK_SIGNAL_FUNC(verify_name_and_url),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(general_info), "cancel",
		       GTK_SIGNAL_FUNC(on_general_info_cancel),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(server_info), "next",
		       GTK_SIGNAL_FUNC(verify_server_details),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(server_info), "cancel",
		       GTK_SIGNAL_FUNC(on_server_info_cancel),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(new_ftp), "toggled",
		       GTK_SIGNAL_FUNC(set_new_port),
		       "ftp");
    gtk_signal_connect(GTK_OBJECT(new_http), "toggled",
		       GTK_SIGNAL_FUNC(set_new_port),
		       "http");
    gtk_signal_connect(GTK_OBJECT(directories), "next",
		       GTK_SIGNAL_FUNC(verify_directories),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(directories), "cancel",
		       GTK_SIGNAL_FUNC(on_directories_cancel),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(file_handling), "next",
		       GTK_SIGNAL_FUNC(verify_file_attributes),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(file_handling), "cancel",
		       GTK_SIGNAL_FUNC(on_file_handling_cancel),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(update_options), "next",
		       GTK_SIGNAL_FUNC(verify_update_options),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(update_options), "cancel",
		       GTK_SIGNAL_FUNC(on_update_options_cancel),
		       NULL);
    gtk_signal_connect_after(GTK_OBJECT(server_state), "next",
			     GTK_SIGNAL_FUNC(should_we_fetch),
			     NULL);
    gtk_signal_connect(GTK_OBJECT(server_state), "next",
		       GTK_SIGNAL_FUNC(set_back_insensitive),
		       NULL);
/*    gtk_signal_connect(GTK_OBJECT(server_state), "next",
		       GTK_SIGNAL_FUNC(make_site_from_druid),
		       NULL);*/
    gtk_signal_connect(GTK_OBJECT(server_state), "cancel",
		       GTK_SIGNAL_FUNC(on_server_state_cancel),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(fetch_a_list), "next",
		       GTK_SIGNAL_FUNC(check_fetch_worked),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(fetch_a_list), "cancel",
		       GTK_SIGNAL_FUNC(on_fetch_a_list_cancel),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(new_fetch_begin), "clicked",
		       GTK_SIGNAL_FUNC(begin_first_time_fetch),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(finish_page), "finish",
		       GTK_SIGNAL_FUNC(druid_finished),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(finish_page), "back",
		       GTK_SIGNAL_FUNC(druid_back),
		       NULL);
    gtk_signal_connect(GTK_OBJECT(finish_page), "cancel",
		       GTK_SIGNAL_FUNC(on_finish_page_cancel),
		       NULL);
/**** End glade generated code ****/

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(new_ftp), TRUE);
    return site_druid;


}

void start_new_site_wizard(void)
{

    /* Bring the druid to the front if it already exists */

    NE_DEBUG(DEBUG_GNOME, "started New site druid.\n");
    if (site_druid) {
	gdk_window_raise(site_druid->window);
	gdk_window_show(site_druid->window);
	return;
    }

    /* Make the bare bones of a site so we can add things to it as the user
     * moves along the druid.
     */
    site_druid = create_site_druid();
    site_to_add = default_site();
    gtk_widget_show(site_druid);
}


/*** Callbacks ***/

gboolean
on_site_druid_delete_event(GtkWidget * widget,
			   GdkEvent * event,
			   gpointer user_data)
{
    NE_DEBUG(DEBUG_GNOME, "Caught delete event for new site druid.\n");
   close_druid();
    return FALSE;
}


gboolean
verify_name_and_url(GnomeDruidPage * gnomedruidpage,
		    gpointer arg1,
		    gpointer user_data)
{
    gchar *name_str;

    NE_DEBUG(DEBUG_GNOME, "Verifying name and URL...\n");

    name_str = grab_druid_entry_into_string("new_name");
    if (!name_str)
      {
	  gnome_error_dialog("You must enter a sensible name for this site");
	  return TRUE;
      }
    
    site_to_add->name = name_str;
    NE_DEBUG(DEBUG_GNOME, "Name is %s.\n", name_str);
    site_to_add->url = grab_druid_entry_into_string("new_url");
    NE_DEBUG(DEBUG_GNOME, "URL is %s.\n", site_to_add->url);
    site_to_add->infofile = g_strconcat(copypath, name_str, NULL);
    return FALSE;
}


gchar *grab_druid_entry_into_string(gchar *widget_name)
{
    GtkWidget *entry;
    gchar *tmp_string;
    
    g_assert(widget_name != NULL);
    
    entry = get_widget(site_druid, widget_name);
    tmp_string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    
    /* If the string didn't get allocated, or something went screwy don't
     * return it. Otherwise do. This is horribly double-negated, but hey.
     */
    if (! ((!tmp_string) || (strlen(tmp_string) < 1)))
      {
	  return tmp_string;
      }
     return NULL;
}


gboolean
verify_server_details(GnomeDruidPage * gnomedruidpage,
		      gpointer arg1,
		      gpointer user_data)
{
    /* Hostname */
    {
      gchar *host_str = grab_druid_entry_into_string("new_hostname");
      if (host_str != NULL)
	site_to_add->server.hostname = host_str;
    }

    /* Username */
    {
      gchar *user_str = grab_druid_entry_into_string("new_username");
      if (user_str != NULL)
	site_to_add->server.username = user_str;
    }

    /* Password */
    {
      gchar *pass_str = grab_druid_entry_into_string("new_password");
      if (pass_str != NULL)
	site_to_add->server.password = pass_str;
    }

    /* Port */
    {
      GtkWidget *port = get_widget(site_druid, "new_port");
      gint port_int = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(port));
      if (port_int)
	site_to_add->server.port = port_int;
    }

    /* Figure out the protocol to use. */
    {
      GtkWidget *protocol = get_widget(site_druid, "new_ftp");

      if (GTK_TOGGLE_BUTTON(protocol)->active) 
	{
	  /* FTP was selected */
	  site_to_add->protocol = siteproto_ftp;
	}
#ifdef USE_DAV
      else
	{
	  /* WebDAV was selected */
	  site_to_add->protocol = siteproto_dav;
	}
#endif	/* USE_DAV */
    }
 
    return FALSE;
}


/* Sets an appropriate default port when a new protocol is selected. */

void set_new_port(GtkToggleButton * togglebutton,
		  gpointer user_data)
{
    gchar *protocol = (gchar *) user_data;

    NE_DEBUG (DEBUG_GNOME, "Setting an appropriate port number based on the new protocol.\n");
    if (strcmp(protocol, "http") == 0) {
	NE_DEBUG (DEBUG_GNOME, "Setting a port for HTTP.\n");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(get_widget(site_druid, "new_port")),
				  (float) 80);
    } else {
	NE_DEBUG (DEBUG_GNOME, "Setting a port for FTP.\n");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(get_widget(site_druid, "new_port")),
				  (float) 21);
    }
}


gboolean
verify_directories(GnomeDruidPage * gnomedruidpage,
		   gpointer arg1,
		   gpointer user_data)
{
  /* (todo: If there are many places where we have almost identical code for
     local,remote cases, then consider changing local_isrel to isrel[LOCAL] etc.) */

  /* Verify & process local directory. */
  {
    gchar *local_str = grab_druid_entry_into_string("new_local_entry");
    if (local_str) 
      {
	  NE_DEBUG(DEBUG_GNOME, "Local dir is %s.\n", local_str);
	  site_to_add->local_root_user = local_str;
	  if (*local_str == '~')
	    site_to_add->local_isrel = TRUE;
      }
    else
      {
	  gnome_error_dialog("You must enter a valid local directory for this site");
	  return TRUE;
      }
  }

  /* Verify & process remote directory. */
  {
    gchar *remote_str = grab_druid_entry_into_string("new_remote_entry");
    if (remote_str)
      {
	  NE_DEBUG(DEBUG_GNOME, "Remote dir is %s.\n", remote_str);
	  site_to_add->remote_root_user = remote_str;
	  if (*remote_str == '~')
	    site_to_add->remote_isrel = TRUE;
      }
    else
      {
	  gnome_error_dialog("You must enter a valid remote directory for this site");
	  return TRUE;
      }
  }

  return FALSE;
}


/* Permissions, sym links, state-change method */

gboolean
verify_file_attributes(GnomeDruidPage * gnomedruidpage,
		       gpointer arg1,
		       gpointer user_data)
{

    GtkWidget *perms1, *perms2, *perms3;
    GtkWidget *links1, *links2, *links3;
    
    /* Permissions */
    perms1 = get_widget(site_druid, "new_perms_all");
    perms2 = get_widget(site_druid, "new_perms_exec");
    perms3 = get_widget(site_druid, "new_perms_ignore");
    
    if (GTK_TOGGLE_BUTTON(perms1)->active)
      {
	  site_to_add->perms = sitep_all;
      }
    else if (GTK_TOGGLE_BUTTON(perms2)->active)
      {
	  site_to_add->perms = sitep_exec;
      }
    else
      {
	  site_to_add->perms = sitep_ignore;
      }

    /* Symbolic links */
    links1 = get_widget(site_druid, "new_sym_follow");
    links2 = get_widget(site_druid, "new_sym_ignore");
    links3 = get_widget(site_druid, "new_sym_maintain");
    
    if (GTK_TOGGLE_BUTTON(links1)->active)
      {
	  site_to_add->symlinks = sitesym_follow;
      }
    else if (GTK_TOGGLE_BUTTON(links2)->active)
      {
	  site_to_add->symlinks = sitesym_ignore;
      }
    else
      {
	  site_to_add->symlinks = sitesym_maintain;
      }
    
    /* File State method */
    if (GTK_TOGGLE_BUTTON(get_widget(site_druid, "new_timesize"))->active) 
      {
	  site_to_add->state_method = state_timesize;
      }
    else
      {
	  site_to_add->state_method = state_checksum;
      }
    
    return FALSE;
}


gboolean
verify_update_options(GnomeDruidPage * gnomedruidpage,
		      gpointer arg1,
		      gpointer user_data)
{
    GtkWidget *del, *move, *change, *lcase, *safe, *pasv;

    del = get_widget(site_druid, "new_nodelete");
    move = get_widget(site_druid, "new_checkmoved");
    change = get_widget(site_druid, "new_nooverwrite");
    lcase = get_widget(site_druid, "new_lowercase");
    safe = get_widget(site_druid, "new_safe");
    pasv = get_widget(site_druid, "new_pasv");
    
    site_to_add->nodelete = !(GTK_TOGGLE_BUTTON(del)->active);
    site_to_add->checkmoved = GTK_TOGGLE_BUTTON(move)->active;
    site_to_add->nooverwrite = GTK_TOGGLE_BUTTON(change)->active;
    site_to_add->lowercase = GTK_TOGGLE_BUTTON(lcase)->active;
    site_to_add->safemode = GTK_TOGGLE_BUTTON(safe)->active;

    /* Enforce a bit of sanity */
    if (site_to_add->safemode)
      site_to_add->nooverwrite = FALSE;
    
    /* If using webdav, let's leave pasv_mode as false */
    if (site_to_add->protocol == siteproto_ftp)
      site_to_add->ftp_pasv_mode = GTK_TOGGLE_BUTTON(pasv)->active;
    
    return FALSE;
}


gboolean
should_we_fetch(GnomeDruidPage * gnomedruidpage,
		gpointer arg1,
		gpointer user_data)
{
    GtkWidget *connect_radio = get_widget(site_druid, "new_fetch_please");

    if (!GTK_TOGGLE_BUTTON(connect_radio)->active) {
	gnome_druid_set_page(GNOME_DRUID(get_widget(site_druid, "new_site_druid")),
			     GNOME_DRUID_PAGE(get_widget(site_druid, "finish_page")));
	return TRUE;
    }
    return FALSE;
}


gboolean
set_back_insensitive(GnomeDruidPage * gnomedruidpage,
		     gpointer arg1,
		     gpointer user_data)
{
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(get_widget(site_druid, "new_site_druid")),
				      FALSE, TRUE, TRUE);
    return FALSE;
}


/* This isn't implemented until the 'fetch' API is nicer... */

gboolean
check_fetch_worked(GnomeDruidPage * gnomedruidpage,
		   gpointer arg1,
		   gpointer user_data)
{

    return FALSE;
}


void begin_first_time_fetch(GtkButton * button,
			    gpointer user_data)
{

}

gboolean druid_back(GnomeDruidPage * gnomedruidpage,
		    gpointer arg1,
		    gpointer user_data)
{
    gnome_druid_set_page(GNOME_DRUID(get_widget(site_druid, "new_site_druid")),
			 GNOME_DRUID_PAGE(get_widget(site_druid, "server_state")));

}

/* The user has choosen Finish on the last page
   of the wizard */
void druid_finished(GnomeDruidPage * gnomedruidpage,
		    gpointer arg1,
		    gpointer user_data)
{
    add_site(user_data);
    close_druid();
}


/* This next stuff is pretty lame really, but it was Glade-generated, hence
 * took very little work, so I'm not complaining.
 */

gboolean
on_general_info_cancel(GnomeDruidPage * gnomedruidpage,
		       gpointer arg1,
		       gpointer user_data)
{
    close_druid();
    return TRUE;
}


gboolean
on_server_info_cancel(GnomeDruidPage * gnomedruidpage,
		      gpointer arg1,
		      gpointer user_data)
{
    close_druid();
    return TRUE;
}


gboolean
on_directories_cancel(GnomeDruidPage * gnomedruidpage,
		      gpointer arg1,
		      gpointer user_data)
{
    close_druid();
    return TRUE;
}


gboolean
on_file_handling_cancel(GnomeDruidPage * gnomedruidpage,
			gpointer arg1,
			gpointer user_data)
{
    close_druid();
    return TRUE;
}


gboolean
on_update_options_cancel(GnomeDruidPage * gnomedruidpage,
			 gpointer arg1,
			 gpointer user_data)
{
    close_druid();
    return TRUE;
}


gboolean
on_server_state_cancel(GnomeDruidPage * gnomedruidpage,
		       gpointer arg1,
		       gpointer user_data)
{
    close_druid();
    return TRUE;
}


gboolean
on_fetch_a_list_cancel(GnomeDruidPage * gnomedruidpage,
		       gpointer arg1,
		       gpointer user_data)
{
    close_druid();
    return TRUE;
}


gboolean
on_finish_page_cancel(GnomeDruidPage * gnomedruidpage,
		      gpointer arg1,
		      gpointer user_data)
{
    close_druid();
    return TRUE;
}













