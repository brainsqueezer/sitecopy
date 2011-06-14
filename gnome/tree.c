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

#include "tree.h"

/* The app's main CTree */
GtkCTree *the_tree;
extern GList *errors;
GtkCTreeNode *site_item, *file_item;
extern struct site *selected_site;
extern GtkCTreeNode *current_site_node;

void color_site_node(GtkCTreeNode * node, struct site *site)
{
    /* Implement this once you can auto generate code for a prefs dialog
     * to make it configurable.
     if (site->is_different) {
     gtk_ctree_node_set_foreground   (the_tree,
     node,
     color);
     }*/
}


/* Generates a GtkCTreeNode from a_file and adds it to parent */

GtkCTreeNode *add_file(char *name, struct site_file *a_file,
		       GtkCTreeNode * parent)
{
/* External icons */
/* dir is closed, dir2 is open */
    extern GdkPixmap *dir, *dir_new, *dir_changed, *dir_del;
    extern GdkPixmap *dir2, *dir2_new, *dir2_changed, *dir2_del;
    extern GdkPixmap *xsitecopy_file, *xsitecopy_file_new, *xsitecopy_file_changed, *xsitecopy_file_del, *xsitecopy_file_moved;
    extern GdkPixmap *symbolic_link, *symbolic_link_new, *symbolic_link_changed, *symbolic_link_del;

    /* Bitmaps */
    extern GdkBitmap *dir_map, *dir_new_map, *dir_changed_map, *dir_del_map;
    extern GdkBitmap *dir2_map, *dir2_new_map, *dir2_changed_map, *dir2_del_map;
    extern GdkBitmap *xsitecopy_file_map, *xsitecopy_file_new_map, *xsitecopy_file_changed_map, *xsitecopy_file_del_map, *xsitecopy_file_moved_map;
    extern GdkBitmap *symbolic_link_map, *symbolic_link_new_map, *symbolic_link_changed_map, *symbolic_link_del_map;

    
    GtkCTreeNode *file_im_adding;
    struct ctree_attachment *file_info_to_add;
    gchar *node_label[1];
    GdkPixmap *image, *image2 = NULL;
    GdkBitmap *mask, *mask2 = NULL;

    file_info_to_add = g_malloc0(sizeof(struct ctree_attachment));
    file_info_to_add->file_or_site = IS_A_FILE;
    /* Pointer to this file */
    file_info_to_add->info_struct = (gpointer) a_file;

    
    /* Check if this file is a symbolic_link or directory */
        
    if (a_file->type == file_link)
      {
	  if (a_file->diff != file_unchanged) 
	    {
		if (a_file->diff == file_changed)
		  {
		      image = symbolic_link_changed;
		      mask = symbolic_link_changed_map;
		  } 
		else if (a_file->diff == file_deleted)
		  {
		      image = symbolic_link_del;
		      mask = symbolic_link_del_map;
		  }
		else 
		  {
		      image = symbolic_link_new;
		      mask = symbolic_link_new_map;
		  }
	    }
	  else 
	    {
		image = symbolic_link; mask = symbolic_link_map;
	    }
      }
    
    /* Check if it's actually a directory */
    else if (a_file->type == file_dir) 
      {
	  if (a_file->diff != file_unchanged) 
	    {
		if (a_file->diff == file_changed)
		  {
		      image = dir_changed;
		      mask = dir_changed_map;
		      image2 = dir2_changed;
		      mask2 = dir2_changed_map;
		  } 
		else if (a_file->diff == file_deleted)
		  {
		      image = dir_del;
		      mask = dir_del_map;
		      image2 = dir2_del;
		      mask2 = dir2_del_map;
		  }
		else 
		  {
		      image = dir_new;
		      mask = dir_new_map;
		      image2 = dir2_new;
		      mask2 = dir2_new_map;
		  }
	    }
	  else 
	    {
		image = dir; mask = dir_map;
		image2 = dir2; mask2 = dir2_map;
	    }
      }

    else
      {
	  
	  /* Set the image of this new file depending upon file status */
	  if (a_file->diff != file_unchanged) 
	    {
		if (a_file->diff == file_changed)
		  {
		      image = xsitecopy_file_changed;
		      mask = xsitecopy_file_changed_map;
		  } 
		else if (a_file->diff == file_deleted)
		  {
		      image = xsitecopy_file_del;
		      mask = xsitecopy_file_del_map;
		  }
		else if (a_file->diff == file_moved)
		  {
		      image = xsitecopy_file_moved;
		      mask = xsitecopy_file_moved_map;
		  }
		else 
		  {
		      image = xsitecopy_file_new;
		      mask = xsitecopy_file_new_map;
		  }
	    }
	  else 
	    {
		image = xsitecopy_file;
		mask = xsitecopy_file_map;
	    }
      }
    
    /* Set the node's text label accordingly */
    node_label[0] = name;

    /* Add the node */
    file_im_adding = gtk_ctree_insert_node(GTK_CTREE(the_tree),
					   GTK_CTREE_NODE(parent),
					   NULL, node_label, 3,
					   image,
					   mask,
					   a_file->type == file_dir ? image2 : image,
					   a_file->type == file_dir ? mask2 : mask,
					   FALSE, FALSE);
    gtk_ctree_node_set_row_data(GTK_CTREE(the_tree), file_im_adding,
				(gpointer) file_info_to_add);
    return file_im_adding;
}

