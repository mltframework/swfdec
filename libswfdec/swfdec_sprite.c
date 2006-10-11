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

void
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
  unsigned int i;

  if (sprite->frames) {
    for (i = 0; i < sprite->n_frames; i++) {
      g_free (sprite->frames[i].name);
      if (sprite->frames[i].sound_head)
	g_object_unref (sprite->frames[i].sound_head);
      if (sprite->frames[i].sound_block) {
        swfdec_buffer_unref (sprite->frames[i].sound_block);
      }
      if (sprite->frames[i].actions) {
	JSContext *cx = SWFDEC_OBJECT (sprite)->decoder->jscx;
	guint j;
	for (j = 0; j < sprite->frames[i].actions->len; j++) {
	  SwfdecSpriteAction *action = 
	    &g_array_index (sprite->frames[i].actions, SwfdecSpriteAction, j);
	  switch (action->type) {
	    case SWFDEC_SPRITE_ACTION_SCRIPT:
	      JS_DestroyScript (cx, action->data);
	      break;
	    case SWFDEC_SPRITE_ACTION_ADD:
	    case SWFDEC_SPRITE_ACTION_UPDATE:
	      swfdec_sprite_content_free (action->data);
	      break;
	    case SWFDEC_SPRITE_ACTION_REMOVE:
	      break;
	    default:
	      g_assert_not_reached ();
	  }
	}
	g_array_free (sprite->frames[i].actions, TRUE);
      }
      g_slist_foreach (sprite->frames[i].sound, (GFunc) swfdec_sound_chunk_free, NULL);
      g_slist_free (sprite->frames[i].sound);
    }
    g_free(sprite->frames);
  }

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (sprite));
}

void
swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite, unsigned int frame,
    SwfdecBuffer * chunk, int skip, guint n_samples)
{
  g_assert (sprite->frames != NULL);

  if (sprite->frames[frame].sound_head == NULL) {
    SWFDEC_ERROR ("attempting to add a sound block without previous sound head");
    swfdec_buffer_unref (chunk);
    return;
  }
  if (sprite->frames[frame].sound_block) {
    SWFDEC_ERROR ("attempting to add 2 sound blocks to one frame");
    swfdec_buffer_unref (chunk);
    return;
  }
  SWFDEC_LOG ("adding %u samples in %u bytes to frame %u", n_samples, chunk->length, frame);
  sprite->frames[frame].sound_skip = skip;
  sprite->frames[frame].sound_block = chunk;
  sprite->frames[frame].sound_samples = n_samples * sprite->frames[frame].sound_head->rate_multiplier;
}

/* find the last action in this depth if it exists */
/* NB: we look in the current frame, too - so call this before adding actions
 * that might modify the frame you're looking for */
static SwfdecSpriteContent *
swfdec_sprite_content_find (SwfdecSprite *sprite, unsigned int frame_id,
    unsigned int depth)
{
  guint i, j;
  SwfdecSpriteContent *content;

  for (i = frame_id; i <= frame_id /* wait for underflow */; i--) {
    SwfdecSpriteFrame *frame = &sprite->frames[i];
    if (frame->actions == NULL)
      continue;
    for (j = frame->actions->len - 1; j < frame->actions->len; j--) {
      SwfdecSpriteAction *action = 
	&g_array_index (frame->actions, SwfdecSpriteAction, j);
      switch (action->type) {
	case SWFDEC_SPRITE_ACTION_SCRIPT:
	  break;
	case SWFDEC_SPRITE_ACTION_ADD:
	case SWFDEC_SPRITE_ACTION_UPDATE:
	  content = action->data;
	  if (content->depth == depth)
	    return content;
	  break;
	case SWFDEC_SPRITE_ACTION_REMOVE:
	  if (GPOINTER_TO_UINT (action->data) == depth)
	    return NULL;
	  break;
	default:
	  g_assert_not_reached ();
      }
    }
  }
  return NULL;
}

static void
swfdec_sprite_content_update_lifetime (SwfdecSprite *sprite, unsigned int frame_id,
    SwfdecSpriteActionType type, gpointer data)
{
  SwfdecSpriteContent *content;
  guint depth;
  switch (type) {
    case SWFDEC_SPRITE_ACTION_SCRIPT:
    case SWFDEC_SPRITE_ACTION_UPDATE:
      return;
    case SWFDEC_SPRITE_ACTION_ADD:
      depth = ((SwfdecSpriteContent *) data)->depth;
      break;
    case SWFDEC_SPRITE_ACTION_REMOVE:
      depth = GPOINTER_TO_UINT (data);
      break;
    default:
      g_assert_not_reached ();
      return;
  }
  content = swfdec_sprite_content_find (sprite, frame_id, depth);
  if (content == NULL)
    return;
  content->sequence->end = frame_id;
}

