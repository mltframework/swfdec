#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include <js/jsapi.h>

#include "swfdec_sprite.h"
#include "swfdec_internal.h"

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
	JS_DestroyScript (SWFDEC_OBJECT (sprite)->decoder->jscx, sprite->frames[i].action);
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

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (sprite));
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
swfdec_sprite_add_script (SwfdecSprite * sprite, int frame, JSScript *script)
{
  g_assert (sprite->frames != NULL);

  if (sprite->frames[frame].action) {
    SWFDEC_ERROR ("frame %d already contains a script!", frame);
  } else {
    sprite->frames[frame].action = script;
  }
}

#if 0
static SwfdecMouseResult 
swfdec_sprite_handle_mouse (SwfdecDecoder *s, SwfdecObject *object,
      double x, double y, int button, SwfdecRect *inval)
{
  SwfdecSprite *sprite = SWFDEC_SPRITE (object);
  SwfdecSpriteFrame *frame;
  GList *g;
  SwfdecMouseResult ret;
  double tmpx, tmpy;
  SwfdecObject *child_object;
  SwfdecSpriteSegment *child_seg;
  int clip_depth = 0;
  SwfdecRect rect;

  frame = &sprite->frames[s->current_frame];
  g = frame->segments;
  if (sprite->mouse_grab) {
    child_seg = sprite->mouse_grab;
    sprite->mouse_grab = NULL;
    goto grab_exists;
  }
  while (g) {
    child_seg = g->data;
    g = g->next;
grab_exists:
    if (child_seg->clip_depth) {
      SWFDEC_INFO ("clip_depth=%d", child_seg->clip_depth);
      clip_depth = child_seg->clip_depth;
    }

    if (clip_depth && child_seg->depth <= clip_depth) {
      SWFDEC_INFO ("clipping depth=%d", child_seg->clip_depth);
      continue;
    }

    child_object = swfdec_object_get (s, child_seg->id);
    if (child_object == NULL)
      continue;
    tmpx = x;
    tmpy = y;
    swfdec_matrix_transform_point_inverse (&child_seg->transform, &tmpx, &tmpy);
    /* ignore extents for grab object */
    ret = swfdec_object_handle_mouse (s, child_object, tmpx, tmpy, button, 
	g == frame->segments, &rect);
    swfdec_rect_union (inval, inval, &rect);
    switch (ret) {
      case SWFDEC_MOUSE_GRABBED:
	sprite->mouse_grab = child_seg;
	/* fall through */
      case SWFDEC_MOUSE_HIT:
	return ret;
      case SWFDEC_MOUSE_MISSED:
	break;
    }
  }
  return ret;
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

  frame = &sprite->frames[s->current_frame];
  
  /* FIXME: we don't cairo_paint because there's no clipping yet */
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
#endif

#if 0
static int
swfdec_get_clipeventflags (SwfdecDecoder * s, SwfdecBits * bits)
{
  if (s->version <= 5) {
    return swfdec_bits_get_u16 (bits);
  } else {
    return swfdec_bits_get_u32 (bits);
  }
}
#endif

static void
swfdec_sprite_add_action (SwfdecSprite *sprite, guint frame_nr, SwfdecSpriteAction *action)
{
  SwfdecSpriteFrame *frame;

  g_return_if_fail (SWFDEC_IS_SPRITE (sprite));
  g_return_if_fail (frame_nr < sprite->n_frames);
  g_return_if_fail (action != NULL);

  frame = &sprite->frames[frame_nr];

  if (frame->actions == NULL)
    frame->actions = g_array_new (FALSE, FALSE, sizeof (SwfdecSpriteAction));

  g_array_append_vals (frame->actions, action, 1);
}

int
tag_func_set_background_color (SwfdecDecoder * s)
{
  SwfdecSpriteAction action;

  action.type = SWFDEC_SPRITE_ACTION_BG_COLOR;
  swfdec_color_transform_init_color (&action.color.transform, swfdec_bits_get_color (&s->b));
  swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);

  return SWF_OK;
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
  SwfdecSpriteAction action;

  has_clip_actions = swfdec_bits_getbit (bits);
  has_clip_depth = swfdec_bits_getbit (bits);
  has_name = swfdec_bits_getbit (bits);
  has_ratio = swfdec_bits_getbit (bits);
  has_color_transform = swfdec_bits_getbit (bits);
  has_matrix = swfdec_bits_getbit (bits);
  has_character = swfdec_bits_getbit (bits);
  move = swfdec_bits_getbit (bits);
  depth = swfdec_bits_get_u16 (bits);

  SWFDEC_LOG ("  has_clip_actions = %d", has_clip_actions);
  SWFDEC_LOG ("  has_clip_depth = %d", has_clip_depth);
  SWFDEC_LOG ("  has_name = %d", has_name);
  SWFDEC_LOG ("  has_ratio = %d", has_ratio);
  SWFDEC_LOG ("  has_color_transform = %d", has_color_transform);
  SWFDEC_LOG ("  has_matrix = %d", has_matrix);
  SWFDEC_LOG ("  has_character = %d", has_character);

  if (has_character) {
    action.type = SWFDEC_SPRITE_ACTION_PLACE_OBJECT;
    action.uint.value[0] = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  id = %d", action.uint.value[0]);
  } else {
    action.type = SWFDEC_SPRITE_ACTION_GET_OBJECT;
    if (!move)
      SWFDEC_ERROR ("neither character nor move flag are set in PlaceObject2");
  }
  action.uint.value[1] = depth;
  swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);

  if (has_matrix) {
    action.type = SWFDEC_SPRITE_ACTION_TRANSFORM;
    swfdec_bits_get_matrix (bits, &action.matrix.matrix);
    swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);
  }
  if (has_color_transform) {
    action.type = SWFDEC_SPRITE_ACTION_COLOR_TRANSFORM;
    swfdec_bits_get_color_transform (bits, &action.color.transform);
    swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);
  }
  swfdec_bits_syncbits (bits);
  if (has_ratio) {
    action.type = SWFDEC_SPRITE_ACTION_RATIO;
    action.uint.value[0] = swfdec_bits_get_u16 (bits);
    swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);
    SWFDEC_LOG ("  ratio = %d", action.uint.value[0]);
  }
  if (has_name) {
    action.type = SWFDEC_SPRITE_ACTION_NAME;
    action.string.string = swfdec_bits_get_string (bits);
    swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);
  }
  if (has_clip_depth) {
    action.type = SWFDEC_SPRITE_ACTION_CLIP_DEPTH;
    action.uint.value[0] = swfdec_bits_get_u16 (bits);
    swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);
    SWFDEC_LOG ("clip_depth = %04x", action.uint.value[0]);
  }
  if (has_clip_actions) {
    /* FIXME */
    g_assert_not_reached ();
#if 0
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
#endif
  }

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object (SwfdecDecoder * s)
{
  SwfdecSpriteAction action;

  swfdec_bits_get_u16 (&s->b);
  action.type = SWFDEC_SPRITE_ACTION_REMOVE_OBJECT;
  action.uint.value[0] = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG ("  depth = %d", action.uint.value[0]);
  swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s)
{
  SwfdecSpriteAction action;

  action.type = SWFDEC_SPRITE_ACTION_REMOVE_OBJECT;
  action.uint.value[0] = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG ("  depth = %d", action.uint.value[0]);
  swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, &action);

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
}

void
swfdec_sprite_set_n_frames (SwfdecSprite *sprite, unsigned int n_frames)
{
  g_return_if_fail (SWFDEC_IS_SPRITE (sprite));

  sprite->frames = g_new0 (SwfdecSpriteFrame, n_frames);
  sprite->n_frames = n_frames;

  SWFDEC_LOG ("n_frames = %d", sprite->n_frames);
}

