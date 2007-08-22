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

#ifndef _VIVI_DOCKLET_H_
#define _VIVI_DOCKLET_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS


typedef struct _ViviDocklet ViviDocklet;
typedef struct _ViviDockletClass ViviDockletClass;

#define VIVI_TYPE_DOCKLET                    (vivi_docklet_get_type())
#define VIVI_IS_DOCKLET(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_DOCKLET))
#define VIVI_IS_DOCKLET_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_DOCKLET))
#define VIVI_DOCKLET(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_DOCKLET, ViviDocklet))
#define VIVI_DOCKLET_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_DOCKLET, ViviDockletClass))
#define VIVI_DOCKLET_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_DOCKLET, ViviDockletClass))

struct _ViviDocklet {
  GtkBin		bin;

  char *		title;		/* title to be used */
  char *		icon;		/* name of icon for docklet or "gtk-missing-image" */
};

struct _ViviDockletClass
{
  GtkBinClass		bin_class;
};

GType			vivi_docklet_get_type   	(void);

void			vivi_docklet_set_title		(ViviDocklet *	docklet,
							 const char *	titlename);
const char *		vivi_docklet_get_title		(ViviDocklet *	docklet);
void			vivi_docklet_set_icon		(ViviDocklet *	docklet,
							 const char *	titlename);
const char *		vivi_docklet_get_icon		(ViviDocklet *	docklet);


G_END_DECLS
#endif
