

#include <swfdec_shape.h>

#include <math.h>

#include "swfdec_internal.h"
#include <swfdec_render_libart.h>


static void swfdec_shapevec_free (SwfdecShapeVec *shapevec);


SWFDEC_OBJECT_BOILERPLATE (SwfdecShape, swfdec_shape)

static void swfdec_shape_base_init (gpointer g_class)
{

}

static void swfdec_shape_class_init (SwfdecShapeClass *g_class)
{
  SWFDEC_OBJECT_CLASS (g_class)->render = swfdec_shape_render;
}

static void swfdec_shape_init (SwfdecShape *shape)
{
  shape->fills = g_ptr_array_new();
  shape->fills2 = g_ptr_array_new();
  shape->lines = g_ptr_array_new();
}

static void swfdec_shape_dispose (SwfdecShape *shape)
{
  SwfdecShapeVec *shapevec;
  int i;

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

static void swfdec_shapevec_free (SwfdecShapeVec *shapevec)
{
  if (shapevec->grad) {
    g_free (shapevec->grad);
  }
  g_array_free (shapevec->path, TRUE);
  g_free (shapevec);
}

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
      SWFDEC_DEBUG ("   control_delta = %d,%d\n", control_delta_x, control_delta_y);
      SWFDEC_DEBUG ("   anchor_delta = %d,%d\n", anchor_delta_x, anchor_delta_y);
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

int
tag_func_define_shape (SwfdecDecoder * s)
{
  SwfdecBits *b = &s->b;
  int id;
  int n_fill_styles;
  int n_line_styles;
  int n_fill_bits;
  int n_line_bits;
  int rect[4];
  int i;

  id = swfdec_bits_get_u16 (b);
  SWFDEC_INFO("id=%d", id);
  SWFDEC_DEBUG ("  bounds = %s\n", "rect");
  swfdec_bits_get_rect (b, rect);
  swfdec_bits_syncbits (b);
  n_fill_styles = swfdec_bits_get_u8 (b);
  SWFDEC_LOG("  n_fill_styles = %d", n_fill_styles);
  for (i = 0; i < n_fill_styles; i++) {
    swfdec_bits_get_fill_style (b);
  }
  swfdec_bits_syncbits (b);
  n_line_styles = swfdec_bits_get_u8 (b);
  SWFDEC_LOG("  n_line_styles = %d", n_line_styles);
  for (i = 0; i < n_line_styles; i++) {
    swfdec_bits_get_line_style (b);
  }
  swfdec_bits_syncbits (b);
  n_fill_bits = swfdec_bits_getbits (b, 4);
  n_line_bits = swfdec_bits_getbits (b, 4);
  SWFDEC_LOG("  n_fill_bits = %d", n_fill_bits);
  SWFDEC_LOG("  n_line_bits = %d", n_line_bits);
  do {
    SWFDEC_LOG("  shape_rec:");
  } while (get_shape_rec (b, n_fill_bits, n_line_bits));

  swfdec_bits_syncbits (b);

  return SWF_OK;
}

SwfdecShapeVec *
swf_shape_vec_new (void)
{
  SwfdecShapeVec *shapevec;

  shapevec = g_new0 (SwfdecShapeVec, 1);

  shapevec->path = g_array_new (FALSE, TRUE, sizeof (SwfdecShapePoint));

  return shapevec;
}

int
tag_define_shape (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int rect[4];
  int id;

  id = swfdec_bits_get_u16 (bits);

  shape = swfdec_object_new (SWFDEC_TYPE_SHAPE);
  SWFDEC_OBJECT (shape)->id = id;
  s->objects = g_list_append (s->objects, shape);

  SWFDEC_INFO("id=%d", id);

  swfdec_bits_get_rect (bits, rect);

  shape->fills = g_ptr_array_new ();
  shape->fills2 = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();

  swf_shape_add_styles (s, shape, bits);

  swf_shape_get_recs (s, bits, shape);

  return SWF_OK;
}

int
tag_define_shape_3 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int rect[4];
  int id;

  id = swfdec_bits_get_u16 (bits);
  shape = swfdec_object_new (SWFDEC_TYPE_SHAPE);
  SWFDEC_OBJECT (shape)->id = id;
  s->objects = g_list_append (s->objects, shape);

  SWFDEC_INFO("id=%d", id);

  swfdec_bits_get_rect (bits, rect);

  shape->fills = g_ptr_array_new ();
  shape->fills2 = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();

  shape->rgba = 1;

  swf_shape_add_styles (s, shape, bits);

  swf_shape_get_recs (s, bits, shape);

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
  SWFDEC_LOG("   n_fill_styles %d", n_fill_styles);
  for (i = 0; i < n_fill_styles; i++) {
    int fill_style_type;
    SwfdecShapeVec *shapevec;

    SWFDEC_LOG("   fill style %d:", i);

    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills2, shapevec);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);

    shapevec->color = SWF_COLOR_COMBINE (0, 255, 0, 255);

    fill_style_type = swfdec_bits_get_u8 (bits);
    SWFDEC_LOG("    type 0x%02x", fill_style_type);
    if (fill_style_type == 0x00) {
      if (shape->rgba) {
	shapevec->color = swfdec_bits_get_rgba (bits);
      } else {
	shapevec->color = swfdec_bits_get_color (bits);
      }
      SWFDEC_LOG("    color %08x", shapevec->color);
    } else if (fill_style_type == 0x10 || fill_style_type == 0x12) {
      shapevec->fill_type = fill_style_type;
      swfdec_bits_get_transform (bits, &shapevec->fill_transform);
      if (shape->rgba) {
	shapevec->grad = swfdec_bits_get_gradient_rgba (bits);
      } else {
	shapevec->grad = swfdec_bits_get_gradient (bits);
      }
      shapevec->fill_transform.trans[0] *= SWF_SCALE_FACTOR;
      shapevec->fill_transform.trans[1] *= SWF_SCALE_FACTOR;
      shapevec->fill_transform.trans[2] *= SWF_SCALE_FACTOR;
      shapevec->fill_transform.trans[3] *= SWF_SCALE_FACTOR;
    } else if (fill_style_type == 0x40 || fill_style_type == 0x41) {
      shapevec->fill_type = fill_style_type;
      shapevec->fill_id = swfdec_bits_get_u16 (bits);
      SWFDEC_LOG("   background fill id = %d (type 0x%02x)",
	  shapevec->fill_id, fill_style_type);

      if (shapevec->fill_id == 65535) {
	shapevec->fill_id = 0;
	shapevec->color = SWF_COLOR_COMBINE (0, 255, 255, 255);
      }

      swfdec_bits_get_transform (bits, &shapevec->fill_transform);
      /* FIXME: the 0.965 is a mysterious factor that seems to improve
       * rendering of images. */
      shapevec->fill_transform.trans[0] *= SWF_SCALE_FACTOR * 0.965;
      shapevec->fill_transform.trans[1] *= SWF_SCALE_FACTOR * 0.965;
      shapevec->fill_transform.trans[2] *= SWF_SCALE_FACTOR * 0.965;
      shapevec->fill_transform.trans[3] *= SWF_SCALE_FACTOR * 0.965;
    } else {
      SWFDEC_ERROR ("unknown fill style type 0x%02x", fill_style_type);
    }
  }

  swfdec_bits_syncbits (bits);
  shape->lines_offset = shape->lines->len;
  n_line_styles = swfdec_bits_get_u8 (bits);
  if (n_line_styles == 0xff) {
    n_line_styles = swfdec_bits_get_u16 (bits);
  }
  SWFDEC_LOG("   n_line_styles %d", n_line_styles);
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
    SWFDEC_LOG("%d: %g %08x", i, shapevec->width, shapevec->color);
  }

  swfdec_bits_syncbits (bits);
  shape->n_fill_bits = swfdec_bits_getbits (bits, 4);
  shape->n_line_bits = swfdec_bits_getbits (bits, 4);
}

