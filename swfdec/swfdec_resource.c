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
#include "swfdec_as_object.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_interpret.h"
#include "swfdec_as_strings.h"
#include "swfdec_character.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_image_decoder.h"
#include "swfdec_loader_internal.h"
#include "swfdec_movie_clip_loader.h"
#include "swfdec_player_internal.h"
#include "swfdec_sandbox.h"
#include "swfdec_script.h"
#include "swfdec_sprite.h"
#include "swfdec_stream_target.h"
#include "swfdec_swf_decoder.h"
#include "swfdec_utils.h"


static void swfdec_resource_stream_target_init (SwfdecStreamTargetInterface *iface);
G_DEFINE_TYPE_WITH_CODE (SwfdecResource, swfdec_resource, SWFDEC_TYPE_AS_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_STREAM_TARGET, swfdec_resource_stream_target_init))

/*** SWFDEC_STREAM_TARGET interface ***/

static gboolean 
swfdec_resource_is_root (SwfdecResource *resource)
{
  g_return_val_if_fail (SWFDEC_IS_RESOURCE (resource), FALSE);

  return
    resource->movie == SWFDEC_PLAYER (SWFDEC_AS_OBJECT (resource)->context)->priv->roots->data;
}

static SwfdecPlayer *
swfdec_resource_stream_target_get_player (SwfdecStreamTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_AS_OBJECT (target)->context);
}

static void
swfdec_resource_stream_target_image (SwfdecResource *instance)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (instance)->context);
  SwfdecSpriteMovie *movie = instance->movie;

  if (movie->sprite != NULL)
    return;

  if (SWFDEC_IS_SWF_DECODER (instance->decoder)) {
    SwfdecSwfDecoder *dec = SWFDEC_SWF_DECODER (instance->decoder);
    SwfdecSandbox *old_sandbox;

    old_sandbox = instance->sandbox;
    instance->sandbox = swfdec_sandbox_get_for_url (player,
	swfdec_loader_get_url (instance->loader), instance->version,
	SWFDEC_SWF_DECODER (instance->decoder)->use_network);
    if (instance->sandbox) {
      movie->sprite = dec->main_sprite;
      g_assert (movie->sprite->parse_frame > 0);
      movie->n_frames = movie->sprite->n_frames;
      swfdec_movie_invalidate_last (SWFDEC_MOVIE (movie));
      swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie), instance->sandbox->MovieClip);
      if (swfdec_resource_is_root (instance)) {
	swfdec_player_start_ticking (player);
	swfdec_movie_initialize (SWFDEC_MOVIE (movie));
	swfdec_player_perform_actions (player);
      }
    } else {
      SWFDEC_FIXME ("cannot continue loading %s, invalid rights", 
	  swfdec_url_get_url (swfdec_loader_get_url (instance->loader)));
      swfdec_stream_set_target (SWFDEC_STREAM (instance->loader), NULL);
      instance->sandbox = old_sandbox;
      /* FIXME: anyting else on the movie we need to clear? */
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
  SwfdecMovie *movie;
  guint skip = progress ? 4 : 2;
  SwfdecAsValue vals[n_args + skip];

  if (resource->clip_loader == NULL)
    return;
  cx = SWFDEC_AS_OBJECT (resource->clip_loader)->context;
  /* This feels wrong. Why do we resolve here by real name? */
  if (resource->target) {
    SwfdecMovie *parent = swfdec_movie_resolve (resource->target->parent);
    movie = swfdec_movie_get_by_name (parent, resource->target->name, FALSE);
  } else {
    movie = NULL;
  }
  if (movie == NULL && resource->movie != NULL) {
    SWFDEC_DEBUG ("no movie, not emitting signal");
    return;
  }
  if (name == SWFDEC_AS_STR_onLoadInit &&
      movie != SWFDEC_MOVIE (resource->movie)) {
    SWFDEC_INFO ("not emitting onLoadInit - the movie is different");
    return;
  }

  SWFDEC_AS_VALUE_SET_STRING (&vals[0], name);
  if (movie) {
    SWFDEC_AS_VALUE_SET_OBJECT (&vals[1], SWFDEC_AS_OBJECT (movie));
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&vals[1]);
  }
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
  /* FIXME: what's the correct sandbox here? */
  swfdec_sandbox_use (resource->clip_loader_sandbox);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (resource->clip_loader), SWFDEC_AS_STR_broadcastMessage, 
      n_args + skip, vals, NULL);
  swfdec_sandbox_unuse (resource->clip_loader_sandbox);
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
  swfdec_movie_begin_update_matrix (copy);
  copy->matrix = mov->matrix;
  copy->original_name = mov->original_name;
  copy->modified = mov->modified;
  copy->xscale = mov->xscale;
  copy->yscale = mov->yscale;
  copy->rotation = mov->rotation;
  copy->lockroot = mov->lockroot;
  swfdec_movie_end_update_matrix (copy);
  /* FIXME: are events copied? If so, wouldn't that be a security issue? */
  swfdec_movie_set_static_properties (copy, &mov->original_transform,
      &mov->original_ctrans, mov->original_ratio, mov->clip_depth, 
      mov->blend_mode, NULL);
  swfdec_movie_remove (mov);
  return SWFDEC_SPRITE_MOVIE (copy);
}

