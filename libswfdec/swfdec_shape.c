/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include <math.h>
#include <string.h>

#include "swfdec_shape.h"
#include "swfdec.h"
#include "swfdec_debug.h"
#include "swfdec_stroke.h"

G_DEFINE_TYPE (SwfdecShape, swfdec_shape, SWFDEC_TYPE_GRAPHIC)

/*** PATHS ***/

static void
swfdec_path_init (cairo_path_t *path)
{
  path->status = CAIRO_STATUS_SUCCESS;
  path->data = NULL;
  path->num_data = 0;
}

static void
swfdec_path_reset (cairo_path_t *path)
{
  path->status = CAIRO_STATUS_SUCCESS;
  g_free (path->data);
  path->data = NULL;
  path->num_data = 0;
}

#define swfdec_path_require_size(path, steps) \
  swfdec_path_ensure_size ((path), (path)->num_data + steps)
static void
swfdec_path_ensure_size (cairo_path_t *path, int size)
{
#define SWFDEC_PATH_STEPS 32
  /* round up to next multiple of SWFDEC_PATH_STEPS */
  int current_size = path->num_data - path->num_data % SWFDEC_PATH_STEPS;
  if (path->num_data % SWFDEC_PATH_STEPS)
    current_size += SWFDEC_PATH_STEPS;

  if (size % SWFDEC_PATH_STEPS)
    size += SWFDEC_PATH_STEPS - size % SWFDEC_PATH_STEPS;
  g_assert (current_size % SWFDEC_PATH_STEPS == 0);
  g_assert (size % SWFDEC_PATH_STEPS == 0);
  while (size <= current_size)
    return;
  SWFDEC_LOG ("extending size of %p from %u to %u", path, current_size, size);
  path->data = g_renew (cairo_path_data_t, path->data, size);
}

static void
swfdec_path_move_to (cairo_path_t *path, double x, double y)
{
  cairo_path_data_t *cur;

  swfdec_path_require_size (path, 2);
  cur = &path->data[path->num_data++];
  cur->header.type = CAIRO_PATH_MOVE_TO;
  cur->header.length = 2;
  cur = &path->data[path->num_data++];
  cur->point.x = x;
  cur->point.y = y;
}

static void
swfdec_path_line_to (cairo_path_t *path, double x, double y)
{
  cairo_path_data_t *cur;

  swfdec_path_require_size (path, 2);
  cur = &path->data[path->num_data++];
  cur->header.type = CAIRO_PATH_LINE_TO;
  cur->header.length = 2;
  cur = &path->data[path->num_data++];
  cur->point.x = x;
  cur->point.y = y;
}

static void
swfdec_path_curve_to (cairo_path_t *path, double start_x, double start_y,
    double control_x, double control_y, double end_x, double end_y)
{
  cairo_path_data_t *cur;

  swfdec_path_require_size (path, 4);
  cur = &path->data[path->num_data++];
  cur->header.type = CAIRO_PATH_CURVE_TO;
  cur->header.length = 4;
#define WEIGHT (2.0/3.0)
  cur = &path->data[path->num_data++];
  cur->point.x = control_x * WEIGHT + (1-WEIGHT) * start_x;
  cur->point.y = control_y * WEIGHT + (1-WEIGHT) * start_y;
  cur = &path->data[path->num_data++];
  cur->point.x = control_x * WEIGHT + (1-WEIGHT) * end_x;
  cur->point.y = control_y * WEIGHT + (1-WEIGHT) * end_y;
  cur = &path->data[path->num_data++];
  cur->point.x = end_x;
  cur->point.y = end_y;
}

static void
swfdec_path_append (cairo_path_t *path, const cairo_path_t *append)
{
  swfdec_path_require_size (path, append->num_data);
  memcpy (&path->data[path->num_data], append->data, sizeof (cairo_path_data_t) * append->num_data);
  path->num_data += append->num_data;
}

static void
swfdec_path_append_reverse (cairo_path_t *path, const cairo_path_t *append,
    double x, double y)
{
  cairo_path_data_t *out, *in;
  int i;

  swfdec_path_require_size (path, append->num_data);
  path->num_data += append->num_data;
  out = &path->data[path->num_data - 1];
  in = append->data;
  for (i = 0; i < append->num_data; i++) {
    switch (in[i].header.type) {
      case CAIRO_PATH_LINE_TO:
	out[-i].point.x = x;
	out[-i].point.y = y;
	out[-i - 1].header = in[i].header;
	i++;
	break;
      case CAIRO_PATH_CURVE_TO:
	out[-i].point.x = x;
	out[-i].point.y = y;
	out[-i - 3].header = in[i].header;
	out[-i - 1].point = in[i + 1].point;
	out[-i - 2].point = in[i + 2].point;
	i += 3;
	break;
      case CAIRO_PATH_CLOSE_PATH:
      case CAIRO_PATH_MOVE_TO:
	/* these two don't exist in our code */
      default:
	g_assert_not_reached ();
    }
    x = in[i].point.x;
    y = in[i].point.y;
  }
}

