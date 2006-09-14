

#include <swfdec_shape.h>

#include <math.h>

#include "swfdec_internal.h"

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

static void
swfdec_path_get_current_point (cairo_path_t *path, double *x, double *y)
{
  int i;
  cairo_path_data_t *data;

  *x = *y = 0.0;
  i = 0;
  data = path->data;
  while (i < path->num_data) {
    switch (data[i].header.type) {
      case CAIRO_PATH_CURVE_TO:
	i += 2;
	/* fall through */
      case CAIRO_PATH_MOVE_TO:
      case CAIRO_PATH_LINE_TO:
	i++;
	g_assert (i < path->num_data);
	*x = data[i].point.x;
	*y = data[i].point.y;
	break;
      case CAIRO_PATH_CLOSE_PATH:
	*x = *y = 0.0;
	break;
      default:
	g_assert_not_reached ();
	return;
    }
    i++;
  }
}

#define swfdec_path_require_size(path, steps) \
  swfdec_path_ensure_size ((path), (path)->num_data + steps)
static void
swfdec_path_ensure_size (cairo_path_t *path, int size)
{
#define SWFDEC_PATH_STEPS 32
  /* round up to next multiple of SWFDEC_PATH_STEPS */
  int current_size = path->num_data + 
    (SWFDEC_PATH_STEPS - path->num_data) % SWFDEC_PATH_STEPS;

  g_assert (current_size % SWFDEC_PATH_STEPS == 0);
  while (size > current_size)
    current_size += SWFDEC_PATH_STEPS;
  path->data = g_renew (cairo_path_data_t, path->data, current_size);
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
swfdec_path_curve_to (cairo_path_t *path, double end_x, double end_y, 
    double control_x, double control_y)
{
  cairo_path_data_t *cur;
  double start_x, start_y;

  swfdec_path_require_size (path, 4);
  swfdec_path_get_current_point (path, &start_x, &start_y);
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

#if 0
static void
swfdec_path_append (cairo_path_t *path, const cairo_path_t *append)
{
  swfdec_path_require_size (path, append->num_data);
  memcpy (&path->data[path->num_data], append->data, sizeof (cairo_path_data_t) * append->num_data);
  path->num_data += append->num_data;
}
#endif

/*** SHAPE ***/

static void
swfdec_shape_render (SwfdecDecoder *s, cairo_t *cr, 
    const SwfdecColorTransform *trans, SwfdecObject *obj, SwfdecRect *inval)
{
  SwfdecShape *shape = SWFDEC_SHAPE (obj);
  unsigned int i;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  for (i = 0; i < shape->fills->len; i++) {
    swf_color color;

    shapevec = g_ptr_array_index (shape->fills, i);
    shapevec2 = g_ptr_array_index (shape->fills2, i);

    color = swfdec_color_apply_transform (shapevec->color, trans);

    switch (shapevec->fill_type) {
      case 0x10:
        {
          SwfdecGradient *grad = shapevec->grad;
          int j;
	  cairo_pattern_t *pattern;
          cairo_matrix_t mat;

          mat = shapevec->fill_transform;
          if (cairo_matrix_invert (&mat)) {
	    g_assert_not_reached ();
	  }

          pattern = cairo_pattern_create_linear (-16384.0, 0, 16384.0, 0);
          cairo_pattern_set_matrix(pattern, &mat);
          for (j=0;j<grad->n_gradients;j++){
            color = swfdec_color_apply_transform (grad->array[j].color,
                trans);

            cairo_pattern_add_color_stop_rgba (pattern,
                grad->array[j].ratio/256.0,
                SWF_COLOR_R(color)/255.0, SWF_COLOR_G(color)/255.0,
                SWF_COLOR_B(color)/255.0, SWF_COLOR_A(color)/255.0);
          }
          cairo_set_source (cr, pattern);
	  cairo_pattern_destroy (pattern);
        }
        break;
      case 0x12:
        {
          SwfdecGradient *grad = shapevec->grad;
          int j;
          cairo_matrix_t mat;
	  cairo_pattern_t *pattern;

          mat = shapevec->fill_transform;
          if (cairo_matrix_invert (&mat)) {
	    g_assert_not_reached ();
	  }

          pattern = cairo_pattern_create_radial (0,0,0,
             0, 0, 16384);
          cairo_pattern_set_matrix(pattern, &mat);
          for (j=0;j<grad->n_gradients;j++){
            color = swfdec_color_apply_transform (grad->array[j].color,
		trans);

            cairo_pattern_add_color_stop_rgba (pattern,
                grad->array[j].ratio/256.0,
                SWF_COLOR_R(color)/255.0, SWF_COLOR_G(color)/255.0,
                SWF_COLOR_B(color)/255.0, SWF_COLOR_A(color)/255.0);
          }
          cairo_set_source (cr, pattern);
	  cairo_pattern_destroy (pattern);
        }
        break;
      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
        {
          SwfdecImage *image;
          cairo_matrix_t mat;

          mat = shapevec->fill_transform;
          if (cairo_matrix_invert (&mat)) {
	    g_assert_not_reached ();
	  }

          image = (SwfdecImage *)swfdec_object_get (s, shapevec->fill_id);
          if (image && SWFDEC_IS_IMAGE (image)) {
	    cairo_surface_t *src_surface;
	    cairo_pattern_t *pattern;
            unsigned char *image_data = swfdec_handle_get_data(image->handle);

            src_surface = cairo_image_surface_create_for_data (image_data,
                CAIRO_FORMAT_ARGB32, image->width, image->height,
                image->rowstride);
            pattern = cairo_pattern_create_for_surface (src_surface);
	    cairo_surface_destroy (src_surface);
            cairo_pattern_set_matrix(pattern, &mat);
            if (shapevec->fill_type == 0x40 || shapevec->fill_type == 0x42) {
              cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
            } else {
              cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);
            }
            if (shapevec->fill_type == 0x40 || shapevec->fill_type == 0x41) {
              cairo_pattern_set_filter (pattern, CAIRO_FILTER_BILINEAR);
            } else {
              cairo_pattern_set_filter (pattern, CAIRO_FILTER_NEAREST);
            }
            cairo_set_source (cr, pattern);
	    cairo_pattern_destroy (pattern);
          } else {
            swfdec_color_set_source (cr, color);
          }
        }
        break;
      case 0x00:
        swfdec_color_set_source (cr, color);
        break;
      default:
        SWFDEC_ERROR("unhandled fill type 0x%02x", shapevec->fill_type);
        cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.5);
        break;
    }
    
    if (shapevec->path.num_data || shapevec2->path.num_data) {
      cairo_new_path (cr);
      if (shapevec->path.num_data)
	cairo_append_path (cr, &shapevec->path);
      if (shapevec2->path.num_data)
	cairo_append_path (cr, &shapevec2->path);
      cairo_fill (cr);
    }
  }

  for (i = 0; i < shape->lines->len; i++) {
    swf_color color;

    shapevec = g_ptr_array_index (shape->lines, i);

    color = swfdec_color_apply_transform (shapevec->color, trans);
    swfdec_color_set_source (cr, color);

    cairo_set_line_width (cr, shapevec->width);
    if (shapevec->path.num_data) {
      cairo_new_path (cr);
      cairo_append_path (cr, &shapevec->path);
      cairo_stroke (cr);
    }
  }
}

