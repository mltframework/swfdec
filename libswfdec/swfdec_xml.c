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
#include "swfdec_debug.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"
#include "js/jsapi.h"
#include "js/jsinterp.h"

/*** SWFDEC_LOADER_TARGET ***/

static SwfdecPlayer *
swfdec_xml_loader_target_get_player (SwfdecLoaderTarget *target)
{
  SwfdecXml *xml = SWFDEC_XML (target);

  return xml->player;
}

static void
swfdec_xml_ondata (SwfdecXml *xml)
{
  JSContext *cx = SWFDEC_SCRIPTABLE (xml)->jscx;
  JSObject *obj = SWFDEC_SCRIPTABLE (xml)->jsobj;
  jsval val, fun;
  JSString *string;

  if (!JS_GetProperty (cx, obj, "onData", &fun))
    return;
  if (fun == JSVAL_VOID)
    return;
  if (xml->text) {
    string = JS_NewStringCopyZ (cx, xml->text);
    if (string == NULL) {
      SWFDEC_ERROR ("Could not convert text to JS string");
      return;
    }
    val = STRING_TO_JSVAL (string);
  } else {
    val = JSVAL_VOID;
  }
  js_InternalCall (cx, obj, fun, 1, &val, &fun);
}

static void
swfdec_xml_loader_target_parse (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecXml *xml = SWFDEC_XML (target);

  if (xml->loader != loader ||
      (!loader->eof && !loader->error))
    return;

  /* get the text from the loader */
  if (loader->error) {
    /* nothing to do here */
  } else {
    guint size;
    g_assert (loader->eof);
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

G_DEFINE_TYPE_WITH_CODE (SwfdecXml, swfdec_xml, SWFDEC_TYPE_SCRIPTABLE,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_xml_loader_target_init))

static void
swfdec_xml_reset (SwfdecXml *xml)
{
  if (xml->loader) {
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

extern const JSClass xml_class;
static void
swfdec_xml_class_init (SwfdecXmlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecScriptableClass *scriptable_class = SWFDEC_SCRIPTABLE_CLASS (klass);

  object_class->dispose = swfdec_xml_dispose;

  scriptable_class->jsclass = &xml_class;
}

static void
swfdec_xml_init (SwfdecXml *xml)
{
}

SwfdecXml *
swfdec_xml_new (SwfdecPlayer *player)
{
  SwfdecXml *xml;
  SwfdecScriptable *script;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  xml= g_object_new (SWFDEC_TYPE_XML, NULL);
  xml->player = player;

  script = SWFDEC_SCRIPTABLE (xml);
  script->jscx = player->jscx;

  return xml;
}

void
swfdec_xml_load (SwfdecXml *xml, const char *url)
{
  g_return_if_fail (SWFDEC_IS_XML (xml));
  g_return_if_fail (url != NULL);

  swfdec_xml_reset (xml);
  xml->loader = swfdec_player_load (xml->player, url);
  if (xml->loader == NULL) {
    swfdec_xml_ondata (xml);
  } else {
    swfdec_xml_loader_target_parse (SWFDEC_LOADER_TARGET (xml), xml->loader);
  }
}