static SwfdecShapeVec *swfdec_shape_get_fill0style(SwfdecShape *shape, int fill0style)
{
  if (shape->fills_offset + fill0style - 1 >= shape->fills->len) {
    SWFDEC_WARNING ("fill0style too large (%d >= %d)",
        shape->fills_offset + fill0style - 1, shape->fills->len);
    return NULL;
  }
  if (fill0style < 1) return NULL;
  return g_ptr_array_index (shape->fills,
      shape->fills_offset + fill0style - 1);
}

static SwfdecShapeVec *swfdec_shape_get_fill1style(SwfdecShape *shape, int fill1style)
{
  if (shape->fills_offset + fill1style - 1 >= shape->fills2->len) {
    SWFDEC_WARNING ("fill1style too large (%d >= %d)",
        shape->fills_offset + fill1style - 1, shape->fills2->len);
    return NULL;
  }
  if (fill1style < 1) return NULL;
  return g_ptr_array_index (shape->fills2,
      shape->fills_offset + fill1style - 1);
}

static SwfdecShapeVec *swfdec_shape_get_linestyle(SwfdecShape *shape, int linestyle)
{
  if (shape->lines_offset + linestyle - 1 >= shape->lines->len) {
    SWFDEC_WARNING ("linestyle too large (%d >= %d)",
        shape->lines_offset + linestyle - 1, shape->lines->len);
    return NULL;
  }
  if (linestyle < 1) return NULL;
  return g_ptr_array_index (shape->lines, shape->lines_offset + linestyle - 1);
}