static SwfdecMouseResult 
swfdec_shape_handle_mouse (SwfdecDecoder *decoder, SwfdecObject *object,
      double x, double y, int button, SwfdecRect *inval)
{
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;
  SwfdecShape *shape = SWFDEC_SHAPE (object);
  static cairo_surface_t *surface = NULL;

  if (shape->fill_cr == NULL) {
    /* FIXME: do less memeory intensive fill checking plz */
    if (surface == NULL)
      surface = cairo_image_surface_create (CAIRO_FORMAT_A8, 1000, 1000);
    guint i;
    shape->fill_cr = cairo_create (surface);
    cairo_set_fill_rule (shape->fill_cr, CAIRO_FILL_RULE_EVEN_ODD);
    for (i = 0; i < shape->fills->len; i++) {
      shapevec = g_ptr_array_index (shape->fills, i);
      shapevec2 = g_ptr_array_index (shape->fills2, i);
      if (shapevec->path.num_data || shapevec2->path.num_data) {
	cairo_new_path (shape->fill_cr);
	if (shapevec->path.num_data)
	  cairo_append_path (shape->fill_cr, &shapevec->path);
	if (shapevec2->path.num_data)
	  cairo_append_path (shape->fill_cr, &shapevec2->path);
	cairo_fill (shape->fill_cr);
      }
    }
  }
  /* FIXME: handle strokes */
  if (cairo_in_fill (shape->fill_cr, x, y)) {
    g_print ("HIT %d\n", object->id);
    return SWFDEC_MOUSE_HIT;
  } else {
    return SWFDEC_MOUSE_MISSED;
  }
}

