/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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
#include <math.h>
#include <pango/pangocairo.h>

#include "swfdec_text_field_movie.h"
#include "swfdec_as_context.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_interpret.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"
#include "swfdec_sandbox.h"
#include "swfdec_text_format.h"
#include "swfdec_xml.h"

G_DEFINE_TYPE (SwfdecTextFieldMovie, swfdec_text_field_movie, SWFDEC_TYPE_ACTOR)

#define BORDER_TOP 2
#define BORDER_LEFT 2
#define BORDER_RIGHT 2
#define BORDER_BOTTOM 2

/*** VFUNCS ***/

/* NB: This signal can happen without a locked player */
static void
swfdec_text_field_movie_update_area (SwfdecTextFieldMovie *text)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (text);
  cairo_matrix_t matrix, translate;
  double x, y;

  if (swfdec_player_is_locked (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context)))
    swfdec_movie_invalidate_next (movie);

  /* check if we indeed want to render */
  swfdec_movie_local_to_global_matrix (movie, &matrix);
  cairo_matrix_multiply (&matrix, &matrix,
      &SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context)->priv->global_to_stage);
  if (matrix.xy != 0.0 || matrix.yx != 0.0 ||
      matrix.xx <= 0.0 || matrix.yy <= 0.0) {
    swfdec_rectangle_init_empty (&text->stage_rect);
    return;
  }

  translate = matrix;
  x = text->extents.x0;
  y = text->extents.y0;
  cairo_matrix_transform_point (&matrix, &x, &y);
  cairo_matrix_init_translate (&translate, round (x) - x, round (y) - y);
  cairo_matrix_multiply (&matrix, &matrix, &translate);
  
  x = text->extents.x0;
  y = text->extents.y0;
  cairo_matrix_transform_point (&matrix, &x, &y);
  text->stage_rect.x = x;
  text->stage_rect.y = y;
  x = text->extents.x1;
  y = text->extents.y1;
  cairo_matrix_transform_point (&matrix, &x, &y);
  /* FIXME: floor, ceil or round? */
  text->stage_rect.width = round (x) - text->stage_rect.x;
  text->stage_rect.height = round (y) - text->stage_rect.y;
  text->xscale = matrix.xx * SWFDEC_TWIPS_SCALE_FACTOR;
  text->yscale = matrix.yy * SWFDEC_TWIPS_SCALE_FACTOR;
  swfdec_text_layout_set_scale (text->layout, text->yscale);
  swfdec_text_layout_set_wrap_width (text->layout, text->stage_rect.width - 
      BORDER_LEFT - BORDER_RIGHT);
}

static void
swfdec_text_field_movie_auto_size (SwfdecTextFieldMovie *text)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (text);
  SwfdecRectangle area;
  double x0, z0, x1, z1; /* y0 and y1 are taken by math.h */

  if (text->auto_size == SWFDEC_AUTO_SIZE_NONE)
    return;

  swfdec_text_field_movie_get_visible_area (text, &area);
  x1 = (double) text->layout_width - area.width;
  z1 = (double) text->layout_height - area.height;

  if (x1 == 0 && z1 == 0)
    return;

  /* FIXME: rounding */
  x1 *= SWFDEC_TWIPS_SCALE_FACTOR / text->xscale;
  z1 *= SWFDEC_TWIPS_SCALE_FACTOR / text->yscale;

  switch (text->auto_size) {
    case SWFDEC_AUTO_SIZE_LEFT:
      x0 = 0;
      break;
    case SWFDEC_AUTO_SIZE_RIGHT:
      x0 = -x1;
      x1 = 0;
      break;
    case SWFDEC_AUTO_SIZE_CENTER:
      x1 = x1 / 2;
      x0 = -x1;
      break;
    case SWFDEC_AUTO_SIZE_NONE:
    default:
      g_assert_not_reached ();
  }
  z0 = 0;

  cairo_matrix_transform_distance (&movie->inverse_matrix, &x0, &z0);
  text->extents.x0 += x0;
  text->extents.y0 += z0;

  cairo_matrix_transform_distance (&movie->inverse_matrix, &x1, &z1);
  text->extents.x1 += x1;
  text->extents.y1 += z1;

  swfdec_text_field_movie_update_area (text);
}

