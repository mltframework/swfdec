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

#include "swfdec_policy_file.h"
#include "swfdec_flash_security.h"
#include "swfdec_resource.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_xml.h"
#include "swfdec_xml_node.h"

/*** PARSING THE FILE ***/

static void
swfdec_policy_file_parse (SwfdecPolicyFile *file, const char *text)
{
  SwfdecXml *xml;
  gint32 i, j;

  g_return_if_fail (SWFDEC_IS_POLICY_FILE (file));
  g_return_if_fail (text != NULL);

  xml = swfdec_xml_new_no_properties (SWFDEC_AS_CONTEXT (file->player), text, TRUE);

  if (xml == NULL) {
    SWFDEC_ERROR ("failed to create an XML object for crossdomain policy");
    return;
  }

  if (SWFDEC_XML_NODE (xml)->type != SWFDEC_XML_NODE_ELEMENT) {
    SWFDEC_LOG ("empty crossdomain policy file");
    return;
  }

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
      GPatternSpec *pattern;
      char *value_lower;

      if (node_aaf->type != SWFDEC_XML_NODE_ELEMENT)
	continue;

      if (g_ascii_strcasecmp (node_aaf->name, "allow-access-from") != 0)
	continue;

      // FIXME: secure attribute?

      value = swfdec_xml_node_get_attribute (node_aaf, SWFDEC_AS_STR_domain);
      if (value == NULL)
	continue;

      if (strchr (value, '?') != NULL) {
	SWFDEC_WARNING ("'?' in allowed domain attribute for %s", value);
	continue;
      }

      value_lower = g_ascii_strdown (value, -1);
      pattern = g_pattern_spec_new (value_lower);
      g_free (value_lower);

      file->allowed_hosts = g_slist_prepend (file->allowed_hosts, pattern);
    }
  }
}

/*** SWFDEC_LOADER_TARGET ***/

static SwfdecPlayer *
swfdec_policy_file_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_POLICY_FILE (target)->player;
}

static void
swfdec_policy_file_target_error (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecPolicyFile *file = SWFDEC_POLICY_FILE (target);

  swfdec_loader_set_target (loader, NULL);
  file->stream = NULL;
}

static void
swfdec_policy_file_target_eof (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecPolicyFile *file = SWFDEC_POLICY_FILE (target);
  char *text;

  swfdec_loader_set_target (loader, NULL);
  file->stream = NULL;
  text = swfdec_loader_get_text (loader, 8);

  if (text == NULL) {
    SWFDEC_ERROR ("couldn't get text from crossdomain policy file %s", 
	swfdec_url_get_url (file->load_url));
    return;
  }

  swfdec_policy_file_parse (file, text);
  g_free (text);
}

static void
swfdec_policy_file_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_policy_file_target_get_player;
  iface->eof = swfdec_policy_file_target_eof;
  iface->error = swfdec_policy_file_target_error;
}

/*** SWFDEC_POLICY_FILE ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecPolicyFile, swfdec_policy_file, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_policy_file_loader_target_init))

static void
swfdec_policy_file_dispose (GObject *object)
{
  SwfdecPolicyFile *file = SWFDEC_POLICY_FILE (object);

  if (file->stream) {
    swfdec_loader_set_target (file->stream, NULL);
    g_object_unref (file->stream);
    file->stream = NULL;
  }
  swfdec_url_free (file->load_url);
  swfdec_url_free (file->url);

  G_OBJECT_CLASS (swfdec_policy_file_parent_class)->dispose (object);
}

static void
swfdec_policy_file_class_init (SwfdecPolicyFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_policy_file_dispose;
}

static void
swfdec_policy_file_init (SwfdecPolicyFile *policy_file)
{
}

SwfdecPolicyFile *
swfdec_policy_file_new (SwfdecPlayer *player, const SwfdecURL *url)
{
  SwfdecPolicyFile *file;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  file = g_object_new (SWFDEC_TYPE_POLICY_FILE, NULL);
  file->player = player;
  file->load_url = swfdec_url_copy (url);
  file->url = swfdec_url_new_parent (url);
  if (swfdec_url_has_protocol (url, "xmlsocket")) {
    SWFDEC_FIXME ("implement xmlsocket: protocol");
  } else {
    file->stream = swfdec_loader_load (player->priv->resource->loader, url, 
	SWFDEC_LOADER_REQUEST_DEFAULT, NULL, 0);
    swfdec_loader_set_target (file->stream, SWFDEC_LOADER_TARGET (file));
  }
  player->priv->policy_files = g_slist_prepend (player->priv->policy_files, file);

  return file;
}

gboolean
swfdec_policy_file_is_loading (SwfdecPolicyFile *file)
{
  g_return_val_if_fail (SWFDEC_IS_POLICY_FILE (file), FALSE);

  return file->stream != NULL;
}

gboolean
swfdec_policy_file_allow (SwfdecPolicyFile *file, const SwfdecURL *url)
{
  GSList *walk;
  gsize len;
  char *emantsoh;
  const char *hostname;

  g_return_val_if_fail (SWFDEC_IS_POLICY_FILE (file), FALSE);
  g_return_val_if_fail (url != NULL, FALSE);

  hostname = swfdec_url_get_host (url);
  /* This is a hack that simplifies the following code. As the pattern can not
   * contain any ?, the only pattern that matches the string "?" is the pattern
   * "*" 
   */
  if (hostname == NULL)
    hostname = "?";
  len = strlen (hostname);
  emantsoh = g_utf8_strreverse (emantsoh, len);
  for (walk = file->allowed_hosts; walk; walk = walk->next) {
    GPatternSpec *pattern = walk->data;
    if (g_pattern_match (pattern, len, hostname, emantsoh)) {
      g_free (emantsoh);
      return TRUE;
    }
  }
  g_free (emantsoh);
  return FALSE;
}

void
swfdec_player_load_policy_file (SwfdecPlayer *player, const SwfdecURL *url)
{
  SWFDEC_FIXME ("implement");
}

void
swfdec_player_check_policy_files (SwfdecPlayer *player, 
    SwfdecPolicyFileFunc func, gpointer data)
{
  SWFDEC_FIXME ("implement");
}