/* Generates a string to use for a site on the main ctree, given
 * user preferences.
 */

gchar *getAppropriateTreeLabel(struct site *a_site)
{
    gchar *tmp;
    extern struct xsitecopy_prefs *main_prefs;
    
    switch (main_prefs->label) {
     case local_dir:
	tmp = g_strdup_printf("%s  (%s)", a_site->name,
			      a_site->local_root_user ? a_site->local_root_user : "No local directory!");
	break;
     case remote_dir:
	tmp = g_strdup_printf("%s  (%s)", a_site->name,
			      a_site->remote_root_user ? a_site->remote_root_user : "No remote dir");
	break;
     case remote_host:
	tmp = g_strdup_printf("%s  (%s)", a_site->name,
			      a_site->server.hostname ? a_site->server.hostname : "No hostname found");
	break;
     case url:
	tmp = g_strdup_printf("%s  (%s)", a_site->name,
			      a_site->url ? a_site->url : "No URL");
	break;
     default:
	tmp = g_strdup_printf("%s", a_site->name);
	break;
    }
    return tmp;
}

/* Creates a new tree node given a_site. Adds it with label, and populates
 * the node with the relevant file info.
 * operation determines the following:
 * 
 * 0: nothing extra happens
 * 1: the site is initialised as it is added
 * 2: the site is caught-up as it is added
 * 3: the site is added but the readfiles is not called
 * 
 * Mode 3 is not useless. It is used when the local directory can not be read
 * but the site should still appear in the main tree.
 */

int add_a_site_to_the_tree(struct site *a_site, gint operation) 
{
    struct ctree_attachment *info_to_add;
    gchar *tmp, *node_label[1];
    
    tmp = getAppropriateTreeLabel(a_site);
    node_label[0] = tmp;
    
    site_item = gtk_ctree_insert_node(GTK_CTREE(the_tree), NULL, NULL, node_label,
				      0, NULL, NULL, NULL, NULL, FALSE, FALSE);
    g_free (tmp);
    
    info_to_add = malloc(sizeof(struct ctree_attachment));
    info_to_add->file_or_site = IS_A_SITE;
    info_to_add->info_struct = (void *) a_site;
    gtk_ctree_node_set_row_data(GTK_CTREE(the_tree), site_item,
				(gpointer) info_to_add);

    /* Read site info */
   if (site_readfiles(a_site) != SITE_OK)
     /* We effectively have no info file, so just read the stored state. */
     site_read_local_state(a_site);

    /* Catchup or init, if that's required */
    if (operation == 1)
      {
	  site_initialize(a_site);
	  printf("Initialised site\n");
      }
    else if (operation == 2)
      {
	  site_catchup(a_site);
	  printf("Caught-up new site.\n");
      }
/* As far as I can tell, this doesn't need to be done. 
 * Not sure why it's in here*/
    site_write_stored_state(a_site);
    
    
    if (operation != 3)
      {
	  gtk_clist_freeze(GTK_CLIST(the_tree));
	  populate_site_node(GTK_CTREE_NODE(site_item), a_site);
	  gtk_clist_thaw(GTK_CLIST(the_tree));
      }
    
    return 0;
}