static void
swfdec_text_field_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);

  /* Doing auto-size when invalidating extents is a nasty trick that is 
   * supposed to help in calculating the correct size with the same caching
   * algorithm as the official player. Consider the following code:
   * text.autoSize = "left";
   * if (foo)
   *   y = text._width;
   * text.autoSize = "right";
   * If foo is set, querying width will cause the autosize to happen, which
   * will cause text to be left-aligned. If foo is not set, autosize doesn't
   * happen until after it's set to right-aligned.
   */
  swfdec_text_field_movie_auto_size (text);

  swfdec_rect_union (extents, extents, &text->extents);
}

static void
swfdec_text_field_movie_invalidate (SwfdecMovie *movie, const cairo_matrix_t *matrix, gboolean last)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecRect rect;

  rect.x0 = text->stage_rect.x;
  rect.y0 = text->stage_rect.y;
  rect.x1 = text->stage_rect.x + text->stage_rect.width;
  rect.y1 = text->stage_rect.y + text->stage_rect.height;

  if (text->border) {
    rect.x1++;
    rect.y1++;
  }

  swfdec_rect_transform (&rect, &rect,
      &SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context)->priv->stage_to_global);
  swfdec_player_invalidate (
      SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context), &rect);
}

static gboolean
swfdec_text_field_movie_has_focus (SwfdecTextFieldMovie *text)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context);

  return swfdec_player_has_focus (player, SWFDEC_ACTOR (text));
}

static void
swfdec_text_field_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *ctrans, const SwfdecRect *inval)
{
  static const cairo_matrix_t identity = { 1, 0, 0, 1, 0, 0 };
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecRectangle area;
  SwfdecColor color;

  /* textfields don't mask */
  if (swfdec_color_transform_is_mask (ctrans) ||
      swfdec_rectangle_is_empty (&text->stage_rect))
    return;

  /* FIXME: need to handle the case where we're not drawing with identity */
  cairo_set_matrix (cr, &identity);

  if (text->background) {
    cairo_rectangle (cr, text->stage_rect.x, text->stage_rect.y,
	text->stage_rect.width, text->stage_rect.height);
    color = swfdec_color_apply_transform (text->background_color, ctrans);
    swfdec_color_set_source (cr, SWFDEC_COLOR_OPAQUE (color));
    cairo_fill (cr);
  }
  if (text->border) {
    cairo_rectangle (cr, text->stage_rect.x + 0.5, text->stage_rect.y + 0.5,
	text->stage_rect.width, text->stage_rect.height);
    color = swfdec_color_apply_transform (text->border_color, ctrans);
    swfdec_color_set_source (cr, SWFDEC_COLOR_OPAQUE (color));
    cairo_set_line_width (cr, 1.0);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_stroke (cr);
  }

  swfdec_text_field_movie_get_visible_area (text, &area);
  if (swfdec_text_field_movie_has_focus (text) &&
      (text->editable ||
       (text->selectable && swfdec_text_buffer_has_selection (text->text)))) {
    if (text->background) {
      color = swfdec_color_apply_transform (text->background_color, ctrans);
      color = SWFDEC_COLOR_OPAQUE (color);
    } else {
      color = SWFDEC_COLOR_WHITE;
    }
  } else {
    color = 0;
  }

  /* render the layout */
  cairo_rectangle (cr, area.x, area.y, area.width, area.height);
  cairo_clip (cr);
  cairo_translate (cr, (double) area.x - text->hscroll, area.y);
  swfdec_text_layout_render (text->layout, cr, ctrans,
      text->scroll, area.height, color);
}