static void swfdec_shapevec_free (SwfdecShapeVec * shapevec);


SWFDEC_OBJECT_BOILERPLATE (SwfdecShape, swfdec_shape)

     static void swfdec_shape_base_init (gpointer g_class)
{

}

static void
swfdec_shape_class_init (SwfdecShapeClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);
  
  object_class->render = swfdec_shape_render;
  object_class->handle_mouse = swfdec_shape_handle_mouse;
}

static void
swfdec_shape_init (SwfdecShape * shape)
{
  shape->fills = g_ptr_array_new ();
  shape->fills2 = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();
}

static void
swfdec_shape_dispose (SwfdecShape * shape)
{
  SwfdecShapeVec *shapevec;
  unsigned int i;

  if (shape->fill_cr)
    cairo_destroy (shape->fill_cr);

  for (i = 0; i < shape->fills->len; i++) {
    shapevec = g_ptr_array_index (shape->fills, i);
    swfdec_shapevec_free (shapevec);
  }
  g_ptr_array_free (shape->fills, TRUE);

  for (i = 0; i < shape->fills2->len; i++) {
    shapevec = g_ptr_array_index (shape->fills2, i);
    swfdec_shapevec_free (shapevec);
  }
  g_ptr_array_free (shape->fills2, TRUE);

  for (i = 0; i < shape->lines->len; i++) {
    shapevec = g_ptr_array_index (shape->lines, i);
    swfdec_shapevec_free (shapevec);
  }
  g_ptr_array_free (shape->lines, TRUE);

}

static void
swfdec_shapevec_free (SwfdecShapeVec * shapevec)
{
  if (shapevec->grad) {
    g_free (shapevec->grad);
  }
  swfdec_path_reset (&shapevec->path);
  g_free (shapevec);
}

