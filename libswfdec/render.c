
#include <libart_lgpl/libart.h>
#include <math.h>

#include "swfdec_internal.h"

void
swf_invalidate_irect (SwfdecDecoder * s, ArtIRect * rect)
{
  if (art_irect_empty (&s->drawrect)) {
    art_irect_intersect (&s->drawrect, &s->irect, rect);
  } else {
    ArtIRect tmp1, tmp2;

    art_irect_copy (&tmp1, &s->drawrect);
    art_irect_intersect (&tmp2, &s->irect, rect);
    art_irect_union (&s->drawrect, &tmp1, &tmp2);
  }
}

int
art_place_object_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int reserved;
  int has_compose;
  int has_name;
  int has_ratio;
  int has_color_transform;
  int has_matrix;
  int has_character;
  int move;
  int depth;
  SwfdecSpriteSegment *layer;
  SwfdecSpriteSegment *oldlayer;

  reserved = swfdec_bits_getbit (bits);
  has_compose = swfdec_bits_getbit (bits);
  has_name = swfdec_bits_getbit (bits);
  has_ratio = swfdec_bits_getbit (bits);
  has_color_transform = swfdec_bits_getbit (bits);
  has_matrix = swfdec_bits_getbit (bits);
  has_character = swfdec_bits_getbit (bits);
  move = swfdec_bits_getbit (bits);
  depth = swfdec_bits_get_u16 (bits);

  /* reserved is somehow related to sprites */
  SWFDEC_LOG("  reserved = %d", reserved);
  if (reserved) {
    SWFDEC_WARNING ("  reserved bits non-zero %d", reserved);
  }
  SWFDEC_LOG("  has_compose = %d", has_compose);
  SWFDEC_LOG("  has_name = %d", has_name);
  SWFDEC_LOG("  has_ratio = %d", has_ratio);
  SWFDEC_LOG("  has_color_transform = %d", has_color_transform);
  SWFDEC_LOG("  has_matrix = %d", has_matrix);
  SWFDEC_LOG("  has_character = %d", has_character);

  oldlayer =
      swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame);
  if (oldlayer) {
    oldlayer->last_frame = s->frame_number;
  }

  layer = swfdec_spriteseg_new ();

  layer->depth = depth;
  layer->first_frame = s->frame_number;
  layer->last_frame = 0;

  swfdec_sprite_add_seg (s->main_sprite, layer);

  if (has_character) {
    layer->id = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG("  id = %d", layer->id);
  } else {
    if (oldlayer)
      layer->id = oldlayer->id;
  }
  if (has_matrix) {
    swfdec_bits_get_art_matrix (bits, layer->transform);
  } else {
    if (oldlayer)
      art_affine_copy (layer->transform, oldlayer->transform);
  }
  if (has_color_transform) {
    swfdec_bits_get_art_color_transform (bits, layer->color_mult, layer->color_add);
    swfdec_bits_syncbits (bits);
  } else {
    if (oldlayer) {
      memcpy (layer->color_mult, oldlayer->color_mult, sizeof (double) * 4);
      memcpy (layer->color_add, oldlayer->color_add, sizeof (double) * 4);
    } else {
      layer->color_mult[0] = 1;
      layer->color_mult[1] = 1;
      layer->color_mult[2] = 1;
      layer->color_mult[3] = 1;
      layer->color_add[0] = 0;
      layer->color_add[1] = 0;
      layer->color_add[2] = 0;
      layer->color_add[3] = 0;
    }
  }
  if (has_ratio) {
    layer->ratio = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG("  ratio = %d", layer->ratio);
  } else {
    if (oldlayer)
      layer->ratio = oldlayer->ratio;
  }
  if (has_name) {
    free (swfdec_bits_get_string (bits));
  }
  if (has_compose) {
    int id;

    id = swfdec_bits_get_u16 (bits);
    SWFDEC_WARNING ("composing with %04x", id);
  }
  //swfdec_layer_prerender(s,layer);

  return SWF_OK;
}



int
art_remove_object (SwfdecDecoder * s)
{
  int depth;
  SwfdecSpriteSegment *seg;
  int id;

  id = swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);
  seg = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame);

  seg->last_frame = s->parse_sprite->parse_frame;

  return SWF_OK;
}

int
art_remove_object_2 (SwfdecDecoder * s)
{
  int depth;
  SwfdecSpriteSegment *seg;

  depth = swfdec_bits_get_u16 (&s->b);
  seg = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame);

  seg->last_frame = s->parse_sprite->parse_frame;

  return SWF_OK;
}

void
swfdec_render_clean (SwfdecRender * render, int frame)
{
  SwfdecLayer *l;
  GList *g, *g_next;

  for (g = g_list_first (render->layers); g; g = g_next) {
    g_next = g_list_next (g);
    l = (SwfdecLayer *) g->data;
    if (l->last_frame <= frame + 1) {
      render->layers = g_list_delete_link (render->layers, g);
      swfdec_layer_free (l);
    }
  }
}


int
art_show_frame (SwfdecDecoder * s)
{
  s->frame_number++;
  s->parse_sprite->parse_frame++;
  return SWF_OK;
}