static void
swfdec_resource_stream_target_open (SwfdecStreamTarget *target, SwfdecStream *stream)
{
  SwfdecLoader *loader = SWFDEC_LOADER (stream);
  SwfdecResource *instance = SWFDEC_RESOURCE (target);
  const char *query;

  g_assert (instance->movie);
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

static gboolean
swfdec_resource_stream_target_parse (SwfdecStreamTarget *target, SwfdecStream *stream)
{
  SwfdecLoader *loader = SWFDEC_LOADER (stream);
  SwfdecResource *resource = SWFDEC_RESOURCE (target);
  SwfdecBufferQueue *queue;
  SwfdecBuffer *buffer;
  SwfdecDecoder *dec = resource->decoder;
  SwfdecStatus status;
  guint parsed;

  queue = swfdec_stream_get_queue (stream);
  if (dec == NULL && swfdec_buffer_queue_get_offset (queue) == 0) {
    if (swfdec_buffer_queue_get_depth (queue) < SWFDEC_DECODER_DETECT_LENGTH)
      return FALSE;
    buffer = swfdec_buffer_queue_peek (queue, 4);
    dec = swfdec_decoder_new (buffer);
    swfdec_buffer_unref (buffer);
    if (dec == NULL) {
      SWFDEC_ERROR ("no decoder found for format");
    } else {
      glong total;
      resource->decoder = dec;
      g_signal_connect_swapped (dec, "missing-plugin", 
	  G_CALLBACK (swfdec_player_add_missing_plugin), SWFDEC_AS_OBJECT (resource)->context);
      total = swfdec_loader_get_size (loader);
      if (total >= 0)
	dec->bytes_total = total;
    }
  }
  while (swfdec_buffer_queue_get_depth (queue)) {
    parsed = 0;
    status = 0;
    do {
      buffer = swfdec_buffer_queue_peek_buffer (queue);
      if (buffer == NULL)
	break;
      if (parsed + buffer->length <= 65536) {
	swfdec_buffer_unref (buffer);
	buffer = swfdec_buffer_queue_pull_buffer (queue);
      } else {
	swfdec_buffer_unref (buffer);
	buffer = swfdec_buffer_queue_pull (queue, 65536 - parsed);
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
      swfdec_stream_set_target (SWFDEC_STREAM (loader), NULL);
      return FALSE;
    }
    if ((status & SWFDEC_STATUS_INIT)) {
      if (SWFDEC_IS_SWF_DECODER (dec))
	resource->version = SWFDEC_SWF_DECODER (dec)->version;
      if (swfdec_resource_is_root (resource)) {
	swfdec_player_initialize (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (resource)->context),
	    dec->rate, dec->width, dec->height);
      }
    }
    if (status & SWFDEC_STATUS_IMAGE)
      swfdec_resource_stream_target_image (resource);
    swfdec_resource_emit_signal (resource, SWFDEC_AS_STR_onLoadProgress, TRUE, NULL, 0);
    if (status & SWFDEC_STATUS_EOF)
      return FALSE;
  }
  return FALSE;
}

static gboolean
swfdec_resource_abort_if_not_initialized (SwfdecResource *resource)
{
  if (resource->sandbox)
    return FALSE;

  swfdec_as_context_abort (SWFDEC_AS_OBJECT (resource)->context,
      "This is not a Flash file");
  return TRUE;
}

static void
swfdec_resource_stream_target_close (SwfdecStreamTarget *target, SwfdecStream *stream)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (target);
  SwfdecAsValue val;

  swfdec_resource_emit_signal (resource, SWFDEC_AS_STR_onLoadProgress, TRUE, NULL, 0);
  if (resource->decoder) {
    SwfdecDecoder *dec = resource->decoder;
    swfdec_decoder_eof (dec);
    if (dec->data_type != SWFDEC_LOADER_DATA_UNKNOWN)
      swfdec_loader_set_data_type (SWFDEC_LOADER (stream), dec->data_type);
  }

  SWFDEC_AS_VALUE_SET_INT (&val, 0); /* FIXME */
  swfdec_resource_emit_signal (resource, SWFDEC_AS_STR_onLoadComplete, FALSE, &val, 1);
  resource->state = SWFDEC_RESOURCE_COMPLETE;
  if (swfdec_resource_abort_if_not_initialized (resource))
    return;

  if (resource->movie != NULL)
    swfdec_actor_queue_script (SWFDEC_ACTOR (resource->movie), SWFDEC_EVENT_LOAD);
}

