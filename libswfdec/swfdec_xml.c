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
#include "swfdec_xml.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"

/*** SWFDEC_LOADER_TARGET ***/

static SwfdecPlayer *
swfdec_xml_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_AS_OBJECT (target)->context);
}

static void
swfdec_xml_ondata (SwfdecXml *xml)
{
  SwfdecAsValue val;

  if (xml->text) {
    SWFDEC_AS_VALUE_SET_STRING (&val,
	swfdec_as_context_get_string (SWFDEC_AS_OBJECT (xml)->context, xml->text));
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  }
  swfdec_as_object_call (SWFDEC_AS_OBJECT (xml), SWFDEC_AS_STR_onData, 1, &val, NULL);
}

static void
swfdec_xml_loader_target_parse (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecXml *xml = SWFDEC_XML (target);

  if (xml->loader != loader || loader->state <= SWFDEC_LOADER_STATE_READING)
    return;

  /* get the text from the loader */
  if (loader->state == SWFDEC_LOADER_STATE_ERROR) {
    /* nothing to do here */
  } else {
    guint size;
    g_assert (loader->state == SWFDEC_LOADER_STATE_EOF);
    swfdec_loader_set_data_type (loader, SWFDEC_LOADER_DATA_TEXT);
    size = swfdec_buffer_queue_get_depth (loader->queue);
    xml->text = g_try_malloc (size + 1);
    if (xml->text) {
      SwfdecBuffer *buffer;
      guint i = 0;
      while ((buffer = swfdec_buffer_queue_pull_buffer (loader->queue))) {
	memcpy (xml->text + i, buffer->data, buffer->length);
	i += buffer->length;
	swfdec_buffer_unref (buffer);
      }
      g_assert (i == size);
      xml->text[size] = '\0';
      /* FIXME: validate otherwise? */
      if (!g_utf8_validate (xml->text, size, NULL)) {
	SWFDEC_ERROR ("downloaded data is not valid utf-8");
	g_free (xml->text);
	xml->text = NULL;
      }
    } else {
      SWFDEC_ERROR ("not enough memory to copy %u bytes", size);
    }
  }
  /* break reference to the loader */
  xml->loader = NULL;
  g_object_unref (loader);
  /* emit onData */
  swfdec_xml_ondata (xml);
}

static void
swfdec_xml_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_xml_loader_target_get_player;
  iface->parse = swfdec_xml_loader_target_parse;
}

/*** SWFDEC_XML ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecXml, swfdec_xml, SWFDEC_TYPE_AS_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_xml_loader_target_init))

static void
swfdec_xml_reset (SwfdecXml *xml)
{
  if (xml->loader) {
    swfdec_loader_set_target (xml->loader, NULL);
    g_object_unref (xml->loader);
    xml->loader = NULL;
  }
  g_free (xml->text);
  xml->text = NULL;
}

static void
swfdec_xml_dispose (GObject *object)
{
  SwfdecXml *xml = SWFDEC_XML (object);

  swfdec_xml_reset (xml);

  G_OBJECT_CLASS (swfdec_xml_parent_class)->dispose (object);
}

static void
swfdec_xml_class_init (SwfdecXmlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_xml_dispose;
}

static void
swfdec_xml_init (SwfdecXml *xml)
{
}

SwfdecAsObject *
swfdec_xml_new (SwfdecAsContext *context)
{
  SwfdecAsObject *xml;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecXml)))
    return NULL;
  xml = g_object_new (SWFDEC_TYPE_XML, NULL);
  swfdec_as_object_add (xml, context, sizeof (SwfdecXml));

  return xml;
}

void
swfdec_xml_load (SwfdecXml *xml, const char *url)
{
  g_return_if_fail (SWFDEC_IS_XML (xml));
  g_return_if_fail (url != NULL);

  swfdec_xml_reset (xml);
  xml->loader = swfdec_player_load (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (xml)->context), url);
  swfdec_loader_set_target (xml->loader, SWFDEC_LOADER_TARGET (xml));
}
