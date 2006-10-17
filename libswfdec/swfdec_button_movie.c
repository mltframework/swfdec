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
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecButtonMovie, swfdec_button_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_button_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  swfdec_rect_union (extents, extents, 
      &SWFDEC_GRAPHIC (SWFDEC_BUTTON_MOVIE (movie)->button)->extents);
}

static void
swfdec_button_movie_render (SwfdecMovie *movie, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecButtonState state = SWFDEC_BUTTON_MOVIE (movie)->state;
  SwfdecButton *button = SWFDEC_BUTTON_MOVIE (movie)->button;
  SwfdecButtonRecord *record;
  unsigned int i;

  g_return_if_fail (SWFDEC_IS_BUTTON (button));

  for (i = 0; i < button->records->len; i++) {
    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->states & state) {
      SwfdecColorTransform color_trans;
      SwfdecRect rect;
      swfdec_color_transform_chain (&color_trans, &record->color_transform,
	  trans);
      SWFDEC_LOG ("rendering graphic %u", SWFDEC_CHARACTER (record->graphic)->id);
      cairo_save (cr);
      cairo_transform (cr, &record->transform);
      swfdec_rect_transform_inverse (&rect, inval, &record->transform);
      swfdec_graphic_render (record->graphic, cr, &color_trans, &rect, fill);
      cairo_restore (cr);
    }
  }
}

static void
swfdec_button_execute (SwfdecButtonMovie *movie,
    SwfdecButtonCondition condition)
{
  if (movie->button->menubutton) {
    g_assert ((condition & (SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN \
                         | SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN \
                         | SWFDEC_BUTTON_OUT_DOWN_TO_IDLE)) == 0);
  } else {
    g_assert ((condition & (SWFDEC_BUTTON_IDLE_TO_OVER_DOWN \
                         | SWFDEC_BUTTON_OVER_DOWN_TO_IDLE)) == 0);
  }
  if (movie->button->events)
    swfdec_event_list_execute (movie->button->events, SWFDEC_MOVIE (movie)->parent, condition, 0);
}

SwfdecButtonState
swfdec_button_change_state (SwfdecMovie *movie, gboolean was_in, int old_button)
{
  SwfdecButtonMovie *bmovie = SWFDEC_BUTTON_MOVIE (movie);
  SwfdecButton *button = bmovie->button;
  SwfdecButtonState state = bmovie->state;

  switch (bmovie->state) {
    case SWFDEC_BUTTON_UP:
      if (!movie->mouse_in)
	break;
      if (movie->mouse_button) {
	state = SWFDEC_BUTTON_DOWN;
	if (button->menubutton) {
	  swfdec_button_execute (bmovie, SWFDEC_BUTTON_IDLE_TO_OVER_DOWN);
	} else {
	  /* simulate entering then clicking */
	  swfdec_button_execute (bmovie, SWFDEC_BUTTON_IDLE_TO_OVER_UP);
	  swfdec_button_execute (bmovie, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN);
	}
      } else {
	state = SWFDEC_BUTTON_OVER;
	swfdec_button_execute (bmovie, SWFDEC_BUTTON_IDLE_TO_OVER_UP);
      }
      break;
    case SWFDEC_BUTTON_OVER:
      if (!movie->mouse_in) {
	state = SWFDEC_BUTTON_UP;
	swfdec_button_execute (bmovie, SWFDEC_BUTTON_OVER_UP_TO_IDLE);
      } else if (movie->mouse_button) {
	state = SWFDEC_BUTTON_DOWN;
	swfdec_button_execute (bmovie, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN);
      }
      break;
    case SWFDEC_BUTTON_DOWN:
      /* this is quite different for push and menu buttons */
      if (!movie->mouse_button) {
	if (!movie->mouse_in) {
	  state = SWFDEC_BUTTON_UP;
	  if (button->menubutton || was_in) {
	    swfdec_button_execute (bmovie, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP);
	    swfdec_button_execute (bmovie, SWFDEC_BUTTON_OVER_UP_TO_IDLE);
	  } else {
	    swfdec_button_execute (bmovie, SWFDEC_BUTTON_OUT_DOWN_TO_IDLE);
	  }
	} else {
	  state = SWFDEC_BUTTON_OVER;
	  swfdec_button_execute (bmovie, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP);
	}
      } else {
	if (movie->mouse_in) {
	  if (!was_in && !button->menubutton)
	    swfdec_button_execute (bmovie, SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN);
	} else if (was_in) {
	  if (button->menubutton) {
	    state = SWFDEC_BUTTON_UP;
	    swfdec_button_execute (bmovie, SWFDEC_BUTTON_OVER_DOWN_TO_IDLE);
	  } else {
	    swfdec_button_execute (bmovie, SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN);
	  }
	}
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
  return state;
}

static gboolean
swfdec_button_mouse_in (SwfdecButton *button, double x, double y)
{
  guint i;
  SwfdecButtonRecord *record;

  for (i = 0; i < button->records->len; i++) {
    double tmpx, tmpy;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->states & SWFDEC_BUTTON_HIT) {
      tmpx = x;
      tmpy = y;
      swfdec_matrix_transform_point_inverse (&record->transform, &tmpx, &tmpy);

      SWFDEC_LOG ("Checking button at %g %g (transformed from %g %g)", tmpx, tmpy, x, y);
      if (swfdec_graphic_mouse_in (record->graphic, tmpx, tmpy))
	return TRUE;
      SWFDEC_LOG ("  missed");
    }
  }
  return FALSE;
}

static gboolean
swfdec_button_movie_handle_mouse (SwfdecMovie *movie, double x, double y,
    int mouse_button)
{
  SwfdecButtonMovie *bmovie = SWFDEC_BUTTON_MOVIE (movie);
  SwfdecButton *button = bmovie->button;
  gboolean was_in = movie->mouse_in;

  movie->mouse_in = swfdec_button_mouse_in (button, x, y);
  bmovie->state = swfdec_button_change_state (movie, 
      was_in, bmovie->old_button);
  bmovie->old_button = mouse_button;
  return movie->mouse_in;
}

static void
swfdec_button_movie_class_init (SwfdecButtonMovieClass * g_class)
{
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  movie_class->update_extents = swfdec_button_movie_update_extents;
  movie_class->render = swfdec_button_movie_render;
  movie_class->handle_mouse = swfdec_button_movie_handle_mouse;
}

static void
swfdec_button_movie_init (SwfdecButtonMovie *movie)
{
  movie->state = SWFDEC_BUTTON_UP;
}

