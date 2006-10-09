#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "swfdec_shape.h"
#include "swfdec.h"
#include "swfdec_cache.h"
#include "swfdec_debug.h"
#include "swfdec_image.h"

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
swfdec_shape_render (SwfdecObject *obj, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecShape *shape = SWFDEC_SHAPE (obj);
  unsigned int i;

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  for (i = 0; i < shape->vecs->len; i++) {
    SwfdecShapeVec *vec = &g_array_index (shape->vecs, SwfdecShapeVec, i);

    /* FIXME: catch these two earlier */
    if (vec->pattern == NULL)
      continue;
    if (vec->path.num_data == 0)
      continue;
    
    /* hack to not append paths for lines */
    if (!fill && vec->last_index % 2 != 0) 
      continue;

    cairo_append_path (cr, &vec->path);
    if (fill)
      swfdec_pattern_fill (vec->pattern, cr, trans, 0);
  }
}

static gboolean
swfdec_shape_mouse_in (SwfdecObject *object,
      double x, double y, int button)
{
  SwfdecShapeVec *shapevec;
  SwfdecShape *shape = SWFDEC_SHAPE (object);
  static cairo_surface_t *surface = NULL;

  if (shape->fill_cr == NULL) {
    /* FIXME: do less memory intensive fill checking plz */
    if (surface == NULL)
      surface = cairo_image_surface_create (CAIRO_FORMAT_A8, 1000, 1000);
    guint i;
    shape->fill_cr = cairo_create (surface);
    cairo_set_fill_rule (shape->fill_cr, CAIRO_FILL_RULE_EVEN_ODD);
    /* FIXME: theoretically, we require a cairo_t for every path we wanna check.
     * Currently different paths can cancel each other out due to the fill rule */
    for (i = 0; i < shape->vecs->len; i++) {
      shapevec = &g_array_index (shape->vecs, SwfdecShapeVec, i);
      /* filter out lines */
      if (shapevec->last_index %2 == 1)
	break;
      if (shapevec->path.num_data)
	cairo_append_path (shape->fill_cr, &shapevec->path);
    }
  }
  /* FIXME: handle strokes */
  return cairo_in_fill (shape->fill_cr, x, y);
}

SWFDEC_OBJECT_BOILERPLATE (SwfdecShape, swfdec_shape)

     static void swfdec_shape_base_init (gpointer g_class)
{

}

static void
swfdec_shape_class_init (SwfdecShapeClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);
  
  object_class->render = swfdec_shape_render;
  object_class->mouse_in = swfdec_shape_mouse_in;
}

static void
swfdec_shape_init (SwfdecShape * shape)
{
  shape->fills = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();
  shape->vecs = g_array_new (FALSE, TRUE, sizeof (SwfdecShapeVec));
}

static void
swfdec_shape_vec_finish (SwfdecShapeVec * shapevec)
{
  if (shapevec->pattern) {
    g_object_unref (shapevec->pattern);
    shapevec->pattern = NULL;
  }
  swfdec_path_reset (&shapevec->path);
}

static void
swfdec_shape_vec_init (SwfdecShapeVec *vec)
{
  swfdec_path_init (&vec->path);
}

static void
swfdec_shape_dispose (SwfdecShape * shape)
{
  unsigned int i;

  if (shape->fill_cr)
    cairo_destroy (shape->fill_cr);

  for (i = 0; i < shape->vecs->len; i++) {
    swfdec_shape_vec_finish (&g_array_index (shape->vecs, SwfdecShapeVec, i));
  }
  g_array_free (shape->vecs, TRUE);
  for (i = 0; i < shape->fills->len; i++) {
    g_object_unref (g_ptr_array_index (shape->fills, i));
  }
  g_ptr_array_free (shape->fills, TRUE);

  for (i = 0; i < shape->lines->len; i++) {
    g_object_unref (g_ptr_array_index (shape->lines, i));
  }
  g_ptr_array_free (shape->lines, TRUE);

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (shape));
}