static void
swfdec_resource_stream_target_error (SwfdecStreamTarget *target, SwfdecStream *stream)
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
  swfdec_resource_abort_if_not_initialized (resource);
}

static void
swfdec_resource_stream_target_init (SwfdecStreamTargetInterface *iface)
{
  iface->get_player = swfdec_resource_stream_target_get_player;
  iface->open = swfdec_resource_stream_target_open;
  iface->parse = swfdec_resource_stream_target_parse;
  iface->error = swfdec_resource_stream_target_error;
  iface->close = swfdec_resource_stream_target_close;
}

static void
swfdec_resource_mark (SwfdecAsObject *object)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (object);

  if (resource->clip_loader) {
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (resource->clip_loader));
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (resource->clip_loader_sandbox));
  }
  if (resource->sandbox)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (resource->sandbox));

  SWFDEC_AS_OBJECT_CLASS (swfdec_resource_parent_class)->mark (object);
}

static void
swfdec_resource_dispose (GObject *object)
{
  SwfdecResource *resource = SWFDEC_RESOURCE (object);

  if (resource->loader) {
    swfdec_stream_set_target (SWFDEC_STREAM (resource->loader), NULL);
    g_object_unref (resource->loader);
    resource->loader = NULL;
  }
  if (resource->decoder) {
    g_signal_handlers_disconnect_by_func (resource->decoder,
	  swfdec_player_add_missing_plugin, SWFDEC_AS_OBJECT (resource)->context);
    g_object_unref (resource->decoder);
    resource->decoder = NULL;
  }
  g_free (resource->variables);
  g_hash_table_destroy (resource->exports);
  g_hash_table_destroy (resource->export_names);

  G_OBJECT_CLASS (swfdec_resource_parent_class)->dispose (object);
}

static void
swfdec_resource_class_init (SwfdecResourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_resource_dispose;

  asobject_class->mark = swfdec_resource_mark;
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
  swfdec_stream_set_target (SWFDEC_STREAM (resource->loader), SWFDEC_STREAM_TARGET (resource));
  resource->state = SWFDEC_RESOURCE_REQUESTED;
}

SwfdecResource *
swfdec_resource_new (SwfdecPlayer *player, SwfdecLoader *loader, const char *variables)
{
  SwfdecResource *resource;
  guint size;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);

  size = sizeof (SwfdecResource);
  if (!swfdec_as_context_use_mem (SWFDEC_AS_CONTEXT (player), size))
    size = 0;
  resource = g_object_new (SWFDEC_TYPE_RESOURCE, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (resource), SWFDEC_AS_CONTEXT (player), size);
  resource->version = 8;
  resource->variables = g_strdup (variables);
  swfdec_resource_set_loader (resource, loader);

  return resource;
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

/*** RESOURC LOAD ***/

typedef struct _SwfdecResourceLoad SwfdecResourceLoad;
struct _SwfdecResourceLoad {
  SwfdecSandbox *		sandbox;
  char *			target_string;
  SwfdecSpriteMovie *		target_movie;
  char *			url;
  SwfdecBuffer *		buffer;
  SwfdecMovieClipLoader *	loader;
};

static void
swfdec_resource_load_free (gpointer loadp)
{
  SwfdecResourceLoad *load = loadp;

  swfdec_player_unroot (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (load->sandbox)->context), load);
  g_free (load->url);
  g_free (load->target_string);
  if (load->buffer)
    swfdec_buffer_unref (load->buffer);
  g_slice_free (SwfdecResourceLoad, load);
}