static void
swfdec_text_field_movie_dispose (GObject *object)
{
  SwfdecTextFieldMovie *text;

  text = SWFDEC_TEXT_FIELD_MOVIE (object);

  swfdec_text_attributes_reset (&text->default_attributes);

  if (text->style_sheet) {
    if (SWFDEC_IS_STYLESHEET (text->style_sheet)) {
      g_signal_handlers_disconnect_matched (text->style_sheet, 
	  G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, text);
    }
    text->style_sheet = NULL;
  }

  if (text->text) {
    g_signal_handlers_disconnect_matched (text->text, G_SIGNAL_MATCH_DATA,
	0, 0, NULL, NULL, text);
    g_object_unref (text->text);
    text->text = NULL;
  }
  if (text->layout) {
    g_signal_handlers_disconnect_matched (text->layout, G_SIGNAL_MATCH_DATA,
	0, 0, NULL, NULL, text);
    g_object_unref (text->layout);
    text->layout = NULL;
  }

  G_OBJECT_CLASS (swfdec_text_field_movie_parent_class)->dispose (object);
}

static void
swfdec_text_field_movie_mark (SwfdecAsObject *object)
{
  SwfdecTextFieldMovie *text;

  text = SWFDEC_TEXT_FIELD_MOVIE (object);

  if (text->variable != NULL)
    swfdec_as_string_mark (text->variable);

  swfdec_text_buffer_mark (text->text);
  if (text->style_sheet != NULL)
    swfdec_as_object_mark (text->style_sheet);
  if (text->style_sheet_input != NULL)
    swfdec_as_string_mark (text->style_sheet_input);
  if (text->restrict_ != NULL)
    swfdec_as_string_mark (text->restrict_);

  SWFDEC_AS_OBJECT_CLASS (swfdec_text_field_movie_parent_class)->mark (object);
}

/* NB: can be run with unlocked player */
void
swfdec_text_field_movie_update_scroll (SwfdecTextFieldMovie *text)
{
  guint scroll_max, lines_visible, rows, height;

  height = text->stage_rect.height - BORDER_TOP - BORDER_BOTTOM;
  rows = swfdec_text_layout_get_n_rows (text->layout);
  scroll_max = rows - swfdec_text_layout_get_visible_rows_end (text->layout, height);
  if (scroll_max != text->scroll_max) {
    text->scroll_max = scroll_max;
    text->scroll_changed = TRUE;
  }
  if (scroll_max < text->scroll) {
    text->scroll = scroll_max;
    text->scroll_changed = TRUE;
  }
  lines_visible = swfdec_text_layout_get_visible_rows (text->layout,
      text->scroll, height);
  if (lines_visible != text->lines_visible) {
    text->lines_visible = lines_visible;
    text->scroll_changed = TRUE;
  }
}

/* NB: This signal can happen without a locked player */
static void
swfdec_text_field_movie_layout_changed (SwfdecTextLayout *layout,
    SwfdecTextFieldMovie *text)
{
  guint w, h, max;

  if (swfdec_player_is_locked (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context)))
    swfdec_movie_invalidate_last (SWFDEC_MOVIE (text));

  w = swfdec_text_layout_get_width (layout);
  h = swfdec_text_layout_get_height (layout);

  if (w != text->layout_width || h != text->layout_height) {
    text->layout_width = w;
    text->layout_height = h;
    if (text->auto_size != SWFDEC_AUTO_SIZE_NONE)
      swfdec_movie_queue_update (SWFDEC_MOVIE (text), SWFDEC_MOVIE_INVALID_EXTENTS);
  }

  swfdec_text_field_movie_update_scroll (text);

  max = swfdec_text_field_movie_get_hscroll_max (text);
  if (text->hscroll > max) {
    text->hscroll = max;
    text->scroll_changed = TRUE;
  }
}

