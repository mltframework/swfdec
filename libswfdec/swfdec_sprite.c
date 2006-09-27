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
swfdec_sprite_content_free (SwfdecSpriteContent *content)
{
  g_free (content->name);
  if (content->events)
    swfdec_event_list_free (content->events);
  g_free (content);
}

static void
swfdec_sprite_dispose (SwfdecSprite * sprite)
{
  GList *walk;
  unsigned int i;

  if (sprite->frames) {
    for (i = 0; i < sprite->n_frames; i++) {
      g_free (sprite->frames[i].name);
      if (sprite->frames[i].sound_head)
	g_object_unref (sprite->frames[i].sound_head);
      if (sprite->frames[i].sound_block) {
        swfdec_buffer_unref (sprite->frames[i].sound_block);
      }
      if (sprite->frames[i].do_actions) {
	JSContext *cx = SWFDEC_OBJECT (sprite)->decoder->jscx;
	GSList *walk;
	for (walk = sprite->frames[i].do_actions; walk; walk = walk->next) {
	  JS_DestroyScript (cx, walk->data);
	}
	g_slist_free (sprite->frames[i].do_actions);
      }
      g_slist_foreach (sprite->frames[i].sound, (GFunc) swfdec_sound_chunk_free, NULL);
      g_slist_free (sprite->frames[i].sound);
      for (walk = sprite->frames[i].contents; walk; walk = walk->next) {
	SwfdecSpriteContent *content = walk->data;
	if (content->first_frame == i)
	  swfdec_sprite_content_free (content);
      }
    }
    g_free(sprite->frames);
  }

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (sprite));
}

void
swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite, int frame,
    SwfdecBuffer * chunk, int skip)
{
  g_assert (sprite->frames != NULL);

  if (sprite->frames[frame].sound_block) {
    SWFDEC_ERROR ("attempting to add 2 sound blocks to one frame");
    return;
  }
  sprite->frames[frame].sound_skip = skip;
  sprite->frames[frame].sound_block = chunk;
  swfdec_buffer_ref (chunk);
}