#if 0
static int
get_shape_rec (SwfdecBits * bits, int n_fill_bits, int n_line_bits)
{
  int type;
  int state_new_styles;
  int state_line_styles;
  int state_fill_styles1;
  int state_fill_styles0;
  int state_moveto;
  int movebits = 0;
  int movex;
  int movey;
  int fill0style;
  int fill1style;
  int linestyle = 0;

  type = swfdec_bits_peekbits (bits, 6);
  if (type == 0) {
    swfdec_bits_getbits (bits, 6);
    return 0;
  }

  type = swfdec_bits_getbits (bits, 1);
  SWFDEC_DEBUG ("   type = %d\n", type);

  if (type == 0) {
    state_new_styles = swfdec_bits_getbits (bits, 1);
    state_line_styles = swfdec_bits_getbits (bits, 1);
    state_fill_styles1 = swfdec_bits_getbits (bits, 1);
    state_fill_styles0 = swfdec_bits_getbits (bits, 1);
    state_moveto = swfdec_bits_getbits (bits, 1);

    SWFDEC_DEBUG ("   state_new_styles = %d\n", state_new_styles);
    SWFDEC_DEBUG ("   state_line_styles = %d\n", state_line_styles);
    SWFDEC_DEBUG ("   state_fill_styles1 = %d\n", state_fill_styles1);
    SWFDEC_DEBUG ("   state_fill_styles0 = %d\n", state_fill_styles0);
    SWFDEC_DEBUG ("   state_moveto = %d\n", state_moveto);

    if (state_moveto) {
      movebits = swfdec_bits_getbits (bits, 5);
      SWFDEC_DEBUG ("   movebits = %d\n", movebits);
      movex = swfdec_bits_getsbits (bits, movebits);
      movey = swfdec_bits_getsbits (bits, movebits);
      SWFDEC_DEBUG ("   movex = %d\n", movex);
      SWFDEC_DEBUG ("   movey = %d\n", movey);
    }
    if (state_fill_styles0) {
      fill0style = swfdec_bits_getbits (bits, n_fill_bits);
      SWFDEC_DEBUG ("   fill0style = %d\n", fill0style);
    }
    if (state_fill_styles1) {
      fill1style = swfdec_bits_getbits (bits, n_fill_bits);
      SWFDEC_DEBUG ("   fill1style = %d\n", fill1style);
    }
    if (state_line_styles) {
      linestyle = swfdec_bits_getbits (bits, n_line_bits);
      SWFDEC_DEBUG ("   linestyle = %d\n", linestyle);
    }
    if (state_new_styles) {
      SWFDEC_DEBUG ("***** new styles not implemented\n");
    }


  } else {
    /* edge record */
    int n_bits;
    int edge_flag;

    edge_flag = swfdec_bits_getbits (bits, 1);
    SWFDEC_DEBUG ("   edge_flag = %d\n", edge_flag);

    if (edge_flag == 0) {
      int control_delta_x;
      int control_delta_y;
      int anchor_delta_x;
      int anchor_delta_y;

      n_bits = swfdec_bits_getbits (bits, 4) + 2;
      control_delta_x = swfdec_bits_getsbits (bits, n_bits);
      control_delta_y = swfdec_bits_getsbits (bits, n_bits);
      anchor_delta_x = swfdec_bits_getsbits (bits, n_bits);
      anchor_delta_y = swfdec_bits_getsbits (bits, n_bits);

      SWFDEC_DEBUG ("   n_bits = %d\n", n_bits);
      SWFDEC_DEBUG ("   control_delta = %d,%d\n", control_delta_x,
          control_delta_y);
      SWFDEC_DEBUG ("   anchor_delta = %d,%d\n", anchor_delta_x,
          anchor_delta_y);
    } else {
      int general_line_flag;
      int delta_x;
      int delta_y;
      int vert_line_flag = 0;

      n_bits = swfdec_bits_getbits (bits, 4) + 2;
      general_line_flag = swfdec_bits_getbit (bits);
      if (general_line_flag == 1) {
        delta_x = swfdec_bits_getsbits (bits, n_bits);
        delta_y = swfdec_bits_getsbits (bits, n_bits);
      } else {
        vert_line_flag = swfdec_bits_getbit (bits);
        if (vert_line_flag == 0) {
          delta_x = swfdec_bits_getsbits (bits, n_bits);
          delta_y = 0;
        } else {
          delta_x = 0;
          delta_y = swfdec_bits_getsbits (bits, n_bits);
        }
      }
      SWFDEC_DEBUG ("   general_line_flag = %d\n", general_line_flag);
      if (general_line_flag == 0) {
        SWFDEC_DEBUG ("   vert_line_flag = %d\n", vert_line_flag);
      }
      SWFDEC_DEBUG ("   n_bits = %d\n", n_bits);
      SWFDEC_DEBUG ("   delta_x = %d\n", delta_x);
      SWFDEC_DEBUG ("   delta_y = %d\n", delta_y);
    }
  }

  return 1;
}
#endif

SwfdecShapeVec *
swf_shape_vec_new (void)
{
  SwfdecShapeVec *shapevec;

  shapevec = g_new0 (SwfdecShapeVec, 1);
  swfdec_path_init (&shapevec->path);

  return shapevec;
}

int
tag_define_shape (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int id;

  id = swfdec_bits_get_u16 (bits);

  shape = swfdec_object_new (s, SWFDEC_TYPE_SHAPE);
  SWFDEC_OBJECT (shape)->id = id;
  s->objects = g_list_append (s->objects, shape);

  SWFDEC_INFO ("id=%d", id);

  swfdec_bits_get_rect (bits, &SWFDEC_OBJECT (shape)->extents);

  shape->fills = g_ptr_array_new ();
  shape->fills2 = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();

  swf_shape_add_styles (s, shape, bits);

  swf_shape_get_recs (s, bits, shape, FALSE);

  return SWF_OK;
}