/*** SUBPATH ***/

typedef struct {
  int			x_start, y_start;
  int			x_end, y_end;
  cairo_path_t		path;
  guint			fill0style;
  guint			fill1style;
  guint			linestyle;
  guint			max_index;
} SubPath;

/*** SHAPE ***/

static void
swfdec_shape_vec_finish (SwfdecShapeVec * shapevec)
{
  if (shapevec->pattern) {
    g_object_unref (shapevec->pattern);
    shapevec->pattern = NULL;
  }
  if (shapevec->fill_cr) {
    cairo_destroy (shapevec->fill_cr);
    shapevec->fill_cr = NULL;
  }

  swfdec_path_reset (&shapevec->path);
}

static void
swfdec_shape_vec_init (SwfdecShapeVec *vec)
{
  swfdec_path_init (&vec->path);
}

static void
swfdec_shape_dispose (GObject *object)
{
  guint i;
  SwfdecShape * shape = SWFDEC_SHAPE (object);

  for (i = 0; i < shape->vecs->len; i++) {
    swfdec_shape_vec_finish (&g_array_index (shape->vecs, SwfdecShapeVec, i));
  }
  g_array_free (shape->vecs, TRUE);
  for (i = 0; i < shape->fills->len; i++) {
    if (g_ptr_array_index (shape->fills, i))
      g_object_unref (g_ptr_array_index (shape->fills, i));
  }
  g_ptr_array_free (shape->fills, TRUE);

  for (i = 0; i < shape->lines->len; i++) {
    if (g_ptr_array_index (shape->lines, i))
      g_object_unref (g_ptr_array_index (shape->lines, i));
  }
  g_ptr_array_free (shape->lines, TRUE);

  G_OBJECT_CLASS (swfdec_shape_parent_class)->dispose (G_OBJECT (shape));
}

static void
swfdec_shape_render (SwfdecGraphic *graphic, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecShape *shape = SWFDEC_SHAPE (graphic);
  guint i;

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

  for (i = 0; i < shape->vecs->len; i++) {
    SwfdecShapeVec *vec = &g_array_index (shape->vecs, SwfdecShapeVec, i);

    g_assert (vec->path.num_data);
    g_assert (vec->pattern);
    if (!swfdec_rect_intersect (NULL, &vec->extents, inval))
      continue;
    
    /* hack to not append paths for lines */
    if (!fill && vec->last_index % 2 != 0) 
      continue;

    if (fill) {
      if (SWFDEC_IS_PATTERN (vec->pattern)) {
	swfdec_pattern_paint (vec->pattern, cr, &vec->path, trans, 0);
      } else {
	swfdec_stroke_paint (vec->pattern, cr, &vec->path, trans, 0);
      }
    } else {
      cairo_append_path (cr, &vec->path);
    }
  }
}

static gboolean
swfdec_shape_mouse_in (SwfdecGraphic *graphic, double x, double y)
{
  SwfdecShapeVec *shapevec;
  SwfdecShape *shape = SWFDEC_SHAPE (graphic);
  static cairo_surface_t *surface = NULL;
  guint i;

  for (i = 0; i < shape->vecs->len; i++) {
    shapevec = &g_array_index (shape->vecs, SwfdecShapeVec, i);
  
    g_assert (shapevec->path.num_data);
    g_assert (shapevec->pattern);
    /* FIXME: handle strokes */
    if (SWFDEC_IS_STROKE (shapevec->pattern))
      continue;
    if (shapevec->fill_cr == NULL) {
      /* FIXME: do less memory intensive fill checking plz */
      if (surface == NULL)
	surface = cairo_image_surface_create (CAIRO_FORMAT_A8, 1, 1);
      shapevec->fill_cr = cairo_create (surface);
      cairo_set_fill_rule (shapevec->fill_cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_append_path (shapevec->fill_cr, &shapevec->path);
    }
    if (cairo_in_fill (shapevec->fill_cr, x, y))
      return TRUE;
  }
  return FALSE;
}

static void
swfdec_shape_class_init (SwfdecShapeClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);
  
  object_class->dispose = swfdec_shape_dispose;

  graphic_class->render = swfdec_shape_render;
  graphic_class->mouse_in = swfdec_shape_mouse_in;
}

static void
swfdec_shape_init (SwfdecShape * shape)
{
  shape->fills = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();
  shape->vecs = g_array_new (FALSE, TRUE, sizeof (SwfdecShapeVec));
}

