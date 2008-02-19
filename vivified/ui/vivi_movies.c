/* Vivified
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <swfdec-gtk/swfdec-gtk.h>
#include "vivi_vivi_docklet.h"
#include "vivi_movie_list.h"

void
vivi_movies_application_set (ViviViviDocklet *docklet, ViviApplication *app);
void
vivi_movies_application_set (ViviViviDocklet *docklet, ViviApplication *app)
{
  GtkWidget *view = vivi_vivi_docklet_find_widget_by_type (docklet, GTK_TYPE_TREE_VIEW);
  GtkTreeModel *model;

  model = vivi_movie_list_new (app);
  gtk_tree_view_set_model (GTK_TREE_VIEW (view), model);
  g_object_unref (model);
}

