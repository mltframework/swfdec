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

#include <math.h>

#include "swfdec_as_types.h"
#include "swfdec_as_object.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"

#define SWFDEC_AS_CONSTANT_STRING(str) ((const char *) "\2" str)
const char const * swfdec_as_strings[] = {
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
  SWFDEC_AS_CONSTANT_STRING ("#ERROR#"),
  SWFDEC_AS_CONSTANT_STRING ("number"),
  SWFDEC_AS_CONSTANT_STRING ("boolean"),
  SWFDEC_AS_CONSTANT_STRING ("string"),
  SWFDEC_AS_CONSTANT_STRING ("movieclip"),
  SWFDEC_AS_CONSTANT_STRING ("function"),
  SWFDEC_AS_CONSTANT_STRING ("object"),
  SWFDEC_AS_CONSTANT_STRING ("toString"),
  SWFDEC_AS_CONSTANT_STRING ("valueOf"),
  SWFDEC_AS_CONSTANT_STRING ("Function"),
  SWFDEC_AS_CONSTANT_STRING ("prototype"),
  SWFDEC_AS_CONSTANT_STRING ("constructor"),
  SWFDEC_AS_CONSTANT_STRING ("_parent"),
  SWFDEC_AS_CONSTANT_STRING ("_root"),
  SWFDEC_AS_CONSTANT_STRING ("Object"),
  SWFDEC_AS_CONSTANT_STRING ("hasOwnProperty"),
  SWFDEC_AS_CONSTANT_STRING ("NUMERIC"),
  SWFDEC_AS_CONSTANT_STRING ("RETURNINDEXEDARRAY"),
  SWFDEC_AS_CONSTANT_STRING ("UNIQUESORT"),
  SWFDEC_AS_CONSTANT_STRING ("DESCENDING"),
  SWFDEC_AS_CONSTANT_STRING ("CASEINSENSITIVE"),
  SWFDEC_AS_CONSTANT_STRING ("Array"),
  SWFDEC_AS_CONSTANT_STRING ("ASSetPropFlags"),
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
    case SWFDEC_AS_TYPE_STRING:
      return SWFDEC_AS_VALUE_GET_STRING (value);
    case SWFDEC_AS_TYPE_UNDEFINED:
      if (context->version > 6)
	return SWFDEC_AS_STR_UNDEFINED;
      else
	return SWFDEC_AS_STR_EMPTY;
    case SWFDEC_AS_TYPE_BOOLEAN:
      return SWFDEC_AS_VALUE_GET_BOOLEAN (value) ? SWFDEC_AS_STR_TRUE : SWFDEC_AS_STR_FALSE;
    case SWFDEC_AS_TYPE_NULL:
      return SWFDEC_AS_STR_NULL;
    case SWFDEC_AS_TYPE_NUMBER:
      {
	char *s = g_strdup_printf ("%g", SWFDEC_AS_VALUE_GET_NUMBER (value));
	const char *ret = swfdec_as_context_get_string (context, s);
	g_free (s);
	return ret;
      }
    case SWFDEC_AS_TYPE_OBJECT:
      {
	SwfdecAsValue ret;
	swfdec_as_object_call (SWFDEC_AS_VALUE_GET_OBJECT (value), SWFDEC_AS_STR_TOSTRING,
	    0, NULL, &ret);
	if (SWFDEC_AS_VALUE_IS_STRING (&ret))
	  return SWFDEC_AS_VALUE_GET_STRING (&ret);
	else
	  return SWFDEC_AS_STR_OBJECT_OBJECT;
      }
    default:
      g_assert_not_reached ();
      return SWFDEC_AS_STR_EMPTY;
  }
}

/**
 * swfdec_as_value_to_printable:
 * @context: a #SwfdecAsContext
 * @value: value to be converted to a printable string
 *
 * Converts @value to a string. This function is used by the trace signal. It 
 * uses a slightly different conversion then swfdec_as_value_to_string(). Don't
 * use this function if you can avoid it.
 *
 * Returns: a garbage collected string representing @value.
 **/
