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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_button_movie.h"
#include "swfdec_as_strings.h"
#include "swfdec_audio_event.h"
#include "swfdec_debug.h"
#include "swfdec_event.h"
#include "swfdec_filter.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"

G_DEFINE_TYPE (SwfdecButtonMovie, swfdec_button_movie, SWFDEC_TYPE_ACTOR)

static void
swfdec_button_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  swfdec_rect_union (extents, extents, &movie->graphic->extents);
}

static void
swfdec_button_movie_perform_place (SwfdecButtonMovie *button, SwfdecBits *bits)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (button);
  gboolean has_blend_mode, has_filters, v2;
  SwfdecColorTransform ctrans;
  SwfdecGraphic *graphic;
  SwfdecPlayer *player;
  cairo_matrix_t trans;
  guint id, blend_mode;
  SwfdecMovie *new;
  int depth;

  swfdec_bits_getbits (bits, 2); /* reserved */
  has_blend_mode = swfdec_bits_getbit (bits);
  has_filters = swfdec_bits_getbit (bits);
  SWFDEC_LOG ("  has_blend_mode = %d", has_blend_mode);
  SWFDEC_LOG ("  has_filters = %d", has_filters);
  swfdec_bits_getbits (bits, 4); /* states */
  id = swfdec_bits_get_u16 (bits);
  depth = swfdec_bits_get_u16 (bits);
  depth -= 16384;
  if (swfdec_movie_find (movie, depth)) {
    SWFDEC_WARNING ("depth %d already occupied, skipping placement.", depth + 16384);
    return;
  }
  graphic = swfdec_swf_decoder_get_character (SWFDEC_SWF_DECODER (movie->resource->decoder), id);
  if (!SWFDEC_IS_GRAPHIC (graphic)) {
    SWFDEC_ERROR ("id %u does not specify a graphic", id);
    return;
  }

  player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
  new = swfdec_movie_new (player, depth, movie, movie->resource, graphic, NULL);
  swfdec_bits_get_matrix (bits, &trans, NULL);
  if (swfdec_bits_left (bits)) {
    v2 = TRUE;
    swfdec_bits_get_color_transform (bits, &ctrans);
    if (has_blend_mode) {
      blend_mode = swfdec_bits_get_u8 (bits);
      SWFDEC_LOG ("  blend mode = %u", blend_mode);
    } else {
      blend_mode = 0;
    }
    if (has_filters) {
      GSList *filters = swfdec_filter_parse (bits);
      g_slist_free (filters);
    }
  } else {
    /* DefineButton1 record */
    v2 = FALSE;
    if (has_blend_mode || has_filters) {
      SWFDEC_ERROR ("cool, a DefineButton1 with filters or blend mode");
    }
    blend_mode = 0;
  }
  swfdec_movie_set_static_properties (new, &trans, v2 ? &ctrans : NULL, 0, 0, blend_mode, NULL);
  if (SWFDEC_IS_ACTOR (new)) {
    SwfdecActor *actor = SWFDEC_ACTOR (new);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_INITIALIZE);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_CONSTRUCT);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_LOAD);
  }
  swfdec_movie_initialize (new);
  if (swfdec_bits_left (bits)) {
    SWFDEC_WARNING ("button record for id %u has %u bytes left", id,
	swfdec_bits_left (bits) / 8);
  }
}