static void
swfdec_text_field_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecTextField *text_field = SWFDEC_TEXT_FIELD (movie->graphic);
  SwfdecAsContext *cx;
  SwfdecAsValue val;
  SwfdecMovie *parent;
  gboolean needs_unuse;

  needs_unuse = swfdec_sandbox_try_use (movie->resource->sandbox);

  cx = SWFDEC_AS_OBJECT (movie)->context;

  swfdec_text_field_movie_init_properties (cx);

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie),
	SWFDEC_AS_VALUE_GET_OBJECT (&val));
  }

  // listen self
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (movie));
  swfdec_as_object_call (SWFDEC_AS_OBJECT (movie), SWFDEC_AS_STR_addListener,
      1, &val, NULL);

  text->border_color = SWFDEC_COLOR_COMBINE (0, 0, 0, 0);
  text->background_color = SWFDEC_COLOR_COMBINE (255, 255, 255, 0);

  if (text_field) {
    text->default_attributes.color = text_field->color;
    text->default_attributes.align = text_field->align;
    if (text_field->font != NULL)  {
      text->default_attributes.font =
	swfdec_as_context_get_string (cx, text_field->font);
    }
    text->default_attributes.size = text_field->size / 20;
    text->default_attributes.left_margin = text_field->left_margin / 20;
    text->default_attributes.right_margin = text_field->right_margin / 20;
    text->default_attributes.indent = text_field->indent / 20;
    text->default_attributes.leading = text_field->leading / 20;
  }

  // text
  if (text_field && text_field->input != NULL) {
    swfdec_text_field_movie_set_text (text,
	swfdec_as_context_get_string (cx, text_field->input),
	text->html);
  } else {
    swfdec_text_field_movie_set_text (text, SWFDEC_AS_STR_EMPTY,
	text->html);
  }

  // variable
  if (text_field && text_field->variable != NULL) {
    swfdec_text_field_movie_set_listen_variable (text,
	swfdec_as_context_get_string (cx, text_field->variable));
  }

  if (needs_unuse)
    swfdec_sandbox_unuse (movie->resource->sandbox);

  parent = movie;
  while (parent) {
    g_signal_connect_swapped (parent, "matrix-changed", 
	G_CALLBACK (swfdec_text_field_movie_update_area), movie);
    parent = parent->parent;
  }
  swfdec_text_field_movie_update_area (text);
  swfdec_text_field_movie_layout_changed (text->layout, text);
}

static void
swfdec_text_field_movie_finish_movie (SwfdecMovie *movie)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecMovie *parent;

  parent = SWFDEC_MOVIE (text);
  while (parent) {
    g_signal_handlers_disconnect_matched (parent, 
	  G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, text);
    parent = parent->parent;
  }

  swfdec_text_field_movie_set_listen_variable (text, NULL);
}

static void
swfdec_text_field_movie_iterate (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);

  while (text->changed) {
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_CHANGED);
    text->changed--;
  }
  if (text->scroll_changed) {
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_SCROLL);
    text->scroll_changed = FALSE;
  }
}

static SwfdecMovie *
swfdec_text_field_movie_contains (SwfdecMovie *movie, double x, double y,
    gboolean events)
{
  if (events) {
    /* check for movies in a higher layer that react to events */
    SwfdecMovie *ret;
    ret = SWFDEC_MOVIE_CLASS (swfdec_text_field_movie_parent_class)->contains (
	movie, x, y, TRUE);
    if (ret && SWFDEC_IS_ACTOR (ret) && swfdec_actor_get_mouse_events (SWFDEC_ACTOR (ret)))
      return ret;
  }

  return movie;
}

static void
swfdec_text_field_movie_letter_clicked (SwfdecTextFieldMovie *text,
    guint index_)
{
  const SwfdecTextAttributes *attr;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (index_ <= swfdec_text_buffer_get_length (text->text));

  attr = swfdec_text_buffer_get_attributes (text->text, index_);

  if (attr->url != SWFDEC_AS_STR_EMPTY) {
    swfdec_player_launch (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context),
	SWFDEC_LOADER_REQUEST_DEFAULT, attr->url, attr->target, NULL);
  }
}

static gboolean
swfdec_text_field_movie_xy_to_index (SwfdecTextFieldMovie *text, double x,
    double y, guint *index_, gboolean *before)
{
  SWFDEC_STUB ("swfdec_text_field_movie_xy_to_index");
  if (index_)
    *index_ = 0;
  if (before)
    *before = FALSE;
  return FALSE;
}

