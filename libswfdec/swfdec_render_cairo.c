
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>
#include <cairo.h>

#include "swfdec_internal.h"

void
swfdec_render_be_start (SwfdecDecoder *s)
{
  cairo_t *cr;
  cairo_surface_t *cs;

  if (!s->buffer) {
    s->buffer = g_malloc (s->stride * s->height);
  }
  cs = cairo_image_surface_create_for_data (s->buffer, CAIRO_FORMAT_ARGB32,
      s->width, s->height, s->stride);

  cr = cairo_create (cs);
  s->backend_private = cr;

  cairo_set_tolerance (cr, s->flatness);
}

void
swfdec_render_be_clear (SwfdecDecoder *s)
{
  cairo_t *cr;

  cr = (cairo_t *) s->backend_private;

  cairo_rectangle (cr, 0, 0, s->width, s->height);
  cairo_set_source_rgb (cr, SWF_COLOR_R(s->bg_color)/255.0,
      SWF_COLOR_G(s->bg_color)/255.0, SWF_COLOR_B(s->bg_color)/255.0);
  cairo_fill (cr);
}

void
swfdec_render_be_stop (SwfdecDecoder *s)
{
  cairo_destroy ((cairo_t *)s->backend_private);
}

void
swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec)
{
#if 0
  SwfdecRect rect;
  struct swf_svp_render_struct cb_data;

  swfdec_rect_intersect (&rect, &s->render->drawrect, &layervec->rect);

  if (swfdec_rect_is_empty (&rect)) {
    return;
  }

  cb_data.x0 = rect.x0;
  cb_data.x1 = rect.x1;
  cb_data.buf = s->buffer + rect.y0 * s->stride + rect.x0 * s->bytespp;
  cb_data.color = layervec->color;
  cb_data.rowstride = s->stride;
  cb_data.scanline = s->tmp_scanline;
  cb_data.compose = layervec->compose + (rect.x0 - layervec->rect.x0) * 4;
  cb_data.compose_rowstride = layervec->compose_rowstride;
  cb_data.compose_height = layervec->compose_height;
  cb_data.compose_y = rect.y0 - layervec->rect.y0;
  cb_data.compose_width = layervec->compose_width;

  g_assert (rect.x1 > rect.x0);
  /* This assertion fails occasionally. */
  //g_assert(layervec->svp->n_segs > 0);
  g_assert (layervec->svp->n_segs >= 0);

  if (layervec->svp->n_segs > 0) {
    art_svp_render_aa (layervec->svp, rect.x0, rect.y0,
        rect.x1, rect.y1,
        (ArtSVPRenderAAFunc) (layervec->compose ? s->compose_callback : s->
            callback), &cb_data);
  }

  s->pixels_rendered += (rect.x1 - rect.x0) * (rect.y1 - rect.y0);
#endif
}

static void
draw_line (cairo_t *cr, SwfdecShapePoint *points, int n_points)
{
  int j;
  double old_x = 0, old_y = 0;
  double x,y;

  for (j=0;j<n_points;j++){
    x = points[j].to_x * SWF_SCALE_FACTOR;
    y = points[j].to_y * SWF_SCALE_FACTOR;
    if (points[j].control_x == SWFDEC_SHAPE_POINT_SPECIAL) {
      if (points[j].control_y == SWFDEC_SHAPE_POINT_MOVETO) {
        cairo_move_to (cr, x, y);
      } else {
        cairo_line_to (cr, x, y);
      }
    } else {
      double xa, ya;
      double x1, y1;
      double x2, y2;

#define WEIGHT (2.0/3.0)
      xa = points[j].control_x * SWF_SCALE_FACTOR;
      ya = points[j].control_y * SWF_SCALE_FACTOR;
      x1 = xa * WEIGHT + (1-WEIGHT) * old_x;
      y1 = ya * WEIGHT + (1-WEIGHT) * old_y;
      x2 = xa * WEIGHT + (1-WEIGHT) * x;
      y2 = ya * WEIGHT + (1-WEIGHT) * y;

      cairo_curve_to (cr, x1, y1, x2, y2, x, y);
    }
    old_x = x;
    old_y = y;
  }
}