static void
swfdec_shape_add_styles (SwfdecSwfDecoder * s, SwfdecShape * shape,
    SwfdecPatternFunc parse_fill, SwfdecStrokeFunc parse_stroke)
{
  int n_fill_styles;
  int n_line_styles;
  int i;
  SwfdecBits *bits = &s->b;

  swfdec_bits_syncbits (bits);
  shape->fills_offset = shape->fills->len;
  n_fill_styles = swfdec_bits_get_u8 (bits);
  if (n_fill_styles == 0xff) {
    n_fill_styles = swfdec_bits_get_u16 (bits);
  }
  SWFDEC_LOG ("   n_fill_styles %d", n_fill_styles);
  for (i = 0; i < n_fill_styles; i++) {
    SwfdecPattern *pattern;

    SWFDEC_LOG ("   fill style %d:", i);

    pattern = parse_fill (s);
    g_ptr_array_add (shape->fills, pattern);
  }

  swfdec_bits_syncbits (bits);
  shape->lines_offset = shape->lines->len;
  n_line_styles = swfdec_bits_get_u8 (bits);
  if (n_line_styles == 0xff) {
    n_line_styles = swfdec_bits_get_u16 (bits);
  }
  SWFDEC_LOG ("   n_line_styles %d", n_line_styles);
  for (i = 0; i < n_line_styles; i++) {
    g_ptr_array_add (shape->lines, parse_stroke (s));
  }

  swfdec_bits_syncbits (bits);
  shape->n_fill_bits = swfdec_bits_getbits (bits, 4);
  shape->n_line_bits = swfdec_bits_getbits (bits, 4);
}

int
tag_define_shape (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int id;

  id = swfdec_bits_get_u16 (bits);

  shape = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_SHAPE);
  if (!shape)
    return SWFDEC_STATUS_OK;

  SWFDEC_INFO ("id=%d", id);

  swfdec_bits_get_rect (bits, &SWFDEC_GRAPHIC (shape)->extents);

  swfdec_shape_add_styles (s, shape, swfdec_pattern_parse, swfdec_stroke_parse);

  swfdec_shape_get_recs (s, shape, swfdec_pattern_parse, swfdec_stroke_parse);

  return SWFDEC_STATUS_OK;
}

int
tag_define_shape_3 (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int id;

  id = swfdec_bits_get_u16 (bits);
  shape = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_SHAPE);
  if (!shape)
    return SWFDEC_STATUS_OK;

  swfdec_bits_get_rect (bits, &SWFDEC_GRAPHIC (shape)->extents);

  swfdec_shape_add_styles (s, shape, swfdec_pattern_parse_rgba, swfdec_stroke_parse_rgba);

  swfdec_shape_get_recs (s, shape, swfdec_pattern_parse_rgba, swfdec_stroke_parse_rgba);

  return SWFDEC_STATUS_OK;
}

int
tag_define_shape_2 (SwfdecSwfDecoder * s)
{
  return tag_define_shape (s);
}

int
tag_define_shape_4 (SwfdecSwfDecoder *s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int id;
  SwfdecRect tmp;
  gboolean has_scale_strokes, has_noscale_strokes;

  id = swfdec_bits_get_u16 (bits);
  shape = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_SHAPE);
  if (!shape)
    return SWFDEC_STATUS_OK;

  swfdec_bits_get_rect (bits, &SWFDEC_GRAPHIC (shape)->extents);
  SWFDEC_LOG ("  extents: %g %g x %g %g", 
      SWFDEC_GRAPHIC (shape)->extents.x0, SWFDEC_GRAPHIC (shape)->extents.y0,
      SWFDEC_GRAPHIC (shape)->extents.x1, SWFDEC_GRAPHIC (shape)->extents.y1);
  swfdec_bits_get_rect (bits, &tmp);
  SWFDEC_LOG ("  extents: %g %g x %g %g", 
      tmp.x0, tmp.y0, tmp.x1, tmp.y1);
  swfdec_bits_getbits (bits, 6);
  has_scale_strokes = swfdec_bits_getbit (bits);
  has_noscale_strokes = swfdec_bits_getbit (bits);
  SWFDEC_LOG ("  has scaling strokes: %d", has_scale_strokes);
  SWFDEC_LOG ("  has non-scaling strokes: %d", has_noscale_strokes);

  swfdec_shape_add_styles (s, shape, swfdec_pattern_parse_rgba, swfdec_stroke_parse_extended);

  swfdec_shape_get_recs (s, shape, swfdec_pattern_parse_rgba, swfdec_stroke_parse_extended);

  return SWFDEC_STATUS_OK;
}

/* The shape creation process is a bit complicated since it requires matching 
 * the Flash model to the cairo model. Note that this code is just random 
 * guessing and nothing substantial. Otherwise I'd have a testsuite. :)
 * It does the following steps:
 * - Accumulate all sub path into an array of SubPath structs. a sub path is 
 *   the path between 2 style change records.
 * - For every fill style used, accumulate all paths in order of appearance. 
 *   The code assumes that every fill path is closed.
 * - Collect the lines.
 * - Sort the array so that lines get drawn after their left and right fill, 
 *   but as early as possible. SubPath.max_index and ShapeVec.last_index are 
 *   used for this.
 */
