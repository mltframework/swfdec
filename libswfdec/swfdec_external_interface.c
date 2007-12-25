/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_player_scripting.h"
#include "swfdec_xml.h"

SWFDEC_AS_NATIVE (14, 0, swfdec_external_interface__initJS)
void
swfdec_external_interface__initJS (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  /* FIXME: call an init vfunc here? */
}

SWFDEC_AS_NATIVE (14, 1, swfdec_external_interface__objectID)
void
swfdec_external_interface__objectID (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  SwfdecPlayerScripting *scripting = player->priv->scripting;
  SwfdecPlayerScriptingClass *klass;
  
  if (scripting == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }
  klass = SWFDEC_PLAYER_SCRIPTING_GET_CLASS (scripting);
  if (klass->js_get_id) {
    char *s = klass->js_get_id (scripting, player);
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, s));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

SWFDEC_AS_NATIVE (14, 2, swfdec_external_interface__addCallback)
void
swfdec_external_interface__addCallback (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecPlayerPrivate *priv = SWFDEC_PLAYER (cx)->priv;
  SwfdecAsObject *fun;
  const char *name;

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
  SWFDEC_AS_CHECK (0, NULL, "sO", &name, &fun);

  /* FIXME: do we allow setting if scripting is unsupported? */
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    return;

  g_hash_table_insert (priv->scripting_callbacks, (gpointer) name, fun);
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
}

SWFDEC_AS_NATIVE (14, 3, swfdec_external_interface__evalJS)
void
swfdec_external_interface__evalJS (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  SwfdecPlayerScripting *scripting = player->priv->scripting;
  SwfdecPlayerScriptingClass *klass;
  const char *s;
  
  SWFDEC_AS_VALUE_SET_NULL (ret);
  if (scripting == NULL || argc == 0)
    return;
  s = swfdec_as_value_to_string (cx, &argv[0]);
  klass = SWFDEC_PLAYER_SCRIPTING_GET_CLASS (scripting);
  if (klass->js_call) {
    char *t = klass->js_call (scripting, player, s);
    if (t != NULL) {
      SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, t));
    }
  }
}

SWFDEC_AS_NATIVE (14, 4, swfdec_external_interface__callOut)
void
swfdec_external_interface__callOut (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  SwfdecPlayerScripting *scripting = player->priv->scripting;
  SwfdecPlayerScriptingClass *klass;
  const char *s;
  
  SWFDEC_AS_VALUE_SET_NULL (ret);
  if (scripting == NULL || argc == 0)
    return;
  s = swfdec_as_value_to_string (cx, &argv[0]);
  klass = SWFDEC_PLAYER_SCRIPTING_GET_CLASS (scripting);
  if (klass->xml_call) {
    char *t = klass->xml_call (scripting, player, s);
    if (t != NULL) {
      SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, t));
    }
  }
}

SWFDEC_AS_NATIVE (14, 5, swfdec_external_interface__escapeXML)
void
swfdec_external_interface__escapeXML (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  const char *s;

  if (argc == 0 ||
      (s = swfdec_as_value_to_string (cx, &argv[0])) == SWFDEC_AS_STR_EMPTY) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, swfdec_xml_escape (s)));
}

SWFDEC_AS_NATIVE (14, 6, swfdec_external_interface__unescapeXML)
void
swfdec_external_interface__unescapeXML (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  const char *s;

  if (argc == 0 ||
      (s = swfdec_as_value_to_string (cx, &argv[0])) == SWFDEC_AS_STR_EMPTY) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, 
	swfdec_xml_unescape_len (cx, s, strlen (s), FALSE)));
}

SWFDEC_AS_NATIVE (14, 7, swfdec_external_interface__jsQuoteString)
void
swfdec_external_interface__jsQuoteString (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  const char *s;
  GString *str;
  size_t len;

  if (argc == 0 ||
      (s = swfdec_as_value_to_string (cx, &argv[0])) == SWFDEC_AS_STR_EMPTY) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  str = g_string_new ("");
  do {
    /* Yay, we don't escape backslashes! */
    len = strcspn (s, "\n\r\"");
    g_string_append_len (str, s, len);
    s += len;
    if (*s == '\0')
      break;
    g_string_append_c (str, '\\');
    switch (*s) {
      case '\n':
	g_string_append_c (str, 'n');
	break;
      case '\r':
	g_string_append_c (str, 'r');
	break;
      case '"':
	g_string_append_c (str, '"');
	break;
      default:
	g_assert_not_reached ();
	break;
    };
    s++;
  } while (TRUE);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, g_string_free (str, FALSE)));
}

SWFDEC_AS_NATIVE (14, 100, swfdec_external_interface_get_available)
void
swfdec_external_interface_get_available (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, SWFDEC_PLAYER (cx)->priv->scripting != NULL);
}

SWFDEC_AS_NATIVE (14, 101, swfdec_external_interface_set_available)
void
swfdec_external_interface_set_available (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  /* read-only property */
}
