
#include <math.h>
#include <string.h>
#include <libart_lgpl/libart.h>

#include "swfdec_internal.h"
#include "art.h"

typedef void (*ArtSVPRenderAAFunc) (void *callback_data, int y,
    int start, ArtSVPRenderAAStep *steps, int n_steps);

void
swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec)
{
  SwfdecRect rect;
  struct swf_svp_render_struct cb_data;

  swfdec_rect_intersect (&rect, &s->drawrect, &layervec->rect);

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
	(ArtSVPRenderAAFunc)(layervec->compose ? s->compose_callback : s->callback), &cb_data);
  }

  s->pixels_rendered += (rect.x1 - rect.x0) * (rect.y1 - rect.y0);
}


ArtSVP *
art_svp_translate (ArtSVP * svp, double x, double y)
{
  ArtSVP *newsvp;
  int i, j;

  newsvp = g_malloc (sizeof (ArtSVP) + sizeof (ArtSVPSeg) * svp->n_segs);

  newsvp->n_segs = svp->n_segs;
  for (i = 0; i < svp->n_segs; i++) {
    newsvp->segs[i].n_points = svp->segs[i].n_points;
    newsvp->segs[i].dir = svp->segs[i].dir;
    newsvp->segs[i].bbox.x0 = svp->segs[i].bbox.x0 + x;
    newsvp->segs[i].bbox.x1 = svp->segs[i].bbox.x1 + x;
    newsvp->segs[i].bbox.y0 = svp->segs[i].bbox.y0 + y;
    newsvp->segs[i].bbox.y1 = svp->segs[i].bbox.y1 + y;
    newsvp->segs[i].points = g_new (ArtPoint, svp->segs[i].n_points);
    for (j = 0; j < svp->segs[i].n_points; j++) {
      newsvp->segs[i].points[j].x = svp->segs[i].points[j].x + x;
      newsvp->segs[i].points[j].y = svp->segs[i].points[j].y + y;
    }
  }

  return newsvp;
}

void
art_svp_bbox (ArtSVP * svp, ArtIRect * box)
{
  ArtDRect dbox;

  art_drect_svp (&dbox, svp);

  box->x0 = floor (dbox.x0);
  box->x1 = ceil (dbox.x1);
  box->y0 = floor (dbox.y0);
  box->y1 = ceil (dbox.y1);
}