/* FIXME: It might be that the Flash code uses a different order, namely 
 * drawing all fills followed by all lines per style record. So everything after
 * new styles appeared is drawn later.
 * The current code works though, so I'm not interested in testing that idea ;)
 */

static GSList *
swfdec_shape_accumulate_one_polygon (SwfdecShape *shape, SwfdecShapeVec *target,
    guint style, SubPath *paths, guint start, guint paths_len, guint *max)
{
  guint i;
  GSList *found = NULL;
  int x, y;

  g_assert (style != 0);
  /* paths[start] is our starting point */
  if (paths[start].fill0style == style) {
    paths[start].fill0style = 0;
  } else {
    g_assert (style == paths[start].fill1style);
    paths[start].fill1style = 0;
  }
  x = paths[start].x_end;
  y = paths[start].y_end;
  found = g_slist_prepend (found, &paths[start]);
  *max = start;
  swfdec_path_move_to (&target->path, paths[start].x_start, paths[start].y_start);
  swfdec_path_append (&target->path, &paths[start].path);
  while (x != paths[start].x_start || y != paths[start].y_start) {
    SWFDEC_LOG ("now looking for point %u %u", x, y);
    for (i = start; i < paths_len; i++) {
      if (paths[i].fill0style != style && paths[i].fill1style != style)
	continue;
      if (paths[i].x_start == x && paths[i].y_start == y) {
	SWFDEC_LOG ("  found it %u", i);
	x = paths[i].x_end;
	y = paths[i].y_end;
	swfdec_path_append (&target->path, &paths[i].path);
      } else if (paths[i].x_end == x && paths[i].y_end == y) {
	SWFDEC_LOG ("  found it reverse %u", i);
	x = paths[i].x_start;
	y = paths[i].y_start;
	swfdec_path_append_reverse (&target->path, &paths[i].path, x, y);
      } else {
	continue;
      }
      if (paths[i].fill0style == style)
	paths[i].fill0style = 0;
      else
	paths[i].fill1style = 0;
      found = g_slist_prepend (found, &paths[i]);
      *max = i;
      break;
    }
    if (i == paths_len) {
      SWFDEC_ERROR ("could not find a closed path for style %u, starting at %d %d", style,
	  paths[start].x_start, paths[start].y_start);
      goto fail;
    }
  }
  return found;

fail:
  g_slist_free (found);
  return NULL;
}

static void
swfdec_shape_accumulate_one_fill (SwfdecShape *shape, SubPath *paths, 
    guint start, guint paths_len)
{
  SwfdecShapeVec *target;
  guint i, style, max = 0, cur;
  GSList *walk, *found = NULL;

  g_array_set_size (shape->vecs, shape->vecs->len + 1);
  target = &g_array_index (shape->vecs, SwfdecShapeVec, shape->vecs->len - 1);
  swfdec_shape_vec_init (target);
  if (paths[start].fill0style != 0)
    style = paths[start].fill0style;
  else
    style = paths[start].fill1style;

  for (i = start; i < paths_len; i++) {
    if (paths[i].fill0style == style) {
      walk = swfdec_shape_accumulate_one_polygon (shape, target, style, 
	  paths, i, paths_len, &cur);
      if (walk) {
	found = g_slist_concat (found, walk);
	max = MAX (max, cur);
      } else {
	goto fail;
      }
    }
    if (paths[i].fill1style == style) {
      walk = swfdec_shape_accumulate_one_polygon (shape, target, style, 
	  paths, i, paths_len, &cur);
      if (walk) {
	found = g_slist_concat (found, walk);
	max = MAX (max, cur);
      } else {
	goto fail;
      }
    }
  }
  target->last_index = 2 * max;
  for (walk = found; walk; walk = walk->next) {
    SubPath *sub = walk->data;
    sub->max_index = MAX (sub->max_index, max);
  }
  if (style > shape->fills->len) {
    SWFDEC_ERROR ("style index %u is too big, only %u styles available", style,
	shape->fills->len);
    goto fail;
  } else {
    target->pattern = g_ptr_array_index (shape->fills, style - 1);
    if (target->pattern == NULL)
      goto fail;
    g_object_ref (target->pattern);
  }
  g_slist_free (found);
  return;

fail:
  g_slist_free (found);
  swfdec_shape_vec_finish (target);
  g_array_set_size (shape->vecs, shape->vecs->len - 1);
}

static void
swfdec_shape_accumulate_fills (SwfdecShape *shape, SubPath *paths, guint paths_len)
{
  guint i;

  for (i = 0; i < paths_len; i++) {
    if (paths[i].fill0style != 0)
      swfdec_shape_accumulate_one_fill (shape, paths, i, paths_len);
    if (paths[i].fill1style != 0)
      swfdec_shape_accumulate_one_fill (shape, paths, i, paths_len);
  }
}

