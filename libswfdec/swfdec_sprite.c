
#include <string.h>

#include "swfdec_internal.h"

#include <swfdec_sprite.h>

static void swfdec_sprite_render (SwfdecDecoder * s,
    SwfdecSpriteSegment * seg, SwfdecObject * object);

SWFDEC_OBJECT_BOILERPLATE (SwfdecSprite, swfdec_sprite)

     static void swfdec_sprite_base_init (gpointer g_class)
{

}

static void
swfdec_sprite_class_init (SwfdecSpriteClass * g_class)
{
  SWFDEC_OBJECT_CLASS (g_class)->render = swfdec_sprite_render;
}

static void
swfdec_sprite_init (SwfdecSprite * sprite)
{

}

static void
swfdec_sprite_dispose (SwfdecSprite * sprite)
{
  GList *g;
  int i;

  for (g = g_list_first (sprite->layers); g; g = g_list_next (g)) {
    SwfdecSpriteSegment *seg = (SwfdecSpriteSegment *) g->data;

    swfdec_spriteseg_free (seg);
  }
  g_list_free (sprite->layers);

  if (sprite->sound_chunks) {
    for (i = 0; i < sprite->n_frames; i++) {
      if (sprite->sound_chunks[i]) {
        swfdec_buffer_unref (sprite->sound_chunks[i]);
      }
    }
    g_free (sprite->sound_chunks);
  }
  if (sprite->actions) {
    for (i = 0; i < sprite->n_frames; i++) {
      if (sprite->actions[i]) {
        swfdec_buffer_unref (sprite->actions[i]);
      }
    }
    g_free (sprite->actions);
  }
  if (sprite->sound_play) {
    for (i = 0; i < sprite->n_frames; i++) {
      if (sprite->sound_play[i]) {
        g_free (sprite->sound_play[i]);
      }
    }
    g_free(sprite->sound_play);
  }

}

void
swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite,
    SwfdecBuffer * chunk, int frame)
{
  g_assert (sprite->sound_chunks != NULL);

  sprite->sound_chunks[frame] = chunk;
  swfdec_buffer_ref (chunk);
}

void
swfdec_sprite_add_action (SwfdecSprite * sprite,
    SwfdecBuffer * actions, int frame)
{
  g_assert (sprite->actions != NULL);

  sprite->actions[frame] = actions;
  swfdec_buffer_ref (actions);
}

static void
swfdec_sprite_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
  SwfdecSprite *sprite = SWFDEC_SPRITE (object);
  SwfdecTransform save_transform;
  GList *g;
  SwfdecRenderState *state;
  int clip_depth = 0;

  //memcpy (&layer->transform, &seg->transform, sizeof(SwfdecTransform));

  state = swfdec_render_get_object_state (s->render, seg->depth, seg->id);
  if (state == NULL) {
    state = g_new0 (SwfdecRenderState, 1);
    state->layer = seg->depth;
    state->id = seg->id;
    state->frame_index = 0;
    s->render->object_states = g_list_prepend (s->render->object_states, state);
    SWFDEC_INFO ("new rendering state for layer %d", seg->depth);
  } else {
    if (state->id != seg->id) {
      SWFDEC_INFO ("old id=%d new id=%d", state->id, seg->id);
      state->id = seg->id;
      state->frame_index = 0;
      SWFDEC_INFO ("resetting rendering state of layer %d", seg->depth);
    }
  }
  if (state->frame_index >= sprite->n_frames) {
    state->frame_index = 0;
    SWFDEC_INFO ("looping rendering state of layer %d", seg->depth);
  }

  memcpy (&save_transform, &s->transform, sizeof (SwfdecTransform));
  swfdec_transform_multiply (&s->transform, &seg->transform, &save_transform);

  for (g = g_list_last (sprite->layers); g; g = g_list_previous (g)) {
    SwfdecObject *child_object;
    SwfdecSpriteSegment *child_seg;

    child_seg = (SwfdecSpriteSegment *) g->data;

    if (child_seg->first_frame > state->frame_index)
      continue;
    if (child_seg->last_frame <= state->frame_index)
      continue;

    /* FIXME need to clip layers instead */
    if (child_seg->clip_depth) {
      SWFDEC_INFO ("clip_depth=%d", child_seg->clip_depth);
      clip_depth = child_seg->clip_depth;
    }

    if (clip_depth && child_seg->depth <= clip_depth) {
      SWFDEC_INFO ("clipping depth=%d", child_seg->clip_depth);
      continue;
    }

    child_object = swfdec_object_get (s, child_seg->id);
    if (child_object) {
#if 0
      /* FIXME for now, don't render sprites inside sprites */
      if (!SWFDEC_IS_SPRITE (child_object)) {
        SWFDEC_OBJECT_GET_CLASS (child_object)->render (s, child_seg,
            child_object);
      } else {
        SWFDEC_WARNING ("not rendering sprite inside sprite");
      }
#else
      SWFDEC_OBJECT_GET_CLASS (child_object)->render (s, child_seg,
          child_object);
#endif
    } else {
      SWFDEC_DEBUG ("could not find object (id = %d)", child_seg->id);
    }
  }
  memcpy (&s->transform, &save_transform, sizeof (SwfdecTransform));
}

