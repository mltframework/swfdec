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

#ifndef _VIVI_VDOCK_H_
#define _VIVI_VDOCK_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS


typedef struct _ViviVDock ViviVDock;
typedef struct _ViviVDockClass ViviVDockClass;

#define VIVI_TYPE_VDOCK                    (vivi_vdock_get_type())
#define VIVI_IS_VDOCK(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_VDOCK))
#define VIVI_IS_VDOCK_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_VDOCK))
#define VIVI_VDOCK(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_VDOCK, ViviVDock))
#define VIVI_VDOCK_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_VDOCK, ViviVDockClass))
#define VIVI_VDOCK_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_VDOCK, ViviVDockClass))

struct _ViviVDock {
  GtkBin		bin;

  GList *		docklets;	/* all the docklets that got added to us */
};

struct _ViviVDockClass
{
  GtkBinClass		bin_class;
};

GType			vivi_vdock_get_type   	(void);

GtkWidget *		vivi_vdock_new		(void);

void			vivi_vdock_add		(ViviVDock *	vdock, 
						 GtkWidget *	widget);
void			vivi_vdock_remove	(ViviVDock *	vdock,
						 GtkWidget *	widget);


G_END_DECLS
#endif
