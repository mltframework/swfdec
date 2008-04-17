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
#include "swfdec_tag.h"

G_DEFINE_TYPE (SwfdecSprite, swfdec_sprite, SWFDEC_TYPE_GRAPHIC)

static void
swfdec_sprite_dispose (GObject *object)
{
  SwfdecSprite * sprite = SWFDEC_SPRITE (object);
  guint i;

  if (sprite->frames) {
    for (i = 0; i < sprite->n_frames; i++) {
      g_free (sprite->frames[i].label);
      if (sprite->frames[i].sound_head)
	g_object_unref (sprite->frames[i].sound_head);
      if (sprite->frames[i].sound_block) {
        swfdec_buffer_unref (sprite->frames[i].sound_block);
      }
      g_slist_foreach (sprite->frames[i].sound, (GFunc) swfdec_sound_chunk_free, NULL);
      g_slist_free (sprite->frames[i].sound);
    }
    g_free(sprite->frames);
  }
  for (i = 0; i < sprite->actions->len; i++) {
    SwfdecSpriteAction *cur = &g_array_index (sprite->actions, SwfdecSpriteAction, i);
    if (cur->buffer)
      swfdec_buffer_unref (cur->buffer);
  }
  g_array_free (sprite->actions, TRUE);
  sprite->actions = NULL;
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
  SWFDEC_LOG ("adding %u samples in %"G_GSIZE_FORMAT" bytes to frame %u", n_samples, 
      chunk ? chunk->length : 0, frame);
  sprite->frames[frame].sound_skip = skip;
  sprite->frames[frame].sound_block = chunk;
  sprite->frames[frame].sound_samples = n_samples *
    swfdec_audio_format_get_granularity (sprite->frames[frame].sound_head->format);
}

void
swfdec_sprite_add_action (SwfdecSprite *sprite, guint tag, SwfdecBuffer *buffer)
{
  SwfdecSpriteAction action;
  
  action.tag = tag;
  action.buffer = buffer;
  g_array_append_val (sprite->actions, action);
}

gboolean
swfdec_sprite_get_action (SwfdecSprite *sprite, guint n, guint *tag, SwfdecBuffer **buffer)
{
  SwfdecSpriteAction *action;

  g_return_val_if_fail (SWFDEC_IS_SPRITE (sprite), FALSE);
  g_return_val_if_fail (tag != NULL, FALSE);
  g_return_val_if_fail (buffer != NULL, FALSE);

  if (n >= sprite->actions->len)
    return FALSE;
  action = &g_array_index (sprite->actions, SwfdecSpriteAction, n);
  *tag = action->tag;
  *buffer = action->buffer;
  return TRUE;
}

int
tag_func_set_background_color (SwfdecSwfDecoder * s, guint tag)
{
  s->parse_sprite->bgcolor = swfdec_bits_get_color (&s->b);

  return SWFDEC_STATUS_OK;
}

static SwfdecMovie *
swfdec_sprite_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  SwfdecSpriteMovie *ret = g_object_new (SWFDEC_TYPE_SPRITE_MOVIE, NULL);

  ret->sprite = SWFDEC_SPRITE (graphic);
  ret->n_frames = ret->sprite->n_frames;
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
  sprite->actions = g_array_new (FALSE, FALSE, sizeof (SwfdecSpriteAction));
}

void
swfdec_sprite_set_n_frames (SwfdecSprite *sprite, guint n_frames,
    guint rate)
{
  guint i;

  g_return_if_fail (SWFDEC_IS_SPRITE (sprite));
  if (n_frames > 0) {
    sprite->frames = g_new0 (SwfdecSpriteFrame, n_frames);
    sprite->n_frames = n_frames;

    if (rate > 0) {
      for (i = 0; i < n_frames; i++) {
	sprite->frames[i].sound_samples = 44100 * 256 / rate;
      }
    }
  }

  SWFDEC_LOG ("n_frames = %d", sprite->n_frames);
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

