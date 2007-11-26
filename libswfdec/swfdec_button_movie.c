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

#include "swfdec_button_movie.h"
#include "swfdec_as_strings.h"
#include "swfdec_audio_event.h"
#include "swfdec_debug.h"
#include "swfdec_event.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecButtonMovie, swfdec_button_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_button_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  swfdec_rect_union (extents, extents, 
      &SWFDEC_GRAPHIC (SWFDEC_BUTTON_MOVIE (movie)->button)->extents);
}

#if 0
/* first index is 1 for menubutton, second index is previous state,
 * last index is current state
 * MSB in index is mouse OUT = 0, IN = 1
 * LSB in index is button UP = 0, DOWN = 1
 */
static const SwfdecButtonCondition event_table[2][4][4] = {
  { { -1, -1, SWFDEC_BUTTON_IDLE_TO_OVER_UP, -1 },
    { SWFDEC_BUTTON_OUT_DOWN_TO_IDLE, -1, -1, SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN },
    { SWFDEC_BUTTON_OVER_UP_TO_IDLE, -1, -1, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN },
    { -1, SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP, -1 } },
  { { -1, -1, SWFDEC_BUTTON_IDLE_TO_OVER_UP, -1 },
    { -2, -1, -1, SWFDEC_BUTTON_IDLE_TO_OVER_DOWN },
    { SWFDEC_BUTTON_OVER_UP_TO_IDLE, -1, -1, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN },
    { -1, SWFDEC_BUTTON_OVER_DOWN_TO_IDLE, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP, -1 } }
};
static const int sound_table[2][4][4] = {
  { { -1, -1,  1, -1 },
    {  0, -1, -1, -1 },
    {  0, -1, -1,  2 },
    { -1, -1,  3, -1 } },
  { { -1, -1,  1, -1 },
    { -1, -1, -1,  1 },
    {  0, -1, -1,  2 },
    { -1,  0,  3, -1 } }
};

static const char *
swfdec_button_condition_get_name (SwfdecButtonCondition condition)
{
  /* FIXME: check if these events are based on conditions or if they're independant of button type */
  switch (condition) {
    case SWFDEC_BUTTON_IDLE_TO_OVER_UP:
      return SWFDEC_AS_STR_onRollOver;
    case SWFDEC_BUTTON_OVER_UP_TO_IDLE:
      return SWFDEC_AS_STR_onRollOut;
    case SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN:
      return SWFDEC_AS_STR_onPress;
    case SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP:
      return SWFDEC_AS_STR_onRelease;
    case SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN:
      return SWFDEC_AS_STR_onDragOut;
    case SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN:
      return SWFDEC_AS_STR_onDragOver;
    case SWFDEC_BUTTON_OUT_DOWN_TO_IDLE:
      return SWFDEC_AS_STR_onReleaseOutside;
    case SWFDEC_BUTTON_IDLE_TO_OVER_DOWN:
      return SWFDEC_AS_STR_onDragOver;
    case SWFDEC_BUTTON_OVER_DOWN_TO_IDLE:
      return SWFDEC_AS_STR_onDragOut;
    default:
      g_assert_not_reached ();
      return NULL;
  }
}

static void
swfdec_button_movie_execute (SwfdecButtonMovie *movie,
    SwfdecButtonCondition condition)
{
  const char *name;

  if (movie->button->menubutton) {
    g_assert ((condition & ((1 << SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN) \
                         | (1 << SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN) \
                         | (1 << SWFDEC_BUTTON_OUT_DOWN_TO_IDLE))) == 0);
  } else {
    g_assert ((condition & ((1 << SWFDEC_BUTTON_IDLE_TO_OVER_DOWN) \
                         | (1 << SWFDEC_BUTTON_OVER_DOWN_TO_IDLE))) == 0);
  }
  if (movie->button->events)
    swfdec_event_list_execute (movie->button->events, 
	SWFDEC_AS_OBJECT (SWFDEC_MOVIE (movie)->parent), 
	SWFDEC_SECURITY (SWFDEC_MOVIE (movie)->resource), condition, 0);
  name = swfdec_button_condition_get_name (condition);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (movie), name, 0, NULL, NULL);
}

#define CONTENT_IN_FRAME(content, frame) \
  ((content)->sequence->start <= frame && \
   (content)->sequence->end > frame)
