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

#ifndef _VIVI_PLAYER_H_
#define _VIVI_PLAYER_H_

#include <vivified/core/vivified-core.h>
#include <vivified/dock/vivified-dock.h>

G_BEGIN_DECLS


typedef struct _ViviPlayer ViviPlayer;
typedef struct _ViviPlayerClass ViviPlayerClass;

#define VIVI_TYPE_PLAYER                    (vivi_player_get_type())
#define VIVI_IS_PLAYER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_PLAYER))
#define VIVI_IS_PLAYER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_PLAYER))
#define VIVI_PLAYER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_PLAYER, ViviPlayer))
#define VIVI_PLAYER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_PLAYER, ViviPlayerClass))
#define VIVI_PLAYER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_PLAYER, ViviPlayerClass))

struct _ViviPlayer {
  ViviDocklet		docklet;

  ViviApplication *	app;		/* the application we connect to */
  GtkWidget *		player;		/* SwfdecGtkWidget */
};

struct _ViviPlayerClass
{
  ViviDockletClass    	docklet_class;
};

GType			vivi_player_get_type   	(void);

GtkWidget *		vivi_player_new		(ViviApplication *	app);


G_END_DECLS
#endif