int
tag_define_shape_3 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int id;

  id = swfdec_bits_get_u16 (bits);
  shape = swfdec_object_new (s, SWFDEC_TYPE_SHAPE);
  SWFDEC_OBJECT (shape)->id = id;
  s->objects = g_list_append (s->objects, shape);

  SWFDEC_INFO ("id=%d", id);

  swfdec_bits_get_rect (bits, &SWFDEC_OBJECT (shape)->extents);

  shape->fills = g_ptr_array_new ();
  shape->fills2 = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();

  shape->rgba = 1;

  swf_shape_add_styles (s, shape, bits);

  swf_shape_get_recs (s, bits, shape, FALSE);

  return SWF_OK;
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
    int fill_style_type;
    SwfdecShapeVec *shapevec;

    SWFDEC_LOG ("   fill style %d:", i);

    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills2, shapevec);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);

    shapevec->color = SWF_COLOR_COMBINE (0, 255, 0, 255);

    fill_style_type = swfdec_bits_get_u8 (bits);
    SWFDEC_LOG ("    type 0x%02x", fill_style_type);
    if (fill_style_type == 0x00) {
      shapevec->fill_type = fill_style_type;
      if (shape->rgba) {
        shapevec->color = swfdec_bits_get_rgba (bits);
      } else {
        shapevec->color = swfdec_bits_get_color (bits);
      }
      SWFDEC_LOG ("    color %08x", shapevec->color);
    } else if (fill_style_type == 0x10 || fill_style_type == 0x12) {
      shapevec->fill_type = fill_style_type;
      swfdec_bits_get_matrix (bits, &shapevec->fill_transform);
      if (shape->rgba) {
        shapevec->grad = swfdec_bits_get_gradient_rgba (bits);
      } else {
        shapevec->grad = swfdec_bits_get_gradient (bits);
      }
      swfdec_bits_syncbits (bits);
      cairo_matrix_scale (&shapevec->fill_transform, SWF_SCALE_FACTOR, SWF_SCALE_FACTOR);
    } else if (fill_style_type >= 0x40 && fill_style_type <= 0x43) {
      shapevec->fill_type = fill_style_type;
      shapevec->fill_id = swfdec_bits_get_u16 (bits);
      SWFDEC_LOG ("   background fill id = %d (type 0x%02x)",
          shapevec->fill_id, fill_style_type);

      if (shapevec->fill_id == 65535) {
        shapevec->fill_id = 0;
        shapevec->color = SWF_COLOR_COMBINE (0, 255, 255, 255);
      }

      swfdec_bits_get_matrix (bits, &shapevec->fill_transform);
      swfdec_bits_syncbits (bits);
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

    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->lines, shapevec);

    shapevec->width = swfdec_bits_get_u16 (bits) * SWF_SCALE_FACTOR;
    if (shape->rgba) {
      shapevec->color = swfdec_bits_get_rgba (bits);
    } else {
      shapevec->color = swfdec_bits_get_color (bits);
    }
    SWFDEC_LOG ("%d: %g %08x", i, shapevec->width, shapevec->color);
  }

  swfdec_bits_syncbits (bits);
  shape->n_fill_bits = swfdec_bits_getbits (bits, 4);
  shape->n_line_bits = swfdec_bits_getbits (bits, 4);
}

static SwfdecShapeVec *
swfdec_shape_get_fill0style (SwfdecShape * shape, int fill0style)
{
  if (fill0style < 1)
    return NULL;
  if (shape->fills_offset + fill0style - 1 >= shape->fills->len) {
    SWFDEC_WARNING ("fill0style too large (%d >= %d)",
        shape->fills_offset + fill0style - 1, shape->fills->len);
    return NULL;
  }
  return g_ptr_array_index (shape->fills, shape->fills_offset + fill0style - 1);
}

static SwfdecShapeVec *
swfdec_shape_get_fill1style (SwfdecShape * shape, int fill1style)
{
  if (fill1style < 1)
    return NULL;
  if (shape->fills_offset + fill1style - 1 >= shape->fills2->len) {
    SWFDEC_WARNING ("fill1style too large (%d >= %d)",
        shape->fills_offset + fill1style - 1, shape->fills2->len);
    return NULL;
  }
  return g_ptr_array_index (shape->fills2,
      shape->fills_offset + fill1style - 1);
}

