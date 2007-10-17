/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "swfdec_swf_instance.h"
#include "swfdec_as_internal.h"
#include "swfdec_character.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_flash_security.h"
#include "swfdec_flv_decoder.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"
#include "swfdec_script.h"
#include "swfdec_sprite.h"
#include "swfdec_swf_decoder.h"
#include "swfdec_utils.h"


static void swfdec_swf_instance_loader_target_init (SwfdecLoaderTargetInterface *iface);
G_DEFINE_TYPE_WITH_CODE (SwfdecSwfInstance, swfdec_swf_instance, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_swf_instance_loader_target_init))

/*** SWFDEC_LOADER_TARGET interface ***/

static SwfdecPlayer *
swfdec_swf_instance_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_AS_OBJECT (SWFDEC_SWF_INSTANCE (target)->movie)->context);
}

static void
swfdec_swf_instance_allow_network (SwfdecPlayer *player)
{
  SwfdecFlashSecurity *sec;

  g_print ("enabling network access for %s\n", 
      swfdec_url_get_url (swfdec_loader_get_url (player->loader)));
  SWFDEC_INFO ("enabling network access for %s",
      swfdec_url_get_url (swfdec_loader_get_url (player->loader)));

  sec = SWFDEC_FLASH_SECURITY (player->security);
  sec->allow_remote = TRUE;
  sec->allow_local = FALSE;
}

static void
swfdec_swf_instance_loader_target_image (SwfdecSwfInstance *instance)
{
  SwfdecSpriteMovie *movie = instance->movie;

  if (movie->sprite != NULL)
    return;

  if (SWFDEC_IS_SWF_DECODER (instance->decoder)) {
    SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
    SwfdecSwfDecoder *dec = SWFDEC_SWF_DECODER (instance->decoder);
    movie->sprite = dec->main_sprite;
    swfdec_movie_invalidate (SWFDEC_MOVIE (movie));
    
    /* if first instance */
    if (player->loader == instance->loader && dec->use_network &&
	swfdec_url_has_protocol (swfdec_loader_get_url (instance->loader), "file"))
      swfdec_swf_instance_allow_network (player);
  } else if (SWFDEC_IS_FLV_DECODER (instance->decoder)) {
    /* nothing to do, please move along */
  } else {
    g_assert_not_reached ();
  }
}

static void
swfdec_swf_instance_loader_target_open (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecSwfInstance *instance = SWFDEC_SWF_INSTANCE (target);
  const char *query;

  query = swfdec_url_get_query (swfdec_loader_get_url (loader));
  if (query) {
    SWFDEC_INFO ("set url query movie variables: %s", query);
    swfdec_movie_set_variables (SWFDEC_MOVIE (instance->movie), query);
  }
  if (instance->variables) {
    SWFDEC_INFO ("set manual movie variables: %s", instance->variables);
    swfdec_movie_set_variables (SWFDEC_MOVIE (instance->movie), instance->variables);
  }
}

static void
swfdec_swf_instance_loader_target_parse (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecSwfInstance *instance = SWFDEC_SWF_INSTANCE (target);
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (instance->movie)->context);
  SwfdecDecoder *dec = instance->decoder;
  SwfdecDecoderClass *klass;

  if (dec == NULL) {
    if (!swfdec_decoder_can_detect (loader->queue))
      return;
    dec = swfdec_decoder_new (player, loader->queue);
    if (dec == NULL) {
      SWFDEC_ERROR ("no decoder found");
      swfdec_loader_set_target (loader, NULL);
      return;
    }

    if (SWFDEC_IS_FLV_DECODER (dec)) {
      swfdec_loader_set_data_type (loader, SWFDEC_LOADER_DATA_FLV);
      swfdec_flv_decoder_add_movie (SWFDEC_FLV_DECODER (dec), SWFDEC_MOVIE (instance->movie));
    } else if (SWFDEC_IS_SWF_DECODER (dec)) {
      swfdec_loader_set_data_type (loader, SWFDEC_LOADER_DATA_SWF);
      instance->decoder = dec;
    } else {
      SWFDEC_FIXME ("implement handling of %s", G_OBJECT_TYPE_NAME (dec));
      g_object_unref (dec);
      swfdec_loader_set_target (loader, NULL);
      return;
    }
    /* HACK for flv playback */
    if (target != loader->target) {
      swfdec_loader_target_parse (loader->target, loader);
      return;
    }
  }
  klass = SWFDEC_DECODER_GET_CLASS (dec);
  g_return_if_fail (klass->parse);
  while (TRUE) {
    SwfdecStatus status = klass->parse (dec);
    switch (status) {
      case SWFDEC_STATUS_ERROR:
	SWFDEC_ERROR ("parsing error");
	swfdec_loader_set_target (loader, NULL);
	return;
      case SWFDEC_STATUS_OK:
	break;
      case SWFDEC_STATUS_NEEDBITS:
	return;
      case SWFDEC_STATUS_IMAGE:
	swfdec_swf_instance_loader_target_image (instance);
	break;
      case SWFDEC_STATUS_INIT:
	swfdec_player_initialize (player, 
	    SWFDEC_IS_SWF_DECODER (dec) ? SWFDEC_SWF_DECODER (dec)->version : 7, /* <-- HACK */
	    dec->rate, dec->width, dec->height);
	break;
      case SWFDEC_STATUS_EOF:
	return;
      default:
	g_assert_not_reached ();
	return;
    }
  }
}