static void
swfdec_button_movie_set_state (SwfdecButtonMovie *button, SwfdecButtonState state)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (button);
  SwfdecMovie *child;
  SwfdecBits bits;
  GSList *walk;
  guint old, new, i;
  int depth;
  SwfdecButton *but;

  if (button->state == state) {
    SWFDEC_LOG ("not changing state, it's already in %d", state);
    return;
  }
  SWFDEC_DEBUG ("changing state from %d to %d", button->state, state);
  but = SWFDEC_BUTTON (movie->graphic);
  /* remove all movies that aren't in the new state */
  new = 1 << state;
  if (button->state >= 0) {
    old = 1 << button->state;
    for (walk = but->records; walk; walk = walk->next) {
      swfdec_bits_init (&bits, walk->data);
      i = swfdec_bits_get_u8 (&bits);
      if ((i & old) && !(i & new)) {
	swfdec_bits_get_u16 (&bits);
	depth = swfdec_bits_get_u16 (&bits);
	child = swfdec_movie_find (movie, depth - 16384);
	if (child) {
	  swfdec_movie_remove (child);
	} else {
	  SWFDEC_WARNING ("no child at depth %d, none removed", depth);
	}
      }
    }
  } else {
    /* to make sure that this never triggers when initializing */
    old = 0;
  }
  button->state = state;
  /* add all movies that are in the new state */
  for (walk = but->records; walk; walk = walk->next) {
    swfdec_bits_init (&bits, walk->data);
    i = swfdec_bits_peek_u8 (&bits);
    if ((i & old) || !(i & new))
      continue;
    swfdec_button_movie_perform_place (button, &bits);
  }
}

static gboolean
swfdec_button_movie_mouse_events (SwfdecActor *actor)
{
  return TRUE;
}

static void
swfdec_button_movie_mouse_in (SwfdecActor *actor)
{
  if (swfdec_player_is_mouse_pressed (SWFDEC_PLAYER (swfdec_gc_object_get_context (actor))))
    swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (actor), SWFDEC_BUTTON_DOWN);
  else
    swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (actor), SWFDEC_BUTTON_OVER);

  SWFDEC_ACTOR_CLASS (swfdec_button_movie_parent_class)->mouse_in (actor);
}

static void
swfdec_button_movie_mouse_out (SwfdecActor *actor)
{
  SwfdecButtonMovie *button = SWFDEC_BUTTON_MOVIE (actor);

  if (swfdec_player_is_mouse_pressed (SWFDEC_PLAYER (swfdec_gc_object_get_context (button)))) {
    if (SWFDEC_BUTTON (SWFDEC_MOVIE (actor)->graphic)->menubutton) {
      swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (actor), SWFDEC_BUTTON_UP);
    } else {
      swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (actor), SWFDEC_BUTTON_OVER);
    }
  } else {
    swfdec_button_movie_set_state (button, SWFDEC_BUTTON_UP);
  }

  SWFDEC_ACTOR_CLASS (swfdec_button_movie_parent_class)->mouse_out (actor);
}

static void
swfdec_button_movie_mouse_press (SwfdecActor *actor, guint button)
{
  if (button != 0)
    return;
  swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (actor), SWFDEC_BUTTON_DOWN);

  SWFDEC_ACTOR_CLASS (swfdec_button_movie_parent_class)->mouse_press (actor, button);
}

static void
swfdec_button_movie_mouse_release (SwfdecActor *actor, guint button)
{
  SwfdecPlayer *player;

  if (button != 0)
    return;
  player = SWFDEC_PLAYER (swfdec_gc_object_get_context (actor));
  if (player->priv->mouse_below == actor) {
    swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (actor), SWFDEC_BUTTON_OVER);

    SWFDEC_ACTOR_CLASS (swfdec_button_movie_parent_class)->mouse_release (actor, button);
  } else {
    swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (actor), SWFDEC_BUTTON_UP);

    /* NB: We don't chain to parent here for menubuttons*/
    if (!SWFDEC_BUTTON (SWFDEC_MOVIE (actor)->graphic)->menubutton) {
      SWFDEC_ACTOR_CLASS (swfdec_button_movie_parent_class)->mouse_release (actor, button);
    } else {
      SWFDEC_FIXME ("use mouse_below as recipient for mouse_release events?");
    }
  }
}

static void
swfdec_button_movie_init_movie (SwfdecMovie *mov)
{
  SwfdecButtonMovie *movie = SWFDEC_BUTTON_MOVIE (mov);

  swfdec_button_movie_set_state (movie, SWFDEC_BUTTON_UP);
}

