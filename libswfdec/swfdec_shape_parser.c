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

#include "swfdec_shape_parser.h"
#include "swfdec_debug.h"
#include "swfdec_path.h"
#include "swfdec_stroke.h"

/*** PATH CONSTRUCTION ***/

typedef enum {
  SWFDEC_SHAPE_TYPE_END = 0,
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

typedef struct {
  int			x_start, y_start;
  int			x_end, y_end;
  cairo_path_t		path;
} SwfdecSubPath;

typedef struct {
  SwfdecDraw *		draw;		/* drawing operations that should take the subpaths */
  GSList *		subpaths;	/* indexes into SubPath array */
} SwfdecStyle;

struct _SwfdecShapeParser {
  /* state */
  GSList *		draws;		/* completely accumulated drawing commands */
  SwfdecParseDrawFunc	parse_fill;	/* function to call to parse a fill style */
  SwfdecParseDrawFunc	parse_line;	/* function to call to parse a line style */
  gpointer		data;		/* data to pass to parse functions */
  /* used while parsing */
  GArray *		fillstyles;	/* SwfdecStyle objects */
  GArray *		linestyles;	/* SwfdecStyle objects */
  GArray *		subpaths;	/* SwfdecSubPath partial paths */
  guint			fill0style;
  guint			fill1style;
  guint			linestyle;
  guint			n_fill_bits;
  guint			n_line_bits;
};

static void
swfdec_shape_parser_new_styles (SwfdecShapeParser *parser, SwfdecBits *bits)
{
  guint i, n_fill_styles, n_line_styles;

  swfdec_bits_syncbits (bits);
  if (parser->parse_fill) {
    n_fill_styles = swfdec_bits_get_u8 (bits);
    if (n_fill_styles == 0xff) {
      n_fill_styles = swfdec_bits_get_u16 (bits);
    }
    SWFDEC_LOG ("   n_fill_styles %d", n_fill_styles);
    g_array_set_size (parser->fillstyles, n_fill_styles);
    for (i = 0; i < n_fill_styles && swfdec_bits_left (bits); i++) {
      g_array_index (parser->fillstyles, SwfdecStyle, i).draw = 
	parser->parse_fill (bits, parser->data);
    }

    n_line_styles = swfdec_bits_get_u8 (bits);
    if (n_line_styles == 0xff) {
      n_line_styles = swfdec_bits_get_u16 (bits);
    }
    SWFDEC_LOG ("   n_line_styles %d", n_line_styles);
    g_array_set_size (parser->linestyles, n_line_styles);
    for (i = 0; i < n_line_styles && swfdec_bits_left (bits); i++) {
      g_array_index (parser->linestyles, SwfdecStyle, i).draw = 
	parser->parse_line (bits, parser->data);
    }
  } else {
    /* This is the magic part for DefineFont */
    g_array_set_size (parser->fillstyles, 1);
    g_array_index (parser->fillstyles, SwfdecStyle, 0).draw =
	SWFDEC_DRAW (swfdec_pattern_new_color (0xFFFFFFFF));
  }
  parser->n_fill_bits = swfdec_bits_getbits (bits, 4);
  parser->n_line_bits = swfdec_bits_getbits (bits, 4);
}


SwfdecShapeParser *
swfdec_shape_parser_new (SwfdecParseDrawFunc parse_fill, 
    SwfdecParseDrawFunc parse_line, gpointer data)
{
  SwfdecShapeParser *list;

  list = g_slice_new0 (SwfdecShapeParser);
  list->parse_fill = parse_fill;
  list->parse_line = parse_line;
  list->data = data;
  list->fillstyles = g_array_new (FALSE, TRUE, sizeof (SwfdecStyle));
  list->linestyles = g_array_new (FALSE, TRUE, sizeof (SwfdecStyle));
  list->subpaths = g_array_new (FALSE, TRUE, sizeof (SwfdecSubPath));

  return list;
}

static inline void
swfdec_shape_parser_clear_one (GArray *array)
{
  guint i;

  for (i = 0; i < array->len; i++) {
    SwfdecStyle *style = &g_array_index (array, SwfdecStyle, i);
    g_object_unref (style->draw);
    g_slist_free (style->subpaths);
  }
  g_array_set_size (array, 0);
}

static void
swfdec_shape_parser_clear (SwfdecShapeParser *list)
{
  guint i;

  swfdec_shape_parser_clear_one (list->fillstyles);
  swfdec_shape_parser_clear_one (list->linestyles);

  for (i = 0; i < list->subpaths->len; i++) {
    SwfdecSubPath *path = &g_array_index (list->subpaths, SwfdecSubPath, i);
    swfdec_path_reset (&path->path);
  }
  g_array_set_size (list->subpaths, 0);
}

GSList *
swfdec_shape_parser_reset (SwfdecShapeParser *parser)
{
  GSList *draws = parser->draws;

  parser->draws = NULL;
  return draws;
}

GSList *
swfdec_shape_parser_free (SwfdecShapeParser *parser)
{
  GSList *draws = parser->draws;

  swfdec_shape_parser_clear (parser);
  g_array_free (parser->fillstyles, TRUE);
  g_array_free (parser->linestyles, TRUE);
  g_array_free (parser->subpaths, TRUE);
  g_slice_free (SwfdecShapeParser, parser);

  draws = g_slist_reverse (draws);
  return draws;
}

/* NB: assumes all fill paths are closed */
static void
swfdec_style_collect_path (SwfdecStyle *style, gboolean line)
{
  GSList *walk;
  SwfdecSubPath *start, *cur;
  double x, y;

  start = style->subpaths->data;
  style->subpaths = g_slist_remove (style->subpaths, start);
  swfdec_path_move_to (&style->draw->path, start->x_start, start->y_start);
  swfdec_path_append (&style->draw->path, &start->path);
  x = start->x_end;
  y = start->y_end;
  while (x != start->x_start || y != start->y_start) {
    for (walk = style->subpaths; walk; walk = walk->next) {
      cur = walk->data;
      if (cur->x_start == x && cur->y_start == y) {
	x = cur->x_end;
	y = cur->y_end;
	swfdec_path_append (&style->draw->path, &cur->path);
	break;
      }
    }
    if (walk) {
      style->subpaths = g_slist_delete_link (style->subpaths, walk);
    } else {
      if (!line) {
	SWFDEC_ERROR ("fill path not closed");
      } 
      break;
    }
  }
}

static void
swfdec_style_finish (SwfdecStyle *style, SwfdecSubPath *paths, gboolean line)
{
  GSList *walk;

  /* first, replace numbers with real references to the paths */
  for (walk = style->subpaths; walk; walk = walk->next)
    walk->data = &paths[GPOINTER_TO_UINT (walk->data)];
  /* then, accumulate paths one by one */
  while (style->subpaths)
    swfdec_style_collect_path (style, line);
  swfdec_draw_recompute (style->draw);
}

/* merge subpaths into draws, then reset */
static void
swfdec_shape_parser_finish (SwfdecShapeParser *parser)
{
  guint i;
  for (i = 0; i < parser->fillstyles->len; i++) {
    SwfdecStyle *style = &g_array_index (parser->fillstyles, SwfdecStyle, i);
    if (style->subpaths) {
      swfdec_style_finish (style, (SwfdecSubPath *) parser->subpaths->data, FALSE);
      parser->draws = g_slist_prepend (parser->draws, g_object_ref (style->draw));
    } else if (parser->parse_fill) {
      SWFDEC_WARNING ("fillstyle %u has no path", i);
    } else {
      SWFDEC_INFO ("fillstyle %u has no path (probably a space sign?)", i);
    }
  }
  for (i = 0; i < parser->linestyles->len; i++) {
    SwfdecStyle *style = &g_array_index (parser->linestyles, SwfdecStyle, i);
    if (style->subpaths) {
      swfdec_style_finish (style, (SwfdecSubPath *) parser->subpaths->data, TRUE);
      parser->draws = g_slist_prepend (parser->draws, g_object_ref (style->draw));
    } else {
      SWFDEC_WARNING ("linestyle %u has no path", i);
    }
  }
  swfdec_shape_parser_clear (parser);
}

static void
swfdec_shape_parser_end_path (SwfdecShapeParser *parser, SwfdecSubPath *path, int x, int y)
{
  if (path == NULL)
    return;
#if 0
  if (path->path.num_data == 0) {
    SWFDEC_INFO ("ignoring empty path");
    return;
  }
#endif

  path->x_end = x;
  path->y_end = y;

  /* add the path to their styles */
  if (parser->fill0style) {
    if (parser->fill0style > parser->fillstyles->len) {
      SWFDEC_ERROR ("fillstyle too big (%u > %u)", parser->fill0style,
	  parser->fillstyles->len);
    } else {
      SwfdecStyle *style = &g_array_index (parser->fillstyles, 
	  SwfdecStyle, parser->fill0style - 1);
      style->subpaths = g_slist_prepend (style->subpaths, 
	  GUINT_TO_POINTER (parser->subpaths->len - 1));
    }
  }
  if (parser->fill1style) {
    if (parser->fill1style > parser->fillstyles->len) {
      SWFDEC_ERROR ("fillstyle too big (%u > %u)", parser->fill1style,
	  parser->fillstyles->len);
    } else {
      SwfdecSubPath reverse;
      SwfdecStyle *style = &g_array_index (parser->fillstyles, 
	  SwfdecStyle, parser->fill1style - 1);

      reverse.x_start = path->x_end;
      reverse.y_start = path->y_end;
      reverse.x_end = path->x_start;
      reverse.y_end = path->y_start;
      swfdec_path_init (&reverse.path);
      swfdec_path_append_reverse (&reverse.path, &path->path, reverse.x_end, reverse.y_end);
      style->subpaths = g_slist_prepend (style->subpaths, 
	  GUINT_TO_POINTER (parser->subpaths->len));
      g_array_append_val (parser->subpaths, reverse);
    }
  }
  if (parser->linestyle) {
    if (parser->linestyle > parser->linestyles->len) {
      SWFDEC_ERROR ("fillstyle too big (%u > %u)", parser->linestyle,
	  parser->linestyles->len);
    } else {
      SwfdecStyle *style = &g_array_index (parser->linestyles, 
	  SwfdecStyle, parser->linestyle - 1);
      style->subpaths = g_slist_prepend (style->subpaths, 
	  GUINT_TO_POINTER (parser->subpaths->len - 1));
    }
  }
}

static SwfdecSubPath *
swfdec_shape_parser_parse_change (SwfdecShapeParser *parser, SwfdecBits *bits, 
    SwfdecSubPath *path, int *x, int *y)
{
  int state_new_styles, state_line_styles, state_fill_styles1, state_fill_styles0, state_moveto;

  if (swfdec_bits_getbit (bits) != 0) {
    g_assert_not_reached ();
  }

  state_new_styles = swfdec_bits_getbit (bits);
  state_line_styles = swfdec_bits_getbit (bits);
  state_fill_styles1 = swfdec_bits_getbit (bits);
  state_fill_styles0 = swfdec_bits_getbit (bits);
  state_moveto = swfdec_bits_getbit (bits);

  swfdec_shape_parser_end_path (parser, path, *x, *y);
  g_array_set_size (parser->subpaths, parser->subpaths->len + 1);
  path = &g_array_index (parser->subpaths, SwfdecSubPath, parser->subpaths->len - 1);
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
    parser->fill0style = swfdec_bits_getbits (bits, parser->n_fill_bits);
    SWFDEC_LOG ("   * fill0style = %d", parser->fill0style);
  } else {
    SWFDEC_LOG ("   * not changing fill0style");
  }
  if (state_fill_styles1) {
    parser->fill1style = swfdec_bits_getbits (bits, parser->n_fill_bits);
    SWFDEC_LOG ("   * fill1style = %d", parser->fill1style);
  } else {
    SWFDEC_LOG ("   * not changing fill1style");
  }
  if (state_line_styles) {
    parser->linestyle = swfdec_bits_getbits (bits, parser->n_line_bits);
    SWFDEC_LOG ("   * linestyle = %d", parser->linestyle);
  } else {
    SWFDEC_LOG ("   * not changing linestyle");
  }
  if (state_new_styles) {
    SWFDEC_LOG ("   * new styles");
    swfdec_shape_parser_finish (parser);
    swfdec_shape_parser_new_styles (parser, bits);
  }
  return path;
}