static void
swfdec_shape_accumulate_lines (SwfdecShape *shape, SubPath *paths, guint paths_len)
{
  SwfdecShapeVec target = { 0, };
  guint i;

  for (i = 0; i < paths_len; i++) {
    if (paths[i].linestyle == 0)
      continue;
    if (paths[i].linestyle > shape->lines->len) {
      SWFDEC_ERROR ("linestyle index %u is too big, only %u styles available",
	  paths[i].linestyle, shape->lines->len);
      continue;
    }

    swfdec_shape_vec_init (&target);
    swfdec_path_move_to (&target.path, paths[i].x_start, paths[i].y_start);
    swfdec_path_append (&target.path, &paths[i].path);
    target.pattern = g_ptr_array_index (shape->lines, paths[i].linestyle - 1);
    g_object_ref (target.pattern);
    target.last_index = 2 * paths[i].max_index + 1;
    g_array_append_val (shape->vecs, target);
  }
}

static int
swfdec_shape_vec_compare (gconstpointer a, gconstpointer b)
{
  return ((const SwfdecShapeVec *) a)->last_index - ((const SwfdecShapeVec *) b)->last_index;
}

typedef enum {
  SWFDEC_SHAPE_TYPE_END,
  SWFDEC_SHAPE_TYPE_CHANGE,
  SWFDEC_SHAPE_TYPE_LINE,
  SWFDEC_SHAPE_TYPE_CURVE
} SwfdecShapeType;

static SwfdecShapeType
swfdec_shape_peek_type (SwfdecBits *bits)
{
  guint ret = swfdec_bits_peekbits (bits, 6);

  if (ret == 0)
    return SWFDEC_SHAPE_TYPE_END;
  if ((ret & 0x20) == 0)
    return SWFDEC_SHAPE_TYPE_CHANGE;
  if ((ret & 0x10) == 0)
    return SWFDEC_SHAPE_TYPE_CURVE;
  return SWFDEC_SHAPE_TYPE_LINE;
}

static void
swfdec_shape_parse_curve (SwfdecBits *bits, SubPath *path,
    int *x, int *y)
{
  int n_bits;
  int cur_x, cur_y;
  int control_x, control_y;

  if (swfdec_bits_getbits (bits, 2) != 2)
    g_assert_not_reached ();

  n_bits = swfdec_bits_getbits (bits, 4) + 2;

  cur_x = *x;
  cur_y = *y;

  control_x = cur_x + swfdec_bits_getsbits (bits, n_bits);
  control_y = cur_y + swfdec_bits_getsbits (bits, n_bits);
  SWFDEC_LOG ("   control %d,%d", control_x, control_y);

  *x = control_x + swfdec_bits_getsbits (bits, n_bits);
  *y = control_y + swfdec_bits_getsbits (bits, n_bits);
  SWFDEC_LOG ("   anchor %d,%d", *x, *y);
  if (path) {
    swfdec_path_curve_to (&path->path, 
	cur_x, cur_y,
	control_x, control_y, 
	*x, *y);
  } else {
    SWFDEC_ERROR ("no path to curve in");
  }
}

static void
swfdec_shape_parse_line (SwfdecBits *bits, SubPath *path,
    int *x, int *y, gboolean add_as_curve)
{
  int n_bits;
  int general_line_flag;
  int cur_x, cur_y;

  if (swfdec_bits_getbits (bits, 2) != 3)
    g_assert_not_reached ();

  cur_x = *x;
  cur_y = *y;
  n_bits = swfdec_bits_getbits (bits, 4) + 2;
  general_line_flag = swfdec_bits_getbit (bits);
  if (general_line_flag == 1) {
    *x += swfdec_bits_getsbits (bits, n_bits);
    *y += swfdec_bits_getsbits (bits, n_bits);
  } else {
    int vert_line_flag = swfdec_bits_getbit (bits);
    if (vert_line_flag == 0) {
      *x += swfdec_bits_getsbits (bits, n_bits);
    } else {
      *y += swfdec_bits_getsbits (bits, n_bits);
    }
  }
  SWFDEC_LOG ("   line to %d,%d", *x, *y);
  if (path) {
    if (add_as_curve)
      swfdec_path_curve_to (&path->path, cur_x, cur_y,
	  (cur_x + *x) / 2, (cur_y + *y) / 2, *x, *y);
    else
      swfdec_path_line_to (&path->path, *x, *y);
  } else {
    SWFDEC_ERROR ("no path to line in");
  }
}