void
swf_shape_get_recs (SwfdecDecoder * s, SwfdecBits * bits, SwfdecShape * shape)
{
  int x = 0, y = 0;
  int fill0style = 0;
  int fill1style = 0;
  int linestyle = 0;
  int n_vec = 0;
  SwfdecShapeVec *shapevec;
  SwfdecShapePoint pt;

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

	SWFDEC_LOG("   moveto %d,%d", x, y);
      }
      if (state_fill_styles0) {
	fill0style = swfdec_bits_getbits (bits, shape->n_fill_bits);
	SWFDEC_LOG("   * fill0style = %d", fill0style);
      }
      if (state_fill_styles1) {
	fill1style = swfdec_bits_getbits (bits, shape->n_fill_bits);
	SWFDEC_LOG("   * fill1style = %d", fill1style);
      }
      if (state_line_styles) {
	linestyle = swfdec_bits_getbits (bits, shape->n_line_bits);
	SWFDEC_LOG("   * linestyle = %d", linestyle);
      }
      if (state_new_styles) {
	swf_shape_add_styles (s, shape, bits);
	SWFDEC_LOG("swf_shape_get_recs: new styles");
      }
      pt.control_x = SWFDEC_SHAPE_POINT_SPECIAL;
      pt.control_y = SWFDEC_SHAPE_POINT_MOVETO;
      pt.to_x = x;
      pt.to_y = y;
    } else {
      /* edge record */
      int n_bits;
      int edge_flag;

      edge_flag = swfdec_bits_getbits (bits, 1);

      if (edge_flag == 0) {
	int16_t x0, y0;
	int16_t x1, y1;
	int16_t x2, y2;

	x0 = x;
	y0 = y;
	n_bits = swfdec_bits_getbits (bits, 4) + 2;

	x += swfdec_bits_getsbits (bits, n_bits);
	y += swfdec_bits_getsbits (bits, n_bits);
	SWFDEC_LOG("   control %d,%d", x, y);
	x1 = x;
	y1 = y;

	x += swfdec_bits_getsbits (bits, n_bits);
	y += swfdec_bits_getsbits (bits, n_bits);
	SWFDEC_LOG("   anchor %d,%d", x, y);
	x2 = x;
	y2 = y;

        pt.control_x = x1;
        pt.control_y = y1;
        pt.to_x = x2;
        pt.to_y = y2;
	n_vec++;
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
	SWFDEC_LOG("   delta %d,%d", x, y);

        pt.control_x = SWFDEC_SHAPE_POINT_SPECIAL;
        pt.control_y = SWFDEC_SHAPE_POINT_LINETO;
        pt.to_x = x;
        pt.to_y = y;
      }
    }
    if (fill0style) {
      shapevec = swfdec_shape_get_fill0style (shape, fill0style);
      if (shapevec) {
        g_array_append_val (shapevec->path, pt);
      }
      s->stats_n_points++;
    }
    if (fill1style) {
      shapevec = swfdec_shape_get_fill1style (shape, fill1style);
      if (shapevec) {
        g_array_append_val (shapevec->path, pt);
      }
      s->stats_n_points++;
    }
    if (linestyle) {
      shapevec = swfdec_shape_get_linestyle(shape, linestyle);
      if (shapevec) {
        g_array_append_val (shapevec->path, pt);
      }
      s->stats_n_points++;
    }

  }

  swfdec_bits_getbits (bits, 6);
  swfdec_bits_syncbits (bits);
}