void
swfdec_sprite_add_script (SwfdecSprite * sprite, int frame, JSScript *script)
{
  g_assert (sprite->frames != NULL);

  /* append to keep the order */
  sprite->frames[frame].do_actions = g_slist_append (sprite->frames[frame].do_actions, script);
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
tag_func_set_background_color (SwfdecDecoder * s)
{
  SwfdecSpriteFrame *frame;

  frame = &s->parse_sprite->frames[s->parse_sprite->parse_frame];

  frame->bg_color = swfdec_bits_get_color (&s->b);

  return SWF_OK;
}

static SwfdecSpriteContent *
swfdec_sprite_content_new (unsigned int depth)
{
  SwfdecSpriteContent *content = g_new0 (SwfdecSpriteContent, 1);

  cairo_matrix_init_identity (&content->transform);
  swfdec_color_transform_init_identity (&content->color_transform);
  content->depth = depth;
  return content;
}

static SwfdecSpriteContent *
swfdec_sprite_contents_create (SwfdecSprite *sprite, unsigned int frame_id, unsigned int depth, gboolean copy)
{
  SwfdecSpriteFrame *frame;
  GList *walk;
  SwfdecSpriteContent *content = swfdec_sprite_content_new (depth);

  frame = &sprite->frames[frame_id];
  content->first_frame = frame_id;
  content->last_frame = sprite->n_frames;

  for (walk = frame->contents; walk; walk = walk->next) {
    SwfdecSpriteContent *cur = walk->data;

    if (cur->depth == depth) {
      if (copy) {
	*content = *cur;
	if (content->name)
	  content->name = g_strdup (content->name);
	if (content->events)
	  content->events = swfdec_event_list_copy (content->events);
      }
      content->first_frame = frame_id;
      cur->last_frame = frame_id;
      walk->data = content;
      SWFDEC_LOG ("changed object at depth %u", depth);
      return content;
    }
    if (cur->depth > depth) {
      frame->contents = g_list_insert_before (frame->contents, walk, content);
      goto add;
    }
  }
  SWFDEC_LOG ("appended object at depth %u", depth);
  frame->contents = g_list_append (frame->contents, content);

add:
  if (copy)
    SWFDEC_ERROR ("can't copy depth %u, no character was there", depth);
  return content;
}

static void
swfdec_sprite_contents_remove (SwfdecSprite *sprite, unsigned int frame_id, unsigned int depth)
{
  SwfdecSpriteFrame *frame;
  GList *walk;

  frame = &sprite->frames[frame_id];

  for (walk = frame->contents; walk; walk = walk->next) {
    SwfdecSpriteContent *content = walk->data;

    if (content->depth == depth) {
      frame->contents = g_list_delete_link (frame->contents, walk);
      content->last_frame = frame_id;
      return;
    }
  }
  SWFDEC_WARNING ("remove at depth %u (frame %u), but no character is there", depth, frame_id);
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
  SwfdecSpriteContent *content;

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
  SWFDEC_LOG ("  depth = %d", depth);

  content = swfdec_sprite_contents_create (s->parse_sprite, s->parse_sprite->parse_frame, depth, move);
  if (has_character) {
    int id = swfdec_bits_get_u16 (bits);
    content->object = swfdec_object_get (s, id);
    SWFDEC_LOG ("  id = %d", id);
  }

  if (has_matrix) {
    swfdec_bits_get_matrix (bits, &content->transform);
  }
  if (has_color_transform) {
    swfdec_bits_get_color_transform (bits, &content->color_transform);
  }
  swfdec_bits_syncbits (bits);
  if (has_ratio) {
    content->ratio = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  ratio = %d", content->ratio);
  }
  if (has_name) {
    g_free (content->name);
    content->name = swfdec_bits_get_string (bits);
    SWFDEC_LOG ("  name = %s", content->name);
  }
  if (has_clip_depth) {
    content->clip_depth = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("clip_depth = %04x", content->clip_depth);
  }
  if (has_clip_actions) {
    int reserved, clip_event_flags, event_flags, key_code;
    guint8 * record_end;

    if (content->events) {
      content->events = NULL;
      swfdec_event_list_free (content->events);
    }
    reserved = swfdec_bits_get_u16 (bits);
    clip_event_flags = swfdec_get_clipeventflags (s, bits);

    while ((event_flags = swfdec_get_clipeventflags (s, bits)) != 0) {
      guint tmp = swfdec_bits_get_u32 (bits);
      record_end = bits->ptr + tmp;

      if (event_flags & SWFDEC_EVENT_KEY_PRESS)
	key_code = swfdec_bits_get_u8 (bits);
      else
	key_code = 0;

      SWFDEC_INFO ("clip event with flags 0x%X, key code %d", event_flags, key_code);
      if (event_flags & ~(SWFDEC_EVENT_LOAD | SWFDEC_EVENT_ENTER)) {
	SWFDEC_ERROR ("using non-implemented clip events %u", event_flags);
      }
      if (content->events == NULL)
	content->events = swfdec_event_list_new (s);
      swfdec_event_list_parse (content->events, event_flags, key_code);
      if (bits->ptr != record_end) {
	SWFDEC_ERROR ("record size and actual parsed action differ by %d bytes",
	    (int) (record_end - bits->ptr));
	/* FIXME: who should we trust with parsing here? */
	bits->ptr = record_end;
      }
    }
  }

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object (SwfdecDecoder * s)
{
  unsigned int depth;

  swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);
  swfdec_sprite_contents_remove (s->parse_sprite, s->parse_sprite->parse_frame, depth);
  SWFDEC_LOG ("  depth = %u", depth);

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s)
{
  unsigned int depth;

  depth = swfdec_bits_get_u16 (&s->b);
  swfdec_sprite_contents_remove (s->parse_sprite, s->parse_sprite->parse_frame, depth);
  SWFDEC_LOG ("  depth = %u", depth);

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