#if 0
static SwfdecLayer *
swfdec_sprite_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object, SwfdecLayer * oldlayer)
{
  SwfdecLayer *layer;
  SwfdecLayer *child_layer;
  SwfdecLayer *old_child_layer;
  GList *g;

  //SwfdecSprite *child_decoder = SWFDEC_SPRITE (object);
  /* FIXME */
  //SwfdecSprite *sprite = child_decoder->main_sprite;
  SwfdecSprite *sprite = NULL;
  SwfdecSpriteSegment *tmpseg;
  SwfdecSpriteSegment *child_seg;
  int frame;

  if (oldlayer && oldlayer->seg == seg && sprite->n_frames == 1)
    return oldlayer;

  layer = swfdec_layer_new ();
  layer->seg = seg;

  /* Not sure why this is wrong.  Somehow the top-level transform
   * gets applied twice. */
  //art_affine_multiply(layer->transform, seg->transform, s->transform);
  memcpy (&layer->transform, &seg->transform, sizeof (SwfdecTransform));

  if (oldlayer) {
    layer->frame_number = oldlayer->frame_number + 1;
    if (layer->frame_number >= sprite->n_frames)
      layer->frame_number = 0;
    SWFDEC_DEBUG
        ("iterating old sprite (depth=%d) old_frame=%d frame=%d n_frames=%d\n",
        seg->depth, oldlayer->frame_number, layer->frame_number,
        sprite->n_frames);
  } else {
    SWFDEC_LOG ("iterating new sprite (depth=%d)", seg->depth);
    layer->frame_number = 0;
  }
  frame = layer->frame_number;

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  SWFDEC_LOG ("swfdec_sprite_render %d frame %d", object->id,
      layer->frame_number);

  for (g = g_list_last (sprite->layers); g; g = g_list_previous (g)) {
    child_seg = (SwfdecSpriteSegment *) g->data;

    if (child_seg->first_frame > frame)
      continue;
    if (child_seg->last_frame <= frame)
      continue;
    SWFDEC_LOG ("rendering layer %d", child_seg->depth);

    tmpseg = swfdec_spriteseg_dup (child_seg);
    swfdec_transform_multiply (&tmpseg->transform,
        &child_seg->transform, &layer->transform);

#if 0
    old_child_layer = swfdec_render_get_sublayer (layer,
        child_seg->depth, layer->frame_number - 1);
#endif
    old_child_layer = NULL;

    child_layer = swfdec_spriteseg_render (s, tmpseg, old_child_layer);
    if (child_layer) {
      layer->sublayers = g_list_append (layer->sublayers, child_layer);

      swfdec_rect_union_to_masked (&layer->rect, &child_layer->rect, &s->irect);
    }

    swfdec_spriteseg_free (tmpseg);
  }

  return layer;
}
#endif