void
swfdec_sprite_add_action (SwfdecSprite * sprite, unsigned int frame_id, 
    SwfdecSpriteActionType type, gpointer data)
{
  SwfdecSpriteAction action;
  SwfdecSpriteFrame *frame;
  
  g_assert (frame_id < sprite->n_frames);
  frame = &sprite->frames[frame_id];

  if (frame->actions == NULL)
    frame->actions = g_array_new (FALSE, FALSE, sizeof (SwfdecSpriteAction));

  swfdec_sprite_content_update_lifetime (sprite, frame_id, type, data);
  action.type = type;
  action.data = data;
  g_array_append_val (frame->actions, action);
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
tag_show_frame (SwfdecDecoder * s)
{
  SWFDEC_DEBUG("show_frame %d of id %d", s->parse_sprite->parse_frame,
      s->parse_sprite->object.id);

  s->parse_sprite->parse_frame++;
  if (s->parse_sprite->parse_frame < s->parse_sprite->n_frames) {
    SwfdecSpriteFrame *old = &s->parse_sprite->frames[s->parse_sprite->parse_frame - 1];
    SwfdecSpriteFrame *new = &s->parse_sprite->frames[s->parse_sprite->parse_frame];
    new->bg_color = old->bg_color;
    if (old->sound_head)
      new->sound_head = g_object_ref (old->sound_head);
  }

  return SWFDEC_IMAGE;
}

int
tag_func_set_background_color (SwfdecDecoder * s)
{
  SwfdecSpriteFrame *frame;

  frame = &s->parse_sprite->frames[s->parse_sprite->parse_frame];

  frame->bg_color = swfdec_bits_get_color (&s->b);

  return SWFDEC_OK;
}

SwfdecSpriteContent *
swfdec_sprite_content_new (unsigned int depth)
{
  SwfdecSpriteContent *content = g_new0 (SwfdecSpriteContent, 1);

  cairo_matrix_init_identity (&content->transform);
  swfdec_color_transform_init_identity (&content->color_transform);
  content->depth = depth;
  content->sequence = content;
  content->end = G_MAXUINT;
  return content;
}

static SwfdecSpriteContent *
swfdec_sprite_contents_create (SwfdecSprite *sprite, unsigned int frame_id, 
    unsigned int depth, gboolean copy, gboolean new)
{
  SwfdecSpriteContent *content = swfdec_sprite_content_new (depth);

  content->start = frame_id;
  content->end = sprite->n_frames;
  if (copy) {
    SwfdecSpriteContent *copy;

    copy = swfdec_sprite_content_find (sprite, frame_id, depth);
    if (copy == NULL) {
      SWFDEC_WARNING ("Couldn't copy depth %u in frame %u", depth, frame_id);
    } else {
      *content = *copy;
      SWFDEC_LOG ("Copying from content %p", copy);
      if (content->name)
	content->name = g_strdup (copy->name);
      if (content->events)
	content->events = swfdec_event_list_copy (copy->events);
      if (new) {
	content->start = frame_id;
	content->end = sprite->n_frames;
      }
      swfdec_sprite_add_action (sprite, frame_id, 
	  new ? SWFDEC_SPRITE_ACTION_ADD : SWFDEC_SPRITE_ACTION_UPDATE, content);
      return content;
    }
  }
  swfdec_sprite_add_action (sprite, frame_id, SWFDEC_SPRITE_ACTION_ADD, content);
  return content;
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
  SWFDEC_LOG ("  move = %d", move);
  SWFDEC_LOG ("  depth = %d", depth);

  /* new name always means new object */
  content = swfdec_sprite_contents_create (s->parse_sprite, 
      s->parse_sprite->parse_frame, depth, move, has_character || has_name);
  if (has_character) {
    int id = swfdec_bits_get_u16 (bits);
    content->object = swfdec_object_get (s, id);
    content->sequence = content;
    SWFDEC_LOG ("  id = %d", id);
  }

  if (has_matrix) {
    swfdec_bits_get_matrix (bits, &content->transform);
    SWFDEC_LOG ("  matrix = { %g %g, %g %g } + { %g %g }", 
	content->transform.xx, content->transform.yx,
	content->transform.xy, content->transform.yy,
	content->transform.x0, content->transform.y0);
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
    SWFDEC_LOG ("  clip_depth = %u", content->clip_depth);
  }
  if (has_clip_actions) {
    int reserved, clip_event_flags, event_flags, key_code;
    guint8 * record_end;

    if (content->events) {
      swfdec_event_list_free (content->events);
      content->events = NULL;
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
      if (event_flags & ~(SWFDEC_EVENT_LOAD | SWFDEC_EVENT_UNLOAD | SWFDEC_EVENT_ENTER)) {
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

  return SWFDEC_OK;
}

int
swfdec_spriteseg_remove_object (SwfdecDecoder * s)
{
  unsigned int depth;

  swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);
  swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, 
      SWFDEC_SPRITE_ACTION_REMOVE, GUINT_TO_POINTER (depth));
  SWFDEC_LOG ("  depth = %u", depth);

  return SWFDEC_OK;
}

int
swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s)
{
  unsigned int depth;

  depth = swfdec_bits_get_u16 (&s->b);
  swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, 
      SWFDEC_SPRITE_ACTION_REMOVE, GUINT_TO_POINTER (depth));
  SWFDEC_LOG ("  depth = %u", depth);

  return SWFDEC_OK;
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

unsigned int
swfdec_sprite_get_next_frame (SwfdecSprite *sprite, unsigned int current_frame)
{
  unsigned int next_frame, n_frames;

  g_return_val_if_fail (SWFDEC_IS_SPRITE (sprite), 0);

  n_frames = MIN (sprite->n_frames, sprite->parse_frame);
  next_frame = current_frame + 1;
  if (next_frame >= n_frames)
    next_frame = 0;
  return next_frame;
}