int
tag_define_shape_2 (SwfdecDecoder * s)
{
  return tag_define_shape (s);
}

void
swfdec_shape_compose (SwfdecDecoder * s, SwfdecLayerVec * layervec,
    SwfdecShapeVec * shapevec, SwfdecTransform *trans)
{
  SwfdecObject *image_object;
  SwfdecImage *image;
  SwfdecTransform mat;
  SwfdecTransform mat0;
  int i, j;
  unsigned char *dest;
  unsigned char *src;
  double inv_width, inv_height;
  int width, height;

  image_object = swfdec_object_get (s, shapevec->fill_id);
  if (!image_object)
    return;

  if (!SWFDEC_IS_IMAGE (image_object)) {
    SWFDEC_WARNING ("compose object is not image");
    return;
  }

  SWFDEC_LOG("swfdec_shape_compose: %d", shapevec->fill_id);

  layervec->color = SWF_COLOR_COMBINE (255, 0, 0, 255);

  image = SWFDEC_IMAGE (image_object);
  SWFDEC_LOG("image %p", image->image_data);

  SWFDEC_LOG("%g %g %g %g %g %g",
      shapevec->fill_transform.trans[0],
      shapevec->fill_transform.trans[1],
      shapevec->fill_transform.trans[2],
      shapevec->fill_transform.trans[3],
      shapevec->fill_transform.trans[4], shapevec->fill_transform.trans[5]);

  width = layervec->rect.x1 - layervec->rect.x0;
  height = layervec->rect.y1 - layervec->rect.y0;

  layervec->compose = g_malloc (width * height * 4);
  layervec->compose_rowstride = width * 4;
  layervec->compose_height = height;
  layervec->compose_width = width;

  swfdec_transform_multiply (&mat0, &shapevec->fill_transform, trans);

  /* Need an offset in the compose information */
  mat0.trans[4] -= layervec->rect.x0;
  mat0.trans[5] -= layervec->rect.y0;
  swfdec_transform_invert (&mat, &mat0);
  dest = layervec->compose;
  src = image->image_data;
  inv_width = 1.0 / image->width;
  inv_height = 1.0 / image->height;
  for (j = 0; j < height; j++) {
    double x, y;

    x = mat.trans[2] * j + mat.trans[4];
    y = mat.trans[3] * j + mat.trans[5];
    for (i = 0; i < width; i++) {
      int ix, iy;

#if 0
      ix = x - floor (x * inv_width) * image->width;
      iy = y - floor (y * inv_height) * image->height;
#else
      ix = x;
      iy = y;
      if (x < 0)
	ix = 0;
      if (x > image->width - 1)
	ix = image->width - 1;
      if (y < 0)
	iy = 0;
      if (y > image->height - 1)
	iy = image->height - 1;

#endif
#define RGBA8888_COPY(a,b) (*(guint32 *)(a) = *(guint32 *)(b))
      RGBA8888_COPY (dest, src + ix * 4 + iy * image->rowstride);
      dest += 4;
      x += mat.trans[0];
      y += mat.trans[1];

    }
  }

}