static void
draw (cairo_t *cr, SwfdecShapePoint *points, int n_points)
{
  int j;
  double old_x = 0, old_y = 0;
  double x,y;

  for (j=0;j<n_points;j++){
    x = points[j].to_x * SWF_SCALE_FACTOR;
    y = points[j].to_y * SWF_SCALE_FACTOR;
    if (points[j].control_x == SWFDEC_SHAPE_POINT_SPECIAL) {
      if (points[j].control_y == SWFDEC_SHAPE_POINT_MOVETO) {
        // cairo_move_to (cr, x, y);
      } else {
        cairo_line_to (cr, x, y);
      }
    } else {
      double xa, ya;
      double x1, y1;
      double x2, y2;

#define WEIGHT (2.0/3.0)
      xa = points[j].control_x * SWF_SCALE_FACTOR;
      ya = points[j].control_y * SWF_SCALE_FACTOR;
      x1 = xa * WEIGHT + (1-WEIGHT) * old_x;
      y1 = ya * WEIGHT + (1-WEIGHT) * old_y;
      x2 = xa * WEIGHT + (1-WEIGHT) * x;
      y2 = ya * WEIGHT + (1-WEIGHT) * y;

      cairo_curve_to (cr, x1, y1, x2, y2, x, y);
    }
    old_x = x;
    old_y = y;
  }
}

static void
draw_rev (cairo_t *cr, SwfdecShapePoint *points, int n_points)
{
  int j;
  double x,y;
  double old_x, old_y;

  if (n_points == 0) return;

  x = points[n_points - 1].to_x * SWF_SCALE_FACTOR;
  y = points[n_points - 1].to_y * SWF_SCALE_FACTOR;
  //cairo_move_to (cr, x, y);
  old_x = x;
  old_y = y;

  for (j=n_points-1;j>0;j--){
    x = points[j-1].to_x * SWF_SCALE_FACTOR;
    y = points[j-1].to_y * SWF_SCALE_FACTOR;
    if (points[j].control_x == SWFDEC_SHAPE_POINT_SPECIAL) {
      if (points[j].control_y == SWFDEC_SHAPE_POINT_MOVETO) {
        //cairo_move_to (cr, x, y);
      } else {
        cairo_line_to (cr, x, y);
      }
    } else {
      double xa, ya;
      double x1, y1;
      double x2, y2;

      xa = points[j].control_x * SWF_SCALE_FACTOR;
      ya = points[j].control_y * SWF_SCALE_FACTOR;
      x1 = xa * WEIGHT + (1-WEIGHT) * old_x;
      y1 = ya * WEIGHT + (1-WEIGHT) * old_y;
      x2 = xa * WEIGHT + (1-WEIGHT) * x;
      y2 = ya * WEIGHT + (1-WEIGHT) * y;

      cairo_curve_to (cr, x1, y1, x2, y2, x, y);
    }
    old_x = x;
    old_y = y;
  }
}

typedef struct _LineChunk LineChunk;
struct _LineChunk {
  int used;
  int direction;
  int n_points;
  SwfdecShapePoint *points;
  double start_x, start_y;
  double end_x, end_y;
};