static SwfdecMouseCursor
swfdec_text_field_movie_mouse_cursor (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  double x, y;
  guint index_;
  const SwfdecTextAttributes *attr;

  swfdec_movie_get_mouse (SWFDEC_MOVIE (actor), &x, &y);

  if (swfdec_text_field_movie_xy_to_index (text, x, y, &index_, NULL)) {
    attr = swfdec_text_buffer_get_attributes (text->text, index_);
  } else {
    attr = NULL;
  }

  if (attr != NULL && attr->url != SWFDEC_AS_STR_EMPTY) {
    return SWFDEC_MOUSE_CURSOR_CLICK;
  } else if (text->selectable) {
    return SWFDEC_MOUSE_CURSOR_TEXT;
  } else{
    return SWFDEC_MOUSE_CURSOR_NORMAL;
  }
}

static gboolean
swfdec_text_field_movie_mouse_events (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);

  return text->selectable;
}

static void
swfdec_text_field_movie_mouse_press (SwfdecActor *actor, guint button)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  double x, y;
  guint index_;
  gboolean direct, before;

  if (!text->selectable)
    return;

  if (button != 0) {
    SWFDEC_FIXME ("implement popup menus, scrollwheel and middle mouse paste");
    return;
  }

  swfdec_movie_get_mouse (SWFDEC_MOVIE (actor), &x, &y);

  direct = swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &before);

  text->mouse_pressed = TRUE;
  if (!before && index_ < swfdec_text_buffer_get_length (text->text))
    index_++;
  swfdec_text_buffer_set_cursor (text->text, index_, index_);

  if (direct) {
    text->character_pressed = index_;
  } else {
    text->character_pressed = 0;
  }

  swfdec_player_grab_focus (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (text)->context), actor);
}

static void
swfdec_text_field_movie_mouse_move (SwfdecActor *actor, double x, double y)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  guint index_;
  gboolean direct, before;

  if (!text->selectable)
    return;

  if (!text->mouse_pressed)
    return;

  direct = swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &before);

  if (!before && index_ < swfdec_text_buffer_get_length (text->text))
    index_++;

  swfdec_text_buffer_set_cursor (text->text, swfdec_text_buffer_get_cursor (text->text), index_);
}

static void
swfdec_text_field_movie_mouse_release (SwfdecActor *actor, guint button)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  double x, y;
  guint index_;
  gboolean direct, before;

  if (!text->selectable)
    return;

  if (button != 0) {
    SWFDEC_FIXME ("implement popup menus, scrollwheel and middle mouse paste");
    return;
  }

  swfdec_movie_get_mouse (SWFDEC_MOVIE (text), &x, &y);

  text->mouse_pressed = FALSE;

  direct = swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &before);

  if (text->character_pressed != 0) {
    if (direct && text->character_pressed == index_ + 1 - (before ? 1 : 0)) {
      swfdec_text_field_movie_letter_clicked (text,
	  text->character_pressed - 1);
    }

    text->character_pressed = 0;
  }
}

static void
swfdec_text_field_movie_focus_in (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  
  swfdec_text_buffer_set_cursor (text->text, 0,
      swfdec_text_buffer_get_length (text->text));
  if (text->editable || text->selectable)
    swfdec_movie_invalidate_last (SWFDEC_MOVIE (actor));
}

static void
swfdec_text_field_movie_focus_out (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  
  if (text->editable || text->selectable)
    swfdec_movie_invalidate_last (SWFDEC_MOVIE (actor));
}

static void
swfdec_text_field_movie_key_press (SwfdecActor *actor, guint keycode, guint character)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  char insert[7];
  guint len;
  gsize start, end;
  const char *string;
