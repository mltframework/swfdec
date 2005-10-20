
#include <math.h>
#include <string.h>

#include "swfdec_internal.h"

void
swf_invalidate_irect (SwfdecDecoder * s, SwfdecRect * rect)
{
  if (swfdec_rect_is_empty (&s->render->drawrect)) {
    swfdec_rect_intersect (&s->render->drawrect, &s->irect, rect);
  } else {
    SwfdecRect tmp1, tmp2;

    swfdec_rect_copy (&tmp1, &s->render->drawrect);
    swfdec_rect_intersect (&tmp2, &s->irect, rect);
    swfdec_rect_union (&s->render->drawrect, &tmp1, &tmp2);
  }
}

int
tag_place_object_2 (SwfdecDecoder * s)
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
  SWFDEC_LOG ("  reserved = %d", reserved);
  if (reserved) {
    SWFDEC_WARNING ("  reserved bits non-zero %d", reserved);
  }
  SWFDEC_LOG ("  has_compose = %d", has_compose);
  SWFDEC_LOG ("  has_name = %d", has_name);
  SWFDEC_LOG ("  has_ratio = %d", has_ratio);
  SWFDEC_LOG ("  has_color_transform = %d", has_color_transform);
  SWFDEC_LOG ("  has_matrix = %d", has_matrix);
  SWFDEC_LOG ("  has_character = %d", has_character);

  oldlayer = swfdec_sprite_get_seg (s->main_sprite, depth, s->frame_number);
  swfdec_sprite_frame_remove_seg (&s->main_sprite->frames[s->frame_number],
      depth);

  layer = swfdec_spriteseg_new ();

  layer->depth = depth;

  /* FIXME: s->parse_sprite, probably */
  swfdec_sprite_frame_add_seg (&s->main_sprite->frames[s->frame_number], layer);

  if (has_character) {
    layer->id = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  id = %d", layer->id);
  } else {
    if (oldlayer)
      layer->id = oldlayer->id;
  }

  SWFDEC_INFO ("placing %sobject layer=%d id=%d",
      (has_character) ? "" : "(existing) ", depth, layer->id);

  if (has_matrix) {
    swfdec_bits_get_transform (bits, &layer->transform);
  } else {
    if (oldlayer) {
      memcpy (&layer->transform, &oldlayer->transform,
          sizeof (SwfdecTransform));
    }
  }
  if (has_color_transform) {
    swfdec_bits_get_color_transform (bits, &layer->color_transform);
    swfdec_bits_syncbits (bits);
  } else {
    if (oldlayer) {
      memcpy (&layer->color_transform, &oldlayer->color_transform,
          sizeof (SwfdecColorTransform));
    } else {
      layer->color_transform.mult[0] = 1;
      layer->color_transform.mult[1] = 1;
      layer->color_transform.mult[2] = 1;
      layer->color_transform.mult[3] = 1;
      layer->color_transform.add[0] = 0;
      layer->color_transform.add[1] = 0;
      layer->color_transform.add[2] = 0;
      layer->color_transform.add[3] = 0;
    }
  }
  if (has_ratio) {
    layer->ratio = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  ratio = %d", layer->ratio);
  } else {
    if (oldlayer)
      layer->ratio = oldlayer->ratio;
  }
  if (has_name) {
    g_free (swfdec_bits_get_string (bits));
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
tag_remove_object (SwfdecDecoder * s)
{
  int depth;
  int id;

  id = swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);
  swfdec_sprite_frame_remove_seg (&s->parse_sprite->frames[
      s->parse_sprite->parse_frame], depth);

  return SWF_OK;
}

int
tag_remove_object_2 (SwfdecDecoder * s)
{
  int depth;

  depth = swfdec_bits_get_u16 (&s->b);
  swfdec_sprite_frame_remove_seg (&s->parse_sprite->frames[
      s->parse_sprite->parse_frame], depth);

  return SWF_OK;
}

int
tag_show_frame (SwfdecDecoder * s)
{
  SWFDEC_DEBUG("show_frame %d of id %d", s->parse_sprite->parse_frame,
      s->parse_sprite->object.id);

  s->frame_number++;
  s->parse_sprite->parse_frame++;
  if (s->parse_sprite->parse_frame < s->parse_sprite->n_frames) {
    s->parse_sprite->frames[s->parse_sprite->parse_frame].segments =
      g_list_copy (
          s->parse_sprite->frames[s->parse_sprite->parse_frame - 1].segments);
  }

  return SWF_OK;
}

#ifdef unused
void
swfdec_spriteseg_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg)
{
  SwfdecObject *object;
  SwfdecObjectClass *klass;

  object = swfdec_object_get (s, seg->id);
  if (!object)
    return;

  klass = SWFDEC_OBJECT_GET_CLASS (object);

  if (klass->render) {
    klass->render (s, seg, object);
  }
}
#endif

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