static void
swfdec_button_movie_change_state (SwfdecButtonMovie *movie, SwfdecButtonState state)
{
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  GList *walk;

  g_assert (movie->state != state);
  SWFDEC_LOG ("changing state on button movie %p from %d to %d", movie,
      movie->state, state);
  /* do this in 2 loops - otherwise going down DOWN=>UP would
   * remove children that are only in these 2 states (due to how
   * adding them is handled)
   */
  for (walk = movie->button->records; walk; walk = walk->next) {
    SwfdecContent *content = walk->data;
    if (CONTENT_IN_FRAME (content, (guint) movie->state) &&
	!CONTENT_IN_FRAME (content, (guint) state)) {
      SwfdecMovie *child = swfdec_movie_find (mov, content->depth);
      if (child)
	swfdec_movie_remove (child);
    }
  }
  for (walk = movie->button->records; walk; walk = walk->next) {
    SwfdecContent *content = walk->data;
    if (!CONTENT_IN_FRAME (content, (guint) movie->state) &&
	CONTENT_IN_FRAME (content, (guint) state)) {
      SwfdecMovie *child = swfdec_movie_find (mov, content->depth);
      if (child) {
	g_assert_not_reached ();
      }
      swfdec_movie_new_for_content (mov, content);
    }
  }
  movie->state = state;
}

static void
swfdec_button_movie_change_mouse (SwfdecButtonMovie *movie, gboolean mouse_in, int button)
{
  int event;
  int sound;

  if (movie->mouse_in == mouse_in &&
      movie->mouse_button == button)
    return;
  SWFDEC_LOG ("changing mouse state %s: %s %s (%u) => %s %s (%u)", 
      movie->button->menubutton ? "MENU" : "BUTTON",
      movie->mouse_in ? "IN" : "OUT", movie->mouse_button ? "DOWN" : "UP",
      (movie->mouse_in ? 2 : 0) + movie->mouse_button,
      mouse_in ? "IN" : "OUT", button ? "DOWN" : "UP",
      (mouse_in ? 2 : 0) + button);
  event = event_table[movie->button->menubutton ? 1 : 0]
		     [(movie->mouse_in ? 2 : 0) + movie->mouse_button]
		     [(mouse_in ? 2 : 0) + button];

#ifndef G_DISABLE_ASSERT
  if (event == -1) {
    g_error ("Unhandled event for %s: %u => %u",
	movie->button->menubutton ? "menu" : "button",
	(movie->mouse_in ? 2 : 0) + movie->mouse_button,
	(mouse_in ? 2 : 0) + button);
  }
#endif
  if (event >= 0) {
    SWFDEC_LOG ("emitting event for condition %u", event);
    swfdec_button_movie_execute (movie, event);
  }
  sound = sound_table[movie->button->menubutton ? 1 : 0]
		     [(movie->mouse_in ? 2 : 0) + movie->mouse_button]
		     [(mouse_in ? 2 : 0) + button];
  if (sound >= 0 && movie->button->sounds[sound]) {
    SwfdecAudio *audio;
    SWFDEC_LOG ("playing button sound %d", sound);
    audio = swfdec_audio_event_new_from_chunk (
	SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context),
	movie->button->sounds[sound]);
    if (audio)
      g_object_unref (audio);
  }
  movie->mouse_in = mouse_in;
  movie->mouse_button = button;
}
#endif

static void
swfdec_button_movie_set_state (SwfdecButtonMovie *movie, SwfdecButtonState state)
{
  if (movie->state == state) {
    SWFDEC_LOG ("not changing state, it's already in %d", state);
    return;
  }
  SWFDEC_DEBUG ("changing state from %d to %d", movie->state, state);
  movie->state = state;
}

static gboolean
swfdec_button_movie_mouse_events (SwfdecMovie *movie)
{
  return TRUE;
}

static void
swfdec_button_movie_mouse_in (SwfdecMovie *movie)
{
  SWFDEC_MOVIE_CLASS (swfdec_button_movie_parent_class)->mouse_in (movie);
  if (swfdec_player_is_mouse_pressed (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context)))
    swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (movie), SWFDEC_BUTTON_DOWN);
  else
    swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (movie), SWFDEC_BUTTON_OVER);
}

