/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AS_MOVIE_VALUE_H_
#define _SWFDEC_AS_MOVIE_VALUE_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsMovieValue SwfdecAsMovieValue;
struct _SwfdecAsMovieValue {
  SwfdecAsMovieValue *	next;		/* required for GC'd stuff */
  /* FIXME: I'd like to get rid of this */
  SwfdecPlayer *	player;		/* current player instance */
  SwfdecMovie *		movie;		/* reference to the movie while it isn't destroyed */
  guint			n_names;	/* number of names */
  const char *		names[];	/* gc'd strings of the names of all movies */
};

SwfdecAsMovieValue *
		swfdec_as_movie_value_new	(SwfdecMovie *		movie,
						 const char *		name);
#define swfdec_as_movie_value_free(value) swfdec_as_gcable_free (SWFDEC_AS_CONTEXT ((value)->player), \
    (value), sizeof (SwfdecAsMovieValue) + (value)->n_names * sizeof (const char *))

void		swfdec_as_movie_value_mark	(SwfdecAsMovieValue *	value);

SwfdecMovie *	swfdec_as_movie_value_get	(SwfdecAsMovieValue *	value);


G_END_DECLS
#endif