static SwfdecShapeVec *
swfdec_shape_get_linestyle (SwfdecShape * shape, int linestyle)
{
  if (linestyle < 1)
    return NULL;
  if (shape->lines_offset + linestyle - 1 >= shape->lines->len) {
    SWFDEC_WARNING ("linestyle too large (%d >= %d)",
        shape->lines_offset + linestyle - 1, shape->lines->len);
    return NULL;
  }
  return g_ptr_array_index (shape->lines, shape->lines_offset + linestyle - 1);
}

int
tag_define_shape_2 (SwfdecDecoder * s)
{
  return tag_define_shape (s);
}

unsigned char *
swfdec_gradient_to_palette (SwfdecGradient * grad,
    SwfdecColorTransform * color_transform)
{
  swf_color color;
  unsigned char *p;
  int i, j;

  p = g_malloc (256 * 4);

  color = swfdec_color_apply_transform (grad->array[0].color, color_transform);
  if (grad->array[0].ratio > 256) {
    SWFDEC_ERROR ("gradient ratio > 256 (%d)", grad->array[0].ratio);
  }
  for (i = 0; i < grad->array[0].ratio; i++) {
    p[i * 4 + 0] = SWF_COLOR_B (color);
    p[i * 4 + 1] = SWF_COLOR_G (color);
    p[i * 4 + 2] = SWF_COLOR_R (color);
    p[i * 4 + 3] = SWF_COLOR_A (color);
  }

  for (j = 0; j < grad->n_gradients - 1; j++) {
    double len = grad->array[j + 1].ratio - grad->array[j].ratio;
    double x;
    swf_color color0 = swfdec_color_apply_transform (grad->array[j].color,
        color_transform);
    swf_color color1 = swfdec_color_apply_transform (grad->array[j + 1].color,
        color_transform);

    for (i = grad->array[j].ratio; i < grad->array[j + 1].ratio; i++) {
      x = (i - grad->array[j].ratio) / len;

      p[i * 4 + 0] = SWF_COLOR_B (color0) * (1 - x) + SWF_COLOR_B (color1) * x;
      p[i * 4 + 1] = SWF_COLOR_G (color0) * (1 - x) + SWF_COLOR_G (color1) * x;
      p[i * 4 + 2] = SWF_COLOR_R (color0) * (1 - x) + SWF_COLOR_R (color1) * x;
      p[i * 4 + 3] = SWF_COLOR_A (color0) * (1 - x) + SWF_COLOR_A (color1) * x;
    }
  }

  color = swfdec_color_apply_transform (grad->array[j].color, color_transform);
  for (i = grad->array[j].ratio; i < 256; i++) {
    p[i * 4 + 0] = SWF_COLOR_B (color);
    p[i * 4 + 1] = SWF_COLOR_G (color);
    p[i * 4 + 2] = SWF_COLOR_R (color);
    p[i * 4 + 3] = SWF_COLOR_A (color);
  }

  return p;
}

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

  shape->fills = g_ptr_array_new ();
  shape->fills2 = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();

  swf_morphshape_add_styles (s, shape, bits);

  swfdec_bits_syncbits (bits);
  swf_shape_get_recs (s, bits, shape, TRUE);
  swfdec_bits_syncbits (bits);
  if (1) {
    g_assert_not_reached ();
    swf_shape_get_recs (s, bits, shape, TRUE);
  }

  return SWF_OK;
}