static void
swfdec_shape_parser_parse_curve (SwfdecBits *bits, SwfdecSubPath *path,
    int *x, int *y)
{
  int n_bits;
  int cur_x, cur_y;
  int control_x, control_y;

  if (swfdec_bits_getbits (bits, 2) != 2) {
    g_assert_not_reached ();
  }

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
swfdec_shape_parser_parse_line (SwfdecBits *bits, SwfdecSubPath *path,
    int *x, int *y, gboolean add_as_curve)
{
  int n_bits;
  int general_line_flag;
  int cur_x, cur_y;

  if (swfdec_bits_getbits (bits, 2) != 3) {
    g_assert_not_reached ();
  }

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

void
swfdec_shape_parser_parse (SwfdecShapeParser *parser, SwfdecBits *bits)
{
  int x = 0, y = 0;
  SwfdecSubPath *path = NULL;
  SwfdecShapeType type;

  swfdec_shape_parser_new_styles (parser, bits);
  while ((type = swfdec_shape_peek_type (bits))) {
    switch (type) {
      case SWFDEC_SHAPE_TYPE_CHANGE:
	path = swfdec_shape_parser_parse_change (parser, bits, path, &x, &y);
	break;
      case SWFDEC_SHAPE_TYPE_LINE:
	swfdec_shape_parser_parse_line (bits, path, &x, &y, FALSE);
	break;
      case SWFDEC_SHAPE_TYPE_CURVE:
	swfdec_shape_parser_parse_curve (bits, path, &x, &y);
	break;
      case SWFDEC_SHAPE_TYPE_END:
      default:
	g_assert_not_reached ();
	break;
    }
  }
  swfdec_shape_parser_end_path (parser, path, x, y);
  swfdec_bits_getbits (bits, 6);
  swfdec_bits_syncbits (bits);

  swfdec_shape_parser_finish (parser);
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