const char *
swfdec_as_value_to_printable (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), SWFDEC_AS_STR_EMPTY);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (value), SWFDEC_AS_STR_EMPTY);

  switch (value->type) {
    case SWFDEC_AS_TYPE_UNDEFINED:
      return SWFDEC_AS_STR_UNDEFINED;
    default:
      break;
  }
  return swfdec_as_value_to_string (context, value);
}

double
swfdec_as_value_to_number (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), 0.0);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (value), 0.0);

  switch (value->type) {
    case SWFDEC_AS_TYPE_UNDEFINED:
    case SWFDEC_AS_TYPE_NULL:
      return (context->version >= 7) ? NAN : 0.0;
    case SWFDEC_AS_TYPE_BOOLEAN:
      return SWFDEC_AS_VALUE_GET_BOOLEAN (value) ? 1 : 0;
    case SWFDEC_AS_TYPE_NUMBER:
      return SWFDEC_AS_VALUE_GET_NUMBER (value);
    case SWFDEC_AS_TYPE_STRING:
      {
	char *end;
	double d = g_ascii_strtod (SWFDEC_AS_VALUE_GET_STRING (value), &end);
	if (*end == '\0')
	  return d;
	else
	  return NAN;
      }
    case SWFDEC_AS_TYPE_OBJECT:
      {
	SwfdecAsValue ret;
	swfdec_as_object_call (SWFDEC_AS_VALUE_GET_OBJECT (value), SWFDEC_AS_STR_VALUEOF,
	    0, NULL, &ret);
	if (SWFDEC_AS_VALUE_IS_NUMBER (&ret))
	  return SWFDEC_AS_VALUE_GET_NUMBER (&ret);
	else
	  return NAN;
      }
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

/**
 * swfdec_as_value_to_object:
 * @context: a #SwfdecAsContext
 * @value: value to convert
 *
 * Converts a given value to its representation as an object. The object 
 * representation for primitive types is a wrapper object of the corresponding 
 * class (Number for numbers, String for strings, Boolean for bools). If the 
 * value does not have an object representing it, such as undefined and null 
 * values, %NULL is returned.
 *
 * Returns: object representing @value or %NULL.
 **/
SwfdecAsObject *
swfdec_as_value_to_object (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (value), NULL);

  switch (value->type) {
    case SWFDEC_AS_TYPE_UNDEFINED:
    case SWFDEC_AS_TYPE_NULL:
      return NULL;
    case SWFDEC_AS_TYPE_BOOLEAN:
    case SWFDEC_AS_TYPE_NUMBER:
    case SWFDEC_AS_TYPE_STRING:
      SWFDEC_ERROR ("FIXME: implement conversion to object");
      return NULL;
    case SWFDEC_AS_TYPE_OBJECT:
      return SWFDEC_AS_VALUE_GET_OBJECT (value);
    default:
      g_assert_not_reached ();
      return NULL;
  }
}

/**
 * swfdec_as_value_to_boolean:
 * @context: a #SwfdecAsContext
 * @value: value to convert
 *
 * Converts the given value to a boolean according to Flash's rules. Note that
 * these rules changed significantly for strings between Flash 6 and 7.
 *
 * Returns: either %TRUE or %FALSE.
 **/
gboolean
swfdec_as_value_to_boolean (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), FALSE);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (value), FALSE);

  /* FIXME: what do we do when called in flash 4? */
  switch (value->type) {
    case SWFDEC_AS_TYPE_UNDEFINED:
    case SWFDEC_AS_TYPE_NULL:
      return FALSE;
    case SWFDEC_AS_TYPE_BOOLEAN:
      return SWFDEC_AS_VALUE_GET_BOOLEAN (value);
    case SWFDEC_AS_TYPE_NUMBER:
      {
	double d = SWFDEC_AS_VALUE_GET_NUMBER (value);
	return d != 0.0 && !isnan (d);
      }
    case SWFDEC_AS_TYPE_STRING:
      if (context->version <= 6) {
	double d = swfdec_as_value_to_number (context, value);
	return d != 0.0 && !isnan (d);
      } else {
	return SWFDEC_AS_VALUE_GET_STRING (value) != SWFDEC_AS_STR_EMPTY;
      }
    case SWFDEC_AS_TYPE_OBJECT:
      return TRUE;
    default:
      g_assert_not_reached ();
      return FALSE;
  }
}

