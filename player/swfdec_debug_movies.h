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

#ifndef _SWFDEC_DEBUG_MOVIES_H_
#define _SWFDEC_DEBUG_MOVIES_H_

#include <libswfdec/swfdec.h>

G_BEGIN_DECLS

enum {
  SWFDEC_DEBUG_MOVIES_COLUMN_MOVIE,
  SWFDEC_DEBUG_MOVIES_COLUMN_NAME,
  SWFDEC_DEBUG_MOVIES_COLUMN_VISIBLE,
  SWFDEC_DEBUG_MOVIES_COLUMN_TYPE,
  /* add more */
  SWFDEC_DEBUG_MOVIES_N_COLUMNS
};

typedef struct _SwfdecDebugMovies SwfdecDebugMovies;
typedef struct _SwfdecDebugMoviesClass SwfdecDebugMoviesClass;

#define SWFDEC_TYPE_DEBUG_MOVIES                    (swfdec_debug_movies_get_type())
#define SWFDEC_IS_DEBUG_MOVIES(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DEBUG_MOVIES))
#define SWFDEC_IS_DEBUG_MOVIES_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DEBUG_MOVIES))
#define SWFDEC_DEBUG_MOVIES(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DEBUG_MOVIES, SwfdecDebugMovies))
#define SWFDEC_DEBUG_MOVIES_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DEBUG_MOVIES, SwfdecDebugMoviesClass))

struct _SwfdecDebugMovies
{
  GObject		object;

  SwfdecPlayer *	player;		/* the video we play */
};

struct _SwfdecDebugMoviesClass
{
  GObjectClass		object_class;
};

GType		swfdec_debug_movies_get_type		(void);

SwfdecDebugMovies *
		swfdec_debug_movies_new			(SwfdecPlayer *		player);


G_END_DECLS
#endif
