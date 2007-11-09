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

#include "swfdec_resource_request.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"

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
  g_slice_free (SwfdecResourceRequest, request);
}

SwfdecLoader *
swfdec_player_request_resource_now (SwfdecPlayer *player, SwfdecSecurity *security,
    const char *url, SwfdecLoaderRequest req, SwfdecBuffer *buffer)
{
  SwfdecLoader *loader;
  SwfdecURL *absolute;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (SWFDEC_IS_SECURITY (security), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  /* create absolute url first */
  absolute = swfdec_url_new_relative (swfdec_loader_get_url (player->resource->loader), url);
  if (!swfdec_security_allow_url (security, absolute)) {
    /* FIXME: Need to load policy file from given URL */
    SWFDEC_ERROR ("not allowing access to %s", swfdec_url_get_url (absolute));
    loader = NULL;
  } else {
    if (buffer) {
      loader = swfdec_loader_load (player->resource->loader, absolute, req, 
	  (const char *) buffer->data, buffer->length);
    } else {
      loader = swfdec_loader_load (player->resource->loader, absolute, req, NULL, 0);
    }
  }
  swfdec_url_free (absolute);
  return loader;
}

static void
swfdec_request_resource_perform_fscommand (SwfdecPlayer *player, SwfdecResourceRequest *request)
{
  g_signal_emit_by_name (player, "fscommand", request->command, request->value);
}

static void
swfdec_request_resource_perform_load(SwfdecPlayer *player, SwfdecResourceRequest *request)
{
  SwfdecLoader *loader;

  g_assert (player->resource);
  if (request->url[0] == '\0') {
    /* special case for unloadMovie */
    loader = NULL;
  } else {
    loader = swfdec_player_request_resource_now (player, request->security, 
	request->url, request->request, request->buffer);
  }
  request->func (player, loader, request->data);
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
    default:
      g_assert_not_reached ();
      break;
  }
  swfdec_resource_request_free (request);
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

