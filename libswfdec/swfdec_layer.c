
#include <libart_lgpl/libart.h>
#include <math.h>

#include "swfdec_internal.h"


SwfdecRender *
swfdec_render_new (void)
{
  SwfdecRender *render;

  render = g_new0 (SwfdecRender, 1);

  return render;
}

void
swfdec_render_free (SwfdecRender * render)
{
  GList *g;

  for (g = g_list_first (render->layers); g; g = g_list_next (g)) {
    swfdec_layer_free ((SwfdecLayer *) g->data);
  }
  g_list_free (render->layers);

  g_free (render);
}


SwfdecLayer *
swfdec_layer_new (void)
{
  SwfdecLayer *layer;

  layer = g_new0 (SwfdecLayer, 1);

  layer->fills = g_array_new (FALSE, TRUE, sizeof (SwfdecLayerVec));
  layer->lines = g_array_new (FALSE, TRUE, sizeof (SwfdecLayerVec));

  return layer;
}

void
swfdec_layer_free (SwfdecLayer * layer)
{
  int i;
  SwfdecLayerVec *layervec;

  if (!layer) {
    g_warning ("layer==NULL");
    return;
  }

  for (i = 0; i < layer->fills->len; i++) {
    layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
    art_svp_free (layervec->svp);
    if (layervec->compose)
      g_free (layervec->compose);
  }
  g_array_free (layer->fills, TRUE);
  for (i = 0; i < layer->lines->len; i++) {
    layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
    art_svp_free (layervec->svp);
    if (layervec->compose)
      g_free (layervec->compose);
  }
  g_array_free (layer->lines, TRUE);

  if (layer->sublayers) {
    GList *g;

    for (g = g_list_first (layer->sublayers); g; g = g_list_next (g)) {
      swfdec_layer_free ((SwfdecLayer *) g->data);
    }
    g_list_free (layer->sublayers);
  }

  g_free (layer);
}

SwfdecLayer *
swfdec_layer_get (SwfdecDecoder * s, int depth)
{
  SwfdecLayer *l;
  GList *g;

  for (g = g_list_first (s->render->layers); g; g = g_list_next (g)) {
    l = (SwfdecLayer *) g->data;
    if (l->seg->depth == depth && l->seg->first_frame <= s->frame_number - 1
	&& (l->last_frame > s->frame_number - 1))
      return l;
  }

  return NULL;
}

SwfdecLayer *
swfdec_render_get_layer (SwfdecRender * render, int depth, int frame)
{
  SwfdecLayer *l;
  GList *g;

  if (!render)
    return NULL;

  for (g = g_list_first (render->layers); g; g = g_list_next (g)) {
    l = (SwfdecLayer *) g->data;
#if 0
    printf ("compare %d==%d %d <= %d < %d\n",
	l->seg->depth, depth, l->seg->first_frame, frame, l->last_frame);
#endif
    if (l->seg->depth == depth && l->first_frame <= frame
	&& frame < l->last_frame)
      return l;
  }

  return NULL;
}

SwfdecLayer *
swfdec_render_get_sublayer (SwfdecLayer * layer, int depth, int frame)
{
  SwfdecLayer *l;
  GList *g;

  if (!layer)
    return NULL;

  for (g = g_list_first (layer->sublayers); g; g = g_list_next (g)) {
    l = (SwfdecLayer *) g->data;
#if 0
    printf ("compare %d==%d %d <= %d < %d\n",
	l->seg->depth, depth, l->seg->first_frame, frame, l->last_frame);
#endif
    if (l->seg->depth == depth && l->first_frame <= frame
	&& frame < l->last_frame)
      return l;
  }

  return NULL;
}

SwfdecLayer *
swfdec_render_get_seg (SwfdecRender * render, SwfdecSpriteSegment * seg)
{
  SwfdecLayer *l;
  GList *g;

  for (g = g_list_first (render->layers); g; g = g_list_next (g)) {
    l = (SwfdecLayer *) g->data;
    if (l->seg == seg)
      return l;
  }

  return NULL;
}

void
swfdec_render_add_layer (SwfdecRender * render, SwfdecLayer * lnew)
{
  GList *g;
  SwfdecLayer *l;

  for (g = g_list_first (render->layers); g; g = g_list_next (g)) {
    l = (SwfdecLayer *) g->data;
    if (l->seg->depth < lnew->seg->depth) {
      render->layers = g_list_insert_before (render->layers, g, lnew);
      return;
    }
  }

  render->layers = g_list_append (render->layers, lnew);
}

#if 0
void
swfdec_render_delete_layer (SwfdecRender * render, SwfdecLayer * layer)
{
  GList *g;
  SwfdecLayer *l;

  for (g = g_list_first (render->layers); g; g = g_list_next (g)) {
    l = (SwfdecLayer *) g->data;
    if (l == layer) {
      render->layers = g_list_delete_link (render->layers, g);
      swfdec_layer_free (l);
      return;
    }
  }
}
#endif

void
swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec)
{
  ArtIRect rect;
  struct swf_svp_render_struct cb_data;
  ArtIRect drawrect;

  if (s->subpixel) {
    drawrect.x0 = s->drawrect.x0 * 3;
    drawrect.y0 = s->drawrect.y0;
    drawrect.x1 = s->drawrect.x1 * 3;
    drawrect.y1 = s->drawrect.y1;
    art_irect_intersect (&rect, &drawrect, &layervec->rect);
  } else {
    art_irect_intersect (&rect, &s->drawrect, &layervec->rect);
  }

  if (art_irect_empty (&rect))
    return;

  cb_data.subpixel = s->subpixel;
  cb_data.x0 = rect.x0;
  cb_data.x1 = rect.x1;
  if (cb_data.subpixel) {
    cb_data.buf = s->buffer + rect.y0 * s->stride + (rect.x0 / 3) * s->bytespp;
  } else {
    cb_data.buf = s->buffer + rect.y0 * s->stride + rect.x0 * s->bytespp;
  }
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
	layervec->compose ? s->compose_callback : s->callback, &cb_data);
  }

  s->pixels_rendered += (rect.x1 - rect.x0) * (rect.y1 - rect.y0);
}