int
tag_define_shape (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int id;

  id = swfdec_bits_get_u16 (bits);

  shape = swfdec_object_create (s, id, SWFDEC_TYPE_SHAPE);
  if (!shape)
    return SWFDEC_OK;

  SWFDEC_INFO ("id=%d", id);

  swfdec_bits_get_rect (bits, &SWFDEC_OBJECT (shape)->extents);

  swf_shape_add_styles (s, shape, bits);

  swf_shape_get_recs (s, bits, shape, FALSE);

  return SWFDEC_OK;
}

int
tag_define_shape_3 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int id;

  id = swfdec_bits_get_u16 (bits);
  shape = swfdec_object_create (s, id, SWFDEC_TYPE_SHAPE);
  if (!shape)
    return SWFDEC_OK;

  swfdec_bits_get_rect (bits, &SWFDEC_OBJECT (shape)->extents);

  shape->rgba = 1;

  swf_shape_add_styles (s, shape, bits);

  swf_shape_get_recs (s, bits, shape, FALSE);

  return SWFDEC_OK;
}

void
swf_shape_add_styles (SwfdecDecoder * s, SwfdecShape * shape, SwfdecBits * bits)
{
  int n_fill_styles;
  int n_line_styles;
  int i;

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

    pattern = swfdec_pattern_parse (s, shape->rgba);
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
    SwfdecPattern *pattern;
    guint width;
    swf_color color;
    
    width = swfdec_bits_get_u16 (bits);
    if (shape->rgba) {
      color = swfdec_bits_get_rgba (bits);
    } else {
      color = swfdec_bits_get_color (bits);
    }
    pattern = swfdec_pattern_new_stroke (width, color);
    g_ptr_array_add (shape->lines, pattern);
    SWFDEC_LOG ("%d: %u %08x", i, width, color);
  }

  swfdec_bits_syncbits (bits);
  shape->n_fill_bits = swfdec_bits_getbits (bits, 4);
  shape->n_line_bits = swfdec_bits_getbits (bits, 4);
}

int
tag_define_shape_2 (SwfdecDecoder * s)
{
  return tag_define_shape (s);
}

#if 0
void swf_morphshape_add_styles (SwfdecDecoder * s, SwfdecShape * shape,
    SwfdecBits * bits);

int
tag_define_morph_shape (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  SwfdecRect start_rect;
  SwfdecRect end_rect;
  int id;
  int offset;

  id = swfdec_bits_get_u16 (bits);

  shape = swfdec_object_new (s, SWFDEC_TYPE_SHAPE);
  SWFDEC_OBJECT (shape)->id = id;
  //s->objects = g_list_append (s->objects, shape);

  SWFDEC_INFO ("id=%d", id);

  swfdec_bits_get_rect (bits, &start_rect);
  swfdec_bits_syncbits (bits);
  swfdec_bits_get_rect (bits, &end_rect);

  swfdec_bits_syncbits (bits);
  offset = swfdec_bits_get_u32 (bits);
  SWFDEC_INFO ("offset=%d", offset);

  swf_morphshape_add_styles (s, shape, bits);

  swfdec_bits_syncbits (bits);
  swf_shape_get_recs (s, bits, shape, TRUE);
  swfdec_bits_syncbits (bits);
  if (1) {
    g_assert_not_reached ();
    swf_shape_get_recs (s, bits, shape, TRUE);
  }

  return SWFDEC_OK;
}
#endif

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
      SWFDEC_ERROR ("could not find a closed path for style %u", style);
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
    g_object_ref (target->pattern);
  }
  return;

fail:
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
  SwfdecShapeVec target;
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

