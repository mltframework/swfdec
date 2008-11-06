/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_movie.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_bitmap_pattern.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_gradient_pattern.h"
#include "swfdec_path.h"
#include "swfdec_pattern.h"
#include "swfdec_stroke.h"
#include "swfdec_utils.h"

/* FIXME: This whole code assumes it works for MovieClip, Button and TextField
 * objects. If it only works for MovieClip objects, fix this. */

static SwfdecDraw *
swfdec_stroke_copy (SwfdecDraw *draw)
{
  SwfdecStroke *sstroke = SWFDEC_STROKE (draw);
  SwfdecStroke *dstroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);

  dstroke->start_width = sstroke->start_width;
  dstroke->start_color = sstroke->start_color;
  if (sstroke->pattern)
    dstroke->pattern = g_object_ref (sstroke->pattern);
  dstroke->start_cap = sstroke->start_cap;
  dstroke->end_cap = sstroke->end_cap;
  dstroke->join = sstroke->join;
  dstroke->miter_limit = sstroke->miter_limit;
  dstroke->no_vscale = sstroke->no_vscale;
  dstroke->no_hscale = sstroke->no_hscale;

  return SWFDEC_DRAW (dstroke);
}

static void
swfdec_sprite_movie_end_fill (SwfdecMovie *movie, SwfdecDraw *new)
{
  /* FIXME: need to cairo_close_path()? */
  movie->draw_fill = new;
  if (new == NULL)
    return;

  movie->draws = g_slist_append (movie->draws, new);

  /* need to begin a new line segment to ensure proper stacking order */
  if (movie->draw_line) {
    movie->draw_line = swfdec_stroke_copy (movie->draw_line);
    movie->draws = g_slist_append (movie->draws, movie->draw_line);
  }
}

#define SWFDEC_COLOR_FROM_COLOR_ALPHA(color, alpha) \
  (((color) & 0xFFFFFF) | SWFDEC_COLOR_COMBINE (0, 0, 0, CLAMP ((alpha), 0, 100) * 255 / 100))

SWFDEC_AS_NATIVE (901, 1, swfdec_sprite_movie_beginFill)
void
swfdec_sprite_movie_beginFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecDraw *draw;
  int color, alpha = 100;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "|ii", &color, &alpha);
  movie->draw_fill = NULL;
  
  if (argc == 0 || SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0])) {
    color = 0;
  } else {
    color = color & 0xFFFFFF;
    color = SWFDEC_COLOR_FROM_COLOR_ALPHA (color, alpha);
  }
  draw = SWFDEC_DRAW (swfdec_pattern_new_color (color));
  swfdec_path_move_to (&draw->path, movie->draw_x, movie->draw_y);
  swfdec_sprite_movie_end_fill (movie, draw);
}

static guint
swfdec_sprite_movie_gradient_fill_get_length (SwfdecAsObject *o)
{
  int length;
  SwfdecAsValue val;

  swfdec_as_object_get_variable (o, SWFDEC_AS_STR_length, &val);
  length = swfdec_as_value_to_integer (o->context, &val);
  return MAX (length, 0);
}

static int
swfdec_sprite_movie_gradient_fill_check_length (SwfdecAsObject *colors, SwfdecAsObject *alphas, SwfdecAsObject *ratios)
{
  guint c, a, r;

  c = swfdec_sprite_movie_gradient_fill_get_length (colors);
  a = swfdec_sprite_movie_gradient_fill_get_length (alphas);
  r = swfdec_sprite_movie_gradient_fill_get_length (ratios);
  if (c != a || a != r)
    return -1;
  return c;
}