SwfdecLayer *
swfdec_shape_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * obj, SwfdecLayer * oldlayer)
{
  SwfdecLayer *layer;
  SwfdecShape *shape = SWFDEC_SHAPE (obj);
  int i;
  SwfdecLayerVec *layervec;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;

  if (oldlayer && oldlayer->seg == seg)
    return oldlayer;

  layer = swfdec_layer_new ();
  layer->seg = seg;
  swfdec_transform_multiply (&layer->transform, &seg->transform, &s->transform);

#if 1
  if (oldlayer &&
      oldlayer->seg->id == seg->id &&
      swfdec_transform_is_translation (&layer->transform,
        &oldlayer->transform)) {
    double x, y;
    SwfdecLayerVec *oldlayervec;

    x = layer->transform.trans[4] - oldlayer->transform.trans[4];
    y = layer->transform.trans[5] - oldlayer->transform.trans[5];

    SWFDEC_LOG("translation");

    g_array_set_size (layer->fills, shape->fills->len);
    for (i = 0; i < shape->fills->len; i++) {
      oldlayervec = &g_array_index (oldlayer->fills, SwfdecLayerVec, i);
      layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
      shapevec = g_ptr_array_index (shape->fills, i);

      layervec->svp = art_svp_translate (oldlayervec->svp, x, y);
      layervec->color = swfdec_color_apply_transform (shapevec->color,
          &seg->color_transform);
      art_svp_bbox (layervec->svp, (ArtIRect *)&layervec->rect);
      swfdec_rect_union_to_masked (&layer->rect, &layervec->rect, &s->irect);
      layervec->compose = NULL;
      if (shapevec->fill_id) {
	swfdec_shape_compose (s, layervec, shapevec, &layer->transform);
      }
      if (shapevec->grad) {
	swfdec_shape_compose_gradient (s, layervec, shapevec, &layer->transform,
	    seg);
      }
    }

    g_array_set_size (layer->lines, shape->lines->len);
    for (i = 0; i < shape->lines->len; i++) {
      oldlayervec = &g_array_index (oldlayer->lines, SwfdecLayerVec, i);
      layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
      shapevec = g_ptr_array_index (shape->lines, i);

      layervec->svp = art_svp_translate (oldlayervec->svp, x, y);
      layervec->color = swfdec_color_apply_transform (shapevec->color,
	  &seg->color_transform);
      art_svp_bbox (layervec->svp, (ArtIRect *)&layervec->rect);
      swfdec_rect_union_to_masked (&layer->rect, &layervec->rect, &s->irect);
      layervec->compose = NULL;
#if 0
      if (shapevec->fill_id) {
	swfdec_shape_compose (s, layervec, shapevec, layer->transform);
      }
#endif
    }

    return layer;
  }
#endif

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  g_array_set_size (layer->fills, shape->fills->len);
  for (i = 0; i < shape->fills->len; i++) {
    ArtVpath *vpath, *vpath0, *vpath1;
    ArtBpath *bpath0, *bpath1;
    SwfdecTransform trans;

    layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
    shapevec = g_ptr_array_index (shape->fills, i);
    shapevec2 = g_ptr_array_index (shape->fills2, i);

    memcpy (&trans, &layer->transform, sizeof (SwfdecTransform));

    bpath0 = swfdec_art_bpath_from_points (shapevec->path, &trans);
    bpath1 = swfdec_art_bpath_from_points (shapevec2->path, &trans);
    vpath0 = art_bez_path_to_vec (bpath0, s->flatness);
    vpath1 = art_bez_path_to_vec (bpath1, s->flatness);
    vpath1 = art_vpath_reverse_free (vpath1);
    vpath = art_vpath_cat (vpath0, vpath1);
    art_vpath_bbox_irect (vpath, (ArtIRect *)&layervec->rect);
    layervec->svp = art_svp_from_vpath (vpath);
    art_svp_make_convex (layervec->svp);
    swfdec_rect_union_to_masked (&layer->rect, &layervec->rect, &s->irect);

    art_free (bpath0);
    art_free (bpath1);
    art_free (vpath0);
    art_free (vpath1);
    art_free (vpath);

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
  }

  g_array_set_size (layer->lines, shape->lines->len);
  for (i = 0; i < shape->lines->len; i++) {
    ArtVpath *vpath;
    ArtBpath *bpath;
    double width;
    int half_width;
    SwfdecTransform trans;

    layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
    shapevec = g_ptr_array_index (shape->lines, i);

    memcpy (&trans, &layer->transform, sizeof (SwfdecTransform));

    bpath = swfdec_art_bpath_from_points (shapevec->path, &trans);
    vpath = art_bez_path_to_vec (bpath, s->flatness);
    art_vpath_bbox_irect (vpath, (ArtIRect *)&layervec->rect);

    /* FIXME for subpixel */
    width = shapevec->width * swfdec_transform_get_expansion (&trans);
    if (width < 1)
      width = 1;

    half_width = floor (width * 0.5) + 1;
    layervec->rect.x0 -= half_width;
    layervec->rect.y0 -= half_width;
    layervec->rect.x1 += half_width;
    layervec->rect.y1 += half_width;
    swfdec_rect_union_to_masked (&layer->rect, &layervec->rect, &s->irect);
    layervec->svp = art_svp_vpath_stroke (vpath,
	ART_PATH_STROKE_JOIN_ROUND,
	ART_PATH_STROKE_CAP_ROUND, width, 1.0, s->flatness);

    art_free (vpath);
    art_free (bpath);
    layervec->color = swfdec_color_apply_transform (shapevec->color,
	&seg->color_transform);
  }

  return layer;
}


SwfdecLayer *
swfdec_text_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object, SwfdecLayer * oldlayer)
{
  int i;
  SwfdecText *text;
  SwfdecLayerVec *layervec;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;
  SwfdecObject *fontobj;
  SwfdecLayer *layer;

  if (oldlayer && oldlayer->seg == seg)
    return oldlayer;

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
      SWFDEC_ERROR("failed getting glyph %d\n", glyph->glyph);
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
    art_vpath_bbox_irect (vpath, (ArtIRect *)&layervec->rect);
    layervec->svp = art_svp_from_vpath (vpath);
    art_svp_make_convex (layervec->svp);
    swfdec_rect_union_to_masked (&layer->rect, &layervec->rect, &s->irect);

    art_free (bpath0);
    art_free (bpath1);
    art_free (vpath0);
    art_free (vpath1);
    art_free (vpath);
  }

  return layer;
}

void
swf_config_colorspace (SwfdecDecoder * s)
{
  switch (s->colorspace) {
    case SWF_COLORSPACE_RGB565:
      s->stride = s->width * 2;
      s->bytespp = 2;
      s->callback = (void *)art_rgb565_svp_alpha_callback;
      s->compose_callback = (void *)art_rgb565_svp_alpha_callback;
      s->fillrect = art_rgb565_fillrect;
      break;
    case SWF_COLORSPACE_RGB888:
    default:
      s->stride = s->width * 4;
      s->bytespp = 4;
      s->callback = (void *)art_rgb_svp_alpha_callback;
      s->compose_callback = (void *)art_rgb_svp_alpha_compose_callback;
      s->fillrect = art_rgb_fillrect;
      break;
  }
}

void swfdec_render_layervec_free (SwfdecLayerVec *layervec)
{
  if (layervec->svp) {
    art_svp_free (layervec->svp);
  }

}