static void
swfdec_resource_load_mark (gpointer loadp, gpointer playerp)
{
  SwfdecResourceLoad *load = loadp;

  swfdec_as_object_mark (SWFDEC_AS_OBJECT (load->sandbox));
  if (load->loader)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (load->loader));
  if (load->target_movie)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (load->target_movie));
}

static gboolean
swfdec_resource_create_movie (SwfdecResource *resource, SwfdecResourceLoad *load)
{
  SwfdecPlayer *player;
  SwfdecSpriteMovie *movie;

  if (resource->movie)
    return TRUE;
  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (resource)->context);
  if (load->target_movie) {
    movie = (SwfdecSpriteMovie *) swfdec_movie_resolve (SWFDEC_MOVIE (load->target_movie));
    if (SWFDEC_IS_SPRITE_MOVIE (movie))
      movie = swfdec_resource_replace_movie (movie, resource);
    else
      movie = NULL;
  } else {
    int level = swfdec_player_get_level (player, load->target_string);
    if (level >= 0)
      movie = swfdec_player_create_movie_at_level (player, resource, level);
    else
      movie = NULL;
  }
  if (movie == NULL) {
    SWFDEC_WARNING ("target does not reference a movie, not loading %s", load->url);
    return FALSE;
  }
  /* FIXME: does this belong here? */
  SWFDEC_ACTOR (movie)->focusrect = SWFDEC_FLASH_YES;
  return TRUE;
}

static void
swfdec_resource_do_load (SwfdecPlayer *player, gboolean allowed, gpointer loadp)
{
  SwfdecResourceLoad *load = loadp;
  SwfdecResource *resource;
  SwfdecLoader *loader;
  
  if (!swfdec_as_context_use_mem (SWFDEC_AS_CONTEXT (player), sizeof (SwfdecResource)))
    return;
  resource = g_object_new (SWFDEC_TYPE_RESOURCE, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (resource), SWFDEC_AS_CONTEXT (player), sizeof (SwfdecResource));
  resource->version = 8;
  if (load->loader) {
    resource->clip_loader = load->loader;
    resource->clip_loader_sandbox = load->sandbox;
  }
  resource->target = SWFDEC_MOVIE (load->target_movie);
  resource->sandbox = load->sandbox;
  if (!allowed) {
    SWFDEC_WARNING ("SECURITY: no access to %s from %s",
	load->url, swfdec_url_get_url (load->sandbox->url));
    /* FIXME: is replacing correct? */
    swfdec_resource_emit_error (resource, SWFDEC_AS_STR_IllegalRequest);
    return;
  }

  /* FIXME: load nonetheless, even if there's no movie? */
  if (!swfdec_resource_create_movie (resource, load))
    return;
  loader = swfdec_player_load (player, load->url, load->buffer);
  swfdec_resource_set_loader (resource, loader);
  g_object_unref (loader);
}

