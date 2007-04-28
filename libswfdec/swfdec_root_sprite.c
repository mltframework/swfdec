/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_root_sprite.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_script.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecRootSprite, swfdec_root_sprite, SWFDEC_TYPE_SPRITE)

void
swfdec_root_sprite_dispose (GObject *object)
{
  SwfdecSprite *sprite = SWFDEC_SPRITE (object);
  SwfdecRootSprite *root = SWFDEC_ROOT_SPRITE (object);
  guint i,j;

  if (root->root_actions) {
    for (i = 0; i < sprite->n_frames; i++) {
      GArray *array = root->root_actions[i];
      if (array) {
	for (j = 0; j < array->len; j++) {
	  SwfdecSpriteAction *action = &g_array_index (array, SwfdecSpriteAction, j);

	  switch (action->type) {
	    case SWFDEC_ROOT_ACTION_EXPORT:
	      {
		SwfdecRootExportData *data = action->data;
		g_free (data->name);
		g_object_unref (data->character);
		g_free (data);
	      }
	      break;
	    case SWFDEC_ROOT_ACTION_INIT_SCRIPT:
	      swfdec_script_unref (action->data);
	      break;
	    default:
	      g_assert_not_reached ();
	      break;
	  }
	}
	g_array_free (array, TRUE);
      }
    }
    g_free (root->root_actions);
    root->root_actions = NULL;
  }

  G_OBJECT_CLASS (swfdec_root_sprite_parent_class)->dispose (object);
}

static void
swfdec_root_sprite_class_init (SwfdecRootSpriteClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_root_sprite_dispose;
}

static void
swfdec_root_sprite_init (SwfdecRootSprite * root_sprite)
{
}

void
swfdec_root_sprite_add_root_action (SwfdecRootSprite *root,
    SwfdecRootActionType type, gpointer data)
{
  SwfdecSprite *sprite;
  GArray *array;
  SwfdecSpriteAction action;

  g_return_if_fail (SWFDEC_IS_ROOT_SPRITE (root));
  sprite = SWFDEC_SPRITE (root);
  g_return_if_fail (sprite->parse_frame < sprite->n_frames);

  if (root->root_actions == NULL)
    root->root_actions = g_new0 (GArray *, sprite->n_frames);

  array = root->root_actions[sprite->parse_frame];
  if (array == NULL) {
    root->root_actions[sprite->parse_frame] = 
      g_array_new (FALSE, FALSE, sizeof (SwfdecSpriteAction));
    array = root->root_actions[sprite->parse_frame];
  }
  action.type = type;
  action.data = data;
  g_array_append_val (array, action);
}

int
tag_func_export_assets (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  guint count, i;

  count = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("exporting %u assets", count);
  for (i = 0; i < count && swfdec_bits_left (bits); i++) {
    guint id;
    SwfdecCharacter *object;
    char *name;
    id = swfdec_bits_get_u16 (bits);
    object = swfdec_swf_decoder_get_character (s, id);
    name = swfdec_bits_get_string (bits);
    if (object == NULL) {
      SWFDEC_ERROR ("cannot export id %u as %s, id wasn't found", id, name);
      g_free (name);
    } else if (name == NULL) {
      SWFDEC_ERROR ("cannot export id %u, no name was given", id);
    } else {
      SwfdecRootExportData *data = g_new (SwfdecRootExportData, 1);
      data->name = name;
      data->character = object;
      SWFDEC_LOG ("exporting %s %u as %s", G_OBJECT_TYPE_NAME (object), id, name);
      g_object_ref (object);
      swfdec_root_sprite_add_root_action (SWFDEC_ROOT_SPRITE (s->main_sprite), 
	  SWFDEC_ROOT_ACTION_EXPORT, data);
    }
  }

  return SWFDEC_STATUS_OK;
}

int
tag_func_do_init_action (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  guint id;
  SwfdecSprite *sprite;
  char *name;

  id = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("  id = %u", id);
  sprite = swfdec_swf_decoder_get_character (s, id);
  if (!SWFDEC_IS_SPRITE (sprite)) {
    SWFDEC_ERROR ("character %u is not a sprite", id);
    return SWFDEC_STATUS_OK;
  }
  if (sprite->init_action != NULL) {
    SWFDEC_ERROR ("sprite %u already has an init action", id);
    return SWFDEC_STATUS_OK;
  }
  name = g_strdup_printf ("InitAction %u", id);
  sprite->init_action = swfdec_script_new_for_context (SWFDEC_AS_CONTEXT (SWFDEC_DECODER (s)->player),
      bits, name, s->version);
  g_free (name);
  if (sprite->init_action) {
    swfdec_script_ref (sprite->init_action);
    swfdec_root_sprite_add_root_action (SWFDEC_ROOT_SPRITE (s->main_sprite),
	SWFDEC_ROOT_ACTION_INIT_SCRIPT, sprite->init_action);
  }

  return SWFDEC_STATUS_OK;
}

