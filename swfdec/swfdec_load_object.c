/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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
#include "swfdec_load_object.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_loader_internal.h"
#include "swfdec_stream_target.h"
#include "swfdec_player_internal.h"

/*** SWFDEC_STREAM_TARGET ***/

static SwfdecPlayer *
swfdec_load_object_stream_target_get_player (SwfdecStreamTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_LOAD_OBJECT (target)->target->context);
}

static gboolean
swfdec_load_object_stream_target_parse (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecLoader *loader = SWFDEC_LOADER (stream);
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);

  if (load_object->progress != NULL) {
    swfdec_sandbox_use (load_object->sandbox);
    load_object->progress (load_object->target,
	swfdec_loader_get_loaded (loader), swfdec_loader_get_size (loader));
    swfdec_sandbox_unuse (load_object->sandbox);
  }
  return FALSE;
}

static void
swfdec_load_object_stream_target_error (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecLoader *loader = SWFDEC_LOADER (stream);
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);

  /* break reference to the loader */
  swfdec_stream_set_target (SWFDEC_STREAM (loader), NULL);
  load_object->loader = NULL;
  g_object_unref (loader);

  /* call finish */
  swfdec_sandbox_use (load_object->sandbox);
  load_object->finish (load_object->target, NULL);
  swfdec_sandbox_unuse (load_object->sandbox);

  /* unroot */
  swfdec_player_unroot (SWFDEC_PLAYER (
	SWFDEC_AS_OBJECT (load_object->sandbox)->context), load_object);
}

static void
swfdec_load_object_stream_target_close (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);
  char *text;

  // get text
  text = swfdec_buffer_queue_pull_text (swfdec_stream_get_queue (stream), load_object->version);

  /* break reference to the loader */
  swfdec_stream_set_target (stream, NULL);
  load_object->loader = NULL;
  g_object_unref (stream);

  /* call finish */
  swfdec_sandbox_use (load_object->sandbox);
  if (text != NULL) {
    load_object->finish (load_object->target, 
	swfdec_as_context_give_string (load_object->target->context, text));
  } else {
    load_object->finish (load_object->target, SWFDEC_AS_STR_EMPTY);
  }
  swfdec_sandbox_unuse (load_object->sandbox);

  /* unroot */
  swfdec_player_unroot (SWFDEC_PLAYER (
	SWFDEC_AS_OBJECT (load_object->sandbox)->context), load_object);
}

static void
swfdec_load_object_stream_target_init (SwfdecStreamTargetInterface *iface)
{
  iface->get_player = swfdec_load_object_stream_target_get_player;
  iface->parse = swfdec_load_object_stream_target_parse;
  iface->close = swfdec_load_object_stream_target_close;
  iface->error = swfdec_load_object_stream_target_error;
}

/*** SWFDEC_LOAD_OBJECT ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecLoadObject, swfdec_load_object, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_STREAM_TARGET, swfdec_load_object_stream_target_init))

static void
swfdec_load_object_dispose (GObject *object)
{
  SwfdecLoadObject *load = SWFDEC_LOAD_OBJECT (object);
  guint i;

  if (load->loader) {
    swfdec_stream_set_target (SWFDEC_STREAM (load->loader), NULL);
    g_object_unref (load->loader);
    load->loader = NULL;
  }
  if (load->buffer) {
    swfdec_buffer_unref (load->buffer);
    load->buffer = NULL;
  }

  for (i = 0; i < load->header_count; i++) {
    g_free (load->header_names[i]);
    g_free (load->header_values[i]);
  }
  g_free (load->header_names);
  g_free (load->header_values);

  G_OBJECT_CLASS (swfdec_load_object_parent_class)->dispose (object);
}

static void
swfdec_load_object_class_init (SwfdecLoadObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_load_object_dispose;
}

static void
swfdec_load_object_init (SwfdecLoadObject *load_object)
{
}

static void
swfdec_load_object_load (SwfdecPlayer *player, gboolean allow, gpointer obj)
{
  SwfdecLoadObject *load = SWFDEC_LOAD_OBJECT (obj);

  if (!allow) {
    SWFDEC_WARNING ("SECURITY: no access to %s from %s",
	load->url, swfdec_url_get_url (load->sandbox->url));

    /* call finish */
    swfdec_sandbox_use (load->sandbox);
    load->finish (load->target, NULL);
    swfdec_sandbox_unuse (load->sandbox);

    /* unroot */
    swfdec_player_unroot (player, load);
    return;
  }

  load->loader = swfdec_player_load_with_headers (player, load->url,
      load->buffer, load->header_count, (const char **)load->header_names,
      (const char **)load->header_values);

  swfdec_stream_set_target (SWFDEC_STREAM (load->loader), SWFDEC_STREAM_TARGET (load));
  swfdec_loader_set_data_type (load->loader, SWFDEC_LOADER_DATA_TEXT);
}