void
swf_shape_get_recs (SwfdecDecoder * s, SwfdecBits * bits,
    SwfdecShape * shape, gboolean morphshape)
{
  guint i;
  int x = 0, y = 0;
  SubPath *path = NULL;
  GArray *path_array;

  /* First, accumulate all sub-paths into an array */
  path_array = g_array_new (FALSE, TRUE, sizeof (SubPath));

  while (swfdec_bits_peekbits (bits, 6) != 0) {
    int type;
    int n_bits;

    type = swfdec_bits_getbits (bits, 1);

    if (type == 0) {
      int state_new_styles = swfdec_bits_getbits (bits, 1);
      int state_line_styles = swfdec_bits_getbits (bits, 1);
      int state_fill_styles1 = swfdec_bits_getbits (bits, 1);
      int state_fill_styles0 = swfdec_bits_getbits (bits, 1);
      int state_moveto = swfdec_bits_getbits (bits, 1);

      if (path) {
	path->x_end = x;
	path->y_end = y;
      }
      g_array_set_size (path_array, path_array->len + 1);
      path = &g_array_index (path_array, SubPath, path_array->len - 1);
      if (path_array->len > 1)
	*path = g_array_index (path_array, SubPath, path_array->len - 2);
      swfdec_path_init (&path->path);
      if (state_moveto) {
        n_bits = swfdec_bits_getbits (bits, 5);
        x = swfdec_bits_getsbits (bits, n_bits);
        y = swfdec_bits_getsbits (bits, n_bits);

        SWFDEC_LOG ("   moveto %d,%d", x, y);
      }
      path->x_start = x;
      path->y_start = y;
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
#if 0
	if (morphshape)
	  swf_morphshape_add_styles (s, shape, bits);
	else
#endif
	  swf_shape_add_styles (s, shape, bits);
	if (path->fill0style)
	  path->fill0style += shape->fills_offset - old_fills_offset;
	if (path->fill1style)
	  path->fill1style += shape->fills_offset - old_fills_offset;
	if (path->linestyle)
	  path->linestyle += shape->lines_offset - old_lines_offset;
      }
    } else {
      /* edge record */
      int n_bits;
      int edge_flag;

      edge_flag = swfdec_bits_getbits (bits, 1);

      if (edge_flag == 0) {
        int cur_x, cur_y;
        int control_x, control_y;

        n_bits = swfdec_bits_getbits (bits, 4) + 2;

	cur_x = x;
	cur_y = y;
        x += swfdec_bits_getsbits (bits, n_bits);
        y += swfdec_bits_getsbits (bits, n_bits);
        SWFDEC_LOG ("   control %d,%d", x, y);
        control_x = x;
        control_y = y;

        x += swfdec_bits_getsbits (bits, n_bits);
        y += swfdec_bits_getsbits (bits, n_bits);
        SWFDEC_LOG ("   anchor %d,%d", x, y);
	if (path) {
	  swfdec_path_curve_to (&path->path, 
	      cur_x, cur_y,
	      control_x, control_y, 
	      x, y);
	} else {
	  SWFDEC_ERROR ("no path to curve in");
	}
      } else {
        int general_line_flag;
        int vert_line_flag = 0;

        n_bits = swfdec_bits_getbits (bits, 4) + 2;
        general_line_flag = swfdec_bits_getbit (bits);
        if (general_line_flag == 1) {
          x += swfdec_bits_getsbits (bits, n_bits);
          y += swfdec_bits_getsbits (bits, n_bits);
        } else {
          vert_line_flag = swfdec_bits_getbit (bits);
          if (vert_line_flag == 0) {
            x += swfdec_bits_getsbits (bits, n_bits);
          } else {
            y += swfdec_bits_getsbits (bits, n_bits);
          }
        }
        SWFDEC_LOG ("   line to %d,%d", x, y);
	if (path) {
	  swfdec_path_line_to (&path->path, x, y);
	} else {
	  SWFDEC_ERROR ("no path to line in");
	}
      }
    }
  }
  if (path) {
    path->x_end = x;
    path->y_end = y;
  }
  swfdec_bits_getbits (bits, 6);
  swfdec_bits_syncbits (bits);

  /* then, find all areas inside it that are supposed to be filled and arrange
   * the paths so that a filled area results */
  swfdec_shape_accumulate_fills (shape, (SubPath *) path_array->data, path_array->len);
  /* finally, grab the lines */
  swfdec_shape_accumulate_lines (shape, (SubPath *) path_array->data, path_array->len);
  for (i = 0; i < path_array->len; i++) {
    swfdec_path_reset (&g_array_index (path_array, SubPath, i).path);
  }
  g_array_free (path_array, TRUE);

  /* now, we just need to sort the array to be done */
  g_array_sort (shape->vecs, swfdec_shape_vec_compare);
}

