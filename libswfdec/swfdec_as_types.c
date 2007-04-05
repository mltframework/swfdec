/* SwfdecAs
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

#include <math.h>

#include "swfdec_as_types.h"
#include "swfdec_as_object.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"

#define SWFDEC_AS_CONSTANT_STRING(str) "\2" str
const char *swfdec_as_strings[] = {
  SWFDEC_AS_CONSTANT_STRING (""),
  SWFDEC_AS_CONSTANT_STRING ("__proto__"),
  SWFDEC_AS_CONSTANT_STRING ("this"),
  SWFDEC_AS_CONSTANT_STRING ("code"),
  SWFDEC_AS_CONSTANT_STRING ("level"),
  SWFDEC_AS_CONSTANT_STRING ("description"),
  SWFDEC_AS_CONSTANT_STRING ("status"),
  SWFDEC_AS_CONSTANT_STRING ("success"),
  SWFDEC_AS_CONSTANT_STRING ("NetConnection.Connect.Success"),
  SWFDEC_AS_CONSTANT_STRING ("onLoad"),
  SWFDEC_AS_CONSTANT_STRING ("onEnterFrame"),
  SWFDEC_AS_CONSTANT_STRING ("onUnload"),
  SWFDEC_AS_CONSTANT_STRING ("onMouseMove"),
  SWFDEC_AS_CONSTANT_STRING ("onMouseDown"),
  SWFDEC_AS_CONSTANT_STRING ("onMouseUp"),
  SWFDEC_AS_CONSTANT_STRING ("onKeyUp"),
  SWFDEC_AS_CONSTANT_STRING ("onKeyDown"),
  SWFDEC_AS_CONSTANT_STRING ("onData"),
  SWFDEC_AS_CONSTANT_STRING ("onPress"),
  SWFDEC_AS_CONSTANT_STRING ("onRelease"),
  SWFDEC_AS_CONSTANT_STRING ("onReleaseOutside"),
  SWFDEC_AS_CONSTANT_STRING ("onRollOver"),
  SWFDEC_AS_CONSTANT_STRING ("onRollOut"),
  SWFDEC_AS_CONSTANT_STRING ("onDragOver"),
  SWFDEC_AS_CONSTANT_STRING ("onDragOut"),
  SWFDEC_AS_CONSTANT_STRING ("onConstruct"),
  SWFDEC_AS_CONSTANT_STRING ("onStatus"),
  SWFDEC_AS_CONSTANT_STRING ("error"),
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Buffer.Empty"),
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Buffer.Full"),
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Buffer.Flush"),
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Play.Start"),
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Play.Stop"),
  SWFDEC_AS_CONSTANT_STRING ("NetStream.Play.StreamNotFound"),
  SWFDEC_AS_CONSTANT_STRING ("undefined"),
  SWFDEC_AS_CONSTANT_STRING ("null"),
  SWFDEC_AS_CONSTANT_STRING ("[object Object]"),
  SWFDEC_AS_CONSTANT_STRING ("true"),
  SWFDEC_AS_CONSTANT_STRING ("false"),
  SWFDEC_AS_CONSTANT_STRING ("_x"),
  SWFDEC_AS_CONSTANT_STRING ("_y"),
  SWFDEC_AS_CONSTANT_STRING ("_xscale"),
  SWFDEC_AS_CONSTANT_STRING ("_yscale"),
  SWFDEC_AS_CONSTANT_STRING ("_currentframe"),
  SWFDEC_AS_CONSTANT_STRING ("_totalframes"),
  SWFDEC_AS_CONSTANT_STRING ("_alpha"),
  SWFDEC_AS_CONSTANT_STRING ("_visible"),
  SWFDEC_AS_CONSTANT_STRING ("_width"),
  SWFDEC_AS_CONSTANT_STRING ("_height"),
  SWFDEC_AS_CONSTANT_STRING ("_rotation"), 
  SWFDEC_AS_CONSTANT_STRING ("_target"),
  SWFDEC_AS_CONSTANT_STRING ("_framesloaded"),
  SWFDEC_AS_CONSTANT_STRING ("_name"), 
  SWFDEC_AS_CONSTANT_STRING ("_droptarget"),
  SWFDEC_AS_CONSTANT_STRING ("_url"), 
  SWFDEC_AS_CONSTANT_STRING ("_highquality"), 
  SWFDEC_AS_CONSTANT_STRING ("_focusrect"), 
  SWFDEC_AS_CONSTANT_STRING ("_soundbuftime"), 
  SWFDEC_AS_CONSTANT_STRING ("_quality"),
  SWFDEC_AS_CONSTANT_STRING ("_xmouse"), 
  SWFDEC_AS_CONSTANT_STRING ("_ymouse"),
  /* add more here */
  NULL
};

/**
 * swfdec_as_value_to_string:
 * @context: a #SwfdecAsContext
 * @value: value to be expressed as string
 *
 * Converts @value to a string.
 * <warning>This function may run the garbage collector.</warning>
 *
 * Returns: a garbage-collected string representing @value
 **/
const char *
swfdec_as_value_to_string (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), SWFDEC_AS_STR_EMPTY);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (value), SWFDEC_AS_STR_EMPTY);

  switch (value->type) {
    case SWFDEC_TYPE_AS_STRING:
      return SWFDEC_AS_VALUE_GET_STRING (value);
    case SWFDEC_TYPE_AS_UNDEFINED:
      if (context->version > 6)
	return SWFDEC_AS_STR_UNDEFINED;
      else
	return SWFDEC_AS_STR_EMPTY;
    case SWFDEC_TYPE_AS_BOOLEAN:
      return SWFDEC_AS_VALUE_GET_BOOLEAN (value) ? SWFDEC_AS_STR_TRUE : SWFDEC_AS_STR_FALSE;
    case SWFDEC_TYPE_AS_NULL:
      return SWFDEC_AS_STR_NULL;
    case SWFDEC_TYPE_AS_NUMBER:
      {
	char *s = g_strdup_printf ("%g", SWFDEC_AS_VALUE_GET_NUMBER (value));
	const char *ret = swfdec_as_context_get_string (context, s);
	g_free (s);
	return ret;
      }
    case SWFDEC_TYPE_AS_ASOBJECT:
      SWFDEC_ERROR ("FIXME");
      return SWFDEC_AS_STR_OBJECT_OBJECT;
    default:
      g_assert_not_reached ();
      return SWFDEC_AS_STR_EMPTY;
  }
}

double
swfdec_as_value_to_number (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), 0.0);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (value), 0.0);

  switch (value->type) {
    case SWFDEC_TYPE_AS_UNDEFINED:
    case SWFDEC_TYPE_AS_NULL:
      return (context->version >= 7) ? NAN : 0.0;
    case SWFDEC_TYPE_AS_BOOLEAN:
      return SWFDEC_AS_VALUE_GET_BOOLEAN (value) ? 1 : 0;
    case SWFDEC_TYPE_AS_NUMBER:
      return SWFDEC_AS_VALUE_GET_NUMBER (value);
    case SWFDEC_TYPE_AS_STRING:
      {
	char *end;
	double d = g_ascii_strtod (SWFDEC_AS_VALUE_GET_STRING (value), &end);
	if (*end == '\0')
	  return d;
	else
	  return NAN;
      }
    case SWFDEC_TYPE_AS_ASOBJECT:
      SWFDEC_ERROR ("FIXME");
      return NAN;
    default:
      g_assert_not_reached ();
      return NAN;
  }
}

int
swfdec_as_value_to_integer (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  /* FIXME */
  return swfdec_as_value_to_number (context, value);
}

