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
#include "swfdec_as_internal.h"
#include "swfdec_as_interpret.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_renderer_internal.h"
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

static void
swfdec_text_field_movie_compute_layout_area (SwfdecTextFieldMovie *text)
{
  int tmpx, tmpy;

  tmpx = round ((BORDER_LEFT + BORDER_RIGHT) * SWFDEC_TWIPS_SCALE_FACTOR * text->to_layout.xx);
  tmpy = round ((BORDER_TOP + BORDER_BOTTOM) * SWFDEC_TWIPS_SCALE_FACTOR * text->to_layout.yy);
  text->layout_area = text->stage_area;
  if (tmpx >= text->layout_area.width ||
      tmpy >= text->layout_area.height) {
    return;
  }
  
  text->layout_area.x += tmpx / 2;
  text->layout_area.y += tmpy / 2;
  text->layout_area.width -= tmpx;
  text->layout_area.height -= tmpy;
}

/* NB: This signal can happen without a locked player */
static void
swfdec_text_field_movie_update_area (SwfdecTextFieldMovie *text)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (text);
  cairo_matrix_t *matrix, translate;
  double x, y;

  if (swfdec_player_is_locked (SWFDEC_PLAYER (swfdec_gc_object_get_context (text))))
    swfdec_movie_invalidate_next (movie);

  /* check if we indeed want to render */
  matrix = &text->to_layout;
  swfdec_movie_local_to_global_matrix (movie, matrix);
  cairo_matrix_multiply (matrix, matrix,
      &SWFDEC_PLAYER (swfdec_gc_object_get_context (movie))->priv->global_to_stage);
  if (matrix->xy != 0.0 || matrix->yx != 0.0 ||
      matrix->xx <= 0.0 || matrix->yy <= 0.0) {
    swfdec_rectangle_init_empty (&text->stage_area);
    swfdec_rectangle_init_empty (&text->layout_area);
    return;
  }

  x = text->extents.x0;
  y = text->extents.y0;
  cairo_matrix_transform_point (matrix, &x, &y);
  cairo_matrix_init_translate (&translate, round (x) - x, round (y) - y);
  cairo_matrix_multiply (matrix, matrix, &translate);
  text->from_layout = *matrix;
  if (cairo_matrix_invert (&text->from_layout)) {
    SWFDEC_ERROR ("cannot invert to-layout matrix");
    swfdec_rectangle_init_empty (&text->stage_area);
    swfdec_rectangle_init_empty (&text->layout_area);
    return;
  }
  
  x = text->extents.x0;
  y = text->extents.y0;
  cairo_matrix_transform_point (matrix, &x, &y);
  text->stage_area.x = x;
  text->stage_area.y = y;
  x = text->extents.x1;
  y = text->extents.y1;
  cairo_matrix_transform_point (matrix, &x, &y);
  /* FIXME: floor, ceil or round? */
  text->stage_area.width = round (x) - text->stage_area.x;
  text->stage_area.height = round (y) - text->stage_area.y;

  swfdec_text_field_movie_compute_layout_area (text);

  swfdec_text_layout_set_scale (text->layout, matrix->yy * SWFDEC_TWIPS_SCALE_FACTOR);
  swfdec_text_layout_set_wrap_width (text->layout, text->layout_area.width);
}

void
swfdec_text_field_movie_autosize (SwfdecTextFieldMovie *text)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (text);
  double x0, z0, x1, z1; /* y0 and y1 are taken by math.h */

  if (text->auto_size == SWFDEC_AUTO_SIZE_NONE)
    return;

  swfdec_text_field_movie_update_layout (text);
  x1 = text->layout_width;
  z1 = text->layout_height;
  cairo_matrix_transform_distance (&text->from_layout, &x1, &z1);
  x1 += (BORDER_LEFT + BORDER_RIGHT) * SWFDEC_TWIPS_SCALE_FACTOR;
  z1 += (BORDER_TOP + BORDER_BOTTOM) * SWFDEC_TWIPS_SCALE_FACTOR;
  cairo_matrix_transform_distance (&movie->inverse_matrix, &x1, &z1);
  x1 -= text->extents.x1 - text->extents.x0;
  z1 -= text->extents.y1 - text->extents.y0;

  /* when word wrapping is enabled, don't resize width */
  if (swfdec_text_layout_get_word_wrap (text->layout))
    x1 = 0;

  if (x1 == 0 && z1 == 0)
    return;

  x1 = ceil (x1);
  z1 = ceil (z1);

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

  swfdec_movie_invalidate_next (movie);
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);

  text->extents.x0 += x0;
  text->extents.y0 += z0;
  text->extents.x1 += x1;
  text->extents.y1 += z1;

  swfdec_text_field_movie_update_area (text);
}

