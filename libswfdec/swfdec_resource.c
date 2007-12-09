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
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_interpret.h"
#include "swfdec_as_strings.h"
#include "swfdec_character.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_flash_security.h"
#include "swfdec_image_decoder.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"
#include "swfdec_movie_clip_loader.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource_request.h"
#include "swfdec_script.h"
#include "swfdec_sprite.h"
#include "swfdec_swf_decoder.h"
#include "swfdec_utils.h"


static void swfdec_resource_loader_target_init (SwfdecLoaderTargetInterface *iface);
G_DEFINE_TYPE_WITH_CODE (SwfdecResource, swfdec_resource, SWFDEC_TYPE_FLASH_SECURITY,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_resource_loader_target_init))

/*** SWFDEC_LOADER_TARGET interface ***/

static gboolean 
swfdec_resource_is_root (SwfdecResource *resource)
{
  g_return_val_if_fail (SWFDEC_IS_RESOURCE (resource), FALSE);

  return
    resource->movie == SWFDEC_FLASH_SECURITY (resource)->player->roots->data;
}

static SwfdecPlayer *
swfdec_resource_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_FLASH_SECURITY (target)->player;
}

static void
swfdec_resource_check_rights (SwfdecResource *resource)
{
  SwfdecFlashSecurity *sec = SWFDEC_FLASH_SECURITY (resource);
  SwfdecSwfDecoder *dec = SWFDEC_SWF_DECODER (resource->decoder);

  if (swfdec_resource_is_root (resource)) {
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
    swfdec_movie_invalidate_last (SWFDEC_MOVIE (movie));
    swfdec_resource_check_rights (instance);
    if (swfdec_resource_is_root (instance)) {
      swfdec_movie_initialize (SWFDEC_MOVIE (movie));
      swfdec_player_perform_actions (SWFDEC_FLASH_SECURITY (instance)->player);
    }
  } else {
    g_assert_not_reached ();
  }
}

/* NB: name must be GC'ed */
static void
swfdec_resource_emit_signal (SwfdecResource *resource, const char *name, gboolean progress, 
    SwfdecAsValue *args, guint n_args)
{
  SwfdecAsContext *cx;
  SwfdecAsObject *movie;
  guint skip = progress ? 4 : 2;
  SwfdecAsValue vals[n_args + skip];

  if (resource->clip_loader == NULL)
    return;
  cx = SWFDEC_AS_OBJECT (resource->clip_loader)->context;
  g_assert (resource->target);
  movie = swfdec_action_lookup_object (cx, SWFDEC_PLAYER (cx)->roots->data, 
      resource->target, resource->target + strlen (resource->target));
  if (!SWFDEC_IS_SPRITE_MOVIE (movie)) {
    SWFDEC_DEBUG ("no movie, not emitting signal");
    return;
  }
  if (name == SWFDEC_AS_STR_onLoadInit &&
      movie != SWFDEC_AS_OBJECT (resource->movie)) {
    SWFDEC_INFO ("not emitting onLoadInit - the movie is different");
    return;
  }

  SWFDEC_AS_VALUE_SET_STRING (&vals[0], name);
  SWFDEC_AS_VALUE_SET_OBJECT (&vals[1], movie);
  if (progress) {
    SwfdecResource *res;
    
    if (SWFDEC_IS_MOVIE (movie))
      res = swfdec_movie_get_own_resource (SWFDEC_MOVIE (movie));
    else
      res = NULL;
    if (res && res->decoder) {
      SwfdecDecoder *dec = res->decoder;
      SWFDEC_AS_VALUE_SET_INT (&vals[2], dec->bytes_loaded);
      SWFDEC_AS_VALUE_SET_INT (&vals[3], dec->bytes_total);
    } else {
      SWFDEC_AS_VALUE_SET_INT (&vals[2], 0);
      SWFDEC_AS_VALUE_SET_INT (&vals[3], 0);
    }
  }
  if (n_args)
    memcpy (&vals[skip], args, sizeof (SwfdecAsValue) * n_args);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (resource->clip_loader), SWFDEC_AS_STR_broadcastMessage, 
      n_args + skip, vals, NULL);
}

static void
swfdec_resource_emit_error (SwfdecResource *resource, const char *message)
{
  SwfdecAsValue vals[2];

  SWFDEC_AS_VALUE_SET_STRING (&vals[0], message);
  SWFDEC_AS_VALUE_SET_INT (&vals[1], 0);

  swfdec_resource_emit_signal (resource, SWFDEC_AS_STR_onLoadError, FALSE, vals, 2);
}

