/* Swfdec
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

#ifndef _SWFDEC_GTK_PLAYER_H_
#define _SWFDEC_GTK_PLAYER_H_

#include <swfdec/swfdec.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _SwfdecGtkPlayer SwfdecGtkPlayer;
typedef struct _SwfdecGtkPlayerPrivate SwfdecGtkPlayerPrivate;
typedef struct _SwfdecGtkPlayerClass SwfdecGtkPlayerClass;

#define SWFDEC_GTK_PRIORITY_ITERATE (GDK_PRIORITY_REDRAW + 10)
#define SWFDEC_GTK_PRIORITY_REDRAW  (GDK_PRIORITY_REDRAW + 20)

#define SWFDEC_TYPE_GTK_PLAYER                    (swfdec_gtk_player_get_type())
#define SWFDEC_IS_GTK_PLAYER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_GTK_PLAYER))
#define SWFDEC_IS_GTK_PLAYER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_GTK_PLAYER))
#define SWFDEC_GTK_PLAYER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_GTK_PLAYER, SwfdecGtkPlayer))
#define SWFDEC_GTK_PLAYER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_GTK_PLAYER, SwfdecGtkPlayerClass))
#define SWFDEC_GTK_PLAYER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_GTK_PLAYER, SwfdecGtkPlayerClass))

struct _SwfdecGtkPlayer
{
  SwfdecPlayer	  		player;

  SwfdecGtkPlayerPrivate *	priv;
};

struct _SwfdecGtkPlayerClass
{
  SwfdecPlayerClass     	player_class;
};

GType 		swfdec_gtk_player_get_type    	(void);

SwfdecPlayer *	swfdec_gtk_player_new	      	(SwfdecAsDebugger *	debugger);

void		swfdec_gtk_player_set_playing 	(SwfdecGtkPlayer *	player,
						 gboolean		playing);
gboolean	swfdec_gtk_player_get_playing 	(SwfdecGtkPlayer *	player);
void		swfdec_gtk_player_set_audio_enabled	
						(SwfdecGtkPlayer *	player,
						 gboolean		enabled);
gboolean	swfdec_gtk_player_get_audio_enabled
						(SwfdecGtkPlayer *	player);
void		swfdec_gtk_player_set_speed	(SwfdecGtkPlayer *	player,
						 double			speed);
double		swfdec_gtk_player_get_speed 	(SwfdecGtkPlayer *	player);
void		swfdec_gtk_player_set_missing_plugins_window
						(SwfdecGtkPlayer *	player,
						 GdkWindow *		window);
GdkWindow *	swfdec_gtk_player_get_missing_plugins_window
						(SwfdecGtkPlayer *	player);



G_END_DECLS
#endif