static void
draw_x (cairo_t *cr, GArray *array, GArray *array2)
{
  SwfdecShapePoint *points;
  GArray *chunks_array;
  LineChunk chunk = { 0 };
  int j;
  double x, y;

  chunks_array = g_array_new (FALSE, TRUE, sizeof (LineChunk));

  points = (SwfdecShapePoint *)array->data;
  for (j=0;j<array->len;j++){
    x = points[j].to_x * SWF_SCALE_FACTOR;
    y = points[j].to_y * SWF_SCALE_FACTOR;
    if (points[j].control_x == SWFDEC_SHAPE_POINT_SPECIAL) {
      if (points[j].control_y == SWFDEC_SHAPE_POINT_MOVETO) {
        if (chunk.points) {
          g_array_append_val (chunks_array, chunk);
        }
        chunk.direction = 0;
        chunk.n_points = 1;
        chunk.points = points + j;
        chunk.start_x = x;
        chunk.start_y = y;
      } else {
        chunk.n_points++;
      }
    } else {
      chunk.n_points++;
    }
    chunk.end_x = x;
    chunk.end_y = y;
  }
  if (chunk.points) {
    g_array_append_val (chunks_array, chunk);
  }

  points = (SwfdecShapePoint *)array2->data;
  chunk.points = NULL;
  for (j=0;j<array2->len;j++){
    x = points[j].to_x * SWF_SCALE_FACTOR;
    y = points[j].to_y * SWF_SCALE_FACTOR;
    if (points[j].control_x == SWFDEC_SHAPE_POINT_SPECIAL) {
      if (points[j].control_y == SWFDEC_SHAPE_POINT_MOVETO) {
        if (chunk.points) {
          g_array_append_val (chunks_array, chunk);
        }
        chunk.direction = 1;
        chunk.n_points = 1;
        chunk.points = points + j;
        chunk.end_x = x;
        chunk.end_y = y;
      } else {
        chunk.n_points++;
      }
    } else {
      chunk.n_points++;
    }
    chunk.start_x = x;
    chunk.start_y = y;
  }
  if (chunk.points) {
    g_array_append_val (chunks_array, chunk);
  }

  for(j=0;j<chunks_array->len;j++){
    LineChunk *c;
    double start_x, start_y;
    double end_x, end_y;

    c = &g_array_index (chunks_array, LineChunk, j);
    if (c->used) continue;

    start_x = c->start_x;
    start_y = c->start_y;
    c->used = 1;
    end_x = c->end_x;
    end_y = c->end_y;
    cairo_move_to (cr, start_x, start_y);
    if (c->direction) {
      draw_rev (cr, c->points, c->n_points);
    } else {
      draw (cr, c->points, c->n_points);
    }

    while (1) {
      int i;

      if (start_x == end_x && start_y == end_y) break;

      for(i=0;i<chunks_array->len;i++){
        c = &g_array_index (chunks_array, LineChunk, i);
        if (c->used) continue;

        if (end_x == c->start_x && end_y == c->start_y) {
          end_x = c->end_x;
          end_y = c->end_y;
          c->used = 1;
          if (c->direction) {
            draw_rev (cr, c->points, c->n_points);
          } else {
            draw (cr, c->points, c->n_points);
          }
          break;
        }
      }
      if (i==chunks_array->len) {
        SWFDEC_ERROR("failed to complete loop");
        break;
      }
    }
  }
  //cairo_fill(cr);

  g_array_free (chunks_array, TRUE);
}

static void
swfdec_matrix_to_cairo (cairo_matrix_t *cm, SwfdecTransform *trans)
{
  cairo_matrix_init (cm, trans->trans[0], trans->trans[1],
      trans->trans[2],trans->trans[3], trans->trans[4], trans->trans[5]);
}

