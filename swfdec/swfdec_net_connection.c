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
#include "swfdec_net_connection.h"
#include "swfdec_as_context.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

/*** SwfdecNetConnection ***/

G_DEFINE_TYPE (SwfdecNetConnection, swfdec_net_connection, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_net_connection_dispose (GObject *object)
{
  SwfdecNetConnection *net_connection = SWFDEC_NET_CONNECTION (object);

  g_free (net_connection->url);
  net_connection->url = NULL;

  G_OBJECT_CLASS (swfdec_net_connection_parent_class)->dispose (object);
}

static void
swfdec_net_connection_class_init (SwfdecNetConnectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_net_connection_dispose;
}

static void
swfdec_net_connection_init (SwfdecNetConnection *net_connection)
{
}

static void
swfdec_net_connection_onstatus (SwfdecNetConnection *conn, const char *code,
    const char *level, const char *description)
{
  SwfdecAsValue value;
  SwfdecAsObject *info;

  info = swfdec_as_object_new (SWFDEC_AS_OBJECT (conn)->context);
  if (info == NULL)
    return;
  SWFDEC_AS_VALUE_SET_STRING (&value, code);
  swfdec_as_object_set_variable (info, SWFDEC_AS_STR_code, &value);
  SWFDEC_AS_VALUE_SET_STRING (&value, level);
  swfdec_as_object_set_variable (info, SWFDEC_AS_STR_level, &value);
  if (description) {
    SWFDEC_AS_VALUE_SET_STRING (&value, description);
    swfdec_as_object_set_variable (info, SWFDEC_AS_STR_description, &value);
  }
  SWFDEC_AS_VALUE_SET_OBJECT (&value, info);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (conn), SWFDEC_AS_STR_onStatus, 1, &value, NULL);
}

SwfdecNetConnection *
swfdec_net_connection_new (SwfdecAsContext *context)
{
  SwfdecNetConnection *conn;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecNetConnection)))
    return NULL;
  conn = g_object_new (SWFDEC_TYPE_NET_CONNECTION, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (conn), context, sizeof (SwfdecNetConnection));

  return conn;
}

void
swfdec_net_connection_connect (SwfdecNetConnection *conn, const char *url)
{
  g_return_if_fail (SWFDEC_IS_NET_CONNECTION (conn));

  g_free (conn->url);
  conn->url = g_strdup (url);
  if (url) {
    SWFDEC_ERROR ("FIXME: using NetConnection with non-null URLs is not implemented");
  }
  swfdec_net_connection_onstatus (conn, SWFDEC_AS_STR_NetConnection_Connect_Success,
       SWFDEC_AS_STR_status, NULL);
}

/*** AS CODE ***/

SWFDEC_AS_NATIVE (2100, 0, swfdec_net_connection_do_connect)
void
swfdec_net_connection_do_connect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecNetConnection *conn;
  SwfdecAsValue val;
  const char *url;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_NET_CONNECTION, &conn, "v", &val);

  if (SWFDEC_AS_VALUE_IS_STRING (&val)) {
    url = SWFDEC_AS_VALUE_GET_STRING (&val);
  } else if (SWFDEC_AS_VALUE_IS_NULL (&val)) {
    url = NULL;
  } else {
    SWFDEC_FIXME ("untested argument to NetConnection.connect: type %u", val.type);
    url = NULL;
  }
  swfdec_net_connection_connect (conn, url);
}

SWFDEC_AS_NATIVE (2100, 1, swfdec_net_connection_do_close)
void
swfdec_net_connection_do_close (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetConnection.close");
}

SWFDEC_AS_NATIVE (2100, 2, swfdec_net_connection_do_call)
void
swfdec_net_connection_do_call (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetConnection.call");
}

SWFDEC_AS_NATIVE (2100, 3, swfdec_net_connection_do_addHeader)
void
swfdec_net_connection_do_addHeader (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetConnection.addHeader");
}

SWFDEC_AS_NATIVE (2100, 4, swfdec_net_connection_get_connectedProxyType)
void
swfdec_net_connection_get_connectedProxyType (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetConnection.connectedProxyType (get)");
}

SWFDEC_AS_NATIVE (2100, 5, swfdec_net_connection_get_usingTLS)
void
swfdec_net_connection_get_usingTLS (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SWFDEC_STUB ("NetConnection.usingTLS (get)");
}

// not actually the constructor, but called from the constructor
SWFDEC_AS_CONSTRUCTOR (2100, 200, swfdec_net_connection_construct, swfdec_net_connection_get_type)
void
swfdec_net_connection_construct (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  // FIXME: Set contentType and possible do some other stuff too
}
