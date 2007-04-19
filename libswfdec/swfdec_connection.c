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
#include "swfdec_connection.h"
#include "swfdec_as_context.h"
#include "swfdec_as_object.h"
#include "swfdec_debug.h"

/*** SwfdecConnection ***/

G_DEFINE_TYPE (SwfdecConnection, swfdec_connection, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_connection_dispose (GObject *object)
{
  SwfdecConnection *connection = SWFDEC_CONNECTION (object);

  g_free (connection->url);
  connection->url = NULL;

  G_OBJECT_CLASS (swfdec_connection_parent_class)->dispose (object);
}

static void
swfdec_connection_class_init (SwfdecConnectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_connection_dispose;
}

static void
swfdec_connection_init (SwfdecConnection *connection)
{
}

static void
swfdec_connection_onstatus (SwfdecConnection *conn, const char *code,
    const char *level, const char *description)
{
  SwfdecAsValue value;
  SwfdecAsObject *info;

  info = swfdec_as_object_new (SWFDEC_AS_OBJECT (conn)->context);
  if (info == NULL)
    return;
  swfdec_as_object_root (info);
  SWFDEC_AS_VALUE_SET_STRING (&value, code);
  swfdec_as_object_set_variable (info, SWFDEC_AS_STR_CODE, &value);
  SWFDEC_AS_VALUE_SET_STRING (&value, level);
  swfdec_as_object_set_variable (info, SWFDEC_AS_STR_LEVEL, &value);
  if (description) {
    SWFDEC_AS_VALUE_SET_STRING (&value, description);
    swfdec_as_object_set_variable (info, SWFDEC_AS_STR_DESCRIPTION, &value);
  }
  SWFDEC_AS_VALUE_SET_OBJECT (&value, info);
  swfdec_as_object_unroot (info);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (conn), SWFDEC_AS_STR_STATUS, 1, &value, NULL);
}

SwfdecConnection *
swfdec_connection_new (SwfdecAsContext *context)
{
  SwfdecConnection *conn;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecConnection)))
    return NULL;
  conn = g_object_new (SWFDEC_TYPE_CONNECTION, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (conn), context, sizeof (SwfdecConnection));

  return conn;
}

void
swfdec_connection_connect (SwfdecConnection *conn, const char *url)
{
  g_return_if_fail (SWFDEC_IS_CONNECTION (conn));

  g_free (conn->url);
  conn->url = g_strdup (url);
  if (url) {
    SWFDEC_ERROR ("FIXME: using NetConnection with non-null URLs is not implemented");
  }
  swfdec_connection_onstatus (conn, SWFDEC_AS_STR_NET_CONNECTION_CONNECT_SUCCESS,
       SWFDEC_AS_STR_SUCCESS, NULL);
}