/* this is some kind of travesty because it does both mouse detection and rendering */
static gboolean
swfdec_shape_render_internal (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * obj, gboolean mouse_check)
{
  SwfdecShape *shape = SWFDEC_SHAPE (obj);
  int i;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;
  cairo_t *cr = s->backend_private;
  SwfdecTransform base;

  swfdec_transform_multiply (&base, &seg->transform, &s->transform);

  for (i = 0; i < shape->fills->len; i++) {
    swf_color color;
    cairo_matrix_t cm;
    cairo_pattern_t *pattern = NULL;
    cairo_surface_t *src_surface = NULL;

    shapevec = g_ptr_array_index (shape->fills, i);
    shapevec2 = g_ptr_array_index (shape->fills2, i);

    color = swfdec_color_apply_transform (shapevec->color,
        &seg->color_transform);

    switch (shapevec->fill_type) {
      case 0x10:
        {
          SwfdecGradient *grad = shapevec->grad;
          int j;
          cairo_matrix_t matrix;
          SwfdecTransform mat;
          SwfdecTransform trans;

          swfdec_transform_multiply (&trans, &shapevec->fill_transform, &base);
          swfdec_transform_invert (&mat, &trans);
          //swfdec_transform_invert (&mat, &shapevec->fill_transform);

          swfdec_matrix_to_cairo (&matrix, &mat);
          pattern = cairo_pattern_create_linear (-16384.0, 0, 16384.0, 0);
          cairo_pattern_set_matrix(pattern, &matrix);
          for (j=0;j<grad->n_gradients;j++){
            color = swfdec_color_apply_transform (grad->array[j].color,
                &seg->color_transform);

            cairo_pattern_add_color_stop_rgba (pattern,
                grad->array[j].ratio/256.0,
                SWF_COLOR_R(color)/255.0, SWF_COLOR_G(color)/255.0,
                SWF_COLOR_B(color)/255.0, SWF_COLOR_A(color)/255.0);
          }
          cairo_set_source (cr, pattern);
        }
        break;
      case 0x12:
        {
          SwfdecGradient *grad = shapevec->grad;
          int j;
          cairo_matrix_t matrix;
          SwfdecTransform mat;
          SwfdecTransform trans;

          swfdec_transform_multiply (&trans, &shapevec->fill_transform, &base);
          swfdec_transform_invert (&mat, &trans);
          //swfdec_transform_invert (&mat, &shapevec->fill_transform);

          swfdec_matrix_to_cairo (&matrix, &mat);
          pattern = cairo_pattern_create_radial (0,0,0,
             0, 0, 16384);
          cairo_pattern_set_matrix(pattern, &matrix);
          for (j=0;j<grad->n_gradients;j++){
            color = swfdec_color_apply_transform (grad->array[j].color,
                &seg->color_transform);

            cairo_pattern_add_color_stop_rgba (pattern,
                grad->array[j].ratio/256.0,
                SWF_COLOR_R(color)/255.0, SWF_COLOR_G(color)/255.0,
                SWF_COLOR_B(color)/255.0, SWF_COLOR_A(color)/255.0);
          }
          cairo_set_source (cr, pattern);
        }
        break;
      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
        {
          SwfdecImage *image;
          cairo_matrix_t matrix;
          SwfdecTransform mat;
          SwfdecTransform trans;

          swfdec_transform_multiply (&trans, &shapevec->fill_transform, &base);
          swfdec_transform_invert (&mat, &trans);
          //swfdec_transform_invert (&mat, &shapevec->fill_transform);

          image = (SwfdecImage *)swfdec_object_get (s, shapevec->fill_id);
          if (image && SWFDEC_IS_IMAGE (image)) {
            unsigned char *image_data = swfdec_handle_get_data(image->handle);

            swfdec_matrix_to_cairo (&matrix, &mat);
            //swfdec_matrix_to_cairo (&matrix, &shapevec->fill_transform);
            src_surface = cairo_image_surface_create_for_data (image_data,
                CAIRO_FORMAT_ARGB32, image->width, image->height,
                image->rowstride);
            pattern = cairo_pattern_create_for_surface (src_surface);
            cairo_pattern_set_matrix(pattern, &matrix);
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
          } else {
            cairo_set_source_rgba (cr, SWF_COLOR_R(color)/255.0,
                SWF_COLOR_G(color)/255.0, SWF_COLOR_B(color)/255.0,
                SWF_COLOR_A(color)/255.0);
          }
        }
        break;
      case 0x00:
        cairo_set_source_rgba (cr, SWF_COLOR_R(color)/255.0,
            SWF_COLOR_G(color)/255.0, SWF_COLOR_B(color)/255.0,
            SWF_COLOR_A(color)/255.0);
        break;
      default:
        SWFDEC_ERROR("unhandled fill type 0x%02x", shapevec->fill_type);
        cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.5);
        break;
    }
    
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_set_line_width (cr, 0.5);
    cairo_save (cr);

    cairo_matrix_init (&cm, base.trans[0], base.trans[1],
        base.trans[2], base.trans[3], base.trans[4], base.trans[5]);
    cairo_transform (cr, &cm);

    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    draw_x (cr, shapevec->path, shapevec2->path);
    if (pattern) {
      cairo_pattern_destroy (pattern);
    }
    if (src_surface) {
      cairo_surface_destroy (src_surface);
    }
    if (mouse_check) {
      double x,y;
      x = s->mouse_x;
      y = s->mouse_y;
      cairo_device_to_user (cr, &x, &y);
      SWFDEC_DEBUG("checking mouse %d,%d",
          s->mouse_x, s->mouse_y);
      if (cairo_in_fill (cr, x, y))
	return TRUE;
    } else {
      cairo_fill (cr);
    }
    cairo_restore (cr);
  }
  if (mouse_check)
    return FALSE;

  for (i = 0; i < shape->lines->len; i++) {
    cairo_matrix_t cm;
    swf_color color;
    double width;

    shapevec = g_ptr_array_index (shape->lines, i);

    color = swfdec_color_apply_transform (shapevec->color,
        &seg->color_transform);
    cairo_set_source_rgba (cr, SWF_COLOR_R(color)/255.0,
        SWF_COLOR_G(color)/255.0, SWF_COLOR_B(color)/255.0,
        SWF_COLOR_A(color)/255.0);

    cairo_save (cr);

    cairo_matrix_init (&cm, base.trans[0], base.trans[1],
        base.trans[2],base.trans[3], base.trans[4], base.trans[5]);
    cairo_transform (cr, &cm);

    width = shapevec->width * swfdec_transform_get_expansion (&base);
    cairo_set_line_width (cr, width);
    draw_line (cr, (void *)shapevec->path->data, shapevec->path->len);
    cairo_stroke (cr);

    cairo_restore (cr);
  }

  return TRUE;
}