static void
swfdec_sprite_movie_extract_matrix (SwfdecAsObject *o, cairo_matrix_t *mat)
{
  SwfdecAsContext *cx = o->context;
  SwfdecAsValue val;

  /* FIXME: This function does not call valueOf in the right order */
  if (swfdec_as_object_get_variable (o, SWFDEC_AS_STR_matrixType, &val)) {
    const char *s = swfdec_as_value_to_string (cx, val);
    cairo_matrix_init_translate (mat, SWFDEC_TWIPS_SCALE_FACTOR / 2.0, SWFDEC_TWIPS_SCALE_FACTOR / 2.0);
    cairo_matrix_scale (mat, SWFDEC_TWIPS_SCALE_FACTOR / 32768.0, SWFDEC_TWIPS_SCALE_FACTOR / 32768.0);
    if (s == SWFDEC_AS_STR_box) {
      double x, y, w, h, r;
      cairo_matrix_t input;
      swfdec_as_object_get_variable (o, SWFDEC_AS_STR_x, &val);
      x = swfdec_as_value_to_number (cx, &val);
      swfdec_as_object_get_variable (o, SWFDEC_AS_STR_y, &val);
      y = swfdec_as_value_to_number (cx, &val);
      swfdec_as_object_get_variable (o, SWFDEC_AS_STR_w, &val);
      w = swfdec_as_value_to_number (cx, &val);
      swfdec_as_object_get_variable (o, SWFDEC_AS_STR_h, &val);
      h = swfdec_as_value_to_number (cx, &val);
      swfdec_as_object_get_variable (o, SWFDEC_AS_STR_r, &val);
      r = swfdec_as_value_to_number (cx, &val);
      cairo_matrix_init_translate (&input, (x + w) / 2, (y + h) / 2);
      cairo_matrix_scale (&input, w, h);
      cairo_matrix_rotate (&input, r);
      cairo_matrix_multiply (mat, mat, &input);
    } else {
      SWFDEC_WARNING ("my friend, there's no other matrixType than \"box\"");
    }
  } else if (cx->version >= 8 && swfdec_matrix_from_as_object (mat, o)) {
    mat->x0 *= SWFDEC_TWIPS_SCALE_FACTOR;
    mat->y0 *= SWFDEC_TWIPS_SCALE_FACTOR;
  } else {
    cairo_matrix_t input;
    swfdec_as_object_get_variable (o, SWFDEC_AS_STR_a, &val);
    input.xx = swfdec_as_value_to_number (cx, &val);
    swfdec_as_object_get_variable (o, SWFDEC_AS_STR_b, &val);
    input.yx = swfdec_as_value_to_number (cx, &val);
    swfdec_as_object_get_variable (o, SWFDEC_AS_STR_d, &val);
    input.xy = swfdec_as_value_to_number (cx, &val);
    swfdec_as_object_get_variable (o, SWFDEC_AS_STR_e, &val);
    input.yy = swfdec_as_value_to_number (cx, &val);
    swfdec_as_object_get_variable (o, SWFDEC_AS_STR_g, &val);
    input.x0 = swfdec_as_value_to_number (cx, &val) * SWFDEC_TWIPS_SCALE_FACTOR;
    swfdec_as_object_get_variable (o, SWFDEC_AS_STR_h, &val);
    input.y0 = swfdec_as_value_to_number (cx, &val) * SWFDEC_TWIPS_SCALE_FACTOR;
    cairo_matrix_init_scale (mat, SWFDEC_TWIPS_SCALE_FACTOR / 32768.0, SWFDEC_TWIPS_SCALE_FACTOR / 32768.0);
    cairo_matrix_multiply (mat, mat, &input);
  }
}

SWFDEC_AS_NATIVE (901, 2, swfdec_sprite_movie_beginGradientFill)
void
swfdec_sprite_movie_beginGradientFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecGradientPattern *gradient;
  SwfdecPattern *pattern;
  SwfdecMovie *movie;
  SwfdecDraw *draw;
  SwfdecAsObject *colors, *alphas, *ratios, *matrix;
  const char *s;
  gboolean radial;
  int i, len;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "soooo", &s, &colors, &alphas, &ratios, &matrix);
  movie->draw_fill = NULL;
  
  if (s == SWFDEC_AS_STR_linear) {
    radial = FALSE;
  } else if (s == SWFDEC_AS_STR_radial) {
    radial = TRUE;
  } else {
    SWFDEC_WARNING ("invalid fill type %s", s);
    return;
  }
  len = swfdec_sprite_movie_gradient_fill_check_length (colors, alphas, ratios);
  if (len < 0) {
    SWFDEC_ERROR ("different lengths for colors, alphas and ratios, aborting");
    return;
  }
  draw = swfdec_gradient_pattern_new ();
  pattern = SWFDEC_PATTERN (draw);
  gradient = SWFDEC_GRADIENT_PATTERN (draw);
  gradient->radial = radial;
  len = MIN (len, 8);
  gradient->n_gradients = len;
  for (i = 0; i < len; i++) {
    int c, a, r;
    SwfdecAsValue v;
    int check = swfdec_sprite_movie_gradient_fill_check_length (colors, alphas, ratios);
    if (check > i) {
      const char *name = swfdec_as_integer_to_string (cx, i);
      if (swfdec_as_object_get_variable (colors, name, &v)
	  && SWFDEC_AS_VALUE_IS_NUMBER (&v))
	c = swfdec_as_value_to_integer (cx, &v);
      else
	c = 0;
      if (!swfdec_as_object_get_variable (alphas, name, &v)) {
	a = c;
      } else if (!SWFDEC_AS_VALUE_IS_NUMBER (&v)) {
	a = 0;
      } else {
	a = swfdec_as_value_to_integer (cx, &v);
      }
      if (!swfdec_as_object_get_variable (ratios, name, &v))
	r = CLAMP (a, 0, 255);
      else if (!SWFDEC_AS_VALUE_IS_NUMBER (&v))
	r = 0;
      else
	r = swfdec_as_value_to_integer (cx, &v);
    } else {
      c = a = r = 0;
    }
    if (r > 255 || r < 0) {
      SWFDEC_WARNING ("ratio %d not in [0, 255], ignoring gradient", r);
      g_object_unref (draw);
      return;
    } else if (r < 0) {
      r = 0;
    }
    gradient->gradient[i].color = SWFDEC_COLOR_FROM_COLOR_ALPHA (c, a);
    gradient->gradient[i].ratio = r;
  }
  swfdec_sprite_movie_extract_matrix (matrix, &pattern->start_transform);
  pattern->transform = pattern->start_transform;
  if (cairo_matrix_invert (&pattern->transform)) {
    SWFDEC_ERROR ("gradient transform matrix not invertible, resetting");
    cairo_matrix_init_identity (&pattern->transform);
  }

  swfdec_path_move_to (&draw->path, movie->draw_x, movie->draw_y);
  swfdec_sprite_movie_end_fill (movie, draw);
}