static void
swfdec_button_movie_mouse_out (SwfdecMovie *movie)
{
  SwfdecButtonMovie *button = SWFDEC_BUTTON_MOVIE (movie);
  SWFDEC_MOVIE_CLASS (swfdec_button_movie_parent_class)->mouse_out (movie);
  if (swfdec_player_is_mouse_pressed (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context))) {
    swfdec_button_movie_set_state (button, SWFDEC_BUTTON_DOWN);
  } else {
    if (button->button->menubutton) {
      swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (movie), SWFDEC_BUTTON_UP);
    } else {
      swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (movie), SWFDEC_BUTTON_OVER);
    }
  }
}

static void
swfdec_button_movie_mouse_press (SwfdecMovie *movie, guint button)
{
  if (button != 0)
    return;
  SWFDEC_MOVIE_CLASS (swfdec_button_movie_parent_class)->mouse_press (movie, button);
  swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (movie), SWFDEC_BUTTON_DOWN);
}

static void
swfdec_button_movie_mouse_release (SwfdecMovie *movie, guint button)
{
  SwfdecPlayer *player;

  if (button != 0)
    return;
  SWFDEC_MOVIE_CLASS (swfdec_button_movie_parent_class)->mouse_release (movie, button);
  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  if (player->mouse_below == movie) {
    swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (movie), SWFDEC_BUTTON_UP);
  } else {
    swfdec_movie_queue_script (movie, SWFDEC_EVENT_ROLL_OVER);
    swfdec_button_movie_set_state (SWFDEC_BUTTON_MOVIE (movie), SWFDEC_BUTTON_OVER);
  }
}

static void
swfdec_button_movie_init_movie (SwfdecMovie *mov)
{
  SwfdecButtonMovie *movie = SWFDEC_BUTTON_MOVIE (mov);

  swfdec_button_movie_set_state (movie, SWFDEC_BUTTON_UP);
}

#if 0
static gboolean G_GNUC_UNUSED
swfdec_button_movie_mouse_in (SwfdecMovie *movie, double x, double y)
{
  GList *walk;
  double tmpx, tmpy;
  SwfdecButton *button = SWFDEC_BUTTON_MOVIE (movie)->button;
  SwfdecContent *content;

  for (walk = button->records; walk; walk = walk->next) {
    cairo_matrix_t inverse;
    content = walk->data;
    if (content->end <= SWFDEC_BUTTON_HIT)
      continue;
    tmpx = x;
    tmpy = y;
    swfdec_matrix_ensure_invertible (&content->transform, &inverse);
    cairo_matrix_transform_point (&inverse, &tmpx, &tmpy);

    SWFDEC_LOG ("Checking button contents at %g %g (transformed from %g %g)", tmpx, tmpy, x, y);
    if (swfdec_graphic_mouse_in (content->graphic, tmpx, tmpy))
      return TRUE;
    SWFDEC_LOG ("  missed");
  }
  return FALSE;
}

static void G_GNUC_UNUSED
swfdec_button_movie_mouse_change (SwfdecMovie *mov, double x, double y, 
    gboolean mouse_in, int button)
{
  SwfdecButtonMovie *movie = SWFDEC_BUTTON_MOVIE (mov);
  SwfdecButtonState new_state = swfdec_button_movie_get_state (movie, mouse_in, button);

  if (new_state != movie->state) {
    swfdec_button_movie_change_state (movie, new_state);
  }
  swfdec_button_movie_change_mouse (movie, mouse_in, button);
}
#endif

static void
swfdec_button_movie_class_init (SwfdecButtonMovieClass * g_class)
{
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  movie_class->init_movie = swfdec_button_movie_init_movie;
  movie_class->update_extents = swfdec_button_movie_update_extents;

  movie_class->mouse_events = swfdec_button_movie_mouse_events;
  movie_class->mouse_in = swfdec_button_movie_mouse_in;
  movie_class->mouse_out = swfdec_button_movie_mouse_out;
  movie_class->mouse_press = swfdec_button_movie_mouse_press;
  movie_class->mouse_release = swfdec_button_movie_mouse_release;
}

static void
swfdec_button_movie_init (SwfdecButtonMovie *movie)
{
  movie->state = SWFDEC_BUTTON_INIT;
}

