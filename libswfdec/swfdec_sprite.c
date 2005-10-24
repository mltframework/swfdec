
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
  //GList *g;
  int i;

  if (sprite->frames) {
    for (i = 0; i < sprite->n_frames; i++) {
      if (sprite->frames[i].sound_chunk) {
        swfdec_buffer_unref (sprite->frames[i].sound_chunk);
      }
      if (sprite->frames[i].action) {
        swfdec_buffer_unref (sprite->frames[i].action);
      }
      if (sprite->frames[i].sound_play) {
        g_free (sprite->frames[i].sound_play);
      }
    }
#if 0
    for (g = g_list_first (frame->segments); g; g = g_list_next (g)) {
      SwfdecSpriteSegment *seg = (SwfdecSpriteSegment *) g->data;

      swfdec_spriteseg_free (seg);
    }
#endif
    g_free(sprite->frames);
  }

}

void
swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite,
    SwfdecBuffer * chunk, int frame)
{
  g_assert (sprite->frames != NULL);

  sprite->frames[frame].sound_chunk = chunk;
  swfdec_buffer_ref (chunk);
}

void
swfdec_sprite_add_action (SwfdecSprite * sprite,
    SwfdecBuffer * actions, int frame)
{
  g_assert (sprite->frames != NULL);

  sprite->frames[frame].action = actions;
  swfdec_buffer_ref (actions);
}

void
swfdec_sprite_render_iterate (SwfdecDecoder * s, SwfdecSpriteSegment *seg,
    SwfdecRender *render)
{  
  SwfdecObject *obj;
  SwfdecSprite *sprite, *save_parse_sprite;
  //SwfdecSpriteSegment *child_seg;
  SwfdecSpriteSegment *save_parse_sprite_seg;
  //GList *g;

  if (seg->stopped)
    return;

  if (seg->id != 0) {
    obj = swfdec_object_get (s, seg->id);
    if (!SWFDEC_IS_SPRITE(obj))
      return;
    sprite = SWFDEC_SPRITE(obj);
  } else {
    sprite = s->main_sprite;
  }

  SWFDEC_INFO ("sprite %d frame %d", seg->id, render->frame_index);

  save_parse_sprite = s->parse_sprite;
  save_parse_sprite_seg = s->parse_sprite_seg;
  s->parse_sprite = sprite;
  s->parse_sprite_seg = seg;

  if (sprite->frames[render->frame_index].action) {
    s->execute_list = g_list_append (s->execute_list,
        sprite->frames[render->frame_index].action);
  }

#if 0
  if (!seg->ran_load && seg->clipevent[CLIPEVENT_LOAD]) {
    seg->ran_load = TRUE;
    s->execute_list = g_list_append (s->execute_list,
        seg->clipevent[CLIPEVENT_LOAD]);
  }
  if (seg->clipevent[CLIPEVENT_ENTERFRAME]) {
    s->execute_list = g_list_append (s->execute_list,
        seg->clipevent[CLIPEVENT_ENTERFRAME]);
  }
#endif

#if 0
  /* FIXME this is wrong */
  if (0) {
    for (g = g_list_last (sprite->layers); g; g = g_list_previous (g)) {
      child_seg = (SwfdecSpriteSegment *) g->data;

      swfdec_sprite_render_iterate(s, child_seg, NULL);
    }
  }
#endif

  s->parse_sprite = save_parse_sprite;
  s->parse_sprite_seg = save_parse_sprite_seg;
}

