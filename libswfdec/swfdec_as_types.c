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
#include <string.h>

#include "swfdec_as_types.h"
#include "swfdec_as_object.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"
#include "swfdec_movie.h"

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
  SWFDEC_AS_CONSTANT_STRING ("_parent"),
  SWFDEC_AS_CONSTANT_STRING ("_root"),
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
  SWFDEC_AS_CONSTANT_STRING ("Object"),
  SWFDEC_AS_CONSTANT_STRING ("hasOwnProperty"),
  SWFDEC_AS_CONSTANT_STRING ("NUMERIC"),
  SWFDEC_AS_CONSTANT_STRING ("RETURNINDEXEDARRAY"),
  SWFDEC_AS_CONSTANT_STRING ("UNIQUESORT"),
  SWFDEC_AS_CONSTANT_STRING ("DESCENDING"),
  SWFDEC_AS_CONSTANT_STRING ("CASEINSENSITIVE"),
  SWFDEC_AS_CONSTANT_STRING ("Array"),
  SWFDEC_AS_CONSTANT_STRING ("ASSetPropFlags"),
  SWFDEC_AS_CONSTANT_STRING ("0"),
  SWFDEC_AS_CONSTANT_STRING ("-Infinity"),
  SWFDEC_AS_CONSTANT_STRING ("Infinity"),
  SWFDEC_AS_CONSTANT_STRING ("NaN"),
  SWFDEC_AS_CONSTANT_STRING ("Number"),
  SWFDEC_AS_CONSTANT_STRING ("NAN"),
  SWFDEC_AS_CONSTANT_STRING ("MAX_VALUE"),
  SWFDEC_AS_CONSTANT_STRING ("MIN_VALUE"),
  SWFDEC_AS_CONSTANT_STRING ("NEGATIVE_INFINITY"),
  SWFDEC_AS_CONSTANT_STRING ("POSITIVE_INFINITY"),
  SWFDEC_AS_CONSTANT_STRING ("[type Object]"),
  SWFDEC_AS_CONSTANT_STRING ("startDrag"),
  SWFDEC_AS_CONSTANT_STRING ("Mouse"),
  SWFDEC_AS_CONSTANT_STRING ("hide"),
  SWFDEC_AS_CONSTANT_STRING ("show"),
  SWFDEC_AS_CONSTANT_STRING ("addListener"),
  SWFDEC_AS_CONSTANT_STRING ("removeListener"),
  SWFDEC_AS_CONSTANT_STRING ("MovieClip"),
  SWFDEC_AS_CONSTANT_STRING ("attachMovie"),
  SWFDEC_AS_CONSTANT_STRING ("duplicateMovieClip"),
  SWFDEC_AS_CONSTANT_STRING ("getBytesLoaded"),
  SWFDEC_AS_CONSTANT_STRING ("getBytesTotal"),
  SWFDEC_AS_CONSTANT_STRING ("getDepth"),
  SWFDEC_AS_CONSTANT_STRING ("getNextHighestDepth"),
  SWFDEC_AS_CONSTANT_STRING ("getURL"),
  SWFDEC_AS_CONSTANT_STRING ("gotoAndPlay"),
  SWFDEC_AS_CONSTANT_STRING ("gotoAndStop"),
  SWFDEC_AS_CONSTANT_STRING ("hitTest"),
  SWFDEC_AS_CONSTANT_STRING ("nextFrame"),
  SWFDEC_AS_CONSTANT_STRING ("play"),
  SWFDEC_AS_CONSTANT_STRING ("prevFrame"),
  SWFDEC_AS_CONSTANT_STRING ("removeMovieClip"),
  SWFDEC_AS_CONSTANT_STRING ("stop"),
  SWFDEC_AS_CONSTANT_STRING ("stopDrag"),
  SWFDEC_AS_CONSTANT_STRING ("swapDepths"),
  SWFDEC_AS_CONSTANT_STRING ("super"),
  SWFDEC_AS_CONSTANT_STRING ("length"),
  SWFDEC_AS_CONSTANT_STRING ("[type Function]"),
  SWFDEC_AS_CONSTANT_STRING ("arguments"),
  /* add more here */
  NULL
};

const char *
swfdec_as_str_concat (SwfdecAsContext *cx, const char * s1, const char *s2)
{
  const char *ret;
  char *s;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (cx), SWFDEC_AS_STR_EMPTY);
  g_return_val_if_fail (s1, SWFDEC_AS_STR_EMPTY);
  g_return_val_if_fail (s2, SWFDEC_AS_STR_EMPTY);

  s = g_strconcat (s1, s2, NULL);
  ret = swfdec_as_context_get_string (cx, ret);
  g_free (s);

  return ret;
}