#define BACKWARD(text, _index) ((_index) == 0 ? 0 : (gsize) (g_utf8_prev_char ((text) + (_index)) - (text)))
#define FORWARD(text, _index) ((text)[_index] == 0 ? (_index) : (gsize) (g_utf8_next_char ((text) + (_index)) - (text)))

  if (!text->editable)
    return;

  string = swfdec_text_buffer_get_text (text->text);
  swfdec_text_buffer_get_selection (text->text, &start, &end);

  switch (keycode) {
    case SWFDEC_KEY_LEFT:
      if (swfdec_text_buffer_has_selection (text->text)) {
	swfdec_text_buffer_set_cursor (text->text, start, start);
      } else {
	start = BACKWARD (string, start);
	swfdec_text_buffer_set_cursor (text->text, start, start);
      }
      return;
    case SWFDEC_KEY_RIGHT:
      if (swfdec_text_buffer_has_selection (text->text)) {
	swfdec_text_buffer_set_cursor (text->text, end, end);
      } else {
	start = FORWARD (string, start);
	swfdec_text_buffer_set_cursor (text->text, start, start);
      }
      return;
    case SWFDEC_KEY_BACKSPACE:
      if (!swfdec_text_buffer_has_selection (text->text)) {
	start = BACKWARD (string, start);
      }
      if (start != end) {
	swfdec_text_buffer_delete_text (text->text, start, end - start);
      }
      return;
    case SWFDEC_KEY_DELETE:
      if (!swfdec_text_buffer_has_selection (text->text)) {
	end = FORWARD (string, end);
      }
      if (start != end) {
	swfdec_text_buffer_delete_text (text->text, start, end - start);
      }
      return;
    default:
      break;
  }

  if (character == 0)
    return;
  len = g_unichar_to_utf8 (character, insert);
  if (len) {
    insert[len] = 0;
    if (start != end)
      swfdec_text_buffer_delete_text (text->text, start, end - start);
    swfdec_text_buffer_insert_text (text->text, start, insert);
  }
}

static void
swfdec_text_field_movie_key_release (SwfdecActor *actor, guint keycode, guint character)
{
  //SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
}

static void
swfdec_text_field_movie_class_init (SwfdecTextFieldMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);
  SwfdecActorClass *actor_class = SWFDEC_ACTOR_CLASS (g_class);

  object_class->dispose = swfdec_text_field_movie_dispose;

  asobject_class->mark = swfdec_text_field_movie_mark;

  movie_class->init_movie = swfdec_text_field_movie_init_movie;
  movie_class->finish_movie = swfdec_text_field_movie_finish_movie;
  movie_class->update_extents = swfdec_text_field_movie_update_extents;
  movie_class->render = swfdec_text_field_movie_render;
  movie_class->invalidate = swfdec_text_field_movie_invalidate;
  movie_class->contains = swfdec_text_field_movie_contains;

  actor_class->mouse_cursor = swfdec_text_field_movie_mouse_cursor;
  actor_class->mouse_events = swfdec_text_field_movie_mouse_events;
  actor_class->mouse_press = swfdec_text_field_movie_mouse_press;
  actor_class->mouse_release = swfdec_text_field_movie_mouse_release;
  actor_class->mouse_move = swfdec_text_field_movie_mouse_move;
  actor_class->focus_in = swfdec_text_field_movie_focus_in;
  actor_class->focus_out = swfdec_text_field_movie_focus_out;
  actor_class->key_press = swfdec_text_field_movie_key_press;
  actor_class->key_release = swfdec_text_field_movie_key_release;

  actor_class->iterate_start = swfdec_text_field_movie_iterate;
}

static void
swfdec_text_field_movie_text_changed (SwfdecTextBuffer *buffer, 
    SwfdecTextFieldMovie *text)
{
  swfdec_movie_invalidate_last (SWFDEC_MOVIE (text));
  text->changed++;
}

static void
swfdec_text_field_movie_cursor_changed (SwfdecTextBuffer *buffer, 
    gulong start, gulong end, SwfdecTextFieldMovie *text)
{
  swfdec_movie_invalidate_last (SWFDEC_MOVIE (text));
  /* FIXME: update selection */
}

static void
swfdec_text_field_movie_init (SwfdecTextFieldMovie *text)
{
  text->text = swfdec_text_buffer_new ();
  g_signal_connect (text->text, "cursor-changed", 
      G_CALLBACK (swfdec_text_field_movie_cursor_changed), text);
  g_signal_connect (text->text, "text-changed", 
      G_CALLBACK (swfdec_text_field_movie_text_changed), text);

  text->layout = swfdec_text_layout_new (text->text);
  g_signal_connect (text->layout, "changed",
      G_CALLBACK (swfdec_text_field_movie_layout_changed), text);

  text->mouse_wheel_enabled = TRUE;

  swfdec_text_attributes_reset (&text->default_attributes);
}

