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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "swfdec_root_movie.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h"


G_DEFINE_TYPE (SwfdecRootMovie, swfdec_root_movie, SWFDEC_TYPE_SPRITE_MOVIE)

static void
swfdec_root_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  SwfdecRootMovie *root = SWFDEC_ROOT_MOVIE (movie);

  extents->x0 = extents->y0 = 0.0;
  if (root->decoder) {
    extents->x1 = root->decoder->width * SWFDEC_SCALE_FACTOR;
    extents->y1 = root->decoder->height * SWFDEC_SCALE_FACTOR;
  } else {
    extents->x1 = extents->y1 = 0.0;
  }
}

static void
swfdec_root_movie_dispose (GObject *object)
{
  SwfdecRootMovie *root = SWFDEC_ROOT_MOVIE (object);

  g_object_unref (root->loader);
  if (root->decoder) {
    g_object_unref (root->decoder);
    root->decoder = NULL;
  }

  G_OBJECT_CLASS (swfdec_root_movie_parent_class)->dispose (object);
}

static void
swfdec_root_movie_class_init (SwfdecRootMovieClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (klass);

  object_class->dispose = swfdec_root_movie_dispose;

  movie_class->update_extents = swfdec_root_movie_update_extents;
  movie_class->init_movie = NULL;
}

static void
swfdec_root_movie_init (SwfdecRootMovie *decoder)
{
}

static void
swfdec_root_movie_do_init (SwfdecRootMovie *movie)
{
  SWFDEC_SPRITE_MOVIE (movie)->sprite = SWFDEC_SWF_DECODER (movie->decoder)->main_sprite;

  SWFDEC_MOVIE_CLASS (swfdec_root_movie_parent_class)->init_movie (SWFDEC_MOVIE (movie));
  swfdec_movie_invalidate (SWFDEC_MOVIE (movie));
}

void
swfdec_root_movie_parse (SwfdecRootMovie *movie)
{
  SwfdecDecoderClass *klass;

  g_return_if_fail (SWFDEC_IS_ROOT_MOVIE (movie));

  if (movie->error)
    return;
  if (movie->decoder == NULL) {
    if (!swfdec_decoder_can_detect (movie->loader->queue))
      return;
    movie->decoder = swfdec_decoder_new (movie->player, movie->loader->queue);
    if (movie->decoder == NULL) {
      movie->error = TRUE;
      return;
    }
  }
  klass = SWFDEC_DECODER_GET_CLASS (movie->decoder);
  g_return_if_fail (klass->parse);
  swfdec_player_lock (movie->player);
  while (TRUE) {
    SwfdecStatus status = klass->parse (movie->decoder);
    switch (status) {
      case SWFDEC_STATUS_ERROR:
	movie->error = TRUE;
	goto out;
      case SWFDEC_STATUS_OK:
	break;
      case SWFDEC_STATUS_NEEDBITS:
	goto out;
      case SWFDEC_STATUS_IMAGE:
	/* first image available - now we can initialize, if we haven't */
	if (SWFDEC_SPRITE_MOVIE (movie)->sprite == NULL)
	  swfdec_root_movie_do_init (movie);
	break;
      /* header parsing is complete, framerate, image size etc are known */
      case SWFDEC_STATUS_CHANGE:
	g_assert (movie->decoder->rate > 0);
	g_assert (movie->decoder->width > 0);
	g_assert (movie->decoder->height > 0);
	if (movie->player->rate == 0) {
	  movie->player->rate = movie->decoder->rate;
	  movie->player->width = movie->decoder->width;
	  movie->player->height = movie->decoder->height;
	  movie->player->samples_overhead = 44100 * 256 % movie->player->rate;
	  movie->player->samples_this_frame = 44100 * 256 / movie->player->rate;
	  movie->player->samples_overhead_left = movie->player->samples_overhead;
	  g_object_notify (G_OBJECT (movie->player), "initialized");
	}
	break;
      case SWFDEC_STATUS_EOF:
	goto out;
      default:
	g_assert_not_reached ();
	goto out;
    }
  }
out:
  swfdec_player_perform_actions (movie->player);
  swfdec_player_unlock (movie->player);
}