#if 0
void
swfdec_sprite_render (SwfdecDecoder * s, SwfdecLayer * parent_layer,
    SwfdecObject * parent_object)
{
  SwfdecLayer *child_layer;
  SwfdecSprite *s2 = SWFDEC_SPRITE (parent_object);
  GList *g;

  SWFDEC_LOG ("rendering sprite frame %d of %d",
      parent_layer->frame_number, s2->n_frames);
  for (g = g_list_first (parent_layer->sublayers); g; g = g_list_next (g)) {
    child_layer = (SwfdecLayer *) g->data;
    if (!child_layer)
      continue;
    swfdec_layer_render (s, child_layer);
  }
}
#endif

int
tag_func_define_sprite (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecBits parse;
  int id;
  SwfdecSprite *sprite;
  int ret;
  SwfdecBits save_bits;

  save_bits = s->b;

  id = swfdec_bits_get_u16 (bits);
  sprite = swfdec_object_new (SWFDEC_TYPE_SPRITE);
  SWFDEC_OBJECT (sprite)->id = id;
  s->objects = g_list_append (s->objects, sprite);

  SWFDEC_LOG ("  ID: %d", id);

  sprite->n_frames = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("n_frames = %d", sprite->n_frames);

  sprite->sound_chunks = g_malloc0 (sizeof (gpointer) * sprite->n_frames);
  sprite->actions = g_malloc0 (sizeof (gpointer) * sprite->n_frames);
  sprite->sound_play = g_malloc0 (sizeof (gpointer) * sprite->n_frames);

  memcpy (&parse, bits, sizeof (SwfdecBits));

  while (1) {
    unsigned char *endptr;
    int x;
    int tag;
    int tag_len;
    SwfdecBuffer *buffer;
    SwfdecTagFunc *func;

    //SWFDEC_INFO ("sprite parsing at %d", parse.ptr - parse.buffer->data);
    x = swfdec_bits_get_u16 (&parse);
    tag = (x >> 6) & 0x3ff;
    tag_len = x & 0x3f;
    if (tag_len == 0x3f) {
      tag_len = swfdec_bits_get_u32 (&parse);
    }
    SWFDEC_INFO ("sprite parsing at %d, tag %d %s, length %d",
        parse.ptr - parse.buffer->data, tag,
        swfdec_decoder_get_tag_name (tag), tag_len);
    //SWFDEC_DEBUG ("tag %d %s", tag, swfdec_decoder_get_tag_name (tag));

    if (tag_len > 0) {
      buffer = swfdec_buffer_new_subbuffer (parse.buffer,
          parse.ptr - parse.buffer->data, tag_len);
      s->b.buffer = buffer;
      s->b.ptr = buffer->data;
      s->b.idx = 0;
      s->b.end = buffer->data + buffer->length;
    } else {
      buffer = NULL;
      s->b.buffer = NULL;
      s->b.ptr = NULL;
      s->b.idx = 0;
      s->b.end = NULL;
    }

    func = swfdec_decoder_get_tag_func (tag);
    if (func == NULL) {
      SWFDEC_WARNING ("tag function not implemented for %d %s",
          tag, swfdec_decoder_get_tag_name (tag));
    } else {
      endptr = parse.ptr + tag_len;
      s->parse_sprite = sprite;
      ret = func (s);
      s->parse_sprite = NULL;

      swfdec_bits_syncbits (bits);
      if (tag_len > 0) {
        if (s->b.ptr < endptr) {
          SWFDEC_WARNING ("early parse finish (%d bytes)", endptr - s->b.ptr);
        }
        if (s->b.ptr > endptr) {
          SWFDEC_WARNING ("parse overrun (%d bytes)", s->b.ptr - endptr);
        }
      }

      parse.ptr = endptr;
    }

    if (buffer)
      swfdec_buffer_unref (buffer);

    if (tag == 0)
      break;
  }

  s->b = save_bits;
  s->b.ptr += s->b.buffer->length;

  return SWF_OK;
}