static void
swfdec_text_field_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);

  swfdec_rect_union (extents, extents, &text->extents);
}

static void
swfdec_text_field_movie_invalidate (SwfdecMovie *movie, const cairo_matrix_t *matrix, gboolean last)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecRect rect;

  rect.x0 = text->stage_area.x;
  rect.y0 = text->stage_area.y;
  rect.x1 = text->stage_area.x + text->stage_area.width;
  rect.y1 = text->stage_area.y + text->stage_area.height;

  if (text->border) {
    rect.x1++;
    rect.y1++;
  }

  swfdec_rect_transform (&rect, &rect,
      &SWFDEC_PLAYER (swfdec_gc_object_get_context (text))->priv->stage_to_global);
  swfdec_player_invalidate (SWFDEC_PLAYER (swfdec_gc_object_get_context (movie)),
      movie, &rect);
}

static gboolean
swfdec_text_field_movie_has_focus (SwfdecTextFieldMovie *text)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (text));

  return swfdec_player_has_focus (player, SWFDEC_ACTOR (text));
}

static void
swfdec_text_field_movie_render (SwfdecMovie *movie, cairo_t *cr,
    const SwfdecColorTransform *ctrans)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecRectangle *area;
  SwfdecColor color;
  SwfdecRect inval;

  /* textfields don't mask */
  if (swfdec_color_transform_is_mask (ctrans) ||
      swfdec_rectangle_is_empty (&text->stage_area))
    return;

  swfdec_renderer_reset_matrix (cr);

  if (text->background) {
    cairo_rectangle (cr, text->stage_area.x, text->stage_area.y,
	text->stage_area.width, text->stage_area.height);
    color = swfdec_color_apply_transform (text->background_color, ctrans);
    swfdec_color_set_source (cr, SWFDEC_COLOR_OPAQUE (color));
    cairo_fill (cr);
  }
  if (text->border) {
    cairo_rectangle (cr, text->stage_area.x + 0.5, text->stage_area.y + 0.5,
	text->stage_area.width, text->stage_area.height);
    color = swfdec_color_apply_transform (text->border_color, ctrans);
    swfdec_color_set_source (cr, SWFDEC_COLOR_OPAQUE (color));
    cairo_set_line_width (cr, 1.0);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_stroke (cr);
  }

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

  area = &text->layout_area;
  /* Don't draw if out of clip */
  cairo_clip_extents (cr, &inval.x0, &inval.y0, &inval.x1, &inval.y1);
  if (inval.x1 <= area->x || inval.x0 >= area->x + area->width ||
      inval.y1 <= area->y || inval.y0 >= area->y + area->height)
    return;
  /* render the layout */
  cairo_rectangle (cr, area->x, area->y, area->width, area->height);
  cairo_clip (cr);
  /* FIXME: This -1 is spacing? */
  cairo_translate (cr, (double) area->x - text->hscroll, area->y - 1);
  swfdec_text_layout_render (text->layout, cr, ctrans,
      text->scroll, area->height, color);
}

static void
swfdec_text_field_movie_dispose (GObject *object)
{
  SwfdecTextFieldMovie *text;

  text = SWFDEC_TEXT_FIELD_MOVIE (object);

  if (text->style_sheet) {
    if (SWFDEC_IS_STYLE_SHEET (text->style_sheet->relay)) {
      g_signal_handlers_disconnect_matched (text->style_sheet->relay, 
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
    g_object_unref (text->layout);
    text->layout = NULL;
  }

  G_OBJECT_CLASS (swfdec_text_field_movie_parent_class)->dispose (object);
}

static void
swfdec_text_field_movie_mark (SwfdecGcObject *object)
{
  SwfdecTextFieldMovie *text;

  text = SWFDEC_TEXT_FIELD_MOVIE (object);

  if (text->variable != NULL)
    swfdec_as_string_mark (text->variable);

  swfdec_text_buffer_mark (text->text);
  if (text->style_sheet != NULL)
    swfdec_gc_object_mark (text->style_sheet);
  if (text->style_sheet_input != NULL)
    swfdec_as_string_mark (text->style_sheet_input);
  if (text->restrict_ != NULL)
    swfdec_as_string_mark (text->restrict_);

  SWFDEC_GC_OBJECT_CLASS (swfdec_text_field_movie_parent_class)->mark (object);
}

void
swfdec_text_field_movie_emit_onScroller (SwfdecTextFieldMovie *text)
{
  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));

  if (!text->onScroller_emitted && swfdec_movie_get_version (SWFDEC_MOVIE (text)) > 6) {
    swfdec_player_add_action (SWFDEC_PLAYER (swfdec_gc_object_get_context (text)), 
	SWFDEC_ACTOR (text), SWFDEC_EVENT_SCROLL, 0, SWFDEC_PLAYER_ACTION_QUEUE_NORMAL);
  }
  text->onScroller_emitted = TRUE;
}