SubPath *
swfdec_shape_parse_change (SwfdecSwfDecoder *s, SwfdecShape *shape, GArray *path_array, SubPath *path,
    int *x, int *y, SwfdecPatternFunc parse_fill, SwfdecStrokeFunc parse_stroke)
{
  int state_new_styles, state_line_styles, state_fill_styles1, state_fill_styles0, state_moveto;
  SwfdecBits *bits = &s->b;

  if (swfdec_bits_getbit (bits) != 0)
    g_assert_not_reached ();

  state_new_styles = swfdec_bits_getbit (bits);
  state_line_styles = swfdec_bits_getbit (bits);
  state_fill_styles1 = swfdec_bits_getbit (bits);
  state_fill_styles0 = swfdec_bits_getbit (bits);
  state_moveto = swfdec_bits_getbit (bits);

  if (path) {
    path->x_end = *x;
    path->y_end = *y;
  }
  g_array_set_size (path_array, path_array->len + 1);
  path = &g_array_index (path_array, SubPath, path_array->len - 1);
  if (path_array->len > 1)
    *path = g_array_index (path_array, SubPath, path_array->len - 2);
  swfdec_path_init (&path->path);
  if (state_moveto) {
    int n_bits = swfdec_bits_getbits (bits, 5);
    *x = swfdec_bits_getsbits (bits, n_bits);
    *y = swfdec_bits_getsbits (bits, n_bits);

    SWFDEC_LOG ("   moveto %d,%d", *x, *y);
  }
  path->x_start = *x;
  path->y_start = *y;
  if (state_fill_styles0) {
    path->fill0style = swfdec_bits_getbits (bits, shape->n_fill_bits);
    if (path->fill0style)
      path->fill0style += shape->fills_offset;
    SWFDEC_LOG ("   * fill0style = %d", path->fill0style);
  } else {
    SWFDEC_LOG ("   * not changing fill0style");
  }
  if (state_fill_styles1) {
    path->fill1style = swfdec_bits_getbits (bits, shape->n_fill_bits);
    if (path->fill1style)
      path->fill1style += shape->fills_offset;
    SWFDEC_LOG ("   * fill1style = %d", path->fill1style);
  } else {
    SWFDEC_LOG ("   * not changing fill1style");
  }
  if (state_line_styles) {
    path->linestyle = swfdec_bits_getbits (bits, shape->n_line_bits);
    if (path->linestyle)
      path->linestyle += shape->lines_offset;
    SWFDEC_LOG ("   * linestyle = %d", path->linestyle);
  } else {
    SWFDEC_LOG ("   * not changing linestyle");
  }
  if (state_new_styles) {
    guint old_fills_offset = shape->fills_offset;
    guint old_lines_offset = shape->lines_offset;
    SWFDEC_LOG ("   * new styles");
    swfdec_shape_add_styles (s, shape, parse_fill, parse_stroke);
    if (path->fill0style)
      path->fill0style += shape->fills_offset - old_fills_offset;
    if (path->fill1style)
      path->fill1style += shape->fills_offset - old_fills_offset;
    if (path->linestyle)
      path->linestyle += shape->lines_offset - old_lines_offset;
  }
  return path;
}

static void
swfdec_shape_initialize_from_sub_paths (SwfdecShape *shape, GArray *path_array)
{
  guint i;

#if 0
  g_print ("\n\n");
  for (i = 0; i < path_array->len; i++) {
    SubPath *path = &g_array_index (path_array, SubPath, i);
    g_print ("%d %d => %d %d  -  %u %u %u\n", path->x_start, path->y_start, path->x_end, path->y_end,
	path->fill0style, path->fill1style, path->linestyle);
  }
#endif
  swfdec_shape_accumulate_fills (shape, (SubPath *) path_array->data, path_array->len);
  swfdec_shape_accumulate_lines (shape, (SubPath *) path_array->data, path_array->len);
  for (i = 0; i < path_array->len; i++) {
    swfdec_path_reset (&g_array_index (path_array, SubPath, i).path);
  }
  g_array_free (path_array, TRUE);

  g_array_sort (shape->vecs, swfdec_shape_vec_compare);
  for (i = 0; i < shape->vecs->len; i++) {
    SwfdecShapeVec *vec = &g_array_index (shape->vecs, SwfdecShapeVec, i);
    swfdec_pattern_get_path_extents (vec->pattern, &vec->path, &vec->extents);
    swfdec_rect_union (&SWFDEC_GRAPHIC (shape)->extents, &SWFDEC_GRAPHIC (shape)->extents, &vec->extents);
  }
}

void
swfdec_shape_get_recs (SwfdecSwfDecoder * s, SwfdecShape * shape,
    SwfdecPatternFunc pattern_func, SwfdecStrokeFunc stroke_func)
{
  int x = 0, y = 0;
  SubPath *path = NULL;
  GArray *path_array;
  SwfdecShapeType type;
  SwfdecBits *bits = &s->b;

  /* First, accumulate all sub-paths into an array */
  path_array = g_array_new (FALSE, TRUE, sizeof (SubPath));

  while ((type = swfdec_shape_peek_type (bits))) {
    switch (type) {
      case SWFDEC_SHAPE_TYPE_CHANGE:
	path = swfdec_shape_parse_change (s, shape, path_array, path, &x, &y, pattern_func, stroke_func);
	break;
      case SWFDEC_SHAPE_TYPE_LINE:
	swfdec_shape_parse_line (bits, path, &x, &y, FALSE);
	break;
      case SWFDEC_SHAPE_TYPE_CURVE:
	swfdec_shape_parse_curve (bits, path, &x, &y);
	break;
      case SWFDEC_SHAPE_TYPE_END:
      default:
	g_assert_not_reached ();
	break;
    }
  }
  if (path) {
    path->x_end = x;
    path->y_end = y;
  }
  swfdec_bits_getbits (bits, 6);
  swfdec_bits_syncbits (bits);

  swfdec_shape_initialize_from_sub_paths (shape, path_array);
}