void
swf_shape_get_recs (SwfdecDecoder * s, SwfdecBits * bits,
    SwfdecShape * shape, gboolean morphshape)
{
  int x = 0, y = 0;
  SwfdecShapeVec *fill0style = NULL;
  SwfdecShapeVec *fill1style = NULL;
  SwfdecShapeVec *linestyle = NULL;

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

      if (state_moveto) {
        n_bits = swfdec_bits_getbits (bits, 5);
        x = swfdec_bits_getsbits (bits, n_bits);
        y = swfdec_bits_getsbits (bits, n_bits);

        SWFDEC_LOG ("   moveto %d,%d", x, y);
      }
      if (state_fill_styles0) {
        state_fill_styles0 = swfdec_bits_getbits (bits, shape->n_fill_bits);
        SWFDEC_LOG ("   * fill0style = %d", state_fill_styles0);
      } else {
	state_fill_styles0 = -1;
	SWFDEC_LOG ("   * not changing fill0style");
      }
      if (state_fill_styles1) {
        state_fill_styles1 = swfdec_bits_getbits (bits, shape->n_fill_bits);
        SWFDEC_LOG ("   * fill1style = %d", state_fill_styles1);
      } else {
	state_fill_styles1 = -1;
	SWFDEC_LOG ("   * not changing fill1style");
      }
      if (state_line_styles) {
        state_line_styles = swfdec_bits_getbits (bits, shape->n_line_bits);
        SWFDEC_LOG ("   * linestyle = %d", state_line_styles);
      } else {
	state_line_styles = -1;
	SWFDEC_LOG ("   * not changing linestyle");
      }
      if (state_new_styles) {
        SWFDEC_LOG ("   * new styles");
	if (morphshape)
	  swf_morphshape_add_styles (s, shape, bits);
	else
	  swf_shape_add_styles (s, shape, bits);
      }
      /* FIXME: reset or ignore when not set? Currently we ignore */
      if (state_fill_styles0 >= 0)
	fill0style = swfdec_shape_get_fill0style (shape, state_fill_styles0);
      if (state_fill_styles1 >= 0)
	fill1style = swfdec_shape_get_fill1style (shape, state_fill_styles1);
      if (state_line_styles >= 0)
	linestyle = swfdec_shape_get_linestyle (shape, state_line_styles);
      if (fill0style)
	swfdec_path_move_to (&fill0style->path, x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
      if (fill1style)
	swfdec_path_move_to (&fill1style->path, x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
      if (linestyle)
	swfdec_path_move_to (&linestyle->path, x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
    } else {
      /* edge record */
      int n_bits;
      int edge_flag;

      edge_flag = swfdec_bits_getbits (bits, 1);

      if (edge_flag == 0) {
        int control_x, control_y;

        n_bits = swfdec_bits_getbits (bits, 4) + 2;

        x += swfdec_bits_getsbits (bits, n_bits);
        y += swfdec_bits_getsbits (bits, n_bits);
        SWFDEC_LOG ("   control %d,%d", x, y);
        control_x = x;
        control_y = y;

        x += swfdec_bits_getsbits (bits, n_bits);
        y += swfdec_bits_getsbits (bits, n_bits);
        SWFDEC_LOG ("   anchor %d,%d", x, y);
	if (fill0style)
	  swfdec_path_curve_to (&fill0style->path, 
	      control_x * SWF_SCALE_FACTOR, control_y * SWF_SCALE_FACTOR, 
	      x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
	if (fill1style)
	  swfdec_path_curve_to (&fill1style->path,
	      control_x * SWF_SCALE_FACTOR, control_y * SWF_SCALE_FACTOR, 
	      x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
	if (linestyle)
	  swfdec_path_curve_to (&linestyle->path,
	      control_x * SWF_SCALE_FACTOR, control_y * SWF_SCALE_FACTOR, 
	      x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
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
	if (fill0style)
	  swfdec_path_line_to (&fill0style->path, x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
	if (fill1style)
	  swfdec_path_line_to (&fill1style->path, x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
	if (linestyle)
	  swfdec_path_line_to (&linestyle->path, x * SWF_SCALE_FACTOR, y * SWF_SCALE_FACTOR);
      }
    }
  }

  swfdec_bits_getbits (bits, 6);
  swfdec_bits_syncbits (bits);
}

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
    g_ptr_array_add (shape->fills2, shapevec);
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

    shapevec->width = swfdec_bits_get_u16 (bits) * SWF_SCALE_FACTOR;
    end_width = swfdec_bits_get_u16 (bits) * SWF_SCALE_FACTOR;
    shapevec->color = swfdec_bits_get_rgba (bits);
    end_color = swfdec_bits_get_rgba (bits);
    SWFDEC_LOG ("%d: %g->%g %08x->%08x", i,
        shapevec->width, end_width, shapevec->color, end_color);
  }

  swfdec_bits_syncbits (bits);
}

