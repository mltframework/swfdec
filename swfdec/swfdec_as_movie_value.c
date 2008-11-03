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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_as_movie_value.h"

#include <string.h>

#include "swfdec_as_gcable.h"
#include "swfdec_as_strings.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"

SwfdecAsMovieValue *
swfdec_as_movie_value_new (SwfdecMovie *movie, const char *name)
{
  SwfdecAsMovieValue *val, *parent;
  SwfdecAsContext *context;
  guint i, n_names;

  g_assert (SWFDEC_IS_MOVIE (movie));
  g_assert (name != NULL);
  g_assert (name != SWFDEC_AS_STR_EMPTY);

  context = swfdec_gc_object_get_context (movie);
  parent = movie->parent ? movie->parent->as_value : NULL;
  n_names = parent ? parent->n_names + 1 : 1;

  val = swfdec_as_gcable_alloc (context, sizeof (SwfdecAsMovieValue) + n_names * sizeof (const char *));
  val->player = SWFDEC_PLAYER (context);
  val->movie = movie;
  val->n_names = n_names;
  val->names[n_names - 1] = name;
  movie = movie->parent;
  for (i = n_names - 2; movie; i--) {
    if (movie->nameasdf != SWFDEC_AS_STR_EMPTY) {
      val->names[i] = movie->nameasdf;
    } else {
      val->names[i] = movie->as_value->names[i];
    };
    movie = movie->parent; 
  }

  SWFDEC_AS_GCABLE_SET_NEXT (val, context->movies);
  context->movies = val;

  return val;
}

void
swfdec_as_movie_value_mark (SwfdecAsMovieValue *value)
{
  guint i;

  g_assert (value != NULL);

  SWFDEC_AS_GCABLE_SET_FLAG ((SwfdecAsGcable *) value, SWFDEC_AS_GC_MARK);

  if (value->movie)
    swfdec_gc_object_mark (value->movie);
  for (i = 0; i < value->n_names; i++) {
    swfdec_as_string_mark (value->names[i]);
  }
}

static SwfdecMovie *
swfdec_player_get_movie_by_name (SwfdecPlayer *player, const char *name)
{
  GList *walk;

  for (walk = player->priv->roots; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (cur->as_value->names[0] == name)
      return cur;
  }
  return NULL;
}

SwfdecMovie *
swfdec_as_movie_value_get (SwfdecAsMovieValue *value)
{
  SwfdecMovie *movie;
  guint i;

  g_assert (value != NULL);
  g_assert (value->movie == NULL); /* checked by macros */

  /* FIXME: getting a root movie by name should be simpler */
  movie = swfdec_player_get_movie_by_name (value->player, value->names[0]);
  for (i = 1; i < value->n_names && movie; i++) {
    movie = swfdec_movie_get_by_name (movie, value->names[i], TRUE);
  }
  return movie;
}