static void
swfdec_resource_load_request (gpointer loadp, gpointer playerp)
{
  SwfdecResourceLoad *load = loadp;
  SwfdecPlayer *player = playerp;
  SwfdecURL *url;

  /* empty URL means unload (yay!) */
  if (load->url[0] == '\0') {
    SwfdecSpriteMovie *movie;
      
    movie = load->target_movie ? (SwfdecSpriteMovie *) swfdec_movie_resolve (SWFDEC_MOVIE (load->target_movie)) : NULL;
    if (!SWFDEC_IS_SPRITE_MOVIE (movie)) {
      SWFDEC_DEBUG ("no movie, not unloading");
      return;
    }
    swfdec_resource_replace_movie (movie, SWFDEC_MOVIE (movie)->resource);
    return;
  }

  /* fscommand? */
  if (g_ascii_strncasecmp (load->url, "FSCommand:", 10) == 0) {
    char *command = load->url + 10;
    if (load->target_movie) {
      char *target = swfdec_movie_get_path (SWFDEC_MOVIE (load->target_movie), TRUE);
      SWFDEC_FIXME ("Adobe 9.0.124.0 and later don't emit fscommands here. "
	  "We just do for compatibility reasons with the testsuite.");
      g_signal_emit_by_name (player, "fscommand", command, target);
      g_free (target);
    } else {
      g_signal_emit_by_name (player, "fscommand", command, load->target_string);
    }
    return;
  }

  /* LAUNCH command (aka getURL) */
  if (load->target_string && swfdec_player_get_level (player, load->target_string) < 0) {
    swfdec_player_launch (player, load->url, load->target_string, load->buffer);
    return;
  }

  if (swfdec_url_path_is_relative (load->url)) {
    swfdec_resource_do_load (player, TRUE, load);
    return;
  }
  url = swfdec_player_create_url (player, load->url);
  if (url == NULL) {
    swfdec_resource_do_load (player, FALSE, load);
    return;
  }
  switch (load->sandbox->type) {
    case SWFDEC_SANDBOX_REMOTE:
      swfdec_resource_do_load (player, !swfdec_url_is_local (url), load);
      break;
    case SWFDEC_SANDBOX_LOCAL_NETWORK:
    case SWFDEC_SANDBOX_LOCAL_TRUSTED:
      if (!swfdec_url_is_local (url)) {
	SWFDEC_FIXME ("Adobe claims you need to be allowed by policy files now, "
	    "we don't check that though");
      }
      swfdec_resource_do_load (player, TRUE, load);
      break;
    case SWFDEC_SANDBOX_LOCAL_FILE:
      swfdec_resource_do_load (player, swfdec_url_is_local (url), load);
      break;
    case SWFDEC_SANDBOX_NONE:
    default:
      g_assert_not_reached ();
      break;
  }
  swfdec_url_free (url);
}

/* NB: must be called from a script */
/* FIXME: 6 arguments?! */
static void
swfdec_resource_load_internal (SwfdecPlayer *player,
    SwfdecSpriteMovie *target_movie, const char *target_string,
    const char *url, SwfdecBuffer *buffer, SwfdecMovieClipLoader *loader)
{
  SwfdecResourceLoad *load;

  g_assert (SWFDEC_AS_CONTEXT (player)->frame != NULL);
  load = g_slice_new (SwfdecResourceLoad);

  load->sandbox = SWFDEC_SANDBOX (SWFDEC_AS_CONTEXT (player)->global);
  load->url = g_strdup (url);
  load->target_movie = target_movie;
  load->target_string = g_strdup (target_string);
  load->buffer = buffer;
  load->loader = loader;

  swfdec_player_root (player, load, swfdec_resource_load_mark);
  swfdec_player_request_resource (player, swfdec_resource_load_request, load, swfdec_resource_load_free);
}

gboolean
swfdec_resource_load_movie (SwfdecPlayer *player, const SwfdecAsValue *target, 
    const char *url, SwfdecBuffer *buffer, SwfdecMovieClipLoader *loader)
{
  SwfdecMovie *movie;
  const char *s;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (target != NULL, FALSE);
  g_return_val_if_fail (url != NULL, FALSE);
  g_return_val_if_fail (loader == NULL || SWFDEC_IS_MOVIE_CLIP_LOADER (loader), FALSE);

  if (SWFDEC_AS_VALUE_IS_OBJECT (target)) {
    SwfdecAsObject *object = SWFDEC_AS_VALUE_GET_OBJECT (target);
    if (SWFDEC_IS_SPRITE_MOVIE (object)) {
      swfdec_resource_load_internal (player, SWFDEC_SPRITE_MOVIE (object),
	  NULL, url, buffer, loader);
      return TRUE;
    }
  }

  s = swfdec_as_value_to_string (SWFDEC_AS_CONTEXT (player), target);
  movie = swfdec_player_get_movie_from_string (player, s);
  if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
    swfdec_resource_load_internal (player, SWFDEC_SPRITE_MOVIE (movie),
	NULL, url, buffer, loader);
    return TRUE;
  }
  if (swfdec_player_get_level (player, s) >= 0) {
    swfdec_resource_load_internal (player, NULL, s, url, buffer, NULL);
    return TRUE;
  }
  SWFDEC_WARNING ("%s does not reference a movie, not loading %s", s, url);
  return FALSE;
}

void
swfdec_resource_load (SwfdecPlayer *player, const char *target, 
    const char *url, SwfdecBuffer *buffer)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (target != NULL);
  g_return_if_fail (url != NULL);

  swfdec_resource_load_internal (player, NULL, target, url, buffer, NULL);
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
  resource->clip_loader = NULL;
  resource->clip_loader_sandbox = NULL;
  return TRUE;
}