static gboolean
swfdec_button_movie_hit_test (SwfdecButtonMovie *button, double x, double y)
{
  SwfdecSwfDecoder *dec;
  GSList *walk;
  double tmpx, tmpy;

  dec = SWFDEC_SWF_DECODER (SWFDEC_MOVIE (button)->resource->decoder);
  for (walk = SWFDEC_BUTTON (SWFDEC_MOVIE (button)->graphic)->records; walk; walk = walk->next) {
    SwfdecGraphic *graphic;
    SwfdecBits bits;
    cairo_matrix_t matrix, inverse;
    guint id;

    swfdec_bits_init (&bits, walk->data);

    if ((swfdec_bits_get_u8 (&bits) & (1 << SWFDEC_BUTTON_HIT)) == 0)
      continue;

    id = swfdec_bits_get_u16 (&bits);
    swfdec_bits_get_u16 (&bits); /* depth */
    graphic = swfdec_swf_decoder_get_character (dec, id);
    if (!SWFDEC_IS_GRAPHIC (graphic)) {
      SWFDEC_ERROR ("id %u is no graphic", id);
      continue;
    }
    tmpx = x;
    tmpy = y;
    swfdec_bits_get_matrix (&bits, &matrix, &inverse);
    cairo_matrix_transform_point (&inverse, &tmpx, &tmpy);

    SWFDEC_LOG ("Checking button contents at %g %g (transformed from %g %g)", tmpx, tmpy, x, y);
    if (swfdec_graphic_mouse_in (graphic, tmpx, tmpy))
      return TRUE;
    SWFDEC_LOG ("  missed");
  }
  return FALSE;
}

static SwfdecMovie *
swfdec_button_movie_contains (SwfdecMovie *movie, double x, double y, gboolean events)
{
  if (events) {
    /* check for movies in a higher layer that react to events */
    SwfdecMovie *ret;
    ret = SWFDEC_MOVIE_CLASS (swfdec_button_movie_parent_class)->contains (movie, x, y, TRUE);
    if (ret && ret != movie && SWFDEC_IS_ACTOR (ret) && swfdec_actor_get_mouse_events (SWFDEC_ACTOR (ret)))
      return ret;
  }
  
  return swfdec_button_movie_hit_test (SWFDEC_BUTTON_MOVIE (movie), x, y) ? movie : NULL;
}

static GObject *
swfdec_button_movie_constructor (GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties)
{
  SwfdecMovie *movie;
  GObject *object;

  object = G_OBJECT_CLASS (swfdec_button_movie_parent_class)->constructor (type, 
      n_construct_properties, construct_properties);

  movie = SWFDEC_MOVIE (object);
  g_assert (movie->graphic);
  if (SWFDEC_BUTTON (movie->graphic)->events) {
    SWFDEC_ACTOR (movie)->events = swfdec_event_list_copy (
	SWFDEC_BUTTON (movie->graphic)->events);
  }

  return object;
}

static void
swfdec_button_movie_class_init (SwfdecButtonMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);
  SwfdecActorClass *actor_class = SWFDEC_ACTOR_CLASS (g_class);

  object_class->constructor = swfdec_button_movie_constructor;

  movie_class->init_movie = swfdec_button_movie_init_movie;
  movie_class->update_extents = swfdec_button_movie_update_extents;
  movie_class->contains = swfdec_button_movie_contains;

  actor_class->mouse_events = swfdec_button_movie_mouse_events;
  actor_class->mouse_in = swfdec_button_movie_mouse_in;
  actor_class->mouse_out = swfdec_button_movie_mouse_out;
  actor_class->mouse_press = swfdec_button_movie_mouse_press;
  actor_class->mouse_release = swfdec_button_movie_mouse_release;
}

static void
swfdec_button_movie_init (SwfdecButtonMovie *movie)
{
  movie->state = SWFDEC_BUTTON_INIT;
}