gboolean
swfdec_shape_has_mouse (SwfdecDecoder *s, SwfdecSpriteSegment *seg,
    SwfdecObject *obj)
{
  return swfdec_shape_render_internal (s, seg, obj, TRUE);
}

void
swfdec_shape_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * obj)
{
  swfdec_shape_render_internal (s, seg, obj, FALSE);
}

void
swfdec_text_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
  int i;
  SwfdecText *text;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;
  SwfdecObject *fontobj;
  SwfdecLayer *layer;
  cairo_t *cr = s->backend_private;

  layer = swfdec_layer_new ();
  layer->seg = seg;
  swfdec_transform_multiply (&layer->transform, &seg->transform, &s->transform);

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  text = SWFDEC_TEXT (object);
  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph;
    SwfdecShape *shape;
    SwfdecTransform glyph_trans;
    SwfdecTransform trans;
    SwfdecTransform pos;
    swf_color color;
    cairo_matrix_t cm;

    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);

    fontobj = swfdec_object_get (s, glyph->font);
    if (fontobj == NULL)
      continue;

    shape = swfdec_font_get_glyph (SWFDEC_FONT (fontobj), glyph->glyph);
    if (shape == NULL) {
      SWFDEC_ERROR ("failed getting glyph %d\n", glyph->glyph);
      continue;
    }

    swfdec_transform_init_identity (&pos);
    swfdec_transform_translate (&pos,
        glyph->x * SWF_SCALE_FACTOR, glyph->y * SWF_SCALE_FACTOR);
    pos.trans[0] = glyph->height * SWF_TEXT_SCALE_FACTOR;
    pos.trans[3] = glyph->height * SWF_TEXT_SCALE_FACTOR;
    swfdec_transform_multiply (&glyph_trans, &pos, &object->transform);
    swfdec_transform_multiply (&trans, &glyph_trans, &layer->transform);

    layer->fills = g_array_set_size (layer->fills, layer->fills->len + 1);

    shapevec = g_ptr_array_index (shape->fills, 0);
    shapevec2 = g_ptr_array_index (shape->fills2, 0);

    color = swfdec_color_apply_transform (glyph->color, &seg->color_transform);
    cairo_set_source_rgba (cr, SWF_COLOR_R(color)/255.0,
        SWF_COLOR_G(color)/255.0, SWF_COLOR_B(color)/255.0,
        SWF_COLOR_A(color)/255.0);

    cairo_save (cr);

    cairo_matrix_init (&cm, trans.trans[0], trans.trans[1],
        trans.trans[2],trans.trans[3], trans.trans[4], trans.trans[5]);
    cairo_transform (cr, &cm);

    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    draw_x (cr, shapevec->path, shapevec2->path);
    cairo_fill (cr);

    cairo_restore (cr);
  }

  swfdec_layer_free (layer);
}

void
swf_config_colorspace (SwfdecDecoder * s)
{
  switch (s->colorspace) {
    case SWF_COLORSPACE_RGB565:
      s->stride = s->width * 2;
      s->bytespp = 2;
//      s->callback = (void *) art_rgb565_svp_alpha_callback;
//      s->compose_callback = (void *) art_rgb565_svp_alpha_callback;
//      s->fillrect = art_rgb565_fillrect;
      break;
    case SWF_COLORSPACE_RGB888:
    default:
      s->stride = s->width * 4;
      s->bytespp = 4;
//      s->callback = (void *) art_rgb_svp_alpha_callback;
//      s->compose_callback = (void *) art_rgb_svp_alpha_compose_callback;
//      s->fillrect = art_rgb_fillrect;
      break;
  }
}

void
swfdec_render_layervec_free (SwfdecLayerVec * layervec)
{
#if 0
  if (layervec->svp) {
    art_svp_free (layervec->svp);
  }
#endif
}