/*** MORPH SHAPE ***/

#include "swfdec_morphshape.h"

static SubPath *
swfdec_morph_shape_do_change (SwfdecBits *end_bits, SubPath *other, SwfdecMorphShape *morph, 
    GArray *path_array, SubPath *path, int *x, int *y)
{
  if (path) {
    path->x_end = *x;
    path->y_end = *y;
  }
  g_array_set_size (path_array, path_array->len + 1);
  path = &g_array_index (path_array, SubPath, path_array->len - 1);
  *path = *other;
  swfdec_path_init (&path->path);
  if (swfdec_shape_peek_type (end_bits) == SWFDEC_SHAPE_TYPE_CHANGE) {
    int state_line_styles, state_fill_styles1, state_fill_styles0, state_moveto;

    if (swfdec_bits_getbit (end_bits) != 0) {
      g_assert_not_reached ();
    }
    if (swfdec_bits_getbit (end_bits)) {
      SWFDEC_ERROR ("new styles aren't allowed in end edges, ignoring");
    }
    state_line_styles = swfdec_bits_getbit (end_bits);
    state_fill_styles1 = swfdec_bits_getbit (end_bits);
    state_fill_styles0 = swfdec_bits_getbit (end_bits);
    state_moveto = swfdec_bits_getbit (end_bits);
    if (state_moveto) {
      int n_bits = swfdec_bits_getbits (end_bits, 5);
      *x = swfdec_bits_getsbits (end_bits, n_bits);
      *y = swfdec_bits_getsbits (end_bits, n_bits);

      SWFDEC_LOG ("   moveto %d,%d", *x, *y);
    }
    if (state_fill_styles0) {
      guint check = swfdec_bits_getbits (end_bits, morph->n_fill_bits) + 
	SWFDEC_SHAPE (morph)->fills_offset;
      if (check != path->fill0style)
	SWFDEC_ERROR ("end fill0style %u differs from start fill0style %u", check, path->fill0style);
    }
    if (state_fill_styles1) {
      guint check = swfdec_bits_getbits (end_bits, morph->n_fill_bits) + 
	SWFDEC_SHAPE (morph)->fills_offset;
      if (check != path->fill1style)
	SWFDEC_ERROR ("end fill1style %u differs from start fill1style %u", check, path->fill1style);
    }
    if (state_line_styles) {
      guint check = swfdec_bits_getbits (end_bits, morph->n_line_bits) + 
	SWFDEC_SHAPE (morph)->lines_offset;
      if (check != path->linestyle)
	SWFDEC_ERROR ("end linestyle %u differs from start linestyle %u", check, path->linestyle);
    }
  }
  path->x_start = *x;
  path->y_start = *y;
  return path;
}

