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
#include "swfdec_flv_decoder.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h"
#include "js/jsapi.h"


static void swfdec_root_movie_loader_target_init (SwfdecLoaderTargetInterface *iface);
G_DEFINE_TYPE_WITH_CODE (SwfdecRootMovie, swfdec_root_movie, SWFDEC_TYPE_SPRITE_MOVIE,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_root_movie_loader_target_init))

/*** SWFDEC_LOADER_TARGET interface ***/

static SwfdecPlayer *
swfdec_root_movie_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_ROOT_MOVIE (target)->player;
}

static SwfdecDecoder *
swfdec_root_movie_loader_target_get_decoder (SwfdecLoaderTarget *target)
{
  return SWFDEC_ROOT_MOVIE (target)->decoder;
}

static gboolean
swfdec_root_movie_loader_target_set_decoder (SwfdecLoaderTarget *target,
    SwfdecDecoder *decoder)
{
  SWFDEC_ROOT_MOVIE (target)->decoder = decoder;
  return TRUE;
}

static gboolean
swfdec_root_movie_loader_target_do_init (SwfdecLoaderTarget *target)
{
  SwfdecRootMovie *movie = SWFDEC_ROOT_MOVIE (target);

  swfdec_player_initialize (movie->player, movie->decoder->rate, 
      movie->decoder->width, movie->decoder->height);
  if (SWFDEC_IS_SWF_DECODER (movie->decoder) &&
      movie->player->roots->next == 0) { 
    /* if we're the only child */
    /* FIXME: check case sensitivity wrt embedding movies of different version */
    JS_SetContextCaseSensitive (movie->player->jscx,
	SWFDEC_SWF_DECODER (movie->decoder)->version > 6);
  }
  if (SWFDEC_IS_FLV_DECODER (movie->decoder)) {
    swfdec_flv_decoder_add_movie (SWFDEC_FLV_DECODER (movie->decoder), 
	SWFDEC_MOVIE (movie));
  }
  return TRUE;
}

static gboolean
swfdec_root_movie_loader_target_image (SwfdecLoaderTarget *target)
{
  SwfdecRootMovie *movie = SWFDEC_ROOT_MOVIE (target);

  if (SWFDEC_SPRITE_MOVIE (movie)->sprite != NULL)
    return TRUE;

  if (SWFDEC_IS_SWF_DECODER (movie->decoder)) {
    SWFDEC_SPRITE_MOVIE (movie)->sprite = SWFDEC_SWF_DECODER (movie->decoder)->main_sprite;

    SWFDEC_MOVIE_CLASS (swfdec_root_movie_parent_class)->init_movie (SWFDEC_MOVIE (movie));
    swfdec_movie_invalidate (SWFDEC_MOVIE (movie));
  } else if (SWFDEC_IS_FLV_DECODER (movie->decoder)) {
    /* nothing to do, please move along */
  } else {
    g_assert_not_reached ();
    return FALSE;
  }
  return TRUE;
}

static void
swfdec_root_movie_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_root_movie_loader_target_get_player;
  iface->get_decoder = swfdec_root_movie_loader_target_get_decoder;
  iface->set_decoder = swfdec_root_movie_loader_target_set_decoder;

  iface->init = swfdec_root_movie_loader_target_do_init;
  iface->image = swfdec_root_movie_loader_target_image;
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

void
swfdec_root_movie_do_parse (SwfdecMovie *movie, gpointer unused)
{
  swfdec_loader_parse_internal (SWFDEC_ROOT_MOVIE (movie)->loader);
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
	  SwfdecRootMovie *added = swfdec_player_add_level_from_loader (root->player, depth, loader, NULL);
	  swfdec_player_add_action (root->player, SWFDEC_MOVIE (added),
	      swfdec_root_movie_do_parse, NULL);
	} else {
	  SWFDEC_WARNING ("didn't get a loader for url \"%s\" at depth %u", url, depth);
	}
      }
    } else {
      SWFDEC_ERROR ("%s does not specify a valid level", target);
    }
    /* FIXME: what do we do here? Is returning correct?*/
    return;
  } else if (g_str_has_prefix (target, "FSCommand:")) {
    const char *command = url + strlen ("FSCommand:");
    SWFDEC_WARNING ("unhandled fscommand: %s %s", command, target);
    return;
  }
  swfdec_player_launch (root->player, url, target);
}