#if 0
void
swf_morphshape_add_styles (SwfdecDecoder * s, SwfdecShape * shape,
    SwfdecBits * bits)
{
  int n_fill_styles;
  int n_line_styles;
  int i;

  swfdec_bits_syncbits (bits);
  shape->fills_offset = shape->fills->len;
  n_fill_styles = swfdec_bits_get_u8 (bits);
  if (n_fill_styles == 0xff) {
    n_fill_styles = swfdec_bits_get_u16 (bits);
  }
  SWFDEC_LOG ("n_fill_styles %d", n_fill_styles);
  for (i = 0; i < n_fill_styles; i++) {
    int fill_style_type;
    SwfdecShapeVec *shapevec;
    cairo_matrix_t end_transform;
    unsigned int end_color;

    SWFDEC_LOG ("fill style %d:", i);

    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);

    shapevec->color = SWF_COLOR_COMBINE (0, 255, 0, 255);

    fill_style_type = swfdec_bits_get_u8 (bits);
    SWFDEC_LOG ("    type 0x%02x", fill_style_type);
    if (fill_style_type == 0x00) {
      shapevec->fill_type = fill_style_type;
      shapevec->color = swfdec_bits_get_rgba (bits);
      end_color = swfdec_bits_get_rgba (bits);
      SWFDEC_LOG ("    color %08x", shapevec->color);
    } else if (fill_style_type == 0x10 || fill_style_type == 0x12) {
      shapevec->fill_type = fill_style_type;
      swfdec_bits_get_matrix (bits, &shapevec->fill_transform);
      swfdec_bits_get_matrix (bits, &end_transform);
      shapevec->grad = swfdec_bits_get_morph_gradient (bits);
    } else if (fill_style_type == 0x40 || fill_style_type == 0x41) {
      shapevec->fill_type = fill_style_type;
      shapevec->fill_id = swfdec_bits_get_u16 (bits);
      SWFDEC_LOG ("   background fill id = %d (type 0x%02x)",
          shapevec->fill_id, fill_style_type);

      if (shapevec->fill_id == 65535) {
        shapevec->fill_id = 0;
        shapevec->color = SWF_COLOR_COMBINE (0, 255, 255, 255);
      }

      swfdec_bits_get_matrix (bits, &shapevec->fill_transform);
      swfdec_bits_get_matrix (bits, &end_transform);
    } else {
      SWFDEC_ERROR ("unknown fill style type 0x%02x", fill_style_type);
      shapevec->fill_type = 0;
    }
  }

  swfdec_bits_syncbits (bits);
  shape->lines_offset = shape->lines->len;
  n_line_styles = swfdec_bits_get_u8 (bits);
  if (n_line_styles == 0xff) {
    n_line_styles = swfdec_bits_get_u16 (bits);
  }
  SWFDEC_LOG ("   n_line_styles %d", n_line_styles);
  for (i = 0; i < n_line_styles; i++) {
    SwfdecShapeVec *shapevec;
    double end_width;
    unsigned int end_color;

    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->lines, shapevec);

    shapevec->width = swfdec_bits_get_u16 (bits) / SWF_SCALE_FACTOR;
    end_width = swfdec_bits_get_u16 (bits) / SWF_SCALE_FACTOR;
    shapevec->color = swfdec_bits_get_rgba (bits);
    end_color = swfdec_bits_get_rgba (bits);
    SWFDEC_LOG ("%d: %g->%g %08x->%08x", i,
        shapevec->width, end_width, shapevec->color, end_color);
  }

  swfdec_bits_syncbits (bits);
}
#endif