SWFDEC_AS_NATIVE (901, 3, swfdec_sprite_movie_moveTo)
void
swfdec_sprite_movie_moveTo (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  double x = 0, y = 0;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "|nn", &x, &y);

  x = SWFDEC_DOUBLE_TO_TWIPS (x);
  y = SWFDEC_DOUBLE_TO_TWIPS (y);
  /* NB: moves do not extend extents */
  if (movie->draw_fill) {
    swfdec_path_move_to (&movie->draw_fill->path, x, y);
  }
  if (movie->draw_line) {
    swfdec_path_move_to (&movie->draw_line->path, x, y);
  }
  movie->draw_x = x;
  movie->draw_y = y;
}

static void
swfdec_spite_movie_recompute_draw (SwfdecMovie *movie, SwfdecDraw *draw)
{
  swfdec_draw_recompute (draw);
  if (swfdec_rect_inside (&movie->draw_extents, &draw->extents)) {
    swfdec_movie_invalidate_last (movie);
  } else {
    swfdec_movie_invalidate_next (movie);
    swfdec_rect_union (&movie->draw_extents, &movie->draw_extents, &draw->extents);
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);
  }
}

SWFDEC_AS_NATIVE (901, 4, swfdec_sprite_movie_lineTo)
void
swfdec_sprite_movie_lineTo (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  double x, y;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "nn", &x, &y);

  x = SWFDEC_DOUBLE_TO_TWIPS (x);
  y = SWFDEC_DOUBLE_TO_TWIPS (y);
  if (movie->draw_fill) {
    swfdec_path_line_to (&movie->draw_fill->path, x, y);
    swfdec_spite_movie_recompute_draw (movie, movie->draw_fill);
  }
  if (movie->draw_line) {
    swfdec_path_line_to (&movie->draw_line->path, x, y);
    swfdec_spite_movie_recompute_draw (movie, movie->draw_line);
  }
  movie->draw_x = x;
  movie->draw_y = y;
}

SWFDEC_AS_NATIVE (901, 5, swfdec_sprite_movie_curveTo)
void
swfdec_sprite_movie_curveTo (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  double x, y, c_x, c_y;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "nnnn", &c_x, &c_y, &x, &y);

  x = SWFDEC_DOUBLE_TO_TWIPS (x);
  y = SWFDEC_DOUBLE_TO_TWIPS (y);
  c_x = SWFDEC_DOUBLE_TO_TWIPS (c_x);
  c_y = SWFDEC_DOUBLE_TO_TWIPS (c_y);
  if (movie->draw_fill) {
    swfdec_path_curve_to (&movie->draw_fill->path, movie->draw_x, movie->draw_y,
	c_x, c_y, x, y);
    swfdec_spite_movie_recompute_draw (movie, movie->draw_fill);
  }
  if (movie->draw_line) {
    swfdec_path_curve_to (&movie->draw_line->path, movie->draw_x, movie->draw_y,
	c_x, c_y, x, y);
    swfdec_spite_movie_recompute_draw (movie, movie->draw_line);
  }
  movie->draw_x = x;
  movie->draw_y = y;
}

