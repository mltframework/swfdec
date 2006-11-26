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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "swfdec_root_movie.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h"


G_DEFINE_TYPE (SwfdecRootMovie, swfdec_root_movie, SWFDEC_TYPE_SPRITE_MOVIE)

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
swfdec_root_movie_iterate_start (SwfdecMovie *movie)
{
  if (SWFDEC_SPRITE_MOVIE (movie)->sprite == NULL)
    return;

  SWFDEC_MOVIE_CLASS (swfdec_root_movie_parent_class)->iterate_start (movie);
}

static gboolean
swfdec_root_movie_iterate_end (SwfdecMovie *movie)
{
  if (SWFDEC_SPRITE_MOVIE (movie)->sprite == NULL)
    return TRUE;

  if (!SWFDEC_MOVIE_CLASS (swfdec_root_movie_parent_class)->iterate_end (movie))
    return FALSE;

  return g_list_find (SWFDEC_ROOT_MOVIE (movie)->player->roots, movie) != NULL;
}

static void
swfdec_root_movie_class_init (SwfdecRootMovieClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (klass);

  object_class->dispose = swfdec_root_movie_dispose;

  movie_class->init_movie = NULL;
  movie_class->iterate_start = swfdec_root_movie_iterate_start;
  movie_class->iterate_end = swfdec_root_movie_iterate_end;
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

static void
swfdec_root_movie_do_parse (SwfdecMovie *mov, gpointer unused)
{
  SwfdecRootMovie *movie = SWFDEC_ROOT_MOVIE (mov);
  SwfdecDecoderClass *klass;

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
  while (TRUE) {
    SwfdecStatus status = klass->parse (movie->decoder);
    switch (status) {
      case SWFDEC_STATUS_ERROR:
	movie->error = TRUE;
	return;
      case SWFDEC_STATUS_OK:
	break;
      case SWFDEC_STATUS_NEEDBITS:
	return;
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
	return;
      default:
	g_assert_not_reached ();
	return;
    }
  }
}

void
swfdec_root_movie_parse (SwfdecRootMovie *movie)
{
  g_return_if_fail (SWFDEC_IS_ROOT_MOVIE (movie));

  swfdec_player_lock (movie->player);
  swfdec_root_movie_do_parse (SWFDEC_MOVIE (movie), NULL);
  swfdec_player_perform_actions (movie->player);
  swfdec_player_unlock (movie->player);
}

void
swfdec_root_movie_load (SwfdecRootMovie *root, const char *url, const char *target)
{
  g_return_if_fail (SWFDEC_IS_ROOT_MOVIE (root));
  g_return_if_fail (url != NULL);
  g_return_if_fail (target != NULL);

  /* yay for the multiple uses of GetURL - one of the crappier Flash things */
  if (g_str_has_prefix (target, "_level")) {
    const char *nr = target + strlen ("_level");
    char *end;
    unsigned int depth;

    errno = 0;
    depth = strtoul (nr, &end, 10);
    if (errno == 0 && *end == '\0') {
      if (url[0] == '\0') {
	swfdec_player_remove_level (root->player, depth);
      } else {
	SwfdecLoader *loader = swfdec_loader_load (root->loader, url);
	if (loader) {
	  SwfdecRootMovie *added = swfdec_player_add_level_from_loader (root->player, depth, loader);
	  swfdec_player_add_action (root->player, SWFDEC_MOVIE (added), swfdec_root_movie_do_parse, NULL);
	} else {
	  SWFDEC_WARNING ("didn't get a loader for url \"%s\" at depth %u", url, depth);
	}
      }
    } else {
      SWFDEC_ERROR ("%s does not specify a valid level", target);
    }
    /* FIXME: what do we do here? Is returning correct?*/
    return;
  }
  g_print ("should load \"%s\" into %s\n", url, target);
}