SwfdecSpriteSegment *
swfdec_sprite_get_seg (SwfdecSprite * sprite, int depth, int frame)
{
  SwfdecSpriteSegment *seg;
  GList *g;

  for (g = g_list_first (sprite->layers); g; g = g_list_next (g)) {
    seg = (SwfdecSpriteSegment *) g->data;
    if (seg->depth == depth && seg->first_frame <= frame
        && seg->last_frame > frame)
      return seg;
  }

  return NULL;
}

void
swfdec_sprite_add_seg (SwfdecSprite * sprite, SwfdecSpriteSegment * segnew)
{
  GList *g;
  SwfdecSpriteSegment *seg;

  for (g = g_list_first (sprite->layers); g; g = g_list_next (g)) {
    seg = (SwfdecSpriteSegment *) g->data;
    if (seg->depth < segnew->depth) {
      sprite->layers = g_list_insert_before (sprite->layers, g, segnew);
      return;
    }
  }

  sprite->layers = g_list_append (sprite->layers, segnew);
}

SwfdecSpriteSegment *
swfdec_spriteseg_new (void)
{
  SwfdecSpriteSegment *seg;

  seg = g_new0 (SwfdecSpriteSegment, 1);

  return seg;
}

SwfdecSpriteSegment *
swfdec_spriteseg_dup (SwfdecSpriteSegment * seg)
{
  SwfdecSpriteSegment *newseg;

  newseg = g_new (SwfdecSpriteSegment, 1);
  memcpy (newseg, seg, sizeof (*seg));

  return newseg;
}

void
swfdec_spriteseg_free (SwfdecSpriteSegment * seg)
{
  g_free (seg);
}

int
swfdec_spriteseg_place_object_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int reserved;
  int has_clip_depth;
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
  has_clip_depth = swfdec_bits_getbit (bits);
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
  SWFDEC_LOG ("  has_clip_depth = %d", has_clip_depth);
  SWFDEC_LOG ("  has_name = %d", has_name);
  SWFDEC_LOG ("  has_ratio = %d", has_ratio);
  SWFDEC_LOG ("  has_color_transform = %d", has_color_transform);
  SWFDEC_LOG ("  has_matrix = %d", has_matrix);
  SWFDEC_LOG ("  has_character = %d", has_character);

  oldlayer = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame);
  if (oldlayer) {
    oldlayer->last_frame = s->parse_sprite->parse_frame;
  }

  layer = swfdec_spriteseg_new ();

  layer->depth = depth;
  layer->first_frame = s->parse_sprite->parse_frame;
  layer->last_frame = s->parse_sprite->n_frames;

  swfdec_sprite_add_seg (s->parse_sprite, layer);

  if (has_character) {
    layer->id = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  id = %d", layer->id);
  } else {
    if (oldlayer)
      layer->id = oldlayer->id;
  }

  SWFDEC_INFO ("%splacing object layer=%d id=%d first_frame=%d",
      (has_character) ? "" : "[re-]", depth, layer->id, layer->first_frame);

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
  swfdec_bits_syncbits (bits);
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
  if (has_clip_depth) {
    layer->clip_depth = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("clip_depth = %04x", layer->clip_depth);
  } else {
    if (oldlayer) {
      layer->clip_depth = oldlayer->clip_depth;
    }
  }

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object (SwfdecDecoder * s)
{
  int depth;
  SwfdecSpriteSegment *seg;
  int id;

  id = swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);
  seg = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame - 1);

  if (seg) {
    seg->last_frame = s->parse_sprite->parse_frame;
  } else {
    SWFDEC_WARNING ("could not find object to remove (depth %d, frame %d)",
        depth, s->parse_sprite->parse_frame - 1);
  }

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s)
{
  int depth;
  SwfdecSpriteSegment *seg;

  depth = swfdec_bits_get_u16 (&s->b);
  seg = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame - 1);

  if (seg) {
    seg->last_frame = s->parse_sprite->parse_frame;
  } else {
    SWFDEC_WARNING ("could not find object to remove (depth %d, frame %d)",
        depth, s->parse_sprite->parse_frame - 1);
  }

  return SWF_OK;
}