/* Reads all_sites and creates a tree from that list */

void fill_tree_from_all_sites(GtkWidget * a_ctree)
{
    struct site *current;
    int ret;
    
    for (current = all_sites; current != NULL; current = current->next) {

	/* Add any errors with the site to the errors list, so that we can
	 * present the user with all the errors, rather than popping lots of
	 * dialogs on screen at once.
	 */
	g_assert(current!=NULL);

	ret = check_site_and_record_errors(current);
	if ((ret == SITE_NOLOCALDIR) || (ret == SITE_ACCESSLOCALDIR))
	  {
	      /* Add a site without scanning the (problematic) local directory */
	      add_a_site_to_the_tree(current, 3);
	  }
	else
	  {
	      add_a_site_to_the_tree(current, 0);
	  }
    }
}


void core_tree_building_function(struct site_file *a_file,
				 GNode * tree)
{
    gchar **comps;
    gchar *leaf;
    int i = 0;
    GNode *branch = NULL;

    struct site_node_data *leaf_node = g_malloc0(sizeof(struct site_node_data));

    /* Tokenize the directory name */
    NE_DEBUG(DEBUG_GNOME, "Componentizing %s.\n", file_name(a_file));
    comps = g_strsplit(file_name(a_file), "/", -1);
    leaf = (char *) base_name(file_name(a_file));

    /* Rather than chopping off the filename when we tokenize the
     * directory, it's easier to just ignore the last element of
     * the resulting array, thus.
     */

    branch = tree;

    while (comps[i + 1] != NULL) {
	char *component = comps[i];
	GNode *tmp2;

	for (tmp2 = g_node_first_child(branch);
	     tmp2; tmp2 = tmp2->next) {
	    /* Get the name from the GNode we're looking at. */
	    char *tmp_name;
	    tmp_name = ((struct site_node_data *) tmp2->data)->name;
	    /* Check if it's the one we're looking for */
	    NE_DEBUG(DEBUG_GNOME, "Comparing %s with %s.\n",
		  component, tmp_name);
	    if (strcmp(component, tmp_name) == 0) {
		branch = tmp2;
		break;
	    }
	}
	if (!branch) {
	    NE_DEBUG(DEBUG_GNOME, "Branch became NULL. Oh dear.\n");
	    break;
	}
	i++;
    }

    /* Initialise a new data struct, and make it a child of whatever
     * branch is pointing to.
     */
    leaf_node->name = leaf;
    leaf_node->file = a_file;
    g_node_append_data(branch, leaf_node);
}


GNode *build_tree_from_site(struct site *a_site)
{

    GNode *tree_root;
    struct site_node_data *tmp;
    struct site_file *current_dir, *current_file;
    
    NE_DEBUG(DEBUG_GNOME, "Called build_tree_from_site for site, %s.\n",
	  a_site->name);

    /* If this site isn't perfect, then don't try and scan the directory.
     */
    /*{
      int ret = rc_verifysite(a_site);
      if ((ret == SITE_NOLOCALDIR) || (ret == SITE_ACCESSLOCALDIR))
	return NULL;
    }*/
    
    tmp = g_malloc0(sizeof(struct site_node_data));
    tmp->name = "ROOT";

    /* Create the root node. */
    tree_root = g_node_new(tmp);

    /* For each dir in site->files, blah... */
    NE_DEBUG(DEBUG_GNOME, "Created root node, about to enter directory loop...\n");

    /* This is not very time efficient, but I'm not the one that dictates the
       * ordering of files/directories, so don't blame me.
       * What we do here is create all the directory nodes first (and the
       * directory nodes _only_), and then add the files once the dirs are known
       * to be in place.
     */
    for (current_dir = a_site->files_tail;
	 (current_dir) && (current_dir->type == file_dir);
	 current_dir = current_dir->prev)
      ;

    /* There! SPLAT! Got you, you little bastard of a bug. */
/*    if (!current_dir)
      return tree_root;*/

    if (current_dir) 
      {
	  for (current_dir = current_dir->next; 
	       current_dir;
	       current_dir = current_dir->next)
	    {
		core_tree_building_function(current_dir, tree_root);
	    }
      }
    

    for (current_file = a_site->files;
	 (current_file) && (current_file->type != file_dir);
	 current_file = current_file->next) {
	core_tree_building_function(current_file, tree_root);
    }

    /* FIXME: do files here somehow? */
    return tree_root;
}


