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

#ifndef _VIVI_VIVI_DOCKLET_H_
#define _VIVI_VIVI_DOCKLET_H_

#include <vivified/core/vivified-core.h>
#include <vivified/dock/vivified-dock.h>

G_BEGIN_DECLS


typedef struct _ViviViviDocklet ViviViviDocklet;
typedef struct _ViviViviDockletClass ViviViviDockletClass;

#define VIVI_TYPE_VIVI_DOCKLET                    (vivi_vivi_docklet_get_type())
#define VIVI_IS_VIVI_DOCKLET(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_VIVI_DOCKLET))
#define VIVI_IS_VIVI_DOCKLET_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_VIVI_DOCKLET))
#define VIVI_VIVI_DOCKLET(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_VIVI_DOCKLET, ViviViviDocklet))
#define VIVI_VIVI_DOCKLET_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_VIVI_DOCKLET, ViviViviDockletClass))
#define VIVI_VIVI_DOCKLET_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_VIVI_DOCKLET, ViviViviDockletClass))

struct _ViviViviDocklet {
  ViviDocklet		docklet;

  ViviApplication *	app;			/* the application we connect to */
};

struct _ViviViviDockletClass
{
  ViviDockletClass    	docklet_class;

  void			(* application_set)   		(ViviViviDocklet *	docklet,
							 ViviApplication *	app);
  void			(* application_unset)	      	(ViviViviDocklet *	docklet,
							 ViviApplication *	app);
};

GType			vivi_vivi_docklet_get_type   	(void);

GtkWidget *		vivi_vivi_docklet_find_widget_by_type 
							(ViviViviDocklet *	docklet,
							 GType			type);

G_END_DECLS
#endif
