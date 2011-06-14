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

#ifndef TREE_H
#define TREE_H

#include <gtk/gtk.h>
#include <malloc.h>
#include "sites.h"
#include "basename.h"
#include "misc.h"
#include "file_widgets.h"
#include "gcommon.h"
#include "site_widgets.h"

struct site_node_data {
    char *name;
    struct site_file *file;
};

void color_site_node(GtkCTreeNode * node, struct site *site);

/** Tree generation functions */
GtkCTreeNode *add_file(char *name, struct site_file *a_file,
		       GtkCTreeNode * parent);
gchar *getAppropriateTreeLabel(struct site *a_site);
int add_a_site_to_the_tree(struct site *a_site, gint operation);
void fill_tree_from_all_sites(GtkWidget * a_ctree);
void core_tree_building_function(struct site_file *a_file,
				 GNode * tree);
GNode *build_tree_from_site(struct site *a_site);
void gnode_to_gui(GNode * root,
		  GtkCTreeNode * a_node);
void populate_site_node(GtkCTreeNode * site_node, struct site *current);

/* Other tree stuff */
void select_ctree_cb(GtkCTree * tree, GtkCTreeNode * node, gpointer data);
void rebuild_node_files(GtkCTreeNode * site_node);

/* Debugging */
void dump_types(struct site *a_site);
void dump_g_node(GNode * tree);

#endif /* TREE_H */
