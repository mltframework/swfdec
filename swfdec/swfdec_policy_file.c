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
#include "swfdec_resource.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_xml.h"
#include "swfdec_xml_node.h"

typedef struct _SwfdecPolicyFileRequest SwfdecPolicyFileRequest;
struct _SwfdecPolicyFileRequest {
  SwfdecURL *	  	url;		/* URL we are supposed to check */
  SwfdecPolicyFunc	func;		/* function to call when we know if access is (not) allowed */
  gpointer		data;		/* data to pass to func */
};

static void
swfdec_policy_file_request_free (SwfdecPolicyFileRequest *request)
{
  swfdec_url_free (request->url);
  g_slice_free (SwfdecPolicyFileRequest, request);
}

/*** PARSING THE FILE ***/

static void
swfdec_policy_file_parse (SwfdecPolicyFile *file, const char *text)
{
  SwfdecXml *xml;
  gint32 i, j;

  g_return_if_fail (SWFDEC_IS_POLICY_FILE (file));
  g_return_if_fail (text != NULL);

  /* FIXME: the sandboxes are a HACK */
  swfdec_sandbox_use (SWFDEC_MOVIE (file->player->priv->roots->data)->resource->sandbox);
  xml = swfdec_xml_new_no_properties (SWFDEC_AS_CONTEXT (file->player), text, TRUE);
  swfdec_sandbox_unuse (SWFDEC_MOVIE (file->player->priv->roots->data)->resource->sandbox);

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

/*** SWFDEC_STREAM_TARGET ***/

static SwfdecPlayer *
swfdec_policy_file_target_get_player (SwfdecStreamTarget *target)
{
  return SWFDEC_POLICY_FILE (target)->player;
}

static void
swfdec_policy_file_finished_loading (SwfdecPolicyFile *file, const char *text)
{
  SwfdecPlayerPrivate *priv;
  SwfdecPolicyFile *next;
  GList *link;

  swfdec_stream_set_target (file->stream, NULL);
  file->stream = NULL;

  if (text)
    swfdec_policy_file_parse (file, text);

  priv = file->player->priv;
  link = g_list_find (priv->loading_policy_files, file);
  next = link->next ? link->next->data : NULL;
  priv->loading_policy_files = g_list_delete_link (priv->loading_policy_files, link);
  priv->policy_files = g_slist_prepend (priv->policy_files, file);
  if (next) {
    next->requests = g_slist_concat (next->requests, file->requests);
  } else {
    GSList *walk;

    for (walk = file->requests; walk; walk = walk->next) {
      SwfdecPolicyFileRequest *request = walk->data;
      gboolean allow = swfdec_player_allow_now (file->player, request->url);
      request->func (file->player, allow, request->data);
      swfdec_policy_file_request_free (request);
    }
    g_slist_free (file->requests);
  }
  file->requests = NULL;
}

static void
swfdec_policy_file_target_open (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  if (SWFDEC_IS_SOCKET (stream)) {
    SwfdecBuffer *buffer = swfdec_buffer_new_static (
	"<policy-file-request/>", 23);
    swfdec_socket_send (SWFDEC_SOCKET (stream), buffer);
  }
}

static void
swfdec_policy_file_target_error (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecPolicyFile *file = SWFDEC_POLICY_FILE (target);

  swfdec_policy_file_finished_loading (file, NULL);
}

static void
swfdec_policy_file_target_close (SwfdecStreamTarget *target,
    SwfdecStream *stream)
{
  SwfdecPolicyFile *file = SWFDEC_POLICY_FILE (target);
  char *text;

  text = swfdec_buffer_queue_pull_text (swfdec_stream_get_queue (stream), 8);

  if (text == NULL) {
    SWFDEC_ERROR ("couldn't get text from crossdomain policy file %s", 
	swfdec_url_get_url (file->load_url));
  }
  swfdec_policy_file_finished_loading (file, text);
  g_free (text);
}

static void
swfdec_policy_file_stream_target_init (SwfdecStreamTargetInterface *iface)
{
  iface->get_player = swfdec_policy_file_target_get_player;
  iface->open = swfdec_policy_file_target_open;
  iface->close = swfdec_policy_file_target_close;
  iface->error = swfdec_policy_file_target_error;
}

/*** SWFDEC_POLICY_FILE ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecPolicyFile, swfdec_policy_file, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_STREAM_TARGET, swfdec_policy_file_stream_target_init))

static void
swfdec_policy_file_dispose (GObject *object)
{
  SwfdecPolicyFile *file = SWFDEC_POLICY_FILE (object);

  if (file->stream) {
    swfdec_stream_set_target (file->stream, NULL);
    g_object_unref (file->stream);
    file->stream = NULL;
    g_slist_foreach (file->requests, (GFunc) swfdec_policy_file_request_free, NULL);
    g_slist_free (file->requests);
    file->requests = NULL;
  } else {
    g_assert (file->requests == NULL);
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
    file->stream = SWFDEC_STREAM (swfdec_player_create_socket (player, 
	swfdec_url_get_host (url), swfdec_url_get_port (url)));
  } else {
    file->stream = SWFDEC_STREAM (swfdec_player_load (player,
	  swfdec_url_get_url (url), SWFDEC_LOADER_REQUEST_DEFAULT, NULL));
  }
  swfdec_stream_set_target (file->stream, SWFDEC_STREAM_TARGET (file));
  player->priv->loading_policy_files = 
    g_list_prepend (player->priv->loading_policy_files, file);

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
  emantsoh = g_utf8_strreverse (hostname, len);
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

/*** PLAYER API ***/

gboolean
swfdec_player_allow_now (SwfdecPlayer *player, const SwfdecURL *url)
{
  GSList *walk;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), FALSE);
  g_return_val_if_fail (url != NULL, FALSE);

  for (walk = player->priv->policy_files; walk; walk = walk->next) {
    if (swfdec_policy_file_allow (walk->data, url))
      return TRUE;
  }
  return FALSE;
}

void
swfdec_player_allow_or_load (SwfdecPlayer *player, const SwfdecURL *url, 
    const SwfdecURL *load_url, SwfdecPolicyFunc func, gpointer data)
{
  SwfdecPlayerPrivate *priv;
  SwfdecPolicyFileRequest *request;
  SwfdecPolicyFile *file;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (url != NULL);
  g_return_if_fail (func);

  if (swfdec_player_allow_now (player, url)) {
    func (player, TRUE, data);
    return;
  }
  if (load_url)
    swfdec_policy_file_new (player, load_url);

  priv = player->priv;
  if (priv->loading_policy_files == NULL) {
    func (player, FALSE, data);
    return;
  }
  request = g_slice_new (SwfdecPolicyFileRequest);
  request->url = swfdec_url_copy (url);
  request->func = func;
  request->data = data;

  file = priv->loading_policy_files->data;
  file->requests = g_slist_append (file->requests, request);
}