void
swfdec_text_field_movie_update_layout (SwfdecTextFieldMovie *text)
{
  guint scroll_max, lines_visible, rows, height, max;
  gboolean scroll_changed = FALSE;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));

  text->layout_width = swfdec_text_layout_get_width (text->layout);
  text->layout_height = swfdec_text_layout_get_height (text->layout);

  height = text->layout_area.height;
  rows = swfdec_text_layout_get_n_rows (text->layout);
  scroll_max = rows - swfdec_text_layout_get_visible_rows_end (text->layout, height);
  if (scroll_max != text->scroll_max) {
    text->scroll_max = scroll_max;
    scroll_changed = TRUE;
  }
  if (scroll_max < text->scroll) {
    text->scroll = scroll_max;
    scroll_changed = TRUE;
  }
  lines_visible = swfdec_text_layout_get_visible_rows (text->layout,
      text->scroll, height);
  if (lines_visible != text->lines_visible) {
    text->lines_visible = lines_visible;
    scroll_changed = TRUE;
  }

  max = swfdec_text_field_movie_get_hscroll_max (text);
  if (text->hscroll > max) {
    text->hscroll = max;
    scroll_changed = TRUE;
  }

  if (scroll_changed)
    swfdec_text_field_movie_emit_onScroller (text);
}

static void
swfdec_text_field_movie_init_movie (SwfdecMovie *movie)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecTextField *text_field = SWFDEC_TEXT_FIELD (movie->graphic);
  SwfdecAsContext *cx;
  SwfdecAsValue val;
  SwfdecMovie *parent;
  SwfdecAsObject *array;
  gboolean needs_unuse;

  needs_unuse = swfdec_sandbox_try_use (movie->resource->sandbox);

  cx = swfdec_gc_object_get_context (movie);
  if (movie->resource->version > 5) {
    SwfdecAsObject *o = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (movie));

    swfdec_text_field_movie_init_properties (cx);

    swfdec_as_object_set_constructor_by_name (o,
	SWFDEC_AS_STR_TextField, NULL);

    /* create _listeners array containing self */
    array = swfdec_as_array_new (cx);
    SWFDEC_AS_VALUE_SET_MOVIE (&val, movie);
    swfdec_as_array_push (array, &val);
    SWFDEC_AS_VALUE_SET_OBJECT (&val, array);
    swfdec_as_object_set_variable_and_flags (o, SWFDEC_AS_STR__listeners, 
	&val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  }

  text->border_color = SWFDEC_COLOR_COMBINE (0, 0, 0, 0);
  text->background_color = SWFDEC_COLOR_COMBINE (255, 255, 255, 0);

  if (text_field) {
    text->extents = SWFDEC_GRAPHIC (text_field)->extents;

    text->html = text_field->html;
    text->editable = text_field->editable;
    swfdec_text_layout_set_password (text->layout, text_field->password);
    text->max_chars = text_field->max_chars;
    text->selectable = text_field->selectable;
    text->embed_fonts = text_field->embed_fonts;
    swfdec_text_layout_set_word_wrap (text->layout, text_field->word_wrap);
    text->multiline = text_field->multiline;
    text->auto_size = text_field->auto_size;
    text->border = text_field->border;

    text->text->default_attributes.color = text_field->color;
    text->text->default_attributes.align = text_field->align;
    if (text_field->font != NULL)  {
      text->text->default_attributes.font =
	swfdec_as_context_get_string (cx, text_field->font);
    }
    text->text->default_attributes.size = text_field->size / 20;
    text->text->default_attributes.left_margin = text_field->left_margin / 20;
    text->text->default_attributes.right_margin = text_field->right_margin / 20;
    text->text->default_attributes.indent = text_field->indent / 20;
    text->text->default_attributes.leading = text_field->leading / 20;
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
  /* don't emit onScroller here, plz */
  text->onScroller_emitted = TRUE;
  swfdec_text_field_movie_update_layout (text);
  if (swfdec_movie_get_version (movie) > 6)
    text->onScroller_emitted = FALSE;
  swfdec_text_field_movie_update_area (text);
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
    swfdec_player_add_action (SWFDEC_PLAYER (swfdec_gc_object_get_context (text)), 
	SWFDEC_ACTOR (text), SWFDEC_EVENT_CHANGED, 0, SWFDEC_PLAYER_ACTION_QUEUE_NORMAL);
    text->changed--;
  }

  if (text->onScroller_emitted && swfdec_movie_get_version (SWFDEC_MOVIE (text)) <= 6) {
    swfdec_player_add_action (SWFDEC_PLAYER (swfdec_gc_object_get_context (text)), 
	SWFDEC_ACTOR (text), SWFDEC_EVENT_SCROLL, 0, SWFDEC_PLAYER_ACTION_QUEUE_NORMAL);
  }
  text->onScroller_emitted = FALSE;
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

  cx = swfdec_gc_object_get_context (text);
  parent = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (SWFDEC_MOVIE (text)->parent));

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

