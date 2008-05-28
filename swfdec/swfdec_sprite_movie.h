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

#include <swfdec/swfdec_actor.h>
#include <swfdec/swfdec_audio.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS


//typedef struct _SwfdecSpriteMovie SwfdecSpriteMovie;
typedef struct _SwfdecSpriteMovieClass SwfdecSpriteMovieClass;

#define SWFDEC_TYPE_SPRITE_MOVIE                    (swfdec_sprite_movie_get_type())
#define SWFDEC_IS_SPRITE_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SPRITE_MOVIE))
#define SWFDEC_IS_SPRITE_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SPRITE_MOVIE))
#define SWFDEC_SPRITE_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SPRITE_MOVIE, SwfdecSpriteMovie))
#define SWFDEC_SPRITE_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SPRITE_MOVIE, SwfdecSpriteMovieClass))

struct _SwfdecSpriteMovie
{
  SwfdecActor		actor;

  SwfdecSprite *	sprite;		/* displayed sprite */

  /* frame information */
  guint			next_action;	/* next action in sprite to perform */
  guint			max_action;	/* next action in sprite tthat has never ben executed (used to detect first-time execution) */
  guint			frame;		/* current frame or -1 if none */
  guint			n_frames;	/* amount of frames */
  gboolean		playing;	/* TRUE if the movie automatically advances */

  /* audio stream handling */
  SwfdecAudio *		sound_stream;	/* stream that currently plays */
  gboolean		sound_active;	/* if the sound stream had a SoundStreamBlock last frame */
};

struct _SwfdecSpriteMovieClass
{
  SwfdecActorClass	actor_class;
};

GType		swfdec_sprite_movie_get_type		(void);

int		swfdec_sprite_movie_get_frames_loaded	(SwfdecSpriteMovie *	movie);
int		swfdec_sprite_movie_get_frames_total	(SwfdecSpriteMovie *	movie);

void		swfdec_sprite_movie_goto		(SwfdecSpriteMovie *	movie,
							 guint			goto_frame);
void		swfdec_sprite_movie_unload		(SwfdecSpriteMovie *	movie);


G_END_DECLS
#endif
