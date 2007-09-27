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
      } else if (cur->x_end == x && cur->y_end == y) {
	x = cur->x_start;
	y = cur->y_start;
	swfdec_path_append_reverse (&style->draw->path, &cur->path, x, y);
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
    } else {
      SWFDEC_WARNING ("fillstyle %u has no path", i);
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

  if (path) {
    path->x_end = *x;
    path->y_end = *y;
  }
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
      SwfdecStyle *style = &g_array_index (parser->fillstyles, 
	  SwfdecStyle, parser->fill1style - 1);
      style->subpaths = g_slist_prepend (style->subpaths, 
	  GUINT_TO_POINTER (parser->subpaths->len - 1));
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
  if (path) {
    path->x_end = x;
    path->y_end = y;
  }
  swfdec_bits_getbits (bits, 6);
  swfdec_bits_syncbits (bits);

  swfdec_shape_parser_finish (parser);
}

