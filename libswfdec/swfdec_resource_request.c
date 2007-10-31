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

  g_object_unref (request->security);
  if (request->destroy)
    request->destroy (request->data);
  g_free (request->url);
  if (request->buffer)
    swfdec_buffer_unref (request->buffer);
  g_slice_free (SwfdecResourceRequest, request);
}

static void
swfdec_request_resource_perform_one (gpointer requestp, gpointer playerp)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (playerp);
  SwfdecResourceRequest *request = requestp;
  SwfdecLoader *loader;
  SwfdecURL *url;

  g_assert (player->resource);
  /* create absolute url first */
  url = swfdec_url_new_relative (swfdec_loader_get_url (player->resource->loader), request->url);
  if (!swfdec_security_allow_url (request->security, url)) {
    /* FIXME: Need to load policy file from given URL */
    SWFDEC_ERROR ("not allowing access to %s", swfdec_url_get_url (url));
    loader = NULL;
  } else {
    if (request->buffer) {
      loader = swfdec_loader_load (player->resource->loader, url, request->request, 
	  (const char *) request->buffer->data, request->buffer->length);
    } else {
      loader = swfdec_loader_load (player->resource->loader, url, request->request, NULL, 0);
    }
  }
  swfdec_url_free (url);
  request->func (player, loader, request->data);
  swfdec_resource_request_free (request);
}

static void
swfdec_request_resource_perform (gpointer playerp, gpointer unused)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (playerp);

  g_slist_foreach (player->resource_requests, swfdec_request_resource_perform_one, player);
  g_slist_free (player->resource_requests);
  player->resource_requests = NULL;
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
  request->security = g_object_ref (security);
  request->url = g_strdup (url);
  request->request = req;
  if (buffer)
    request->buffer = swfdec_buffer_ref (buffer);
  request->func = func;
  request->destroy = destroy;
  request->data = data;

  if (player->resource_requests == NULL) {
    swfdec_player_add_external_action (player, player, swfdec_request_resource_perform, NULL);
  }
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