static void
swfdec_text_field_movie_parse_listen_variable (SwfdecTextFieldMovie *text,
    const char *variable, SwfdecAsObject **object, const char **name)
{
  SwfdecAsContext *cx;
  SwfdecAsObject *parent;
  const char *p1, *p2;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (object != NULL);
  g_return_if_fail (name != NULL);

  *object = NULL;
  *name = NULL;

  if (SWFDEC_MOVIE (text)->parent == NULL)
    return;

  g_assert (SWFDEC_IS_AS_OBJECT (SWFDEC_MOVIE (text)->parent));
  cx = SWFDEC_AS_OBJECT (text)->context;
  parent = SWFDEC_AS_OBJECT (SWFDEC_MOVIE (text)->parent);

  p1 = strrchr (variable, '.');
  p2 = strrchr (variable, ':');
  if (p1 == NULL && p2 == NULL) {
    *object = parent;
    *name = variable;
  } else {
    if (p1 == NULL || (p2 != NULL && p2 > p1))
      p1 = p2;
    if (strlen (p1) == 1)
      return;
    *object = swfdec_action_lookup_object (cx, parent, variable, p1);
    if (*object == NULL)
      return;
    *name = swfdec_as_context_get_string (cx, p1 + 1);
  }
}

void
swfdec_text_field_movie_set_listen_variable_text (SwfdecTextFieldMovie *text,
    const char *value)
{
  SwfdecAsObject *object;
  const char *name;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (text->variable != NULL);
  g_return_if_fail (value != NULL);

  swfdec_text_field_movie_parse_listen_variable (text, text->variable,
      &object, &name);
  if (object != NULL) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_STRING (&val, value);
    swfdec_as_object_set_variable (object, name, &val);
  }
}

static void
swfdec_text_field_movie_variable_listener_callback (SwfdecAsObject *object,
    const char *name, const SwfdecAsValue *val)
{
  SwfdecTextFieldMovie *text;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (object));

  text = SWFDEC_TEXT_FIELD_MOVIE (object);
  swfdec_text_field_movie_set_text (text,
      swfdec_as_value_to_string (object->context, val), text->html);
}

void
swfdec_text_field_movie_set_listen_variable (SwfdecTextFieldMovie *text,
    const char *value)
{
  SwfdecAsObject *object;
  const char *name;

  // FIXME: case-insensitive when v < 7?
  if (text->variable == value)
    return;

  if (text->variable != NULL) {
    swfdec_text_field_movie_parse_listen_variable (text, text->variable,
	&object, &name);
    if (object != NULL && SWFDEC_IS_MOVIE (object)) {
      swfdec_movie_remove_variable_listener (SWFDEC_MOVIE (object),
	  SWFDEC_AS_OBJECT (text), name,
	  swfdec_text_field_movie_variable_listener_callback);
    }
  }

  text->variable = value;

  if (value != NULL) {
    SwfdecAsValue val;

    swfdec_text_field_movie_parse_listen_variable (text, value, &object,
	&name);
    if (object != NULL && swfdec_as_object_get_variable (object, name, &val)) {
      swfdec_text_field_movie_set_text (text,
	  swfdec_as_value_to_string (SWFDEC_AS_OBJECT (text)->context, &val),
	  text->html);
    }
    if (object != NULL && SWFDEC_IS_MOVIE (object)) {
      swfdec_movie_add_variable_listener (SWFDEC_MOVIE (object),
	  SWFDEC_AS_OBJECT (text), name,
	  swfdec_text_field_movie_variable_listener_callback);
    }
  }
}

const char *
swfdec_text_field_movie_get_text (SwfdecTextFieldMovie *text)
{
  char *ret, *p;
  const char *org;
  gsize filled, len;

  org = swfdec_text_buffer_get_text (text->text);
  len = swfdec_text_buffer_get_length (text->text);

  ret = g_new (char, len + 1);
  /* remove all \r */
  filled = 0;
  while ((p = strchr (org, '\r'))) {
    memcpy (ret + filled, org, p - org);
    filled += p - org;
    org = p + 1;
    len--;
  }
  g_assert (len >= filled);
  memcpy (ret + filled, org, len - filled);
  ret[len] = 0;

  /* change all \n to \r */
  p = ret;
  while ((p = strchr (p, '\n')) != NULL) {
    *p = '\r';
  }

  return swfdec_as_context_give_string (SWFDEC_AS_OBJECT (text)->context, ret);
}