/* perform security checks */
static void
swfdec_load_object_request (gpointer objectp, gpointer playerp)
{
  SwfdecLoadObject *load = SWFDEC_LOAD_OBJECT (objectp);
  SwfdecPlayer *player = SWFDEC_PLAYER (playerp);
  SwfdecURL *url;

  if (swfdec_url_path_is_relative (load->url)) {
    swfdec_load_object_load (player, 
	load->sandbox->type != SWFDEC_SANDBOX_LOCAL_NETWORK, load);
    return;
  }
  url = swfdec_player_create_url (player, load->url);
  if (url == NULL) {
    swfdec_load_object_load (player, FALSE, load);
    return;
  }
  switch (load->sandbox->type) {
    case SWFDEC_SANDBOX_REMOTE:
      if (swfdec_url_host_equal(url, swfdec_player_get_url (player))) {
	swfdec_load_object_load (player, TRUE, load);
	break;
      }
      /* fall through */
    case SWFDEC_SANDBOX_LOCAL_NETWORK:
    case SWFDEC_SANDBOX_LOCAL_TRUSTED:
      if (swfdec_url_is_local (url)) {
	swfdec_load_object_load (player, load->sandbox->type == SWFDEC_SANDBOX_LOCAL_TRUSTED, load);
      } else {
	SwfdecURL *load_url = swfdec_url_new_components (
	    swfdec_url_get_protocol (url), swfdec_url_get_host (url), 
	    swfdec_url_get_port (url), "crossdomain.xml", NULL);
	swfdec_player_allow_or_load (player, url, load_url,
	  swfdec_load_object_load, load);
	swfdec_url_free (load_url);
      }
      break;
    case SWFDEC_SANDBOX_LOCAL_FILE:
      swfdec_load_object_load (player, swfdec_url_is_local (url), load);
      break;
    case SWFDEC_SANDBOX_NONE:
    default:
      g_assert_not_reached ();
      break;
  }

  swfdec_url_free (url);
}

static void
swfdec_load_object_mark (gpointer object, gpointer player)
{
  SwfdecLoadObject *load = object;

  swfdec_as_object_mark (SWFDEC_AS_OBJECT (load->sandbox));
  if (load->url)
    swfdec_as_string_mark (load->url);
  swfdec_as_object_mark (load->target);
}

void
swfdec_load_object_create (SwfdecAsObject *target, const char *url,
    SwfdecBuffer *data, guint header_count, char **header_names,
    char **header_values, SwfdecLoadObjectProgress progress,
    SwfdecLoadObjectFinish finish)
{
  SwfdecPlayer *player;
  SwfdecLoadObject *load;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (target));
  g_return_if_fail (url != NULL);
  g_return_if_fail (header_count == 0 || header_names != NULL);
  g_return_if_fail (header_count == 0 || header_values != NULL);
  g_return_if_fail (finish != NULL);

  player = SWFDEC_PLAYER (target->context);
  load = g_object_new (SWFDEC_TYPE_LOAD_OBJECT, NULL);
  swfdec_player_root_full (player, load, swfdec_load_object_mark, g_object_unref);

  load->target = target;
  load->url = url;
  load->buffer = data;
  load->header_count = header_count;
  load->header_names = header_names;
  load->header_values = header_values;
  load->progress = progress;
  load->finish = finish;
  /* get the current security */
  g_assert (SWFDEC_AS_CONTEXT (player)->frame);
  load->sandbox = SWFDEC_SANDBOX (SWFDEC_AS_CONTEXT (player)->global);
  load->version = SWFDEC_AS_CONTEXT (player)->version;
  swfdec_player_request_resource (player, swfdec_load_object_request, load, NULL);
}