SWFDEC_AS_NATIVE (901, 6, swfdec_sprite_movie_lineStyle)
void
swfdec_sprite_movie_lineStyle (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecStroke *stroke;
  int width, color = 0, alpha = 100;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "i|ii", &width, &color, &alpha);

  movie->draw_line = NULL;
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;
  if (argc > 3) {
    SWFDEC_FIXME ("implement Flash 8 arguments to lineStyle");
  }
  color = color & 0xFFFFFF;
  alpha = CLAMP (alpha, 0, 100);
  alpha = SWFDEC_COLOR_COMBINE (0, 0, 0, alpha * 255 / 100);
  color = color | alpha;
  stroke = g_object_new (SWFDEC_TYPE_STROKE, NULL);
  stroke->start_color = color;
  stroke->start_width = SWFDEC_DOUBLE_TO_TWIPS (width);
  movie->draw_line = SWFDEC_DRAW (stroke);
  swfdec_path_move_to (&movie->draw_line->path, movie->draw_x, movie->draw_y);
  movie->draws = g_slist_append (movie->draws, movie->draw_line);
}

SWFDEC_AS_NATIVE (901, 7, swfdec_sprite_movie_endFill)
void
swfdec_sprite_movie_endFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "");
  swfdec_sprite_movie_end_fill (movie, NULL);
}

SWFDEC_AS_NATIVE (901, 8, swfdec_sprite_movie_clear)
void
swfdec_sprite_movie_clear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "");
  if (movie->draws == NULL)
    return;
  swfdec_movie_invalidate_last (movie);
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_EXTENTS);
  swfdec_rect_init_empty (&movie->draw_extents);
  g_slist_foreach (movie->draws, (GFunc) g_object_unref, NULL);
  g_slist_free (movie->draws);
  movie->draws = NULL;
  movie->draw_fill = NULL;
  movie->draw_line = NULL;
}

SWFDEC_AS_NATIVE (901, 9, swfdec_sprite_movie_lineGradientStyle)
void
swfdec_sprite_movie_lineGradientStyle (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.lineGradientStyle");
}

SWFDEC_AS_NATIVE (901, 10, swfdec_sprite_movie_beginMeshFill)
void
swfdec_sprite_movie_beginMeshFill (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.beginMeshFill");
}

SWFDEC_AS_NATIVE (901, 11, swfdec_sprite_movie_beginBitmapFill)
void
swfdec_sprite_movie_beginBitmapFill (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecAsObject *bitmap;
  SwfdecPattern *pattern;
  SwfdecDraw *draw;
  SwfdecAsObject *mat = NULL;
  gboolean repeat = TRUE;
  gboolean smoothing = FALSE;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "O|Obb", 
      &bitmap, &mat, &repeat, &smoothing);
  movie->draw_fill = NULL;
  if (!SWFDEC_IS_BITMAP_DATA (bitmap->relay))
    return;
  
  pattern = swfdec_bitmap_pattern_new (SWFDEC_BITMAP_DATA (bitmap->relay));
  /* NB: This signal assumes that the pattern is destroyed before the movie is,
   * because it is never removed anywhere */
  g_signal_connect_swapped (pattern, "invalidate", G_CALLBACK (swfdec_movie_invalidate_last), movie);

  if (mat != NULL && !swfdec_matrix_from_as_object (&pattern->start_transform, mat))
    cairo_matrix_init_identity (&pattern->start_transform);
  cairo_matrix_scale (&pattern->start_transform, SWFDEC_TWIPS_SCALE_FACTOR, SWFDEC_TWIPS_SCALE_FACTOR);
  pattern->start_transform.x0 *= SWFDEC_TWIPS_SCALE_FACTOR;
  pattern->start_transform.y0 *= SWFDEC_TWIPS_SCALE_FACTOR;
  pattern->transform = pattern->start_transform;
  if (cairo_matrix_invert (&pattern->transform) != CAIRO_STATUS_SUCCESS) {
    SWFDEC_ERROR ("non-invertible matrix used for transform");
    cairo_matrix_init_scale (&pattern->transform, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR,
	1.0 / SWFDEC_TWIPS_SCALE_FACTOR);
  }
  /* FIXME: or use FAST/GOOD? */
  SWFDEC_BITMAP_PATTERN (pattern)->filter = smoothing ? CAIRO_FILTER_BILINEAR : CAIRO_FILTER_NEAREST;
  SWFDEC_BITMAP_PATTERN (pattern)->extend = repeat ? CAIRO_EXTEND_REPEAT : CAIRO_EXTEND_PAD;

  draw = SWFDEC_DRAW (pattern);
  swfdec_path_move_to (&draw->path, movie->draw_x, movie->draw_y);
  swfdec_sprite_movie_end_fill (movie, draw);
}

SWFDEC_AS_NATIVE (901, 12, swfdec_sprite_movie_get_scale9Grid)
void
swfdec_sprite_movie_get_scale9Grid (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.scale9Grid (get)");
}

SWFDEC_AS_NATIVE (901, 13, swfdec_sprite_movie_set_scale9Grid)
void
swfdec_sprite_movie_set_scale9Grid (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.scale9Grid (set)");
}