void
swfdec_text_field_movie_replace_text (SwfdecTextFieldMovie *text,
    guint start_index, guint end_index, const char *str)
{
  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (end_index <= swfdec_text_buffer_get_length (text->text));
  g_return_if_fail (start_index <= end_index);
  g_return_if_fail (str != NULL);

  /* if there was a style sheet set when setting the text, modifications are
   * not allowed */
  if (text->style_sheet_input)
    return;

  if (end_index > start_index)
    swfdec_text_buffer_delete_text (text->text, start_index, end_index - start_index);

  if (start_index == swfdec_text_buffer_get_length (text->text) &&
      start_index > 0) {
    const SwfdecTextAttributes *attr = swfdec_text_buffer_get_attributes (text->text, 0);
    if (SWFDEC_AS_OBJECT (text)->context->version < 8) {
      SWFDEC_FIXME ("replaceText to the end of the TextField might use wrong text format on version 7");
    }
    swfdec_text_buffer_insert_text (text->text, start_index, str);
    swfdec_text_buffer_set_attributes (text->text, start_index, 
	swfdec_text_buffer_get_length (text->text) - start_index, attr, 
	SWFDEC_TEXT_ATTRIBUTES_MASK);
  } else {
    swfdec_text_buffer_insert_text (text->text, start_index, str);
  }
}

void
swfdec_text_field_movie_set_text (SwfdecTextFieldMovie *text, const char *str,
    gboolean html)
{
  gsize length;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (str != NULL);

  /* Flash 7 resets to default style here */
  if (html && SWFDEC_AS_OBJECT (text)->context->version < 8)
    swfdec_text_attributes_reset (&text->default_attributes);

  length = swfdec_text_buffer_get_length (text->text);
  if (length)
    swfdec_text_buffer_delete_text (text->text, 0, length);
  swfdec_text_buffer_set_attributes (text->text, 0, 0, &text->default_attributes,
      SWFDEC_TEXT_ATTRIBUTES_MASK);

  if (SWFDEC_AS_OBJECT (text)->context->version >= 7 &&
      text->style_sheet != NULL)
  {
    text->style_sheet_input = str;
    swfdec_text_field_movie_html_parse (text, str);
  }
  else
  {
    text->style_sheet_input = NULL;
    if (html) {
      swfdec_text_field_movie_html_parse (text, str);
    } else {
      char *s, *p;
      s = p = g_strdup (str);
      while ((p = strchr (p, '\r')))
	*p = '\n';
      swfdec_text_buffer_insert_text (text->text, 0, s);
      g_free (s);
    }
  }
}

gboolean
swfdec_text_field_movie_get_visible_area (SwfdecTextFieldMovie *text, SwfdecRectangle *rect)
{
  int tmp;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), FALSE);
  g_return_val_if_fail (rect != NULL, FALSE);

  tmp = round ((BORDER_LEFT + BORDER_RIGHT) * text->xscale);
  if (tmp >= text->stage_rect.width) {
    *rect = text->stage_rect;
    return FALSE;
  } else {
    rect->width = text->stage_rect.width - tmp;
  }
  tmp = round ((BORDER_TOP + BORDER_BOTTOM) * text->yscale);
  if (tmp >= text->stage_rect.height) {
    *rect = text->stage_rect;
    return FALSE;
  } else {
    rect->height = text->stage_rect.height - tmp;
  }
  rect->x = text->stage_rect.x + round (BORDER_LEFT * text->xscale);
  rect->y = text->stage_rect.y + round (BORDER_TOP * text->yscale) - 1;
  return TRUE;
}

guint
swfdec_text_field_movie_get_hscroll_max (SwfdecTextFieldMovie *text)
{
  SwfdecRectangle area;
  guint width;

  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), 0);

  swfdec_text_field_movie_get_visible_area (text, &area);
  width = text->layout_width;
  if ((guint) area.width >= width)
    return 0;
  else
    return width - area.width;
}

