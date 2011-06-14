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

#ifndef FE_GTK_SITE_WIDGETS_H
#define FE_GTK_SITE_WIDGETS_H

#include <gtk/gtk.h>
#include "rcfile.h"
#include "misc.h"
#include "gcommon.h"
#include "changes.h"
#include "minilist.h"

GtkWidget *make_site_info_area(struct site *the_site);

#endif				/* FE_GTK_SITE_WIDGETS_H */