static void
swfdec_text_field_movie_asfunction (SwfdecTextFieldMovie *text,
    const char *url)
{
  char **parts;
  SwfdecAsObject *object;
  const char *name;
  SwfdecAsContext *cx;

  g_return_if_fail (g_ascii_strncasecmp (url, "asfunction:",
	strlen ("asfunction:")) == 0);

  cx = swfdec_gc_object_get_context (text);

  parts = g_strsplit (url + strlen ("asfunction:"), ",", 2);
  if (parts[0] == NULL) {
    SWFDEC_ERROR ("asfunction link without function name clicked");
    g_strfreev (parts);
    return;
  }

  swfdec_text_field_movie_parse_listen_variable (text,
      swfdec_as_context_get_string (cx, parts[0]), &object, &name);

  if (object == NULL || name == NULL) {
    SWFDEC_ERROR ("Function in asfunction link not found: %s", parts[0]);
    g_strfreev (parts);
    return;
  }

  swfdec_sandbox_use (SWFDEC_MOVIE (text)->resource->sandbox);
  if (parts[1] != NULL) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_STRING (&val,
	swfdec_as_context_get_string (cx, parts[1]));
    swfdec_as_object_call (object, name, 1, &val, NULL);
  } else {
    swfdec_as_object_call (object, name, 0, NULL, NULL);
  }
  swfdec_sandbox_unuse (SWFDEC_MOVIE (text)->resource->sandbox);

  g_strfreev (parts);
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
    if (g_ascii_strncasecmp (attr->url, "asfunction:",
	  strlen ("asfunction:")) == 0) {
      swfdec_text_field_movie_asfunction (text, attr->url);
    } else {
      swfdec_player_launch (SWFDEC_PLAYER (swfdec_gc_object_get_context (text)),
	  attr->url, attr->target, NULL);
    }
  }
}

static void
swfdec_text_field_movie_xy_to_index (SwfdecTextFieldMovie *text, double x,
    double y, gsize *index_, gboolean *hit)
{
  int trailing;

  cairo_matrix_transform_point (&text->to_layout, &x, &y);
  swfdec_text_layout_query_position (text->layout, text->scroll,
      x - text->layout_area.x, y - text->layout_area.y, index_, hit, &trailing);

  if (index_)
    *index_ += trailing;
}

static SwfdecMouseCursor
swfdec_text_field_movie_mouse_cursor (SwfdecActor *actor)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  double x, y;
  gsize index_;
  const SwfdecTextAttributes *attr;
  gboolean hit;

  swfdec_movie_get_mouse (SWFDEC_MOVIE (actor), &x, &y);

  swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &hit);
  if (hit) {
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
  gsize index_;
  gboolean hit;

  if (!text->selectable)
    return;

  if (button != 0) {
    SWFDEC_FIXME ("implement popup menus, scrollwheel and middle mouse paste");
    return;
  }

  swfdec_movie_get_mouse (SWFDEC_MOVIE (actor), &x, &y);

  swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &hit);

  if (hit) {
    text->character_pressed = index_;
  } else {
    text->character_pressed = -1;
  }

  swfdec_player_grab_focus (SWFDEC_PLAYER (swfdec_gc_object_get_context (text)), actor);

  text->mouse_pressed = TRUE;
  swfdec_text_buffer_set_cursor (text->text, index_, index_);
}