void
swfdec_shape_compose_gradient (SwfdecDecoder * s, SwfdecLayerVec * layervec,
    SwfdecShapeVec * shapevec, SwfdecTransform *trans, SwfdecSpriteSegment * seg)
{
  SwfdecGradient *grad;
  SwfdecTransform mat;
  SwfdecTransform mat0;
  int i, j;
  unsigned char *dest;
  unsigned char *palette;
  int width, height;

  SWFDEC_LOG("swfdec_shape_compose: %d", shapevec->fill_id);

  grad = shapevec->grad;

  SWFDEC_LOG("%g %g %g %g %g %g",
      shapevec->fill_transform.trans[0],
      shapevec->fill_transform.trans[1],
      shapevec->fill_transform.trans[2],
      shapevec->fill_transform.trans[3],
      shapevec->fill_transform.trans[4], shapevec->fill_transform.trans[5]);

  width = layervec->rect.x1 - layervec->rect.x0;
  height = layervec->rect.y1 - layervec->rect.y0;

  layervec->compose = g_malloc (width * height * 4);
  layervec->compose_rowstride = width * 4;
  layervec->compose_height = height;
  layervec->compose_width = width;

  swfdec_transform_multiply (&mat0, &shapevec->fill_transform, trans);

  palette = swfdec_gradient_to_palette (grad, &seg->color_transform);

  mat0.trans[4] -= layervec->rect.x0;
  mat0.trans[5] -= layervec->rect.y0;
  swfdec_transform_invert (&mat, &mat0);
  dest = layervec->compose;
  if (shapevec->fill_type == 0x10) {
    for (j = 0; j < height; j++) {
      double x, y;

      x = mat.trans[2] * j + mat.trans[4];
      y = mat.trans[3] * j + mat.trans[5];
      for (i = 0; i < width; i++) {
	double z;
	int index;

	z = ((x + 16384.0) / 32768.0) * 256;
	if (z < 0)
	  z = 0;
	if (z > 255.0)
	  z = 255;
	index = z;
	//index &= 0xff;
	dest[0] = palette[index * 4 + 0];
	dest[1] = palette[index * 4 + 1];
	dest[2] = palette[index * 4 + 2];
	dest[3] = palette[index * 4 + 3];
	dest += 4;
	x += mat.trans[0];
	y += mat.trans[1];
      }
    }
  } else {
    for (j = 0; j < height; j++) {
      double x, y;

      x = mat.trans[2] * j + mat.trans[4];
      y = mat.trans[3] * j + mat.trans[5];
      for (i = 0; i < width; i++) {
	double z;
	int index;

	z = sqrt (x * x + y * y) / 16384.0 * 256;
	if (z < 0)
	  z = 0;
	if (z > 255.0)
	  z = 255;
	index = z;
	//index &= 0xff;
	dest[0] = palette[index * 4 + 0];
	dest[1] = palette[index * 4 + 1];
	dest[2] = palette[index * 4 + 2];
	dest[3] = palette[index * 4 + 3];
	dest += 4;
	x += mat.trans[0];
	y += mat.trans[1];
      }
    }
  }

  g_free (palette);
}