static SwfdecSpriteMovie *
swfdec_resource_replace_movie (SwfdecSpriteMovie *movie, SwfdecResource *resource)
{
  /* can't use swfdec_movie_duplicate() here, we copy to same depth */
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecMovie *copy;
  
  copy = swfdec_movie_new (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context), 
      mov->depth, mov->parent, resource, NULL, mov->name);
  if (copy == NULL)
    return FALSE;
  copy->matrix = mov->matrix;
  copy->original_name = mov->original_name;
  copy->modified = mov->modified;
  copy->xscale = mov->xscale;
  copy->yscale = mov->yscale;
  copy->rotation = mov->rotation;
  /* FIXME: are events copied? If so, wouldn't that be a security issue? */
  swfdec_movie_set_static_properties (copy, &mov->original_transform,
      &mov->original_ctrans, mov->original_ratio, mov->clip_depth, 
      mov->blend_mode, NULL);
  swfdec_movie_remove (mov);
  swfdec_movie_queue_update (copy, SWFDEC_MOVIE_INVALID_MATRIX);
  return SWFDEC_SPRITE_MOVIE (copy);
}

static gboolean
swfdec_resource_create_movie (SwfdecResource *resource)
{
  SwfdecPlayer *player;
  SwfdecSpriteMovie *movie;
  int level = -1;

  if (resource->movie)
    return TRUE;
  player = SWFDEC_FLASH_SECURITY (resource)->player;
  movie = (SwfdecSpriteMovie *) swfdec_action_lookup_object (SWFDEC_AS_CONTEXT (player),
      player->roots->data, resource->target, resource->target + strlen (resource->target));
  if (!SWFDEC_IS_SPRITE_MOVIE (movie)) {
    level = swfdec_player_get_level (player, resource->target);
    if (level < 0) {
      SWFDEC_WARNING ("%s does not reference a movie, not loading %s", resource->target,
	  swfdec_url_get_url (swfdec_loader_get_url (resource->loader)));
      swfdec_loader_close (resource->loader);
      swfdec_player_unroot_object (player, G_OBJECT (resource));
      return FALSE;
    }
    movie = swfdec_player_get_movie_at_level (player, level);
  }
  if (movie == NULL) {
    movie = swfdec_player_create_movie_at_level (player, resource, level);
  } else {
    movie = swfdec_resource_replace_movie (movie, resource);
  }
  swfdec_player_unroot_object (player, G_OBJECT (resource));
  return TRUE;
}

static void
swfdec_resource_loader_target_open (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecResource *instance = SWFDEC_RESOURCE (target);
  const char *query;

  if (!swfdec_resource_create_movie (instance))
    return;
  query = swfdec_url_get_query (swfdec_loader_get_url (loader));
  if (query) {
    SWFDEC_INFO ("set url query movie variables: %s", query);
    swfdec_as_object_decode (SWFDEC_AS_OBJECT (instance->movie), query);
  }
  if (instance->variables) {
    SWFDEC_INFO ("set manual movie variables: %s", instance->variables);
    swfdec_as_object_decode (SWFDEC_AS_OBJECT (instance->movie), instance->variables);
  }
  swfdec_resource_emit_signal (instance, SWFDEC_AS_STR_onLoadStart, FALSE, NULL, 0);
  instance->state = SWFDEC_RESOURCE_OPENED;
}

