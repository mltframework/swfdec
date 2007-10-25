/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_resource.h"
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


static void swfdec_resource_loader_target_init (SwfdecLoaderTargetInterface *iface);
G_DEFINE_TYPE_WITH_CODE (SwfdecResource, swfdec_resource, SWFDEC_TYPE_FLASH_SECURITY,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_resource_loader_target_init))

/*** SWFDEC_LOADER_TARGET interface ***/

static SwfdecPlayer *
swfdec_resource_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_AS_OBJECT (SWFDEC_RESOURCE (target)->movie)->context);
}

static void
swfdec_resource_check_rights (SwfdecResource *resource)
{
  SwfdecFlashSecurity *sec = SWFDEC_FLASH_SECURITY (resource);
  SwfdecSwfDecoder *dec = SWFDEC_SWF_DECODER (resource->decoder);

  if (resource->initial) {
    if (dec->use_network && sec->sandbox == SWFDEC_SANDBOX_LOCAL_FILE)
      sec->sandbox = SWFDEC_SANDBOX_LOCAL_NETWORK;
    SWFDEC_INFO ("enabling local-with-network sandbox for %s",
	swfdec_url_get_url (swfdec_loader_get_url (resource->loader)));
  }
}

static void
swfdec_resource_loader_target_image (SwfdecResource *instance)
{
  SwfdecSpriteMovie *movie = instance->movie;

  if (movie->sprite != NULL)
    return;

  if (SWFDEC_IS_SWF_DECODER (instance->decoder)) {
    SwfdecSwfDecoder *dec = SWFDEC_SWF_DECODER (instance->decoder);

    movie->sprite = dec->main_sprite;
    g_assert (movie->sprite->parse_frame > 0);
    movie->n_frames = movie->sprite->n_frames;
    swfdec_movie_invalidate (SWFDEC_MOVIE (movie));
    swfdec_resource_check_rights (instance);
  } else if (SWFDEC_IS_FLV_DECODER (instance->decoder)) {
    /* nothing to do, please move along */
  } else {
    g_assert_not_reached ();
  }
  swfdec_movie_initialize (SWFDEC_MOVIE (movie));
}

static void
swfdec_resource_open (SwfdecResource *instance, SwfdecLoader *loader)
{
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
swfdec_resource_loader_target_open (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecResource *instance = SWFDEC_RESOURCE (target);

  if (!instance->initial)
    return;

  swfdec_resource_open (instance, loader);
}

static void
swfdec_resource_parse (SwfdecResource *instance, SwfdecLoader *loader)
{
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
	swfdec_resource_loader_target_image (instance);
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
swfdec_resource_loader_target_parse (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecResource *instance = SWFDEC_RESOURCE (target);

  if (!instance->initial)
    return;

  swfdec_resource_parse (instance, loader);
}

static void
swfdec_resource_loader_target_eof (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (target);

  if (resource->initial)
    return;

  swfdec_resource_open (resource, loader);
  swfdec_resource_parse (resource, loader);
}

static void
swfdec_resource_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_resource_loader_target_get_player;
  iface->open = swfdec_resource_loader_target_open;
  iface->parse = swfdec_resource_loader_target_parse;
  iface->eof = swfdec_resource_loader_target_eof;
}

static void
swfdec_resource_dispose (GObject *object)
{
  SwfdecResource *instance = SWFDEC_RESOURCE (object);

  swfdec_loader_set_target (instance->loader, NULL);
  g_object_unref (instance->loader);
  if (instance->decoder) {
    g_object_unref (instance->decoder);
    instance->decoder = NULL;
  }
  g_free (instance->variables);
  g_hash_table_destroy (instance->exports);
  g_hash_table_destroy (instance->export_names);

  G_OBJECT_CLASS (swfdec_resource_parent_class)->dispose (object);
}

static void
swfdec_resource_class_init (SwfdecResourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_resource_dispose;
}

static void
swfdec_resource_init (SwfdecResource *instance)
{
  instance->exports = g_hash_table_new_full (swfdec_str_case_hash, 
      swfdec_str_case_equal, g_free, g_object_unref);
  instance->export_names = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
      g_object_unref, g_free);
}

SwfdecResource *
swfdec_resource_new (SwfdecLoader *loader, const char *variables)
{
  SwfdecResource *swf;

  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);

  swf = g_object_new (SWFDEC_TYPE_RESOURCE, NULL);
  /* set important variables */
  swf->variables = g_strdup (variables);
  /* set loader (that depends on those vars) */
  swf->loader = g_object_ref (loader);
  swfdec_flash_security_set_url (SWFDEC_FLASH_SECURITY (swf),
      swfdec_loader_get_url (loader));

  return swf;
}

void
swfdec_resource_set_movie (SwfdecResource *resource, SwfdecSpriteMovie *movie)
{
  g_return_if_fail (SWFDEC_IS_RESOURCE (resource));
  g_return_if_fail (resource->movie == NULL);
  g_return_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie));

  resource->movie = movie;
  swfdec_loader_set_target (resource->loader, SWFDEC_LOADER_TARGET (resource));
}

gpointer
swfdec_resource_get_export (SwfdecResource *instance, const char *name)
{
  g_return_val_if_fail (SWFDEC_IS_RESOURCE (instance), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return g_hash_table_lookup (instance->exports, name);
}

const char *
swfdec_resource_get_export_name (SwfdecResource *instance, SwfdecCharacter *character)
{
  g_return_val_if_fail (SWFDEC_IS_RESOURCE (instance), NULL);
  g_return_val_if_fail (SWFDEC_IS_CHARACTER (character), NULL);

  return g_hash_table_lookup (instance->export_names, character);
}

void
swfdec_resource_add_export (SwfdecResource *instance, SwfdecCharacter *character, const char *name)
{
  g_return_if_fail (SWFDEC_IS_RESOURCE (instance));
  g_return_if_fail (SWFDEC_IS_CHARACTER (character));
  g_return_if_fail (name != NULL);

  g_hash_table_insert (instance->exports, g_strdup (name), g_object_ref (character));
  g_hash_table_insert (instance->export_names, g_object_ref (character), g_strdup (name));
}

