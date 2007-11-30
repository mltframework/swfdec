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
#include "swfdec_xml.h"
#include "swfdec_xml_node.h"

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

static gboolean
swfdec_policy_loader_check (SwfdecAsContext *context, const char *text,
    const char *host)
{
  SwfdecXml *xml;
  gint32 i, j;
  char *host_lower;

  xml = swfdec_xml_new_no_properties (context, text, TRUE);

  if (xml == NULL) {
    SWFDEC_ERROR ("failed to create an XML object for crossdomain policy");
    return FALSE;
  }

  if (SWFDEC_XML_NODE (xml)->type != SWFDEC_XML_NODE_ELEMENT) {
    SWFDEC_LOG ("empty crossdomain policy file");
    return FALSE;
  }

  host_lower = g_ascii_strdown (host, -1);

  for (i = 0; i < swfdec_xml_node_num_children (SWFDEC_XML_NODE (xml)); i++) {
    SwfdecXmlNode *node_cdp =
      swfdec_xml_node_get_child (SWFDEC_XML_NODE (xml), i);

    if (node_cdp->type != SWFDEC_XML_NODE_ELEMENT)
      continue;

    if (g_ascii_strcasecmp (node_cdp->name, "cross-domain-policy") != 0)
      continue;

    for (j = 0; j < swfdec_xml_node_num_children (node_cdp); j++) {
      SwfdecXmlNode *node_aaf = swfdec_xml_node_get_child (node_cdp, j);
      const char *value;

      if (node_aaf->type != SWFDEC_XML_NODE_ELEMENT)
	continue;

      if (g_ascii_strcasecmp (node_aaf->name, "allow-access-from") != 0)
	continue;

      // FIXME: secure attribute?

      value = swfdec_xml_node_get_attribute (node_aaf, SWFDEC_AS_STR_domain);
      if (value != NULL) {
	GPatternSpec *pattern;
	char *value_lower;

	// GPatternSpec uses ? as a wildcard character, but we won't
	// And there can't be a host that has ? character
	if (strchr (value, '?') != NULL)
	  continue;

	value_lower = g_ascii_strdown (value, -1);
	pattern = g_pattern_spec_new (value_lower);
	g_free (value_lower);

	if (g_pattern_match_string (pattern, host_lower)) {
	  g_free (host_lower);
	  g_pattern_spec_free (pattern);
	  return TRUE;
	}

	g_pattern_spec_free (pattern);
      }
    }
  }

  g_free (host_lower);

  return FALSE;
}

static void
swfdec_policy_loader_target_eof (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecPolicyLoader *policy_loader = SWFDEC_POLICY_LOADER (target);
  gboolean allow;
  char *text;

  text = swfdec_loader_get_text (policy_loader->loader, 8);

  if (text == NULL) {
    SWFDEC_ERROR ("couldn't get text from crossdomain policy");
    allow = FALSE;
  } else {
    allow = swfdec_policy_loader_check (
	SWFDEC_AS_CONTEXT (policy_loader->sec->player), text,
	swfdec_url_get_host (policy_loader->sec->url));
  }

  g_free (text);

  SWFDEC_LOG ("crossdomain policy %s access from %s to %s",
      (allow ? "allows" : "doesn't allow"),
      swfdec_url_get_host (policy_loader->sec->url), policy_loader->host);

  policy_loader->func (policy_loader, allow);
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

void
swfdec_policy_loader_free (SwfdecPolicyLoader *policy_loader)
{
  g_object_unref (policy_loader);
}