/** Takes a root GNode and populates the
 *  GUI component a_node from it.
 *  @param root The file hierarchy that has been built using 
 *              build_tree_from_site().
 *  @param a_node The CTree node for the site these files are a part of.
 */
#define name(node) (((struct site_node_data *)node->data)->name)
#define file(node) (((struct site_node_data *)node->data)->file)

void gnode_to_gui(GNode * root,
		  GtkCTreeNode * a_node)
{

    GNode *tmp;
    GtkCTreeNode *new_node;

    NE_DEBUG(DEBUG_GNOME, "Building GUI tree from GNode hierarchy... \n");

    if (!root)
	return;

    tmp = g_node_first_child(root);

    while (tmp) {
	new_node = add_file(name(tmp),
			    file(tmp),
			    a_node);

	gnode_to_gui(tmp, new_node);
	tmp = g_node_next_sibling(tmp);
    }

    gnode_to_gui(tmp, a_node);

}

void populate_site_node(GtkCTreeNode * site_node, struct site *current)
{
    GNode *tree_structure;
    /*
       NE_DEBUG (DEBUG_GNOME, "*********Dumping file types***********\n");
       dump_types(current);
       NE_DEBUG (DEBUG_GNOME, "*********End of dump***********\n");
     */

    /* Output file info into a tree */
    tree_structure = build_tree_from_site(current);
    /*
       NE_DEBUG (DEBUG_GNOME, "********Dumping tree structure*********\n");
       dump_g_node(tree_structure);
       NE_DEBUG (DEBUG_GNOME, "********End of Tree Dump*********\n");
     */

    /* Turn the newly generated tree into a GUI equivalent. */
    gnode_to_gui(tree_structure,
		 site_node);
    /* Colour the site node as required */
    color_site_node(GTK_CTREE_NODE(site_node), current);
}

/** Selection callback **/

