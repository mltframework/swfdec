/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_as_internal.h"
#include "swfdec_path.h"
#include "swfdec_pattern.h"
#include "swfdec_stroke.h"

/* FIXME: This whole code assumes it works for MovieClip, Button and TextField
 * objects. If it only works for MovieClip objects, fix this. */

SWFDEC_AS_NATIVE (901, 1, swfdec_sprite_movie_beginFill)
void
swfdec_sprite_movie_beginFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  int color, alpha;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "|ii", &color, &alpha);
  movie->draw_fill = NULL;
  
  if (argc == 0)
    return;
  color = color & 0xFFFFFF;
  if (argc > 1) {
    alpha = CLAMP (alpha, 0, 100);
    alpha = SWFDEC_COLOR_COMBINE (0, 0, 0, alpha * 255 / 100);
  } else {
    alpha = SWFDEC_COLOR_COMBINE (0, 0, 0, 255);
  }
  color = color | alpha;
  movie->draw_fill = SWFDEC_DRAW (swfdec_pattern_new_color (color));
  swfdec_path_move_to (&movie->draw_fill->path, movie->draw_x, movie->draw_y);
  movie->draws = g_slist_prepend (movie->draws, movie->draw_fill);
}

SWFDEC_AS_NATIVE (901, 2, swfdec_sprite_movie_beginGradientFill)
void
swfdec_sprite_movie_beginGradientFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_FIXME ("implement");
}

SWFDEC_AS_NATIVE (901, 3, swfdec_sprite_movie_moveTo)
void
swfdec_sprite_movie_moveTo (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  int x, y;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "|ii", &x, &y);

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
    swfdec_movie_invalidate (movie);
  } else {
    swfdec_rect_union (&movie->draw_extents, &movie->draw_extents, &draw->extents);
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_CONTENTS);
  }
}

SWFDEC_AS_NATIVE (901, 4, swfdec_sprite_movie_lineTo)
void
swfdec_sprite_movie_lineTo (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  int x, y;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "ii", &x, &y);

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
  int x, y, c_x, c_y;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "iiii", &c_x, &c_y, &x, &y);

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
  int width, color, alpha;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "|iii", &width, &color, &alpha);

  movie->draw_line = NULL;
  if (argc < 1 && SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;
  if (argc < 3)
    alpha = 100;
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
  movie->draws = g_slist_prepend (movie->draws, movie->draw_line);
}

SWFDEC_AS_NATIVE (901, 7, swfdec_sprite_movie_endFill)
void
swfdec_sprite_movie_endFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "");
  /* FIXME: need to cairo_close_path()? */
  movie->draw_fill = NULL;
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
  swfdec_movie_invalidate (movie);
  swfdec_rect_init_empty (&movie->draw_extents);
  g_slist_foreach (movie->draws, (GFunc) g_object_unref, NULL);
  g_slist_free (movie->draws);
  movie->draws = NULL;
  movie->draw_fill = NULL;
  movie->draw_line = NULL;
}

