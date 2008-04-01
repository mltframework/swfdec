/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_ACTOR_H_
#define _SWFDEC_ACTOR_H_

#include <swfdec/swfdec_movie.h>

G_BEGIN_DECLS


//typedef struct _SwfdecActor SwfdecActor;
typedef struct _SwfdecActorClass SwfdecActorClass;

#define SWFDEC_TYPE_ACTOR                    (swfdec_actor_get_type())
#define SWFDEC_IS_ACTOR(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_ACTOR))
#define SWFDEC_IS_ACTOR_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_ACTOR))
#define SWFDEC_ACTOR(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_ACTOR, SwfdecActor))
#define SWFDEC_ACTOR_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_ACTOR, SwfdecActorClass))
#define SWFDEC_ACTOR_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_ACTOR, SwfdecActorClass))

struct _SwfdecActor
{
  SwfdecMovie		movie;

  /* static properties (set by PlaceObject tags) */
  SwfdecEventList *	events;			/* events queued on this movie */

  SwfdecFlashBool	focusrect;		/* if we should draw a focus rectangle or not */
};

struct _SwfdecActorClass
{
  SwfdecMovieClass    	movie_class;

  /* iterating */
  void			(* iterate_start)     	(SwfdecActor *		actor);
  gboolean		(* iterate_end)		(SwfdecActor *		actor);

  /* mouse handling */
  gboolean		(* mouse_events)	(SwfdecActor *		movie);
  SwfdecMouseCursor	(* mouse_cursor)	(SwfdecActor *		movie);
  void			(* mouse_in)      	(SwfdecActor *		movie);
  void			(* mouse_out)      	(SwfdecActor *		movie);
  void			(* mouse_press)      	(SwfdecActor *		movie,
						 guint			button);
  void			(* mouse_release)      	(SwfdecActor *		movie,
						 guint			button);
  void			(* mouse_move)      	(SwfdecActor *		movie,
						 double			x,
						 double			y);

  /* keyboard handling */
  void			(* focus_in)		(SwfdecActor *		movie);
  void			(* focus_out)		(SwfdecActor *		movie);
  void			(* key_press)		(SwfdecActor *		movie,
						 guint			keycode,
						 guint			character);
  void			(* key_release)      	(SwfdecActor *		movie,
						 guint			keycode,
						 guint			character);
};

GType		swfdec_actor_get_type		(void);

void		swfdec_actor_execute		(SwfdecActor *		actor,
						 SwfdecEventType	condition);
void		swfdec_actor_queue_script	(SwfdecActor *		actor,
  						 SwfdecEventType	condition);

gboolean	swfdec_actor_get_mouse_events	(SwfdecActor *		actor);
gboolean	swfdec_actor_has_focusrect	(SwfdecActor *		actor);


G_END_DECLS
#endif
