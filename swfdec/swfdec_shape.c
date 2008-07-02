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
#include "swfdec_path.h"
#include "swfdec_shape_parser.h"
#include "swfdec_stroke.h"

G_DEFINE_TYPE (SwfdecShape, swfdec_shape, SWFDEC_TYPE_GRAPHIC)

static void
swfdec_shape_dispose (GObject *object)
{
  SwfdecShape * shape = SWFDEC_SHAPE (object);

  g_slist_foreach (shape->draws, (GFunc) g_object_unref, NULL);
  g_slist_free (shape->draws);
  shape->draws = NULL;

  G_OBJECT_CLASS (swfdec_shape_parent_class)->dispose (G_OBJECT (shape));
}

static void
swfdec_shape_render (SwfdecGraphic *graphic, cairo_t *cr, 
    const SwfdecColorTransform *trans)
{
  SwfdecShape *shape = SWFDEC_SHAPE (graphic);
  SwfdecRect inval;
  GSList *walk;

  cairo_clip_extents (cr, &inval.x0, &inval.y0, &inval.x1, &inval.y1);

  for (walk = shape->draws; walk; walk = walk->next) {
    SwfdecDraw *draw = walk->data;

    if (!swfdec_rect_intersect (NULL, &draw->extents, &inval))
      continue;
    
    swfdec_draw_paint (draw, cr, trans);
  }
}

static gboolean
swfdec_shape_mouse_in (SwfdecGraphic *graphic, double x, double y)
{
  SwfdecShape *shape = SWFDEC_SHAPE (graphic);
  GSList *walk;

  for (walk = shape->draws; walk; walk = walk->next) {
    SwfdecDraw *draw = walk->data;

    if (swfdec_draw_contains (draw, x, y))
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
}

int
tag_define_shape (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  SwfdecShapeParser *parser;
  int id;

  id = swfdec_bits_get_u16 (bits);

  shape = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_SHAPE);
  if (!shape)
    return SWFDEC_STATUS_OK;

  SWFDEC_INFO ("id=%d", id);

  swfdec_bits_get_rect (bits, &SWFDEC_GRAPHIC (shape)->extents);

  parser = swfdec_shape_parser_new ((SwfdecParseDrawFunc) swfdec_pattern_parse,
      (SwfdecParseDrawFunc) swfdec_stroke_parse, s);
  swfdec_shape_parser_parse (parser, bits);
  shape->draws = swfdec_shape_parser_free (parser);

  return SWFDEC_STATUS_OK;
}

int
tag_define_shape_3 (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  SwfdecShapeParser *parser;
  int id;

  id = swfdec_bits_get_u16 (bits);
  shape = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_SHAPE);
  if (!shape)
    return SWFDEC_STATUS_OK;

  swfdec_bits_get_rect (bits, &SWFDEC_GRAPHIC (shape)->extents);

  parser = swfdec_shape_parser_new ((SwfdecParseDrawFunc) swfdec_pattern_parse_rgba, 
      (SwfdecParseDrawFunc) swfdec_stroke_parse_rgba, s);
  swfdec_shape_parser_parse (parser, bits);
  shape->draws = swfdec_shape_parser_free (parser);

  return SWFDEC_STATUS_OK;
}

int
tag_define_shape_4 (SwfdecSwfDecoder *s, guint tag)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  SwfdecShapeParser *parser;
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

  parser = swfdec_shape_parser_new ((SwfdecParseDrawFunc) swfdec_pattern_parse_rgba, 
      (SwfdecParseDrawFunc) swfdec_stroke_parse_extended, s);
  swfdec_shape_parser_parse (parser, bits);
  shape->draws = swfdec_shape_parser_free (parser);

  return SWFDEC_STATUS_OK;
}

#if 0
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
#endif

#if 0
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
#endif