static void
swfdec_text_field_movie_mouse_move (SwfdecActor *actor, double x, double y)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  gsize index_;
  gsize start, end;

  if (!text->selectable)
    return;

  if (!text->mouse_pressed)
    return;

  swfdec_text_field_movie_xy_to_index (text, x, y, &index_, NULL);

  swfdec_text_buffer_get_selection (text->text, &start, &end);
  swfdec_text_buffer_set_cursor (text->text, 
      swfdec_text_buffer_get_cursor (text->text) == start ? end : start, index_);
}

static void
swfdec_text_field_movie_mouse_release (SwfdecActor *actor, guint button)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (actor);
  double x, y;
  gsize index_;
  gboolean hit;

  if (!text->selectable)
    return;

  if (button != 0) {
    SWFDEC_FIXME ("implement popup menus, scrollwheel and middle mouse paste");
    return;
  }

  swfdec_movie_get_mouse (SWFDEC_MOVIE (text), &x, &y);

  text->mouse_pressed = FALSE;

  swfdec_text_field_movie_xy_to_index (text, x, y, &index_, &hit);

  if (hit && text->character_pressed == index_) {
    swfdec_text_field_movie_letter_clicked (text, text->character_pressed);
    text->character_pressed = -1;
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
swfdec_text_field_movie_property_get (SwfdecMovie *movie, guint prop_id, 
    SwfdecAsValue *val)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecAsContext *cx = swfdec_gc_object_get_context (text);
  double d;

  switch (prop_id) {
    case SWFDEC_MOVIE_PROPERTY_X:
      swfdec_text_field_movie_autosize (text);
      swfdec_movie_update (movie);
      d = SWFDEC_TWIPS_TO_DOUBLE (movie->matrix.x0 + text->extents.x0);
      swfdec_as_value_set_number (cx, val, d);
      return;
    case SWFDEC_MOVIE_PROPERTY_Y:
      swfdec_text_field_movie_autosize (text);
      swfdec_movie_update (movie);
      d = SWFDEC_TWIPS_TO_DOUBLE (movie->matrix.y0 + text->extents.y0);
      swfdec_as_value_set_number (cx, val, d);
      return;
    case SWFDEC_MOVIE_PROPERTY_WIDTH:
    case SWFDEC_MOVIE_PROPERTY_HEIGHT:
      swfdec_text_field_movie_autosize (text);
      break;
    default:
      break;
  }

  SWFDEC_MOVIE_CLASS (swfdec_text_field_movie_parent_class)->property_get (movie, prop_id, val);
}

static void
swfdec_text_field_movie_property_set (SwfdecMovie *movie, guint prop_id, 
    const SwfdecAsValue *val)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (movie);
  SwfdecAsContext *cx = swfdec_gc_object_get_context (movie);
  SwfdecTwips twips;

  switch (prop_id) {
    case SWFDEC_MOVIE_PROPERTY_X:
      if (!swfdec_as_value_to_twips (swfdec_gc_object_get_context (movie), val, FALSE, &twips))
	return;
      movie->modified = TRUE;
      twips -= text->extents.x0;
      if (twips != movie->matrix.x0) {
	swfdec_movie_begin_update_matrix (movie);
	movie->matrix.x0 = twips;
	swfdec_movie_end_update_matrix (movie);
      }
      return;
    case SWFDEC_MOVIE_PROPERTY_Y:
      if (!swfdec_as_value_to_twips (swfdec_gc_object_get_context (movie), val, FALSE, &twips))
	return;
      movie->modified = TRUE;
      twips -= text->extents.y0;
      if (twips != movie->matrix.y0) {
	swfdec_movie_begin_update_matrix (movie);
	movie->matrix.y0 = twips;
	swfdec_movie_end_update_matrix (movie);
      }
      return;
    case SWFDEC_MOVIE_PROPERTY_WIDTH:
      if (swfdec_as_value_to_twips (cx, val, TRUE, &twips)) {
	movie->modified = TRUE;
	if (text->extents.x1 != text->extents.x0 + twips) {
	  swfdec_movie_invalidate_next (movie);
	  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);
	  text->extents.x1 = text->extents.x0 + twips;
	  swfdec_text_field_movie_update_area (text);
	}
      }
      return;
    case SWFDEC_MOVIE_PROPERTY_HEIGHT:
      movie->modified = TRUE;
      if (swfdec_as_value_to_twips (cx, val, TRUE, &twips)) {
	movie->modified = TRUE;
	if (text->extents.y1 != text->extents.y0 + twips) {
	  swfdec_movie_invalidate_next (movie);
	  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);
	  text->extents.y1 = text->extents.y0 + twips;
	  swfdec_text_field_movie_update_area (text);
	}
      }
      return;
    default:
      break;
  }

  SWFDEC_MOVIE_CLASS (swfdec_text_field_movie_parent_class)->property_set (movie, prop_id, val);
}