unsigned char *
swfdec_gradient_to_palette (SwfdecGradient * grad,
    SwfdecColorTransform *color_transform)
{
  swf_color color;
  unsigned char *p;
  int i, j;

  p = g_malloc (256 * 4);

  color = swfdec_color_apply_transform (grad->array[0].color, color_transform);
  if (grad->array[0].ratio > 256) {
    SWFDEC_ERROR ("gradient ratio > 256 (%d)", 
        grad->array[0].ratio);
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

static void swf_shape_ignore_recs (SwfdecDecoder * s, SwfdecBits * bits, SwfdecShape * shape);
void swf_morphshape_add_styles (SwfdecDecoder * s, SwfdecShape * shape, SwfdecBits * bits);
void swf_morphshape_get_recs (SwfdecDecoder * s, SwfdecBits * bits, SwfdecShape * shape);

int
tag_define_morph_shape (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecShape *shape;
  int start_rect[4];
  int end_rect[4];
  int id;
  int offset;

  id = swfdec_bits_get_u16 (bits);

  shape = swfdec_object_new (SWFDEC_TYPE_SHAPE);
  SWFDEC_OBJECT (shape)->id = id;
  //s->objects = g_list_append (s->objects, shape);

  SWFDEC_INFO("id=%d", id);

  swfdec_bits_get_rect (bits, start_rect);
  swfdec_bits_syncbits (bits);
  swfdec_bits_get_rect (bits, end_rect);

  swfdec_bits_syncbits (bits);
  offset = swfdec_bits_get_u32 (bits);
  SWFDEC_INFO("offset=%d", offset);

  shape->fills = g_ptr_array_new ();
  shape->fills2 = g_ptr_array_new ();
  shape->lines = g_ptr_array_new ();

  swf_morphshape_add_styles (s, shape, bits);

  swfdec_bits_syncbits (bits);
  swf_morphshape_get_recs (s, bits, shape);
  swfdec_bits_syncbits (bits);
  if (1) {
  swf_shape_ignore_recs (s, bits, shape);
  }

  return SWF_OK;
}

void
swf_shape_ignore_recs (SwfdecDecoder * s, SwfdecBits * bits, SwfdecShape * shape)
{
  int x = 0, y = 0;
  int fill0style = 0;
  int fill1style = 0;
  int linestyle = 0;
  int n_vec = 0;
  SwfdecShapeVec *shapevec;
  SwfdecShapePoint pt;

  while (swfdec_bits_peekbits (bits, 6) != 0) {
    int type;
    int n_bits;

    type = swfdec_bits_getbits (bits, 1);

    if (type == 0) {
      //int state_new_styles = swfdec_bits_getbits (bits, 1);
      int state_new_styles = 0;
      int state_line_styles = swfdec_bits_getbits (bits, 1);
      int state_fill_styles1 = swfdec_bits_getbits (bits, 1);
      int state_fill_styles0 = swfdec_bits_getbits (bits, 1);
      int state_moveto = swfdec_bits_getbits (bits, 1);

      if (state_moveto) {
	n_bits = swfdec_bits_getbits (bits, 5);
	x = swfdec_bits_getsbits (bits, n_bits);
	y = swfdec_bits_getsbits (bits, n_bits);

	SWFDEC_LOG("   moveto %d,%d", x, y);
      }
      if (state_fill_styles0) {
	fill0style = swfdec_bits_getbits (bits, shape->n_fill_bits);
	SWFDEC_LOG("   * fill0style = %d", fill0style);
      }
      if (state_fill_styles1) {
	fill1style = swfdec_bits_getbits (bits, shape->n_fill_bits);
	SWFDEC_LOG("   * fill1style = %d", fill1style);
      }
      if (state_line_styles) {
	linestyle = swfdec_bits_getbits (bits, shape->n_line_bits);
	SWFDEC_LOG("   * linestyle = %d", linestyle);
      }
      if (state_new_styles) {
	SWFDEC_ERROR("unexpected new styles");
	swf_morphshape_add_styles (s, shape, bits);
      }
      pt.control_x = SWFDEC_SHAPE_POINT_SPECIAL;
      pt.control_y = SWFDEC_SHAPE_POINT_MOVETO;
      pt.to_x = x;
      pt.to_y = y;
    } else {
      /* edge record */
      int n_bits;
      int edge_flag;

      edge_flag = swfdec_bits_getbits (bits, 1);

      if (edge_flag == 0) {
	int16_t x0, y0;
	int16_t x1, y1;
	int16_t x2, y2;

	x0 = x;
	y0 = y;
	n_bits = swfdec_bits_getbits (bits, 4) + 2;

	x += swfdec_bits_getsbits (bits, n_bits);
	y += swfdec_bits_getsbits (bits, n_bits);
	SWFDEC_LOG("   control %d,%d", x, y);
	x1 = x;
	y1 = y;

	x += swfdec_bits_getsbits (bits, n_bits);
	y += swfdec_bits_getsbits (bits, n_bits);
	SWFDEC_LOG("   anchor %d,%d", x, y);
	x2 = x;
	y2 = y;

        pt.control_x = x1;
        pt.control_y = y1;
        pt.to_x = x2;
        pt.to_y = y2;
	n_vec++;
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
	SWFDEC_LOG("   delta %d,%d", x, y);

        pt.control_x = SWFDEC_SHAPE_POINT_SPECIAL;
        pt.control_y = SWFDEC_SHAPE_POINT_LINETO;
        pt.to_x = x;
        pt.to_y = y;
      }
    }
    if (fill0style) {
      shapevec = swfdec_shape_get_fill0style (shape, fill0style);
      if (shapevec) {
        swfdec_shapevec_free(shapevec);
      }
      s->stats_n_points++;
    }
    if (fill1style) {
      shapevec = swfdec_shape_get_fill1style (shape, fill1style);
      if (shapevec) {
        swfdec_shapevec_free(shapevec);
      }
      s->stats_n_points++;
    }
    if (linestyle) {
      shapevec = swfdec_shape_get_linestyle(shape, linestyle);
      if (shapevec) {
        swfdec_shapevec_free(shapevec);
      }
      s->stats_n_points++;
    }

  }

  swfdec_bits_getbits (bits, 6);
  swfdec_bits_syncbits (bits);
}

void
swf_morphshape_add_styles (SwfdecDecoder * s, SwfdecShape * shape, SwfdecBits * bits)
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
  SWFDEC_LOG("n_fill_styles %d", n_fill_styles);
  for (i = 0; i < n_fill_styles; i++) {
    int fill_style_type;
    SwfdecShapeVec *shapevec;
    SwfdecTransform end_transform;
    unsigned int end_color;

    SWFDEC_LOG("fill style %d:", i);

    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills2, shapevec);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);

    shapevec->color = SWF_COLOR_COMBINE (0, 255, 0, 255);

    fill_style_type = swfdec_bits_get_u8 (bits);
    SWFDEC_LOG("    type 0x%02x", fill_style_type);
    if (fill_style_type == 0x00) {
      shapevec->color = swfdec_bits_get_rgba (bits);
      end_color = swfdec_bits_get_rgba (bits);
      SWFDEC_LOG("    color %08x", shapevec->color);
    } else if (fill_style_type == 0x10 || fill_style_type == 0x12) {
      shapevec->fill_type = fill_style_type;
      swfdec_bits_get_transform (bits, &shapevec->fill_transform);
      swfdec_bits_get_transform (bits, &end_transform);
      shapevec->grad = swfdec_bits_get_morph_gradient (bits);
      shapevec->fill_transform.trans[0] *= SWF_SCALE_FACTOR;
      shapevec->fill_transform.trans[1] *= SWF_SCALE_FACTOR;
      shapevec->fill_transform.trans[2] *= SWF_SCALE_FACTOR;
      shapevec->fill_transform.trans[3] *= SWF_SCALE_FACTOR;
    } else if (fill_style_type == 0x40 || fill_style_type == 0x41) {
      shapevec->fill_type = fill_style_type;
      shapevec->fill_id = swfdec_bits_get_u16 (bits);
      SWFDEC_LOG("   background fill id = %d (type 0x%02x)",
	  shapevec->fill_id, fill_style_type);

      if (shapevec->fill_id == 65535) {
	shapevec->fill_id = 0;
	shapevec->color = SWF_COLOR_COMBINE (0, 255, 255, 255);
      }

      swfdec_bits_get_transform (bits, &shapevec->fill_transform);
      swfdec_bits_get_transform (bits, &end_transform);
      /* FIXME: the 0.965 is a mysterious factor that seems to improve
       * rendering of images. */
      shapevec->fill_transform.trans[0] *= SWF_SCALE_FACTOR * 0.965;
      shapevec->fill_transform.trans[1] *= SWF_SCALE_FACTOR * 0.965;
      shapevec->fill_transform.trans[2] *= SWF_SCALE_FACTOR * 0.965;
      shapevec->fill_transform.trans[3] *= SWF_SCALE_FACTOR * 0.965;
    } else {
      SWFDEC_ERROR ("unknown fill style type 0x%02x", fill_style_type);
    }
  }

  swfdec_bits_syncbits (bits);
  shape->lines_offset = shape->lines->len;
  n_line_styles = swfdec_bits_get_u8 (bits);
  if (n_line_styles == 0xff) {
    n_line_styles = swfdec_bits_get_u16 (bits);
  }
  SWFDEC_LOG("   n_line_styles %d", n_line_styles);
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
    SWFDEC_LOG("%d: %g->%g %08x->%08x", i,
        shapevec->width, end_width,
        shapevec->color, end_color);
  }

  swfdec_bits_syncbits (bits);
}