void select_ctree_cb(GtkCTree * ctree, GtkCTreeNode * node, gpointer data)
{
    GtkCTreeNode *parent;
    struct site *current_site;
    struct site_file *current_file;
    void *data_to_get;
    struct ctree_attachment *actual_data;
   extern GtkWidget *main_area_box, *area_data;
   
    /* Make sure that any changes have been confirmed on the selected site,
     * before selected_site gets changed. This seems to get automatically done
     * by the signal handler.
     */

    if (GTK_CTREE_ROW(node)->parent == NULL) {	/* We're at the top level */
	if ((data_to_get = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), GTK_CTREE_NODE(node))) == NULL) {
	    NE_DEBUG(DEBUG_GNOME, "\"Data get\" returned NULL. Oh dear.\n");
	}
	actual_data = (struct ctree_attachment *) data_to_get;
	if (actual_data->file_or_site != IS_A_SITE)
	    NE_DEBUG(DEBUG_GNOME, "Somehow you've clicked on a site, but ended up with file data!\n");
	current_site = (struct site *) actual_data->info_struct;

	/* Make the data shared */
	selected_site = current_site;
	current_site_node = node;

	NE_DEBUG (DEBUG_GNOME, "Removing widgets from main data area...\n");	
	gtk_container_remove(GTK_CONTAINER(main_area_box), area_data);
	NE_DEBUG (DEBUG_GNOME, "Building site info widgets...\n");
	area_data = make_site_info_area(current_site);
	NE_DEBUG (DEBUG_GNOME, "Adding widgets to main data area...");
	gtk_container_add(GTK_CONTAINER(main_area_box), area_data);
	NE_DEBUG (DEBUG_GNOME, "done.\n");
    } else if (GTK_CTREE_ROW(node)->parent != NULL) {

	parent = GTK_CTREE_ROW(node)->parent;

	/* Grab the file data from that row */
	if ((data_to_get = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), GTK_CTREE_NODE(node))) == NULL) {
	    NE_DEBUG(DEBUG_GNOME, "\"Data get\" for the file returned NULL. Oh dear.\n");
	}
	actual_data = (struct ctree_attachment *) data_to_get;
	if (actual_data->file_or_site != IS_A_FILE)
	    NE_DEBUG(DEBUG_GNOME, "Somehow you've clicked on a file, but ended up with site data!\n");
	current_file = (struct site_file *) actual_data->info_struct;

	/* Grab the site data from the file's parent */
	while (GTK_CTREE_ROW(parent)->parent != NULL) {
	    parent = GTK_CTREE_ROW(parent)->parent;
	    NE_DEBUG(DEBUG_GNOME, "Assigned parent to node->parent.\n");
	}

	if ((data_to_get = gtk_ctree_node_get_row_data(GTK_CTREE(ctree), GTK_CTREE_NODE(parent))) == NULL) {
	    NE_DEBUG(DEBUG_GNOME, "\"Data get\" for the file's parent returned NULL. Oh dear.\n");
	}
	actual_data = (struct ctree_attachment *) data_to_get;
	if (actual_data->file_or_site != IS_A_SITE)
	    NE_DEBUG(DEBUG_GNOME, "Somehow you've clicked on a file, but it's parent was a file too!?\n");
	current_site = (struct site *) actual_data->info_struct;
	selected_site = current_site;
	current_site_node = parent;
	if (current_file == NULL) {
	    gfe_status("Unable to access info about the selected file.");
	} else {
	    /* This line is causing a mutex to deadlock. why....? */
	    NE_DEBUG (DEBUG_GNOME, "Removing widgets from main data area...\n");
	    gtk_container_remove(GTK_CONTAINER(main_area_box), area_data);
	    NE_DEBUG (DEBUG_GNOME, "Making file info area...\n");
	    area_data = make_file_info_area(current_file);
	    NE_DEBUG (DEBUG_GNOME, "Adding file info area to main area...");
	    gtk_container_add(GTK_CONTAINER(main_area_box), area_data);
	    NE_DEBUG (DEBUG_GNOME, "done.\n");
	}
    } else {
	g_warning("If you got to here, something is really buggered, quite frankly.\n");
    }
}


/** Does the equivalent of what refresh_selected used to do;
 *  looks at each file in a site and rebuilds the tree accordingly.
 */
void rebuild_node_files(GtkCTreeNode * site_node)
{
    /* Get rid of all the files first */
    while (GTK_CTREE_ROW(site_node)->children) {
	gtk_ctree_remove_node(GTK_CTREE(the_tree),
		     GTK_CTREE_NODE(GTK_CTREE_ROW(site_node)->children));
    }
    NE_DEBUG(DEBUG_GNOME, "Removed all children, freezing CTree...\n");
    gtk_clist_freeze(GTK_CLIST(the_tree));
    populate_site_node(site_node, selected_site);
    gtk_clist_thaw(GTK_CLIST(the_tree));
    NE_DEBUG(DEBUG_GNOME, "Repopulation complete, CTree thawed.\n");
}


/**** Debugging stuff ****/

void dump_types(struct site *a_site)
{
    struct site_file *tmp = NULL;

    for (tmp = a_site->files;
	 tmp;
	 tmp = tmp->next) {
	printf("%s is of type, %s.\n",
	       tmp->local.filename,
	       (tmp->type == file_dir) ? "directory" : ((tmp->type == file_file) ? "file" : "link"));
    }
}


void dump_g_node(GNode * tree)
{

    struct site_node_data *tmp;
    GNode *tmp2 = tree;
    GNode *tmp3;

    while (tmp2) {
	tmp = tmp2->data;
	printf("%s has siblings:\n", tmp->name);
	tmp3 = tmp2;
	for (; g_node_next_sibling(tmp3); tmp3 = tmp3->next) {
	    printf(" %s ", ((struct site_node_data *) tmp3->data)->name);
	}
	printf("\n");
	tmp2 = g_node_first_child(tmp2);
    }
}
