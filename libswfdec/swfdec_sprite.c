/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include "swfdec_sprite.h"
#include "swfdec_debug.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_script.h"
#include "swfdec_sound.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecSprite, swfdec_sprite, SWFDEC_TYPE_GRAPHIC)

void
swfdec_content_free (SwfdecContent *content)
{
  g_free (content->name);
  if (content->events)
    swfdec_event_list_free (content->events);
  g_free (content);
}

static void
swfdec_sprite_dispose (GObject *object)
{
  SwfdecSprite * sprite = SWFDEC_SPRITE (object);
  guint i;

  if (sprite->live_content) {
    g_hash_table_destroy (sprite->live_content);
    sprite->live_content = NULL;
  }
  if (sprite->frames) {
    for (i = 0; i < sprite->n_frames; i++) {
      g_free (sprite->frames[i].label);
      if (sprite->frames[i].sound_head)
	g_object_unref (sprite->frames[i].sound_head);
      if (sprite->frames[i].sound_block) {
        swfdec_buffer_unref (sprite->frames[i].sound_block);
      }
      if (sprite->frames[i].actions) {
	guint j;
	for (j = 0; j < sprite->frames[i].actions->len; j++) {
	  SwfdecSpriteAction *action = 
	    &g_array_index (sprite->frames[i].actions, SwfdecSpriteAction, j);
	  switch (action->type) {
	    case SWFDEC_SPRITE_ACTION_SCRIPT:
	      swfdec_script_unref (action->data);
	      break;
	    case SWFDEC_SPRITE_ACTION_ADD:
	    case SWFDEC_SPRITE_ACTION_UPDATE:
	      swfdec_content_free (action->data);
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
  if (sprite->init_action) {
    swfdec_script_unref (sprite->init_action);
    sprite->init_action = NULL;
  }

  G_OBJECT_CLASS (swfdec_sprite_parent_class)->dispose (object);
}

void
swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite, guint frame,
    SwfdecBuffer * chunk, int skip, guint n_samples)
{
  g_assert (sprite->frames != NULL);
  g_assert (chunk != NULL || n_samples == 0);

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
  SWFDEC_LOG ("adding %u samples in %u bytes to frame %u", n_samples, 
      chunk ? chunk->length : 0, frame);
  sprite->frames[frame].sound_skip = skip;
  sprite->frames[frame].sound_block = chunk;
  sprite->frames[frame].sound_samples = n_samples *
    SWFDEC_AUDIO_OUT_GRANULARITY (sprite->frames[frame].sound_head->original_format);
}

/* find the last action in this depth if it exists */
/* NB: we look in the current frame, too - so call this before adding actions
 * that might modify the frame you're looking for */
static SwfdecContent *
swfdec_content_find (SwfdecSprite *sprite, int depth)
{
  return g_hash_table_lookup (sprite->live_content, GINT_TO_POINTER (depth));
}

static void
swfdec_content_update_lifetime (SwfdecSprite *sprite,
    SwfdecSpriteActionType type, gpointer data)
{
  SwfdecContent *content;
  int depth;
  switch (type) {
    case SWFDEC_SPRITE_ACTION_SCRIPT:
    case SWFDEC_SPRITE_ACTION_UPDATE:
      return;
    case SWFDEC_SPRITE_ACTION_ADD:
      depth = ((SwfdecContent *) data)->depth;
      break;
    case SWFDEC_SPRITE_ACTION_REMOVE:
      depth = GPOINTER_TO_INT (data);
      break;
    default:
      g_assert_not_reached ();
      return;
  }
  content = swfdec_content_find (sprite, depth);
  if (content == NULL)
    return;
  content->sequence->end = sprite->parse_frame;
}

static void
swfdec_content_update_live (SwfdecSprite *sprite,
    SwfdecSpriteActionType type, gpointer data)
{
  SwfdecContent *content;
  switch (type) {
    case SWFDEC_SPRITE_ACTION_SCRIPT:
      return;
    case SWFDEC_SPRITE_ACTION_UPDATE:
    case SWFDEC_SPRITE_ACTION_ADD:
      content = data;
      g_hash_table_insert (sprite->live_content, 
	  GINT_TO_POINTER (content->depth), content);
      break;
    case SWFDEC_SPRITE_ACTION_REMOVE:
      /* data is GINT_TO_POINTER (depth) */
      g_hash_table_remove (sprite->live_content, data);
      break;
    default:
      g_assert_not_reached ();
      return;
  }
}

/* NB: does not free the action data */
static void
swfdec_sprite_remove_last_action (SwfdecSprite * sprite, guint frame_id)
{
  SwfdecSpriteFrame *frame;
  
  g_assert (frame_id < sprite->n_frames);
  frame = &sprite->frames[frame_id];

  g_assert (frame->actions != NULL);
  g_assert (frame->actions->len > 0);
  g_array_set_size (frame->actions, frame->actions->len - 1);
}

void
swfdec_sprite_add_action (SwfdecSprite *sprite, SwfdecSpriteActionType type, 
    gpointer data)
{
  SwfdecSpriteAction action;
  SwfdecSpriteFrame *frame;
  
  g_assert (sprite->parse_frame < sprite->n_frames);
  frame = &sprite->frames[sprite->parse_frame];

  if (frame->actions == NULL)
    frame->actions = g_array_new (FALSE, FALSE, sizeof (SwfdecSpriteAction));

  swfdec_content_update_lifetime (sprite, type, data);
  swfdec_content_update_live (sprite, type, data);
  action.type = type;
  action.data = data;
  g_array_append_val (frame->actions, action);
}

static int
swfdec_get_clipeventflags (SwfdecSwfDecoder * s, SwfdecBits * bits)
{
  if (s->version <= 5) {
    return swfdec_bits_get_u16 (bits);
  } else {
    return swfdec_bits_get_u32 (bits);
  }
}

int
tag_show_frame (SwfdecSwfDecoder * s)
{
  SWFDEC_DEBUG("show_frame %d of id %d", s->parse_sprite->parse_frame,
      SWFDEC_CHARACTER (s->parse_sprite)->id);

  s->parse_sprite->parse_frame++;
  if (s->parse_sprite->parse_frame < s->parse_sprite->n_frames) {
    SwfdecSpriteFrame *old = &s->parse_sprite->frames[s->parse_sprite->parse_frame - 1];
    SwfdecSpriteFrame *new = &s->parse_sprite->frames[s->parse_sprite->parse_frame];
    if (old->sound_head)
      new->sound_head = g_object_ref (old->sound_head);
  }

  return SWFDEC_STATUS_IMAGE;
}

int
tag_func_set_background_color (SwfdecSwfDecoder * s)
{
  SwfdecPlayer *player = SWFDEC_DECODER (s)->player;
  SwfdecColor color = swfdec_bits_get_color (&s->b);

  if (player->bgcolor_set) {
    /* only an INFO because it can be set by user, should be error if we check duplication of tag */
    SWFDEC_INFO ("background color has been set to %X already, setting to %X ignored",
	player->bgcolor, color);
  } else {
    SWFDEC_LOG ("setting background color to %X", color);
    /* can't use swfdec_player_set_background_color() here, because the player is locked and doesn't emit signals */
    player->bgcolor = color;
    player->bgcolor_set = TRUE;
    player->invalid.x0 = SWFDEC_DOUBLE_TO_TWIPS (0.0);
    player->invalid.y0 = SWFDEC_DOUBLE_TO_TWIPS (0.0);
    player->invalid.x1 = SWFDEC_DOUBLE_TO_TWIPS (player->width);
    player->invalid.y1 = SWFDEC_DOUBLE_TO_TWIPS (player->height);
    g_object_notify (G_OBJECT (player), "background-color");
  }

  return SWFDEC_STATUS_OK;
}

SwfdecContent *
swfdec_content_new (int depth)
{
  SwfdecContent *content = g_new0 (SwfdecContent, 1);

  cairo_matrix_init_identity (&content->transform);
  swfdec_color_transform_init_identity (&content->color_transform);
  content->depth = depth;
  content->operator = CAIRO_OPERATOR_OVER;
  content->sequence = content;
  content->end = G_MAXUINT;
  return content;
}

static SwfdecContent *
swfdec_contents_create (SwfdecSprite *sprite, 
    int depth, gboolean copy, gboolean new)
{
  SwfdecContent *content = swfdec_content_new (depth);

  content->start = sprite->parse_frame;
  content->end = sprite->n_frames;
  if (copy) {
    SwfdecContent *copy;

    copy = swfdec_content_find (sprite, depth);
    if (copy == NULL) {
      SWFDEC_WARNING ("Couldn't copy depth %d in frame %u", (depth + 16384), sprite->parse_frame);
    } else {
      *content = *copy;
      SWFDEC_LOG ("Copying from content %p", copy);
      if (content->name)
	content->name = g_strdup (copy->name);
      if (content->events)
	content->events = swfdec_event_list_copy (copy->events);
      if (new) {
	content->start = sprite->parse_frame;
	content->end = sprite->n_frames;
      }
      swfdec_sprite_add_action (sprite, 
	  new ? SWFDEC_SPRITE_ACTION_ADD : SWFDEC_SPRITE_ACTION_UPDATE, content);
      return content;
    }
  }
  swfdec_sprite_add_action (sprite, SWFDEC_SPRITE_ACTION_ADD, content);
  return content;
}

static cairo_operator_t
swfdec_sprite_convert_operator (guint operator)
{
  return CAIRO_OPERATOR_OVER;
}

static int
swfdec_spriteseg_do_place_object (SwfdecSwfDecoder *s, unsigned int version)
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
  int cache;
  int has_blend_mode = 0;
  int has_filter = 0;
  SwfdecContent *content;

  has_clip_actions = swfdec_bits_getbit (bits);
  has_clip_depth = swfdec_bits_getbit (bits);
  has_name = swfdec_bits_getbit (bits);
  has_ratio = swfdec_bits_getbit (bits);
  has_color_transform = swfdec_bits_getbit (bits);
  has_matrix = swfdec_bits_getbit (bits);
  has_character = swfdec_bits_getbit (bits);
  move = swfdec_bits_getbit (bits);

  SWFDEC_LOG ("  has_clip_actions = %d", has_clip_actions);
  SWFDEC_LOG ("  has_clip_depth = %d", has_clip_depth);
  SWFDEC_LOG ("  has_name = %d", has_name);
  SWFDEC_LOG ("  has_ratio = %d", has_ratio);
  SWFDEC_LOG ("  has_color_transform = %d", has_color_transform);
  SWFDEC_LOG ("  has_matrix = %d", has_matrix);
  SWFDEC_LOG ("  has_character = %d", has_character);
  SWFDEC_LOG ("  move = %d", move);

  if (version > 2) {
    swfdec_bits_getbits (bits, 5);
    cache = swfdec_bits_getbit (bits);
    has_blend_mode = swfdec_bits_getbit (bits);
    has_filter = swfdec_bits_getbit (bits);
    SWFDEC_LOG ("  cache = %d", cache);
    SWFDEC_LOG ("  has filter = %d", has_filter);
    SWFDEC_LOG ("  has blend mode = %d", has_blend_mode);
  }

  depth = swfdec_bits_get_u16 (bits);
  if (depth >= 16384) {
    SWFDEC_WARNING ("depth of placement too high: %u >= 16384", depth);
  }
  SWFDEC_LOG ("  depth = %d (=> %d)", depth, depth - 16384);
  depth -= 16384;

  /* new name always means new object */
  content = swfdec_contents_create (s->parse_sprite, depth, move, has_character || has_name);
  if (has_character) {
    int id = swfdec_bits_get_u16 (bits);
    content->graphic = swfdec_swf_decoder_get_character (s, id);
    if (!SWFDEC_IS_GRAPHIC (content->graphic)) {
      g_hash_table_remove (s->parse_sprite->live_content, GUINT_TO_POINTER (content->depth));
      swfdec_content_free (content);
      swfdec_sprite_remove_last_action (s->parse_sprite,
	        s->parse_sprite->parse_frame);
      SWFDEC_ERROR ("id %u does not specify a graphic", id);
      return SWFDEC_STATUS_OK;
    }
    content->sequence = content;
    SWFDEC_LOG ("  id = %d", id);
  } else if (content->graphic == NULL) {
    SWFDEC_ERROR ("no character specified and copying didn't give one");
    g_hash_table_remove (s->parse_sprite->live_content, GUINT_TO_POINTER (content->depth));
    swfdec_content_free (content);
    swfdec_sprite_remove_last_action (s->parse_sprite,
	      s->parse_sprite->parse_frame);
    return SWFDEC_STATUS_OK;
  }

  if (has_matrix) {
    swfdec_bits_get_matrix (bits, &content->transform, NULL);
    SWFDEC_LOG ("  matrix = { %g %g, %g %g } + { %g %g }", 
	content->transform.xx, content->transform.yx,
	content->transform.xy, content->transform.yy,
	content->transform.x0, content->transform.y0);
  }
  if (has_color_transform) {
    swfdec_bits_get_color_transform (bits, &content->color_transform);
    SWFDEC_LOG ("  color transform = %d %d  %d %d  %d %d  %d %d",
	content->color_transform.ra, content->color_transform.rb,
	content->color_transform.ga, content->color_transform.gb,
	content->color_transform.ba, content->color_transform.bb,
	content->color_transform.aa, content->color_transform.ab);
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
    content->clip_depth = swfdec_bits_get_u16 (bits) - 16384;
    SWFDEC_LOG ("  clip_depth = %d (=> %d)", content->clip_depth + 16384, content->clip_depth);
  }
  if (has_filter) {
    SWFDEC_ERROR ("filters aren't implemented, skipping PlaceObject tag!");
    g_hash_table_remove (s->parse_sprite->live_content, GUINT_TO_POINTER (content->depth));
    swfdec_content_free (content);
    swfdec_sprite_remove_last_action (s->parse_sprite,
	      s->parse_sprite->parse_frame);
    return SWFDEC_STATUS_OK;
  }
  if (has_blend_mode) {
    guint operator = swfdec_bits_get_u8 (bits);
    content->operator = swfdec_sprite_convert_operator (operator);
    SWFDEC_ERROR ("  operator = %u", operator);
  }
  if (has_clip_actions) {
    int reserved, clip_event_flags, event_flags, key_code;
    char *script_name;

    if (content->events) {
      swfdec_event_list_free (content->events);
      content->events = NULL;
    }
    reserved = swfdec_bits_get_u16 (bits);
    clip_event_flags = swfdec_get_clipeventflags (s, bits);

    if (content->name)
      script_name = g_strdup (content->name);
    else
      script_name = g_strdup_printf ("Sprite%u", SWFDEC_CHARACTER (content->graphic)->id);
    while ((event_flags = swfdec_get_clipeventflags (s, bits)) != 0) {
      guint length = swfdec_bits_get_u32 (bits);
      SwfdecBits action_bits;

      swfdec_bits_init_bits (&action_bits, bits, length);
      if (event_flags & SWFDEC_EVENT_KEY_PRESS)
	key_code = swfdec_bits_get_u8 (&action_bits);
      else
	key_code = 0;

      SWFDEC_INFO ("clip event with flags 0x%X, key code %d", event_flags, key_code);
#define SWFDEC_IMPLEMENTED_EVENTS \
  (SWFDEC_EVENT_LOAD | SWFDEC_EVENT_UNLOAD | SWFDEC_EVENT_ENTER | \
   SWFDEC_EVENT_MOUSE_DOWN | SWFDEC_EVENT_MOUSE_MOVE | SWFDEC_EVENT_MOUSE_UP)
      if (event_flags & ~SWFDEC_IMPLEMENTED_EVENTS) {
	SWFDEC_ERROR ("using non-implemented clip events %u", event_flags & ~SWFDEC_IMPLEMENTED_EVENTS);
      }
      if (content->events == NULL)
	content->events = swfdec_event_list_new (SWFDEC_DECODER (s)->player);
      swfdec_event_list_parse (content->events, &action_bits, s->version, 
	  event_flags, key_code, script_name);
      if (swfdec_bits_left (&action_bits)) {
	SWFDEC_ERROR ("not all action data was parsed: %u bytes left",
	    swfdec_bits_left (&action_bits));
      }
    }
    g_free (script_name);
  }

  return SWFDEC_STATUS_OK;
}

int
swfdec_spriteseg_place_object_2 (SwfdecSwfDecoder * s)
{
  return swfdec_spriteseg_do_place_object (s, 2);
}

int
swfdec_spriteseg_place_object_3 (SwfdecSwfDecoder * s)
{
  return swfdec_spriteseg_do_place_object (s, 3);
}

int
swfdec_spriteseg_remove_object (SwfdecSwfDecoder * s)
{
  int depth;

  swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG ("  depth = %d (=> %d)", depth, depth - 16384);
  depth -= 16384;
  swfdec_sprite_add_action (s->parse_sprite, SWFDEC_SPRITE_ACTION_REMOVE, GINT_TO_POINTER (depth));

  return SWFDEC_STATUS_OK;
}

int
swfdec_spriteseg_remove_object_2 (SwfdecSwfDecoder * s)
{
  guint depth;

  depth = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG ("  depth = %u", depth);
  depth -= 16384;
  swfdec_sprite_add_action (s->parse_sprite, SWFDEC_SPRITE_ACTION_REMOVE, GINT_TO_POINTER (depth));

  return SWFDEC_STATUS_OK;
}

static SwfdecMovie *
swfdec_sprite_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  SwfdecSpriteMovie *ret = g_object_new (SWFDEC_TYPE_SPRITE_MOVIE, NULL);

  ret->sprite = SWFDEC_SPRITE (graphic);
  *size = sizeof (SwfdecSpriteMovie);

  return SWFDEC_MOVIE (ret);
}

static void
swfdec_sprite_class_init (SwfdecSpriteClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_sprite_dispose;

  graphic_class->create_movie = swfdec_sprite_create_movie;
}

static void
swfdec_sprite_init (SwfdecSprite * sprite)
{
  sprite->live_content = g_hash_table_new (g_direct_hash, g_direct_equal);
}

void
swfdec_sprite_set_n_frames (SwfdecSprite *sprite, guint n_frames,
    guint rate)
{
  guint i;

  g_return_if_fail (SWFDEC_IS_SPRITE (sprite));
  if (n_frames == 0)
    n_frames = 1;

  sprite->frames = g_new0 (SwfdecSpriteFrame, n_frames);
  sprite->n_frames = n_frames;

  if (rate > 0) {
    for (i = 0; i < n_frames; i++) {
      sprite->frames[i].sound_samples = 44100 * 256 / rate;
    }
  }

  SWFDEC_LOG ("n_frames = %d", sprite->n_frames);
}

guint
swfdec_sprite_get_next_frame (SwfdecSprite *sprite, guint current_frame)
{
  guint next_frame;

  g_return_val_if_fail (SWFDEC_IS_SPRITE (sprite), 0);

  next_frame = current_frame + 1;
  if (next_frame >= sprite->n_frames)
    next_frame = 0;
  if (next_frame >= sprite->parse_frame)
    next_frame = current_frame;
  return next_frame;
}

int
swfdec_sprite_get_frame (SwfdecSprite *sprite, const char *label)
{
  guint i;

  g_return_val_if_fail (SWFDEC_IS_SPRITE (sprite), -1);
  g_return_val_if_fail (label != NULL, -1);

  for (i = 0; i < SWFDEC_SPRITE (sprite)->n_frames; i++) {
    SwfdecSpriteFrame *frame = &sprite->frames[i];
    if (frame->label == NULL)
      continue;
    if (g_str_equal (frame->label, label))
      return i;
  }
  return -1;
}

