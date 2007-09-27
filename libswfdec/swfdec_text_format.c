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

#include "swfdec_text_format.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_array.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecTextFormat, swfdec_text_format, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_text_format_class_init (SwfdecTextFormatClass *klass)
{
}

static void
swfdec_text_format_init (SwfdecTextFormat *text_format)
{
}

static void
swfdec_text_format_get_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "");

  switch (format->align) {
    case SWFDEC_TEXT_ALIGN_LEFT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_left);
      break;
    case SWFDEC_TEXT_ALIGN_CENTER:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_center);
      break;
    case SWFDEC_TEXT_ALIGN_RIGHT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_right);
      break;
    case SWFDEC_TEXT_ALIGN_JUSTIFY:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_justify);
      break;
    case SWFDEC_TEXT_ALIGN_UNDEFINED:
      SWFDEC_AS_VALUE_SET_NULL (ret);
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_text_format_set_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecTextFormat *format;
  const char *value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEXTFORMAT, (gpointer)&format, "s", &value);

  if (!g_ascii_strcasecmp (value, "left")) {
    format->align = SWFDEC_TEXT_ALIGN_LEFT;
  }
}

static void
swfdec_text_format_add_variable (SwfdecAsObject *object, const char *variable,
    SwfdecAsNative get, SwfdecAsNative set)
{
  SwfdecAsFunction *get_func, *set_func;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (get != NULL);

  get_func =
    swfdec_as_native_function_new (object->context, variable, get, 0, NULL);
  if (get_func == NULL)
    return;

  if (set != NULL) {
    set_func =
      swfdec_as_native_function_new (object->context, variable, set, 0, NULL);
  } else {
    set_func = NULL;
  }

  swfdec_as_object_add_variable (object, variable, get_func, set_func, 0);
}

SWFDEC_AS_CONSTRUCTOR (110, 0, swfdec_text_format_construct, swfdec_text_format_get_type)
void
swfdec_text_format_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  g_assert (SWFDEC_IS_TEXTFORMAT (object));

  if (!SWFDEC_PLAYER (cx)->text_format_properties_initialized) {
    SwfdecAsValue val;
    SwfdecAsObject *proto;

    swfdec_as_object_get_variable (object, SWFDEC_AS_STR___proto__, &val);
    g_return_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val));
    proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

    swfdec_text_format_add_variable (proto, SWFDEC_AS_STR_align,
	swfdec_text_format_get_align, swfdec_text_format_set_align);

    SWFDEC_PLAYER (cx)->text_format_properties_initialized = TRUE;
  }
}
