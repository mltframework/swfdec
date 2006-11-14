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

#ifndef _SWFDEC_SPRITE_MOVIE_H_
#define _SWFDEC_SPRITE_MOVIE_H_

#include <libswfdec/swfdec_movie.h>
#include <libswfdec/swfdec_audio.h>
#include <libswfdec/swfdec_types.h>

G_BEGIN_DECLS


typedef struct _SwfdecSpriteMovie SwfdecSpriteMovie;
typedef struct _SwfdecSpriteMovieClass SwfdecSpriteMovieClass;

#define SWFDEC_TYPE_SPRITE_MOVIE                    (swfdec_sprite_movie_get_type())
#define SWFDEC_IS_SPRITE_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SPRITE_MOVIE))
#define SWFDEC_IS_SPRITE_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SPRITE_MOVIE))
#define SWFDEC_SPRITE_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SPRITE_MOVIE, SwfdecSpriteMovie))
#define SWFDEC_SPRITE_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SPRITE_MOVIE, SwfdecSpriteMovieClass))

struct _SwfdecSpriteMovie
{
  SwfdecMovie		movie;

  SwfdecSprite *	sprite;			/* displayed sprite */

  /* frame information */
  unsigned int		current_frame;		/* frame that is currently displayed (NB: indexed from 0) */

  /* color information */
  swf_color		bg_color;		/* background color (only used on main sprite) */

  /* audio stream handling */
  unsigned int		sound_frame;		/* current sound frame */
  SwfdecAudio *		sound_stream;		/* stream that currently plays */
};

struct _SwfdecSpriteMovieClass
{
  SwfdecMovieClass	movie_class;
};

GType		swfdec_sprite_movie_get_type		(void);

void		swfdec_sprite_movie_paint_background	(SwfdecSpriteMovie *    movie,
							 cairo_t *		cr);


G_END_DECLS
#endif