static void
swfdec_morph_shape_get_recs (SwfdecSwfDecoder * s, SwfdecMorphShape *morph, SwfdecBits *end_bits)
{
  int start_x = 0, start_y = 0, end_x = 0, end_y = 0;
  SubPath *start_path = NULL, *end_path = NULL;
  GArray *start_path_array, *end_path_array, *tmp;
  SwfdecShapeType start_type, end_type;
  SwfdecBits *start_bits = &s->b;
  SwfdecShape *shape = SWFDEC_SHAPE (morph);

  /* First, accumulate all sub-paths into an array */
  start_path_array = g_array_new (FALSE, TRUE, sizeof (SubPath));
  end_path_array = g_array_new (FALSE, TRUE, sizeof (SubPath));

  while ((start_type = swfdec_shape_peek_type (start_bits))) {
    end_type = swfdec_shape_peek_type (end_bits);
    if (end_type == SWFDEC_SHAPE_TYPE_CHANGE && start_type != SWFDEC_SHAPE_TYPE_CHANGE) {
      SubPath *path;
      if (start_path) {
	start_path->x_end = start_x;
	start_path->y_end = start_y;
      }
      g_array_set_size (start_path_array, start_path_array->len + 1);
      path = &g_array_index (start_path_array, SubPath, start_path_array->len - 1);
      if (start_path)
	*path = *start_path;
      start_path = path;
      swfdec_path_init (&start_path->path);
      start_path->x_start = start_x;
      start_path->y_start = start_y;
      end_path = swfdec_morph_shape_do_change (end_bits, start_path, morph, end_path_array, end_path, &end_x, &end_y);
      continue;
    }
    switch (start_type) {
      case SWFDEC_SHAPE_TYPE_CHANGE:
	start_path = swfdec_shape_parse_change (s, shape, start_path_array, start_path, &start_x, &start_y, 
	    swfdec_pattern_parse_morph, swfdec_stroke_parse_morph);
	end_path = swfdec_morph_shape_do_change (end_bits, start_path, morph, end_path_array, end_path, &end_x, &end_y);
	break;
      case SWFDEC_SHAPE_TYPE_LINE:
	if (end_type == SWFDEC_SHAPE_TYPE_LINE) {
	  swfdec_shape_parse_line (start_bits, start_path, &start_x, &start_y, FALSE);
	  swfdec_shape_parse_line (end_bits, end_path, &end_x, &end_y, FALSE);
	} else if (end_type == SWFDEC_SHAPE_TYPE_CURVE) {
	  swfdec_shape_parse_line (start_bits, start_path, &start_x, &start_y, TRUE);
	  swfdec_shape_parse_curve (end_bits, end_path, &end_x, &end_y);
	} else {
	  SWFDEC_ERROR ("edge type didn't match, wanted line or curve, but got %s",
	      end_type ? "change" : "end");
	  goto error;
	}
	break;
      case SWFDEC_SHAPE_TYPE_CURVE:
	swfdec_shape_parse_curve (start_bits, start_path, &start_x, &start_y);
	if (end_type == SWFDEC_SHAPE_TYPE_LINE) {
	  swfdec_shape_parse_line (end_bits, end_path, &end_x, &end_y, TRUE);
	} else if (end_type == SWFDEC_SHAPE_TYPE_CURVE) {
	  swfdec_shape_parse_curve (end_bits, end_path, &end_x, &end_y);
	} else {
	  SWFDEC_ERROR ("edge type didn't match, wanted line or curve, but got %s",
	      end_type ? "change" : "end");
	  goto error;
	}
	break;
      case SWFDEC_SHAPE_TYPE_END:
      default:
	g_assert_not_reached ();
	break;
    }
  }
  if (start_path) {
    start_path->x_end = start_x;
    start_path->y_end = start_y;
  }
  if (end_path) {
    end_path->x_end = end_x;
    end_path->y_end = end_y;
  }
  swfdec_bits_getbits (start_bits, 6);
  swfdec_bits_syncbits (start_bits);
  if (swfdec_bits_getbits (end_bits, 6) != 0) {
    SWFDEC_ERROR ("end shapes are not finished when start shapes are");
  }
  swfdec_bits_syncbits (end_bits);

error:
  /* FIXME: there's probably a problem if start and end paths get accumulated in 
   * different ways, this could lead to the morphs not looking like they should. 
   * Need a good testcase for this first though.
   * FIXME: Also, due to error handling, there needs to be syncing of code paths
   */
  tmp = shape->vecs;
  shape->vecs = morph->end_vecs;
  swfdec_shape_initialize_from_sub_paths (shape, end_path_array);
  morph->end_vecs = shape->vecs;
  shape->vecs = tmp;
  swfdec_shape_initialize_from_sub_paths (shape, start_path_array);
  g_assert (morph->end_vecs->len == shape->vecs->len);
}

int
tag_define_morph_shape (SwfdecSwfDecoder * s)
{
  SwfdecBits end_bits;
  SwfdecBits *bits = &s->b;
  SwfdecMorphShape *morph;
  guint offset;
  int id;
  id = swfdec_bits_get_u16 (bits);

  morph = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_MORPH_SHAPE);
  if (!morph)
    return SWFDEC_STATUS_OK;

  SWFDEC_INFO ("id=%d", id);

  swfdec_bits_get_rect (bits, &SWFDEC_GRAPHIC (morph)->extents);
  swfdec_bits_get_rect (bits, &morph->end_extents);
  offset = swfdec_bits_get_u32 (bits);
  end_bits = *bits;
  if (swfdec_bits_skip_bytes (&end_bits, offset) != offset) {
    SWFDEC_ERROR ("wrong offset in DefineMorphShape");
    return SWFDEC_STATUS_OK;
  }
  bits->end = end_bits.ptr;

  swfdec_shape_add_styles (s, SWFDEC_SHAPE (morph),
      swfdec_pattern_parse_morph, swfdec_stroke_parse_morph);

  morph->n_fill_bits = swfdec_bits_getbits (&end_bits, 4);
  morph->n_line_bits = swfdec_bits_getbits (&end_bits, 4);
  SWFDEC_LOG ("%u fill bits, %u line bits in end shape", morph->n_fill_bits, morph->n_line_bits);

  swfdec_morph_shape_get_recs (s, morph, &end_bits);
  if (swfdec_bits_left (&s->b)) {
    SWFDEC_WARNING ("early finish when parsing start shapes: %u bytes",
        swfdec_bits_left (&s->b));
  }

  s->b = end_bits;

  return SWFDEC_STATUS_OK;
}