void
swf_morphshape_get_recs (SwfdecDecoder * s, SwfdecBits * bits, SwfdecShape * shape)
{
  int x = 0, y = 0;
  int fill0style = 0;
  int fill1style = 0;
  int linestyle = 0;
  int n_vec = 0;
  SwfdecShapeVec *shapevec;
  SwfdecShapePoint pt;

  shape->n_line_bits = swfdec_bits_getbits (bits, 4);
  shape->n_fill_bits = swfdec_bits_getbits (bits, 4);
  while (swfdec_bits_peekbits (bits, 6) != 0) {
    int type;
    int n_bits;

    type = swfdec_bits_getbits (bits, 1);

    if (type == 0) {
      //int state_new_styles = swfdec_bits_getbits (bits, 1);
      int state_new_styles = 0;
      int state_line_styles = swfdec_bits_getbits (bits, 1);
      int state_fill_styles1 = swfdec_bits_getbits (bits, 1);
      int state_fill_styles0 = swfdec_bits_getbits (bits, 1);
      int state_moveto = swfdec_bits_getbits (bits, 1);

      if (state_moveto) {
	n_bits = swfdec_bits_getbits (bits, 5);
	x = swfdec_bits_getsbits (bits, n_bits);
	y = swfdec_bits_getsbits (bits, n_bits);

	SWFDEC_LOG("   moveto %d,%d", x, y);
      }
      if (state_fill_styles0) {
	fill0style = swfdec_bits_getbits (bits, shape->n_fill_bits);
	SWFDEC_LOG("   * fill0style = %d", fill0style);
      }
      if (state_fill_styles1) {
	fill1style = swfdec_bits_getbits (bits, shape->n_fill_bits);
	SWFDEC_LOG("   * fill1style = %d", fill1style);
      }
      if (state_line_styles) {
	linestyle = swfdec_bits_getbits (bits, shape->n_line_bits);
	SWFDEC_LOG("   * linestyle = %d", linestyle);
      }
      if (state_new_styles) {
	SWFDEC_ERROR("unexpected new styles");
	//swf_morphshape_add_styles (s, shape, bits);
      }
      pt.control_x = SWFDEC_SHAPE_POINT_SPECIAL;
      pt.control_y = SWFDEC_SHAPE_POINT_MOVETO;
      pt.to_x = x;
      pt.to_y = y;
    } else {
      /* edge record */
      int n_bits;
      int edge_flag;

      edge_flag = swfdec_bits_getbits (bits, 1);

      if (edge_flag == 0) {
	int16_t x0, y0;
	int16_t x1, y1;
	int16_t x2, y2;

	x0 = x;
	y0 = y;
	n_bits = swfdec_bits_getbits (bits, 4) + 2;

	x += swfdec_bits_getsbits (bits, n_bits);
	y += swfdec_bits_getsbits (bits, n_bits);
	SWFDEC_LOG("   control %d,%d", x, y);
	x1 = x;
	y1 = y;

	x += swfdec_bits_getsbits (bits, n_bits);
	y += swfdec_bits_getsbits (bits, n_bits);
	SWFDEC_LOG("   anchor %d,%d", x, y);
	x2 = x;
	y2 = y;

        pt.control_x = x1;
        pt.control_y = y1;
        pt.to_x = x2;
        pt.to_y = y2;
	n_vec++;
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
	SWFDEC_LOG("   delta %d,%d", x, y);

        pt.control_x = SWFDEC_SHAPE_POINT_SPECIAL;
        pt.control_y = SWFDEC_SHAPE_POINT_LINETO;
        pt.to_x = x;
        pt.to_y = y;
      }
    }
    if (fill0style) {
      shapevec = swfdec_shape_get_fill0style (shape, fill0style);
      if (shapevec) {
        g_array_append_val (shapevec->path, pt);
      }
      s->stats_n_points++;
    }
    if (fill1style) {
      shapevec = swfdec_shape_get_fill1style (shape, fill1style);
      if (shapevec) {
        g_array_append_val (shapevec->path, pt);
      }
      s->stats_n_points++;
    }
    if (linestyle) {
      shapevec = swfdec_shape_get_linestyle(shape, linestyle);
      if (shapevec) {
        g_array_append_val (shapevec->path, pt);
      }
      s->stats_n_points++;
    }

  }

  swfdec_bits_getbits (bits, 6);
  swfdec_bits_syncbits (bits);
}

