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

#include "swfdec_xml_as.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_load_object_as.h"

G_DEFINE_TYPE (SwfdecXml, swfdec_xml, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_xml_class_init (SwfdecXmlClass *klass)
{
}

static void
swfdec_xml_init (SwfdecXml *xml)
{
}

void
swfdec_xml_init_context (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context;
  SwfdecAsObject *xml, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  context = SWFDEC_AS_CONTEXT (player);
  proto = swfdec_as_object_new_empty (context);
  if (proto == NULL)
    return;
  xml = SWFDEC_AS_OBJECT (swfdec_as_object_add_constructor (context->global, 
      SWFDEC_AS_STR_XML, 0, SWFDEC_TYPE_XML, NULL, 0, proto));
  if (xml == NULL)
    return;
  /* set the right properties on the NetStream.prototype object */
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_load, SWFDEC_TYPE_XML,
      swfdec_load_object_load, 0);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, xml);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR___proto__, &val);
}

