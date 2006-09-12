
#include <string.h>

#include "swfdec_internal.h"

#include <swfdec_sprite.h>

SWFDEC_OBJECT_BOILERPLATE (SwfdecSprite, swfdec_sprite)

     static void swfdec_sprite_base_init (gpointer g_class)
{

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

#if 0
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
#endif

static void 
swfdec_sprite_iterate (SwfdecDecoder *decoder, SwfdecObject *object, 
      unsigned int frame, const SwfdecMouseInfo *info, SwfdecRect *inval)
{
  SwfdecSprite *sprite = SWFDEC_SPRITE (object);
  
  sprite->current_frame = frame;
}

static void
swfdec_sprite_render (SwfdecDecoder * s, cairo_t *cr,
    const SwfdecColorTransform *trans, SwfdecObject * object, 
    SwfdecRect *inval)
{
  SwfdecSprite *sprite = SWFDEC_SPRITE (object);
  SwfdecSpriteFrame *frame;
  GList *g;
  int clip_depth = 0;

  frame = &sprite->frames[sprite->current_frame];
  
  /* FIXME: we don't paint because there's no clipping yet */
  swfdec_color_set_source (cr, frame->bg_color);
  cairo_rectangle (cr, inval->x0, inval->y0, inval->x1 - inval->x0, inval->y1 - inval->y0);
  cairo_fill (cr);

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
      SwfdecColorTransform color_trans;
      swfdec_color_transform_chain (&color_trans, &child_seg->color_transform,
	  trans);
      SWFDEC_LOG ("rendering %s %p with depth %d", G_OBJECT_TYPE_NAME (child_object),
	  child_object, child_seg->depth);
      swfdec_object_render (s, child_object, cr, &child_seg->transform,
	  &color_trans, inval);
    } else {
      SWFDEC_WARNING ("could not find object (id = %d)", child_seg->id);
    }
  }
}

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
    } else if (seg->depth == segnew->depth) {
      SWFDEC_WARNING ("replacing frame with id %d, is that legal?", seg->depth);
      g->data = segnew;
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
  cairo_matrix_init_identity (&seg->transform);
  swfdec_color_transform_init_identity (&seg->color_transform);

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
    swfdec_bits_get_matrix (bits, &layer->transform);
  } else if (oldlayer) {
    layer->transform = oldlayer->transform;
  } else {
    cairo_matrix_init_identity (&layer->transform);
  }
  if (has_color_transform) {
    swfdec_bits_get_color_transform (bits, &layer->color_transform);
  } else if (oldlayer) {
    layer->color_transform = oldlayer->color_transform;
  } else {
    swfdec_color_transform_init_identity (&layer->color_transform);
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

static void
swfdec_sprite_class_init (SwfdecSpriteClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);

  object_class->iterate = swfdec_sprite_iterate;
  object_class->render = swfdec_sprite_render;
}

void
swfdec_sprite_set_n_frames (SwfdecSprite *sprite, unsigned int n_frames)
{
  g_return_if_fail (SWFDEC_IS_SPRITE (sprite));

  sprite->frames = g_new0 (SwfdecSpriteFrame, n_frames);
  sprite->n_frames = n_frames;

  SWFDEC_LOG ("n_frames = %d", sprite->n_frames);
}

