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
#include "swfdec_resource_request.h"
#include "swfdec_as_interpret.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"
#include "swfdec_sprite_movie.h"

static void
swfdec_resource_request_free (SwfdecResourceRequest *request)
{
  g_return_if_fail (request != NULL);

  if (request->security)
    g_object_unref (request->security);
  if (request->destroy)
    request->destroy (request->data);
  g_free (request->url);
  if (request->buffer)
    swfdec_buffer_unref (request->buffer);
  g_free (request->command);
  g_free (request->value);
  g_free (request->target);
  g_slice_free (SwfdecResourceRequest, request);
}

typedef struct {
  SwfdecPlayer *		player;
  SwfdecLoaderRequest		request;
  SwfdecBuffer *		buffer;
  SwfdecResourceFunc		callback;
  gpointer			user_data;
} AllowCallbackData;

static void
swfdec_player_request_resource_allow_callback (const SwfdecURL *url,
    gboolean allowed, gpointer data_)
{
  AllowCallbackData *data = data_;
  SwfdecLoader *loader;

  if (!allowed) {
    SWFDEC_ERROR ("not allowing access to %s", swfdec_url_get_url (url));
    loader = NULL;
  } else {
    if (data->buffer) {
      loader = swfdec_loader_load (data->player->resource->loader, url,
	  data->request, (const char *) data->buffer->data,
	  data->buffer->length);
    } else {
      loader = swfdec_loader_load (data->player->resource->loader, url,
	  data->request, NULL, 0);
    }
  }

  data->callback (data->player, loader, data->user_data);

  g_free (data);
}

void
swfdec_player_request_resource_now (SwfdecPlayer *player,
    SwfdecSecurity *security, const char *url, SwfdecLoaderRequest req,
    SwfdecBuffer *buffer, SwfdecResourceFunc callback, gpointer user_data)
{
  SwfdecURL *absolute;
  AllowCallbackData *data;

  // FIXME
  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (SWFDEC_IS_SECURITY (security));
  g_return_if_fail (url != NULL);

  data = g_new (AllowCallbackData, 1);
  data->player = player;
  data->request = req;
  data->buffer = buffer;
  data->callback = callback;
  data->user_data = user_data;

  /* create absolute url first */
  absolute = swfdec_url_new_relative (swfdec_loader_get_url (player->resource->loader), url);

  swfdec_security_allow_url (security, absolute,
      swfdec_player_request_resource_allow_callback, data);

  swfdec_url_free (absolute);
}

static void
swfdec_request_resource_perform_fscommand (SwfdecPlayer *player, SwfdecResourceRequest *request)
{
  g_signal_emit_by_name (player, "fscommand", request->command, request->value);
  swfdec_resource_request_free (request);
}

static void
swfdec_request_resource_perform_load_callback (SwfdecPlayer *player,
    SwfdecLoader *loader, gpointer data)
{
  SwfdecResourceRequest *request = data;

  request->func (player, loader, request->data);
  swfdec_resource_request_free (request);
}

static void
swfdec_request_resource_perform_load (SwfdecPlayer *player, SwfdecResourceRequest *request)
{
  g_assert (player->resource);
  swfdec_player_request_resource_now (player, request->security,
      request->url, request->request, request->buffer,
      swfdec_request_resource_perform_load_callback, request);
}

static void
swfdec_request_resource_perform_unload (SwfdecPlayer *player, SwfdecResourceRequest *request)
{
  request->unload (player, request->target, request->data);
  swfdec_resource_request_free (request);
}

static void
swfdec_request_resource_perform_one (gpointer requestp, gpointer player)
{
  SwfdecResourceRequest *request = requestp;

  switch (request->type) {
    case SWFDEC_RESOURCE_REQUEST_LOAD:
      swfdec_request_resource_perform_load (player, request);
      break;
    case SWFDEC_RESOURCE_REQUEST_FSCOMMAND:
      swfdec_request_resource_perform_fscommand (player, request);
      break;
    case SWFDEC_RESOURCE_REQUEST_UNLOAD:
      swfdec_request_resource_perform_unload (player, request);
      break;
    default:
      g_assert_not_reached ();
      swfdec_resource_request_free (request);
      break;
  }
}

void
swfdec_player_resource_request_perform (SwfdecPlayer *player)
{
  GSList *list;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  list = player->resource_requests;
  player->resource_requests = NULL;
  g_slist_foreach (list, swfdec_request_resource_perform_one, player);
  g_slist_free (list);
}

void
swfdec_player_request_resource (SwfdecPlayer *player, SwfdecSecurity *security,
    const char *url, SwfdecLoaderRequest req, SwfdecBuffer *buffer, 
    SwfdecResourceFunc func, gpointer data, GDestroyNotify destroy)
{
  SwfdecResourceRequest *request;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (SWFDEC_IS_SECURITY (security));
  g_return_if_fail (url != NULL);
  g_return_if_fail (func != NULL);

  request = g_slice_new0 (SwfdecResourceRequest);
  request->type = SWFDEC_RESOURCE_REQUEST_LOAD;
  request->security = g_object_ref (security);
  request->url = g_strdup (url);
  request->request = req;
  if (buffer)
    request->buffer = swfdec_buffer_ref (buffer);
  request->func = func;
  request->destroy = destroy;
  request->data = data;

  player->resource_requests = g_slist_append (player->resource_requests, request);
}

static gboolean
is_ascii (const char *s)
{
  while (*s) {
    if (*s & 0x80)
      return FALSE;
    s++;
  }
  return TRUE;
}

/**
 * swfdec_player_request_fscommand:
 * @player: a #SwfdecPlayer
 * @command: the command to parse
 * @value: the value passed to the command
 *
 * Checks if @command is an FSCommand and if so, queues emission of the 
 * SwfdecPlayer::fscommand signal. 
 *
 * Returns: %TRUE if an fscommand was found and the signal emitted, %FALSE 
 *          otherwise.
 **/
gboolean
swfdec_player_request_fscommand (SwfdecPlayer *player, const char *command,
    const char *value)
{
  SwfdecResourceRequest *request;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (command != NULL, FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  if (g_ascii_strncasecmp (command, "FSCommand:", 10) != 0)
    return FALSE;

  command += 10;
  if (!is_ascii (command)) {
    SWFDEC_ERROR ("command \"%s\" are not ascii, skipping fscommand", command);
    return TRUE;
  }
  request = g_slice_new0 (SwfdecResourceRequest);
  request->type = SWFDEC_RESOURCE_REQUEST_FSCOMMAND;
  request->command = g_ascii_strdown (command, -1);
  request->value = g_strdup (value);

  player->resource_requests = g_slist_append (player->resource_requests, request);
  return TRUE;
}

void
swfdec_player_request_unload (SwfdecPlayer *player, const char *target,
    SwfdecResourceUnloadFunc func, gpointer data, GDestroyNotify destroy)
{
  SwfdecResourceRequest *request;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (target != NULL);

  request = g_slice_new0 (SwfdecResourceRequest);
  request->type = SWFDEC_RESOURCE_REQUEST_UNLOAD;
  request->target = g_strdup (target);
  request->unload = func;
  request->data = data;
  request->destroy = destroy;

  player->resource_requests = g_slist_append (player->resource_requests, request);
}

void
swfdec_player_resource_request_init (SwfdecPlayer *player)
{
  /* empty */
}

void
swfdec_player_resource_request_finish (SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  g_slist_foreach (player->resource_requests, (GFunc) swfdec_resource_request_free, NULL);
  g_slist_free (player->resource_requests);
  player->resource_requests = NULL;
}