static void
swfdec_resource_loader_target_parse (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (target);
  SwfdecBuffer *buffer;
  SwfdecDecoder *dec = resource->decoder;
  SwfdecStatus status;
  guint parsed;

  if (dec == NULL && swfdec_buffer_queue_get_offset (loader->queue) == 0) {
    if (swfdec_buffer_queue_get_depth (loader->queue) < SWFDEC_DECODER_DETECT_LENGTH)
      return;
    buffer = swfdec_buffer_queue_peek (loader->queue, 4);
    dec =
      swfdec_decoder_new (SWFDEC_FLASH_SECURITY (resource)->player, buffer);
    swfdec_buffer_unref (buffer);
    if (dec == NULL) {
      SWFDEC_ERROR ("no decoder found for format");
    } else {
      glong total;
      resource->decoder = dec;
      total = swfdec_loader_get_size (loader);
      if (total >= 0)
	dec->bytes_total = total;
    }
  }
  while (swfdec_buffer_queue_get_depth (loader->queue)) {
    parsed = 0;
    status = 0;
    do {
      buffer = swfdec_buffer_queue_peek_buffer (loader->queue);
      if (buffer == NULL)
	break;
      if (parsed + buffer->length <= 65536) {
	swfdec_buffer_unref (buffer);
	buffer = swfdec_buffer_queue_pull_buffer (loader->queue);
      } else {
	swfdec_buffer_unref (buffer);
	buffer = swfdec_buffer_queue_pull (loader->queue, 65536 - parsed);
      }
      parsed += buffer->length;
      if (dec) {
	status |= swfdec_decoder_parse (dec, buffer);
      } else {
	swfdec_buffer_unref (buffer);
      }
    } while (parsed < 65536 && (status & (SWFDEC_STATUS_ERROR | SWFDEC_STATUS_EOF)) == 0);
    if (status & SWFDEC_STATUS_ERROR) {
      SWFDEC_ERROR ("parsing error");
      swfdec_loader_set_target (loader, NULL);
      return;
    }
    if ((status & SWFDEC_STATUS_INIT)) {
      if (SWFDEC_IS_SWF_DECODER (dec))
	resource->version = SWFDEC_SWF_DECODER (dec)->version;
      if (swfdec_resource_is_root (resource)) {
	swfdec_player_initialize (SWFDEC_FLASH_SECURITY (resource)->player,
	    resource->version, dec->rate, dec->width, dec->height);
      }
    }
    if (status & SWFDEC_STATUS_IMAGE)
      swfdec_resource_loader_target_image (resource);
    swfdec_resource_emit_signal (resource, SWFDEC_AS_STR_onLoadProgress, TRUE, NULL, 0);
    if (status & SWFDEC_STATUS_EOF)
      return;
  }
}

static void
swfdec_resource_loader_target_eof (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (target);
  SwfdecAsValue val;

  swfdec_resource_emit_signal (resource, SWFDEC_AS_STR_onLoadProgress, TRUE, NULL, 0);
  if (resource->decoder) {
    SwfdecDecoder *dec = resource->decoder;
    swfdec_decoder_eof (dec);
    if (dec->data_type != SWFDEC_LOADER_DATA_UNKNOWN)
      swfdec_loader_set_data_type (loader, dec->data_type);
  }
  SWFDEC_AS_VALUE_SET_INT (&val, 0); /* FIXME */
  swfdec_resource_emit_signal (resource, SWFDEC_AS_STR_onLoadComplete, FALSE, &val, 1);
  resource->state = SWFDEC_RESOURCE_COMPLETE;
}

static void
swfdec_resource_loader_target_error (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (target);
  const char *message;

  switch (resource->state) {
    case SWFDEC_RESOURCE_REQUESTED:
      message = SWFDEC_AS_STR_URLNotFound;
      break;
    case SWFDEC_RESOURCE_OPENED:
      message = SWFDEC_AS_STR_LoadNeverCompleted;
      break;
    case SWFDEC_RESOURCE_NEW:
    case SWFDEC_RESOURCE_COMPLETE:
    case SWFDEC_RESOURCE_DONE:
    default:
      g_assert_not_reached ();
      message = SWFDEC_AS_STR_EMPTY;
      break;
  }
  swfdec_resource_emit_error (resource, message);
}

static void
swfdec_resource_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_resource_loader_target_get_player;
  iface->open = swfdec_resource_loader_target_open;
  iface->parse = swfdec_resource_loader_target_parse;
  iface->error = swfdec_resource_loader_target_error;
  iface->eof = swfdec_resource_loader_target_eof;
}

static void
swfdec_resource_dispose (GObject *object)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (object);

  if (resource->loader) {
    swfdec_loader_set_target (resource->loader, NULL);
    g_object_unref (resource->loader);
    resource->loader = NULL;
  }
  if (resource->decoder) {
    g_object_unref (resource->decoder);
    resource->decoder = NULL;
  }
  if (resource->clip_loader) {
    g_object_unref (resource->clip_loader);
    resource->clip_loader = NULL;
  }
  g_free (resource->target);
  g_free (resource->variables);
  g_hash_table_destroy (resource->exports);
  g_hash_table_destroy (resource->export_names);

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

static void
swfdec_resource_set_loader (SwfdecResource *resource, SwfdecLoader *loader)
{
  g_return_if_fail (SWFDEC_IS_RESOURCE (resource));
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (resource->loader == NULL);

  resource->loader = g_object_ref (loader);
  swfdec_loader_set_target (resource->loader, SWFDEC_LOADER_TARGET (resource));
  swfdec_flash_security_set_url (SWFDEC_FLASH_SECURITY (resource),
      swfdec_loader_get_url (loader));
  resource->state = SWFDEC_RESOURCE_REQUESTED;
}