static void
swfdec_swf_instance_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_swf_instance_loader_target_get_player;
  iface->open = swfdec_swf_instance_loader_target_open;
  iface->parse = swfdec_swf_instance_loader_target_parse;
}

static void
swfdec_swf_instance_dispose (GObject *object)
{
  SwfdecSwfInstance *instance = SWFDEC_SWF_INSTANCE (object);

  swfdec_loader_set_target (instance->loader, NULL);
  g_object_unref (instance->loader);
  if (instance->decoder) {
    g_object_unref (instance->decoder);
    instance->decoder = NULL;
  }
  g_free (instance->variables);
  g_hash_table_destroy (instance->exports);
  g_hash_table_destroy (instance->export_names);

  G_OBJECT_CLASS (swfdec_swf_instance_parent_class)->dispose (object);
}

static void
swfdec_swf_instance_class_init (SwfdecSwfInstanceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_swf_instance_dispose;
}

static void
swfdec_swf_instance_init (SwfdecSwfInstance *instance)
{
  instance->exports = g_hash_table_new (swfdec_str_case_hash, swfdec_str_case_equal);
  instance->export_names = g_hash_table_new (g_direct_hash, g_direct_equal);
}

SwfdecSwfInstance *
swfdec_swf_instance_new (SwfdecSpriteMovie *movie, SwfdecLoader *loader, const char *variables)
{
  SwfdecMovie *mov;
  SwfdecSwfInstance *swf;

  g_return_val_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie), NULL);
  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);

  mov = SWFDEC_MOVIE (movie);
  swf = g_object_new (SWFDEC_TYPE_SWF_INSTANCE, NULL);
  /* set important variables */
  swf->variables = g_strdup (variables);
  swf->movie = movie;
  if (mov->swf)
    g_object_unref (mov->swf);
  mov->swf = swf;
  /* set loader (that depends on those vars) */
  swf->loader = g_object_ref (loader);
  swfdec_loader_set_target (loader, SWFDEC_LOADER_TARGET (swf));

  return swf;
}

gpointer
swfdec_swf_instance_get_export (SwfdecSwfInstance *instance, const char *name)
{
  g_return_val_if_fail (SWFDEC_IS_SWF_INSTANCE (instance), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return g_hash_table_lookup (instance->exports, name);
}

const char *
swfdec_swf_instance_get_export_name (SwfdecSwfInstance *instance, SwfdecCharacter *character)
{
  g_return_val_if_fail (SWFDEC_IS_SWF_INSTANCE (instance), NULL);
  g_return_val_if_fail (SWFDEC_IS_CHARACTER (character), NULL);

  return g_hash_table_lookup (instance->export_names, character);
}

static void
swfdec_swf_instance_add_export (SwfdecSwfInstance *instance, SwfdecCharacter *character, const char *name)
{
  g_return_if_fail (SWFDEC_IS_SWF_INSTANCE (instance));
  g_return_if_fail (SWFDEC_IS_CHARACTER (character));
  g_return_if_fail (name != NULL);

  g_hash_table_insert (instance->exports, (char *) name, character);
  g_hash_table_insert (instance->export_names, character, (char *) name);
}

void
swfdec_swf_instance_advance (SwfdecSwfInstance *instance)
{
  SwfdecSwfDecoder *s;
  GArray *array;
  guint i;

  g_return_if_fail (SWFDEC_IS_SWF_INSTANCE (instance));

  s = SWFDEC_SWF_DECODER (instance->decoder);
  SWFDEC_LOG ("performing actions for frame %u", instance->parse_frame);
  if (s->root_actions) {
    array = s->root_actions[instance->parse_frame];
  } else {
    array = NULL;
  }
  instance->parse_frame++;
  if (array == NULL)
    return;
  for (i = 0; i < array->len; i++) {
    SwfdecRootAction *action = &g_array_index (array, SwfdecRootAction, i);
    switch (action->type) {
      case SWFDEC_ROOT_ACTION_INIT_SCRIPT:
	swfdec_as_object_run (SWFDEC_AS_OBJECT (instance->movie), action->data);
	break;
      case SWFDEC_ROOT_ACTION_EXPORT:
	{
	  SwfdecRootExportData *data = action->data;
	  swfdec_swf_instance_add_export (instance, data->character, data->name);
	}
	break;
      default:
	g_assert_not_reached ();
    }
  }
}