/**
 * swfdec_as_double_to_string:
 * @context: a #SwfdecAsContext
 * @d: a double
 *
 * Converts @d into a string using the same conversion algorithm as the 
 * official Flash player.
 *
 * Returns: a garbage-collected string
 **/
/* FIXME: this function is still buggy - and it's ugly as hell.
 * Someone with the right expertise should rewrite it 
 * Some pointers:
 * http://www.cs.indiana.edu/~burger/FP-Printing-PLDI96.pdf
 * http://lxr.mozilla.org/mozilla/source/js/tamarin/core/MathUtils.cpp
 */
const char *
swfdec_as_double_to_string (SwfdecAsContext *context, double d)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), SWFDEC_AS_STR_EMPTY);

  switch (fpclassify (d)) {
    case FP_ZERO:
      return SWFDEC_AS_STR_0;
    case FP_INFINITE:
      return d < 0 ? SWFDEC_AS_STR__Infinity : SWFDEC_AS_STR_Infinity;
    case FP_NAN:
      return SWFDEC_AS_STR_NaN;
    default:
      {
	gboolean found = FALSE, gotdot = FALSE;
	guint digits = 15;
	char s[50], *end, *start;
	if (ABS (d) > 0.00001 && ABS (d) < 1e+15) {
	  g_ascii_formatd (s, 50, "%.22f", d);
	} else {
	  g_ascii_formatd (s, 50, "%.25e", d);
	}
	start = s;
	/* skip - sign */
	if (*start == '-')
	  start++;
	/* count digits (maximum allowed is 15) */
	while (digits) {
	  if (*start == '.') {
	    start++;
	    gotdot = TRUE;
	    continue;
	  }
	  if (*start < '0' || *start > '9')
	    break;
	  if (found || *start != '0') {
	    digits--;
	    found = TRUE;
	  }
	  start++;
	}
	end = start;
	/* go to end of string */
	while (*end != 'e' && *end != 0)
	  end++;
	/* round using the next digit */
	if (*start >= '5' && *start <= '9') {
	  while (TRUE) {
	    if (start[-1] == '.') {
	      SWFDEC_ERROR ("FIXME: fix rounding!");
	      break;
	    }
	    if (start[-1] != '9') {
	      start[-1]++;
	      break;
	    }
	    start--;
	  }
	}
	/* remove trailing zeros (note we skipped zero above, so there will be non-0 bytes left) */
	if (gotdot) {
	  while (start[-1] == '0')
	    start--;
	  if (start[-1] == '.')
	    start--;
	}
	/* add exponent */
	if (*end == 'e') {
	  /* 'e' */
	  *start++ = *end++;
	  /* + or - */
	  *start++ = *end++;
	  /* skip 0s */
	  while (*end == '0')
	    end++;
	  /* add rest */
	  while (*end != 0)
	    *start++ = *end++;
	}
	/* end string */
	*start = 0;
	return swfdec_as_context_get_string (context, s);
      }
  }
}

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
      return swfdec_as_double_to_string (context, SWFDEC_AS_VALUE_GET_NUMBER (value));
    case SWFDEC_AS_TYPE_OBJECT:
      if (SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (value))) {
	char *str = swfdec_movie_get_path (SWFDEC_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (value)));
	const char *ret = swfdec_as_context_get_string (context, str);
	g_free (str);
	return ret;
      } else {
	SwfdecAsValue ret;
	swfdec_as_object_call (SWFDEC_AS_VALUE_GET_OBJECT (value), SWFDEC_AS_STR_toString,
	    0, NULL, &ret);
	if (SWFDEC_AS_VALUE_IS_STRING (&ret))
	  return SWFDEC_AS_VALUE_GET_STRING (&ret);
	else
	  return SWFDEC_AS_STR_type_Object;
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
	double d;
	
	if (SWFDEC_AS_VALUE_GET_STRING (value) == SWFDEC_AS_STR_EMPTY)
	  return NAN;
	d = g_ascii_strtod (SWFDEC_AS_VALUE_GET_STRING (value), &end);
	if (*end == '\0')
	  return d;
	else
	  return NAN;
      }
    case SWFDEC_AS_TYPE_OBJECT:
      {
	SwfdecAsValue ret;
	swfdec_as_object_call (SWFDEC_AS_VALUE_GET_OBJECT (value), SWFDEC_AS_STR_valueOf,
	    0, NULL, &ret);
	if (SWFDEC_AS_VALUE_IS_OBJECT (&ret))
	  return NAN;
	else
	  return swfdec_as_value_to_number (context, &ret);
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