SwfdecResource *
swfdec_resource_new (SwfdecPlayer *player, SwfdecLoader *loader, const char *variables)
{
  SwfdecResource *resource;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);

  resource = g_object_new (SWFDEC_TYPE_RESOURCE, NULL);
  resource->version = 7;
  SWFDEC_FLASH_SECURITY (resource)->player = player;
  resource->variables = g_strdup (variables);
  swfdec_resource_set_loader (resource, loader);

  return resource;
}

void
swfdec_resource_mark (SwfdecResource *resource)
{
  g_return_if_fail (SWFDEC_IS_RESOURCE (resource));

  if (resource->clip_loader)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (resource->clip_loader));
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

static void
swfdec_resource_do_load (SwfdecPlayer *player, SwfdecLoader *loader, gpointer resourcep)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (resourcep);

  if (loader == NULL) {
    /* *** Security Sandbox Violation *** */
    swfdec_resource_emit_error (resource, SWFDEC_AS_STR_IllegalRequest);
    return;
  }
  swfdec_resource_set_loader (resource, loader);
  g_object_unref (loader);
}

static void
swfdec_resource_do_unload (SwfdecPlayer *player, const char *target, gpointer resourcep)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (resourcep);
  SwfdecSpriteMovie *movie;
  
  movie = (SwfdecSpriteMovie *) swfdec_action_lookup_object (
      SWFDEC_AS_CONTEXT (player), player->roots->data, 
      target, target + strlen (target));
  if (!SWFDEC_IS_SPRITE_MOVIE (movie)) {
    SWFDEC_DEBUG ("no movie, not unloading");
    return;
  }
  swfdec_resource_replace_movie (movie, resource);
}

/* NB: must be called from a script */
void
swfdec_resource_load (SwfdecPlayer *player, const char *target, const char *url, 
    SwfdecLoaderRequest request, SwfdecBuffer *buffer, SwfdecMovieClipLoader *loader)
{
  SwfdecSpriteMovie *movie;
  SwfdecResource *resource;
  char *path;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (target != NULL);
  g_return_if_fail (url != NULL);
  g_return_if_fail (loader == NULL || SWFDEC_IS_MOVIE_CLIP_LOADER (loader));

  g_assert (SWFDEC_AS_CONTEXT (player)->frame != NULL);
  movie = (SwfdecSpriteMovie *) swfdec_player_get_movie_from_string (player, target);
  if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
    path = swfdec_movie_get_path (SWFDEC_MOVIE (movie), TRUE);
  } else if (swfdec_player_get_level (player, target) >= 0) {
    path = g_strdup (target);
  } else {
    SWFDEC_WARNING ("%s does not reference a movie, not loading %s", target, url);
    return;
  }
  if (url[0] == '\0') {
    if (movie) {
      resource = g_object_ref (SWFDEC_MOVIE (movie)->resource);
      swfdec_player_request_unload (player, path, swfdec_resource_do_unload, resource, g_object_unref);
    }
    g_free (path);
  } else {
    resource = g_object_new (SWFDEC_TYPE_RESOURCE, NULL);
    resource->version = SWFDEC_AS_CONTEXT (player)->version;
    SWFDEC_FLASH_SECURITY (resource)->player = player;
    resource->target = path;
    if (loader)
      resource->clip_loader = g_object_ref (loader);
    swfdec_player_root_object (player, G_OBJECT (resource));
    swfdec_player_request_resource (player, SWFDEC_AS_CONTEXT (player)->frame->security, 
	url, request, buffer, swfdec_resource_do_load, resource, g_object_unref);
  }
}

gboolean
swfdec_resource_emit_on_load_init (SwfdecResource *resource)
{
  g_return_val_if_fail (SWFDEC_IS_RESOURCE (resource), FALSE);

  if (resource->state != SWFDEC_RESOURCE_COMPLETE)
    return FALSE;

  if (resource->movie && SWFDEC_IS_IMAGE_DECODER (resource->decoder)) {
    SwfdecImage *image = SWFDEC_IMAGE_DECODER (resource->decoder)->image;
    if (image) {
      swfdec_movie_invalidate_next (SWFDEC_MOVIE (resource->movie));
      swfdec_movie_queue_update (SWFDEC_MOVIE (resource->movie), SWFDEC_MOVIE_INVALID_EXTENTS);
      SWFDEC_MOVIE (resource->movie)->image = g_object_ref (image);
    }
  }
  swfdec_resource_emit_signal (resource, SWFDEC_AS_STR_onLoadInit, FALSE, NULL, 0);
  resource->state = SWFDEC_RESOURCE_DONE;
  /* free now unneeded resources */
  if (resource->clip_loader) {
    g_object_unref (resource->clip_loader);
    resource->clip_loader = NULL;
  }
  return TRUE;
}
