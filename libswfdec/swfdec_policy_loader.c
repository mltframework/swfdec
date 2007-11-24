/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_policy_loader.h"
#include "swfdec_flash_security.h"
#include "swfdec_resource.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource_request.h"

/*** SWFDEC_LOADER_TARGET ***/

static SwfdecPlayer *
swfdec_policy_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_POLICY_LOADER (target)->sec->player;
}

static void
swfdec_policy_loader_target_error (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecPolicyLoader *policy_loader = SWFDEC_POLICY_LOADER (target);

  policy_loader->func (policy_loader, FALSE);
}

typedef struct {
  const char		*name;
  guint			length;
  guchar		data[4];
} ByteOrderMark;

static ByteOrderMark boms[] = {
  { "UTF-8", 3, {0xEF, 0xBB, 0xBF, 0} },
  { "UTF-16BE", 2, {0xFE, 0xFF, 0, 0} },
  { "UTF-16LE", 2, {0xFF, 0xFE, 0, 0} },
  { "UTF-8", 0, {0, 0, 0, 0} }
};

static void
swfdec_policy_loader_target_eof (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecPolicyLoader *policy_loader = SWFDEC_POLICY_LOADER (target);
  char *text, *xml;
  guint size;

  /* get the text from the loader */
  // TODO: Get rid of extra alloc when getting UTF-8 with bom
  size = swfdec_buffer_queue_get_depth (loader->queue);
  text = g_try_malloc (size + 1);
  if (text) {
    SwfdecBuffer *buffer;
    guint i = 0, j;
    while ((buffer = swfdec_buffer_queue_pull_buffer (loader->queue))) {
      memcpy (text + i, buffer->data, buffer->length);
      i += buffer->length;
      swfdec_buffer_unref (buffer);
    }
    g_assert (i == size);
    text[size] = '\0';

    for (i = 0; boms[i].length > 0; i++) {
      if (size < boms[i].length)
	continue;

      for (j = 0; j < boms[i].length; j++) {
	if ((guchar)text[j] != boms[i].data[j])
	  break;
      }
      if (j == boms[i].length)
	break;
    }

    if (!strcmp (boms[i].name, "UTF-8")) {
      if (!g_utf8_validate (text + boms[i].length, size - boms[i].length,
	    NULL)) {
	SWFDEC_ERROR ("downloaded data is not valid UTF-8");
	g_free (text);
	text = NULL;
	xml = NULL;
      } else {
	if (boms[i].length == 0) {
	  xml = text;
	  text = NULL;
	} else {
	  xml = g_strdup (text + boms[i].length);
	  g_free (text);
	  text = NULL;
	}
      }
    } else {
      xml = g_convert (text + boms[i].length,
	  size - boms[i].length, "UTF-8", boms[i].name, NULL, NULL, NULL);
      if (xml == NULL)
	SWFDEC_ERROR ("downloaded data is not valid %s", boms[i].name);
      g_free (text);
      text = NULL;
    }
  } else {
    SWFDEC_ERROR ("not enough memory to copy %u bytes", size);
    xml = NULL;
  }

  // TODO
  policy_loader->func (policy_loader, TRUE);

  if (xml != NULL)
    g_free (xml);
}

static void
swfdec_policy_loader_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_policy_loader_target_get_player;
  iface->eof = swfdec_policy_loader_target_eof;
  iface->error = swfdec_policy_loader_target_error;
}

/*** SWFDEC_POLICY_LOADER ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecPolicyLoader, swfdec_policy_loader, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_policy_loader_loader_target_init))

static void
swfdec_policy_loader_dispose (GObject *object)
{
  SwfdecPolicyLoader *policy_loader = SWFDEC_POLICY_LOADER (object);

  g_assert (policy_loader->loader);
  swfdec_loader_set_target (policy_loader->loader, NULL);
  g_object_unref (policy_loader->loader);
  policy_loader->loader = NULL;

  g_assert (policy_loader->host);
  g_free (policy_loader->host);

  G_OBJECT_CLASS (swfdec_policy_loader_parent_class)->dispose (object);
}

static void
swfdec_policy_loader_class_init (SwfdecPolicyLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_policy_loader_dispose;
}

static void
swfdec_policy_loader_init (SwfdecPolicyLoader *policy_loader)
{
}

SwfdecPolicyLoader *
swfdec_policy_loader_new (SwfdecFlashSecurity *sec, const char *host,
    SwfdecPolicyLoaderFunc func)
{
  SwfdecPolicyLoader *policy_loader;
  SwfdecURL *url;
  char *url_str;

  g_return_val_if_fail (SWFDEC_IS_FLASH_SECURITY (sec), NULL);
  g_return_val_if_fail (host != NULL, NULL);
  g_return_val_if_fail (func != NULL, NULL);

  policy_loader = SWFDEC_POLICY_LOADER (g_object_new (
	SWFDEC_TYPE_POLICY_LOADER, NULL));

  url_str = g_strdup_printf ("http://%s/crossdomain.xml", host);
  url = swfdec_url_new (url_str);
  g_free (url_str);
  policy_loader->loader = swfdec_loader_load (sec->player->resource->loader,
      url, SWFDEC_LOADER_REQUEST_DEFAULT, NULL, 0);
  swfdec_url_free (url);

  if (!policy_loader->loader) {
    g_free (policy_loader);
    return NULL;
  }

  policy_loader->sec = sec;
  policy_loader->host = g_strdup (host);
  policy_loader->func = func;

  swfdec_loader_set_target (policy_loader->loader,
      SWFDEC_LOADER_TARGET (policy_loader));
  swfdec_loader_set_data_type (policy_loader->loader, SWFDEC_LOADER_DATA_TEXT);

  return policy_loader;
}