static void
swfdec_sprite_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
  SwfdecSprite *sprite = SWFDEC_SPRITE (object);
  SwfdecSpriteFrame *frame;
  SwfdecTransform save_transform;
  GList *g;
  int clip_depth = 0;

  //memcpy (&layer->transform, &seg->transform, sizeof(SwfdecTransform));

  memcpy (&save_transform, &s->transform, sizeof (SwfdecTransform));
  swfdec_transform_multiply (&s->transform, &seg->transform, &save_transform);

  /* FIXME 0 is wrong */
  frame = &sprite->frames[0];
  for (g = g_list_last (frame->segments); g; g = g_list_previous (g)) {
    SwfdecObject *child_object;
    SwfdecSpriteSegment *child_seg;

    child_seg = (SwfdecSpriteSegment *) g->data;

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

SwfdecSpriteSegment *
swfdec_sprite_get_seg (SwfdecSprite * sprite, int depth, int frame_index)
{
  SwfdecSpriteSegment *seg;
  GList *g;
  SwfdecSpriteFrame *frame;

  frame = &sprite->frames[frame_index];
  for (g = g_list_first (frame->segments); g; g = g_list_next (g)) {
    seg = (SwfdecSpriteSegment *) g->data;
    if (seg->depth == depth)
      return seg;
  }

  return NULL;
}

void
swfdec_sprite_frame_add_seg (SwfdecSpriteFrame * frame, SwfdecSpriteSegment * segnew)
{
  SwfdecSpriteSegment *seg;
  GList *g;

  for (g = g_list_first (frame->segments); g; g = g_list_next (g)) {
    seg = (SwfdecSpriteSegment *) g->data;
    if (seg->depth < segnew->depth) {
      frame->segments = g_list_insert_before (frame->segments, g, segnew);
      return;
    }
  }
  frame->segments = g_list_append (frame->segments, segnew);
}

void
swfdec_sprite_frame_remove_seg (SwfdecSpriteFrame * frame, int depth)
{
  SwfdecSpriteSegment *seg;
  GList *g;

  for (g = g_list_first (frame->segments); g; g = g_list_next (g)) {
    seg = (SwfdecSpriteSegment *) g->data;
    if (seg->depth == depth) {
      frame->segments = g_list_delete_link (frame->segments, g);
      return;
    }
  }
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
  if (seg->name)
    newseg->name = g_strdup(seg->name);

  return newseg;
}

void
swfdec_spriteseg_free (SwfdecSpriteSegment * seg)
{
  int i;

  for (i = 0; i < CLIPEVENT_MAX; i++) {
    if (seg->clipevent[i])
      swfdec_buffer_unref(seg->clipevent[i]);
  }
  if (seg->name)
    g_free (seg->name);
  g_free (seg);
}

static int
swfdec_get_clipeventflags (SwfdecDecoder * s, SwfdecBits * bits)
{
  if (s->version <= 5) {
    return swfdec_bits_get_u16 (bits);
  } else {
    return swfdec_bits_get_u32 (bits);
  }
}

int
swfdec_spriteseg_place_object_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int has_clip_actions;
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

  has_clip_actions = swfdec_bits_getbit (bits);
  has_clip_depth = swfdec_bits_getbit (bits);
  has_name = swfdec_bits_getbit (bits);
  has_ratio = swfdec_bits_getbit (bits);
  has_color_transform = swfdec_bits_getbit (bits);
  has_matrix = swfdec_bits_getbit (bits);
  has_character = swfdec_bits_getbit (bits);
  move = swfdec_bits_getbit (bits);
  depth = swfdec_bits_get_u16 (bits);

  /* reserved is somehow related to sprites */
  SWFDEC_LOG ("  has_clip_actions = %d", has_clip_actions);
  SWFDEC_LOG ("  has_clip_depth = %d", has_clip_depth);
  SWFDEC_LOG ("  has_name = %d", has_name);
  SWFDEC_LOG ("  has_ratio = %d", has_ratio);
  SWFDEC_LOG ("  has_color_transform = %d", has_color_transform);
  SWFDEC_LOG ("  has_matrix = %d", has_matrix);
  SWFDEC_LOG ("  has_character = %d", has_character);

  oldlayer = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame);
  swfdec_sprite_frame_remove_seg (&s->parse_sprite->frames[
      s->parse_sprite->parse_frame], depth);

  layer = swfdec_spriteseg_new ();

  layer->depth = depth;

  swfdec_sprite_frame_add_seg (
      &s->parse_sprite->frames[s->parse_sprite->parse_frame], layer);

  if (has_character) {
    layer->id = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  id = %d", layer->id);
  } else {
    if (oldlayer)
      layer->id = oldlayer->id;
  }

  SWFDEC_INFO ("%splacing object layer=%d id=%d",
      (has_character) ? "" : "[re-]", depth, layer->id);

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
    layer->name = swfdec_bits_get_string (bits);
  }
  if (has_clip_depth) {
    layer->clip_depth = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("clip_depth = %04x", layer->clip_depth);
  } else {
    if (oldlayer) {
      layer->clip_depth = oldlayer->clip_depth;
    }
  }
  if (has_clip_actions) {
    int reserved, clip_event_flags, event_flags, record_size;
    int i;

    reserved = swfdec_bits_get_u16 (bits);
    clip_event_flags = swfdec_get_clipeventflags (s, bits);

    while ((event_flags = swfdec_get_clipeventflags (s, bits)) != 0) {
      record_size = swfdec_bits_get_u32 (bits);

      /* This appears to be a copy'n'paste-o in the spec. */
      /*key_code = swfdec_bits_get_u8 (bits);*/

      SWFDEC_INFO ("clip event with flags 0x%x, %d record length (v%d)",
	  event_flags, record_size, s->version);

      for (i = 0; i < CLIPEVENT_MAX; i++) {
        if (!(event_flags & (1 << i)))
          continue;
        layer->clipevent[i] = swfdec_buffer_new_subbuffer (bits->buffer,
            bits->ptr - bits->buffer->data, record_size);
	event_flags &= ~(1 << i);
      }
      if (event_flags != 0) {
        SWFDEC_WARNING ("  clip actions other than onLoad/enterFrame unimplemented");
      }
      bits->ptr += record_size;
    }
  }

  //action_register_sprite_seg (s, layer);

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object (SwfdecDecoder * s)
{
  int depth;
  int id;

  id = swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);
  swfdec_sprite_frame_remove_seg (
      &s->parse_sprite->frames[s->parse_sprite->parse_frame], depth);

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s)
{
  int depth;

  depth = swfdec_bits_get_u16 (&s->b);
  swfdec_sprite_frame_remove_seg (
      &s->parse_sprite->frames[s->parse_sprite->parse_frame], depth);

  return SWF_OK;
}

SwfdecObject *
swfdec_exports_lookup (SwfdecDecoder * s, char *name)
{
  GList *g;

  for (g = g_list_first (s->exports); g; g = g_list_next (g)) {
    SwfdecExport *exp = g->data;

    if (strcmp(exp->name, name) == 0) {
      return swfdec_object_get (s, exp->id);
    }
  }
  return NULL;
}