static void
swfdec_text_field_movie_class_init (SwfdecTextFieldMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);
  SwfdecActorClass *actor_class = SWFDEC_ACTOR_CLASS (g_class);

  object_class->dispose = swfdec_text_field_movie_dispose;

  gc_class->mark = swfdec_text_field_movie_mark;

  movie_class->init_movie = swfdec_text_field_movie_init_movie;
  movie_class->finish_movie = swfdec_text_field_movie_finish_movie;
  movie_class->update_extents = swfdec_text_field_movie_update_extents;
  movie_class->render = swfdec_text_field_movie_render;
  movie_class->invalidate = swfdec_text_field_movie_invalidate;
  movie_class->contains = swfdec_text_field_movie_contains;
  movie_class->property_get = swfdec_text_field_movie_property_get;
  movie_class->property_set = swfdec_text_field_movie_property_set;

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

  text->mouse_wheel_enabled = TRUE;
  text->character_pressed = -1;
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
swfdec_text_field_movie_variable_listener_callback (gpointer data,
    const char *name, const SwfdecAsValue *val)
{
  SwfdecTextFieldMovie *text = SWFDEC_TEXT_FIELD_MOVIE (data);

  swfdec_text_field_movie_set_text (text,
      swfdec_as_value_to_string (swfdec_gc_object_get_context (text), val), text->html);
}

void
swfdec_text_field_movie_set_listen_variable (SwfdecTextFieldMovie *text,
    const char *value)
{
  SwfdecAsObject *object;
  const char *name;

  if (text->variable != NULL) {
    swfdec_text_field_movie_parse_listen_variable (text, text->variable,
	&object, &name);
    if (object != NULL && object->movie) {
      swfdec_movie_remove_variable_listener (SWFDEC_MOVIE (object->relay),
	  text, name, swfdec_text_field_movie_variable_listener_callback);
    }
  }

  text->variable = value;

  if (value != NULL) {
    SwfdecTextField *text_field =
      SWFDEC_TEXT_FIELD (SWFDEC_MOVIE (text)->graphic);
    SwfdecAsValue val;

    swfdec_text_field_movie_parse_listen_variable (text, value, &object,
	&name);
    if (object != NULL && swfdec_as_object_get_variable (object, name, &val)) {
      swfdec_text_field_movie_set_text (text,
	  swfdec_as_value_to_string (swfdec_gc_object_get_context (text), &val),
	  text->html);
    } else if (text_field != NULL && text_field->input != NULL) {
      // Set to the original value from the tag, not current value
      const char *initial = swfdec_as_context_get_string (
	  swfdec_gc_object_get_context (text), text_field->input);
      swfdec_text_field_movie_set_listen_variable_text (text, initial);
      // FIXME: html value correct here?
      swfdec_text_field_movie_set_text (text, initial, text_field->html);
    }
    if (object != NULL && object->movie) {
      swfdec_movie_add_variable_listener (SWFDEC_MOVIE (object->relay),
	  text, name, swfdec_text_field_movie_variable_listener_callback);
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

  return swfdec_as_context_give_string (swfdec_gc_object_get_context (text), ret);
}

void
swfdec_text_field_movie_set_text (SwfdecTextFieldMovie *text, const char *str,
    gboolean html)
{
  gsize length;

  g_return_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text));
  g_return_if_fail (str != NULL);

  /* Flash 7 resets to default style here */
  if (html && swfdec_gc_object_get_context (text)->version < 8)
    swfdec_text_buffer_reset_default_attributes (text->text);

  length = swfdec_text_buffer_get_length (text->text);
  if (length)
    swfdec_text_buffer_delete_text (text->text, 0, length);

  if (swfdec_gc_object_get_context (text)->version >= 7 &&
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

guint
swfdec_text_field_movie_get_hscroll_max (SwfdecTextFieldMovie *text)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_FIELD_MOVIE (text), 0);

  if ((guint) text->layout_area.width >= text->layout_width ||
      swfdec_text_layout_get_word_wrap (text->layout))
    return 0;
  else
    return text->layout_width - text->layout_area.width;
}

