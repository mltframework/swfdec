/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_PLAYER_MANAGER_H_
#define _SWFDEC_PLAYER_MANAGER_H_

#include <gtk/gtk.h>
#include <swfdec/swfdec.h>
#include <swfdec/swfdec_debugger.h>

G_BEGIN_DECLS

typedef struct _SwfdecPlayerManager SwfdecPlayerManager;
typedef struct _SwfdecPlayerManagerClass SwfdecPlayerManagerClass;

#define SWFDEC_TYPE_PLAYER_MANAGER                    (swfdec_player_manager_get_type())
#define SWFDEC_IS_PLAYER_MANAGER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_PLAYER_MANAGER))
#define SWFDEC_IS_PLAYER_MANAGER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_PLAYER_MANAGER))
#define SWFDEC_PLAYER_MANAGER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_PLAYER_MANAGER, SwfdecPlayerManager))
#define SWFDEC_PLAYER_MANAGER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_PLAYER_MANAGER, SwfdecPlayerManagerClass))

struct _SwfdecPlayerManager
{
  GObject		object;

  SwfdecPlayer *	player;		/* the video we play */
  GSource *		source;		/* the source that is ticking */
  double		speed;		/* speed of playback */
  gboolean		playing;	/* if we should be playing */
  SwfdecDebuggerScript *interrupt_script;	/* script that caused the interrupt */
  guint			interrupt_line;	/* line in script that caused the interrupt */
  GMainLoop *		interrupt_loop;	/* loop we run during an interruption */
  guint			next_id;	/* id of breakpoint for "next" command */
};

struct _SwfdecPlayerManagerClass
{
  GObjectClass		object_class;
};

GType		swfdec_player_manager_get_type		(void);

SwfdecPlayerManager *
		swfdec_player_manager_new		(SwfdecPlayer *		player);

void		swfdec_player_manager_set_speed		(SwfdecPlayerManager *	manager,
							 double			speed);
double		swfdec_player_manager_get_speed		(SwfdecPlayerManager *  manager);
void		swfdec_player_manager_set_playing	(SwfdecPlayerManager *	manager,
							 gboolean		playing);
gboolean	swfdec_player_manager_get_playing	(SwfdecPlayerManager *  manager);
gboolean	swfdec_player_manager_get_interrupted	(SwfdecPlayerManager *  manager);

guint		swfdec_player_manager_iterate		(SwfdecPlayerManager *	manager);

void		swfdec_player_manager_continue		(SwfdecPlayerManager *	manager);
void		swfdec_player_manager_next		(SwfdecPlayerManager *	manager);
void		swfdec_player_manager_get_interrupt	(SwfdecPlayerManager *	manager,
							 SwfdecDebuggerScript **script,
							 guint *		line);

void		swfdec_player_manager_execute		(SwfdecPlayerManager *	manager,
							 const char *		command);

G_END_DECLS
#endif