void
swf_render_frame (SwfdecDecoder * s, int frame_index)
{
  SwfdecSpriteSegment *seg;
  SwfdecLayer *layer;
  //SwfdecLayer *oldlayer;
  GList *g;

  SWFDEC_DEBUG ("swf_render_frame");

  s->drawrect.x0 = 0;
  s->drawrect.x1 = 0;
  s->drawrect.y0 = 0;
  s->drawrect.y1 = 0;
  if (!s->buffer) {
    s->buffer = art_new (art_u8, s->stride * s->height);
    swf_invalidate_irect (s, &s->irect);
  }
swf_invalidate_irect (s, &s->irect);
  if (!s->tmp_scanline) {
    s->tmp_scanline = malloc (s->width);
  }

  SWFDEC_DEBUG ("rendering frame %d", frame_index);
#if 0
  for (g = g_list_last (s->main_sprite->layers); g; g = g_list_previous (g)) {
    seg = (SwfdecSpriteSegment *) g->data;

    SWFDEC_LOG("testing seg %d <= %d < %d",
	seg->first_frame, frame_index, seg->last_frame);
    if (seg->first_frame > frame_index)
      continue;
    if (seg->last_frame <= frame_index)
      continue;

    oldlayer = swfdec_render_get_layer (s->render, seg->depth,
        frame_index - 1);

    SWFDEC_ERROR ("layer %d seg=%p oldlayer=%p", seg->depth, seg, oldlayer);

    layer = swfdec_spriteseg_prerender (s, seg, oldlayer);
    if (layer == NULL) {
      SWFDEC_ERROR ("prerender returned NULL");
      continue; 
    }

    layer->last_frame = frame_index + 1;
    if (layer != oldlayer) {
      layer->first_frame = frame_index;
      swfdec_render_add_layer (s->render, layer);
      if (oldlayer)
	oldlayer->last_frame = frame_index;
    } else {
      SWFDEC_LOG("cache hit");
    }
  }
#endif

#if 0
  for (g = g_list_last (s->render->layers); g; g = g_list_previous (g)) {
    layer = (SwfdecLayer *) g->data;
    if (layer->seg->first_frame <= frame_index - 1 &&
        layer->last_frame == frame_index) {
      SWFDEC_LOG("invalidating (%d < %d == %d) %d %d %d %d",
	  layer->seg->first_frame, frame_index, layer->last_frame,
	  layer->rect.x0, layer->rect.x1, layer->rect.y0, layer->rect.y1);
      swf_invalidate_irect (s, &layer->rect);
    }
    if (layer->first_frame == frame_index) {
      swf_invalidate_irect (s, &layer->rect);
    }
  }
#endif

  SWFDEC_DEBUG ("inval rect %d %d %d %d", s->drawrect.x0, s->drawrect.x1,
      s->drawrect.y0, s->drawrect.y1);

  switch (s->colorspace) {
    case SWF_COLORSPACE_RGB565:
      art_rgb565_fillrect (s->buffer, s->stride, s->bg_color, &s->drawrect);
      break;
    case SWF_COLORSPACE_RGB888:
    default:
      art_rgb_fillrect (s->buffer, s->stride, s->bg_color, &s->drawrect);
      break;
  }

  for (g = g_list_last (s->main_sprite->layers); g; g = g_list_previous (g)) {
    SwfdecObject *object;

    seg = (SwfdecSpriteSegment *) g->data;

    SWFDEC_LOG("testing seg %d <= %d < %d",
	seg->first_frame, frame_index, seg->last_frame);
    if (seg->first_frame > frame_index)
      continue;
    if (seg->last_frame <= frame_index)
      continue;

    object = swfdec_object_get (s, seg->id);
    if (object) {
      layer = SWFDEC_OBJECT_GET_CLASS(object)->prerender (s, seg, object, NULL);

      if (layer) {
        swfdec_layer_render (s, layer);
        swfdec_layer_free (layer);
      } else {
        SWFDEC_WARNING ("prerender returned NULL");
      }
    } else {
      SWFDEC_DEBUG ("could not find object (id = %d)", seg->id);
    }
  }

#if 0
  for (g = g_list_last (s->render->layers); g; g = g_list_previous (g)) {
    layer = (SwfdecLayer *) g->data;
    if (layer == NULL || layer->seg) {
      SWFDEC_ERROR ("layer == NULL, odd\n");
      continue;
    }
    SWFDEC_ERROR ("rendering %d < %d <= %d",
	layer->seg->first_frame, frame_index, layer->last_frame);
    if (layer->seg->first_frame <= frame_index &&
        frame_index < layer->last_frame) {
      swfdec_layer_render (s, layer);
    }
  }
#endif
}

SwfdecLayer *
swfdec_spriteseg_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecLayer * oldlayer)
{
  SwfdecObject *object;
  SwfdecObjectClass *klass;

  object = swfdec_object_get (s, seg->id);
  if (!object)
    return NULL;

  klass = SWFDEC_OBJECT_GET_CLASS (object);

  if (klass->prerender) {
    return klass->prerender (s, seg, object, oldlayer);
  }

  SWFDEC_ERROR ("why is prerender NULL?");

  return NULL;
}

void
swfdec_layer_render (SwfdecDecoder * s, SwfdecLayer * layer)
{
  int i;
  SwfdecLayerVec *layervec;
  SwfdecLayer *child_layer;
  GList *g;

#if 0
  /* This rendering order seems to mostly fit the observed behavior
   * of Macromedia's player.  */
  /* or not */
  for (i = 0; i < MAX (layer->fills->len, layer->lines->len); i++) {
    if (i < layer->lines->len) {
      layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
      swfdec_layervec_render (s, layervec);
    }
    if (i < layer->fills->len) {
      layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
      swfdec_layervec_render (s, layervec);
    }
  }
#else
  for (i = 0; i < layer->fills->len; i++) {
    layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
    swfdec_layervec_render (s, layervec);
  }
  for (i = 0; i < layer->lines->len; i++) {
    layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
    swfdec_layervec_render (s, layervec);
  }
#endif

  for (g = g_list_first (layer->sublayers); g; g = g_list_next (g)) {
    child_layer = (SwfdecLayer *) g->data;
    swfdec_layer_render (s, child_layer);
  }
}

