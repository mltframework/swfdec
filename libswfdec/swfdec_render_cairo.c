
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

  cr = cairo_create ();
  s->backend_private = cr;

  if (!s->buffer) {
    s->buffer = g_malloc (s->stride * s->height);
  }
  cairo_set_target_image (cr, s->buffer, CAIRO_FORMAT_ARGB32, s->width,
      s->height, s->stride);

  cairo_rectangle (cr, 0, 0, s->width, s->height);
  cairo_set_rgb_color (cr, SWF_COLOR_R(s->bg_color)/255.0,
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
draw (cairo_t *cr, GArray *array)
{
  SwfdecShapePoint *points = (SwfdecShapePoint *)array->data;
  int j;
  double old_x = 0, old_y = 0;
  double x,y;

  for (j=0;j<array->len;j++){
    x = points[j].to_x * SWF_SCALE_FACTOR;
    y = points[j].to_y * SWF_SCALE_FACTOR;
    if (points[j].control_x == SWFDEC_SHAPE_POINT_SPECIAL) {
      if (points[j].control_y == SWFDEC_SHAPE_POINT_MOVETO) {
//cairo_stroke(cr);
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
draw_rev (cairo_t *cr, GArray *array)
{
  SwfdecShapePoint *points = (SwfdecShapePoint *)array->data;
  int j;
  double x,y;
  double old_x, old_y;

  if (array->len == 0) return;

  x = points[array->len - 1].to_x * SWF_SCALE_FACTOR;
  y = points[array->len - 1].to_y * SWF_SCALE_FACTOR;
  cairo_move_to (cr, x, y);
  old_x = x;
  old_y = y;

  for (j=array->len-1;j>0;j--){
    x = points[j-1].to_x * SWF_SCALE_FACTOR;
    y = points[j-1].to_y * SWF_SCALE_FACTOR;
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
draw_x (cairo_t *cr, GArray *array, GArray *array2)
{
  draw (cr, array);
  draw_rev (cr, array2);

}

void
swfdec_shape_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * obj)
{
  SwfdecLayer *layer;
  SwfdecShape *shape = SWFDEC_SHAPE (obj);
  int i;
  //SwfdecLayerVec *layervec;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;
  cairo_t *cr = s->backend_private;

  layer = swfdec_layer_new ();
  layer->seg = seg;
  swfdec_transform_multiply (&layer->transform, &seg->transform, &s->transform);

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  //g_array_set_size (layer->fills, shape->fills->len);
  for (i = 0; i < shape->fills->len; i++) {
    //ArtVpath *vpath, *vpath0, *vpath1;
    //ArtBpath *bpath0, *bpath1;
    SwfdecTransform trans;
    swf_color color;
    cairo_matrix_t *cm;

    shapevec = g_ptr_array_index (shape->fills, i);
    shapevec2 = g_ptr_array_index (shape->fills2, i);

    memcpy (&trans, &layer->transform, sizeof (SwfdecTransform));

    color = swfdec_color_apply_transform (shapevec->color,
        &seg->color_transform);
    cairo_set_rgb_color (cr, SWF_COLOR_R(color)/255.0,
        SWF_COLOR_G(color)/255.0, SWF_COLOR_B(color)/255.0);
    cairo_set_alpha (cr, SWF_COLOR_A(color)/255.0);
    
    cairo_set_line_width (cr, 0.5);
    cairo_save (cr);

    cm = cairo_matrix_create ();
    cairo_matrix_set_affine (cm, trans.trans[0], trans.trans[1],
        trans.trans[2],trans.trans[3], trans.trans[4], trans.trans[5]);
    cairo_concat_matrix (cr, cm);
    cairo_matrix_destroy (cm);

    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);
    draw_x (cr, shapevec->path, shapevec2->path);
    //draw (cr, shapevec->path);
    //draw_rev (cr, shapevec2->path);
    cairo_fill (cr);
#if 0
    {
      SwfdecShapePoint *points = (SwfdecShapePoint *)shapevec->path->data;
      int j;

      for (j=0;j<shapevec->path->len;j++){
        double x,y;
        x = points[j].to_x * SWF_SCALE_FACTOR;
        y = points[j].to_y * SWF_SCALE_FACTOR;
        if (points[j].control_x == SWFDEC_SHAPE_POINT_SPECIAL) {
          if (points[j].control_y == SWFDEC_SHAPE_POINT_MOVETO) {
            cairo_move_to (cr, x, y);
          } else {
            cairo_line_to (cr, x, y);
          }
        } else {
          cairo_line_to (cr, x, y);
        }
      }
    }
    cairo_stroke(cr);
#endif
    cairo_restore (cr);
#if 0
    bpath0 = swfdec_art_bpath_from_points (shapevec->path, &trans);
    bpath1 = swfdec_art_bpath_from_points (shapevec2->path, &trans);
    vpath0 = art_bez_path_to_vec (bpath0, s->flatness);
    vpath1 = art_bez_path_to_vec (bpath1, s->flatness);
    vpath1 = art_vpath_reverse_free (vpath1);
    vpath = art_vpath_cat (vpath0, vpath1);
    art_vpath_bbox_irect (vpath, (ArtIRect *) & layervec->rect);
    layervec->svp = art_svp_from_vpath (vpath);
    art_svp_make_convex (layervec->svp);
    swfdec_rect_union_to_masked (&layer->rect, &layervec->rect, &s->irect);

    g_free (bpath0);
    g_free (bpath1);
    art_free (vpath0);
    g_free (vpath1);
    g_free (vpath);

    layervec->color = swfdec_color_apply_transform (shapevec->color,
        &seg->color_transform);
    layervec->compose = NULL;
    if (shapevec->fill_id) {
      swfdec_shape_compose (s, layervec, shapevec, &layer->transform);
    }
    if (shapevec->grad) {
      swfdec_shape_compose_gradient (s, layervec, shapevec, &layer->transform,
          seg);
    }
#endif
  }

  //g_array_set_size (layer->lines, shape->lines->len);
  for (i = 0; i < shape->lines->len; i++) {
    //ArtVpath *vpath;
    //ArtBpath *bpath;
    SwfdecTransform trans;
    cairo_matrix_t *cm;
    swf_color color;

    shapevec = g_ptr_array_index (shape->lines, i);

    memcpy (&trans, &layer->transform, sizeof (SwfdecTransform));

    color = swfdec_color_apply_transform (shapevec->color,
        &seg->color_transform);
    cairo_set_rgb_color (cr, SWF_COLOR_R(color)/255.0,
        SWF_COLOR_G(color)/255.0, SWF_COLOR_B(color)/255.0);
    cairo_set_alpha (cr, SWF_COLOR_A(color)/255.0);

    cairo_set_line_width (cr, shapevec->width);

    cairo_save (cr);

    cm = cairo_matrix_create ();
    cairo_matrix_set_affine (cm, trans.trans[0], trans.trans[1],
        trans.trans[2],trans.trans[3], trans.trans[4], trans.trans[5]);
    cairo_concat_matrix (cr, cm);
    cairo_matrix_destroy (cm);

    draw (cr, shapevec->path);
    cairo_stroke (cr);

    cairo_restore (cr);
  }

  //swfdec_layer_render (s, layer);
  swfdec_layer_free (layer);
}


void
swfdec_text_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
#if 0
  int i;
  SwfdecText *text;
  SwfdecLayerVec *layervec;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;
  SwfdecObject *fontobj;
  SwfdecLayer *layer;

  layer = swfdec_layer_new ();
  layer->seg = seg;
  swfdec_transform_multiply (&layer->transform, &seg->transform, &s->transform);

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  text = SWFDEC_TEXT (object);
  for (i = 0; i < text->glyphs->len; i++) {
    ArtVpath *vpath, *vpath0, *vpath1;
    ArtBpath *bpath0, *bpath1;
    SwfdecTextGlyph *glyph;
    SwfdecShape *shape;
    SwfdecTransform glyph_trans;
    SwfdecTransform trans;
    SwfdecTransform pos;

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
    layervec =
        &g_array_index (layer->fills, SwfdecLayerVec, layer->fills->len - 1);

    shapevec = g_ptr_array_index (shape->fills, 0);
    shapevec2 = g_ptr_array_index (shape->fills2, 0);
    layervec->color = swfdec_color_apply_transform (glyph->color,
        &seg->color_transform);

    bpath0 = swfdec_art_bpath_from_points (shapevec->path, &trans);
    bpath1 = swfdec_art_bpath_from_points (shapevec2->path, &trans);
    vpath0 = art_bez_path_to_vec (bpath0, s->flatness);
    vpath1 = art_bez_path_to_vec (bpath1, s->flatness);
    vpath1 = art_vpath_reverse_free (vpath1);
    vpath = art_vpath_cat (vpath0, vpath1);
    art_vpath_bbox_irect (vpath, (ArtIRect *) & layervec->rect);
    layervec->svp = art_svp_from_vpath (vpath);
    art_svp_make_convex (layervec->svp);
    swfdec_rect_union_to_masked (&layer->rect, &layervec->rect, &s->irect);

    g_free (bpath0);
    g_free (bpath1);
    art_free (vpath0);
    g_free (vpath1);
    g_free (vpath);
  }

  swfdec_layer_render (s, layer);
  swfdec_layer_free (layer);
#endif
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

