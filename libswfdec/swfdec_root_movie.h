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

#ifndef _SWFDEC_ROOT_MOVIE_H_
#define _SWFDEC_ROOT_MOVIE_H_

#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_sprite_movie.h>

G_BEGIN_DECLS

//typedef struct _SwfdecRootMovie SwfdecRootMovie;
typedef struct _SwfdecRootMovieClass SwfdecRootMovieClass;

#define SWFDEC_TYPE_ROOT_MOVIE                    (swfdec_root_movie_get_type())
#define SWFDEC_IS_ROOT_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_ROOT_MOVIE))
#define SWFDEC_IS_ROOT_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_ROOT_MOVIE))
#define SWFDEC_ROOT_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_ROOT_MOVIE, SwfdecRootMovie))
#define SWFDEC_ROOT_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_ROOT_MOVIE, SwfdecRootMovieClass))

struct _SwfdecRootMovie
{
  SwfdecSpriteMovie	sprite_movie;

  gboolean		error;		/* we're in error */
  SwfdecPlayer *	player;		/* player we're played in */
  SwfdecLoader *	loader;		/* the loader providing data for the decoder */
  SwfdecDecoder *	decoder;	/* decoder that decoded all the stuff used by us */
  guint			unnamed_count;	/* variable used for naming unnamed movies */

  guint			root_actions_performed;	/* root actions been performed in all frames < this*/
};

struct _SwfdecRootMovieClass
{
  SwfdecSpriteMovieClass sprite_movie_class;
};

GType		swfdec_root_movie_get_type	  	(void);

void		swfdec_root_movie_load			(SwfdecRootMovie *	movie,
							 const char *		url,
							 const char *		target);

void		swfdec_root_movie_perform_root_actions	(SwfdecRootMovie *	root,
							 guint			frame);

G_END_DECLS
#endif
