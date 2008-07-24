/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_BUTTON_MOVIE_H_
#define _SWFDEC_BUTTON_MOVIE_H_

#include <glib-object.h>
#include <swfdec/swfdec_actor.h>
#include <swfdec/swfdec_button.h>

G_BEGIN_DECLS


typedef struct _SwfdecButtonMovie SwfdecButtonMovie;
typedef struct _SwfdecButtonMovieClass SwfdecButtonMovieClass;

#define SWFDEC_TYPE_BUTTON_MOVIE                    (swfdec_button_movie_get_type())
#define SWFDEC_IS_BUTTON_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BUTTON_MOVIE))
#define SWFDEC_IS_BUTTON_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BUTTON_MOVIE))
#define SWFDEC_BUTTON_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BUTTON_MOVIE, SwfdecButtonMovie))
#define SWFDEC_BUTTON_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BUTTON_MOVIE, SwfdecButtonMovieClass))

struct _SwfdecButtonMovie {
  SwfdecActor		actor;

  SwfdecButtonState	state;		/* current state we're in */
};

struct _SwfdecButtonMovieClass {
  SwfdecActorClass	actor_class;
};

GType		swfdec_button_movie_get_type		(void);


G_END_DECLS
#endif
