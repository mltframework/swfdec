/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_as_context.h"
#include "swfdec_as_function.h"
#include "swfdec_as_gcable.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_number.h"
#include "swfdec_as_object.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_string.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_super.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_movie.h"

/*** GTK-DOC ***/

/**
 * SECTION:SwfdecAsValue
 * @title: SwfdecAsValue
 * @short_description: exchanging values with the Actionscript engine
 *
 * This section describes how values are handled inside the Actionscript 
 * engine. Since Actionscript is a dynamically typed language, the variable type 
 * has to be carried with every value. #SwfdecAsValue accomplishes that. Swfdec
 * allows two possible ways of accessing these values: The common method is to
 * use the provided functions to explicitly convert the values to a given type
 * with a function such as swfdec_as_value_to_string (). This is convenient, 
 * but can be very slow as it can call back into the Actionscript engine when
 * converting various objects. So it can be unsuitable in some cases.
 * A different possibiltiy is accessing the values directly using the accessor
 * macros. You must check the type before doing so though. For setting values,
 * there only exist macros, since type conversion is not necessary.
 */

/**
 * SwfdecAsValueType:
 * @SWFDEC_AS_TYPE_UNDEFINED: the special undefined value
 * @SWFDEC_AS_TYPE_BOOLEAN: a boolean value - true or false
 * @SWFDEC_AS_TYPE_INT: reserved value for integers. Should the need arise for
 *                      performance enhancements - especially on embedded 
 *                      devices - it might be useful to implement this type.
 *                      For now, this type will never appear in Swfdec. Using 
 *                      it will cause Swfdec to crash.
 * @SWFDEC_AS_TYPE_NUMBER: a double value - also used for integer numbers
 * @SWFDEC_AS_TYPE_STRING: a string. Strings are garbage-collected and unique.
 * @SWFDEC_AS_TYPE_NULL: the spaecial null value
 * @SWFDEC_AS_TYPE_OBJECT: an object - must be of type #SwfdecAsObject
 *
 * These are the possible values the Swfdec Actionscript engine knows about.
 */

/**
 * SwfdecAsValue:
 * @type: the type of this value.
 *
 * This is the type used to present an opaque value in the Actionscript 
 * engine. See #SwfdecAsValueType for possible types. It's similar in 
 * spirit to #GValue. The value held is garbage-collected. Apart from the type 
 * member, use the provided macros to access this structure.
 * <note>If you memset a SwfdecAsValue to 0, it is a valid undefined value.</note>
 */

/**
 * SWFDEC_AS_VALUE_SET_UNDEFINED:
 * @val: value to set as undefined
 *
 * Sets @val to the special undefined value. If you create a temporary value, 
 * you can instead use code such as |[ SwfdecAsValue val = { 0, }; ]|
 */

/**
 * SWFDEC_AS_VALUE_GET_BOOLEAN:
 * @val: value to get, the value must reference a boolean
 *
 * Gets the boolean associated with value. If you are not sure if the value is
 * a boolean, use swfdec_as_value_to_boolean () instead.
 *
 * Returns: %TRUE or %FALSE
 */

/**
 * SWFDEC_AS_VALUE_SET_BOOLEAN:
 * @val: value to set
 * @b: boolean value to set, must be either %TRUE or %FALSE
 *
 * Sets @val to the specified boolean value.
 */

/**
 * SWFDEC_AS_VALUE_GET_NUMBER:
 * @val: value to get, the value must reference a number
 *
 * Gets the number associated with @val. If you are not sure that the value is
 * a valid number value, consider using swfdec_as_value_to_number() or 
 * swfdec_as_value_to_int() instead.
 *
 * Returns: a double. It can be NaN or +-Infinity, but not -0.0
 */

/**
 * SWFDEC_AS_VALUE_GET_STRING:
 * @val: value to get, the value must reference a string
 *
 * Gets the string associated with @val. If you are not sure that the value is
 * a string value, consider using swfdec_as_value_to_string() instead.
 *
 * Returns: a garbage-collected string.
 */

/**
 * SWFDEC_AS_VALUE_SET_STRING:
 * @val: value to set
 * @s: garbage-collected string to use
 *
 * Sets @val to the given string value.
 */

/**
 * SWFDEC_AS_VALUE_SET_NULL:
 * @val: value to set
 *
 * Sets @val to the special null value.
 */

/**
 * SWFDEC_AS_VALUE_GET_OBJECT:
 * @val: value to get, the value must reference an object
 *
 * Gets the object associated with @val. If you are not sure that the value is
 * an object value, consider using swfdec_as_value_to_object() instead.
 *
 * Returns: a #SwfdecAsObject
 */

/**
 * SWFDEC_AS_VALUE_SET_OBJECT:
 * @val: value to set
 * @o: garbage-collected #SwfdecAsObject to use
 *
 * Sets @val to the given object. The object must have been added to the 
 * garbage collector via swfdec_as_object_add() previously.
 */

/*** actual code ***/

/**
 * swfdec_as_value_set_int:
 * @val: value to set
 * @i: integer value to set
 *
 * Sets @val to the given value. Currently this function is a macro that calls
 * swfdec_as_value_set_number(), but this may change in future versions of
 * Swfdec.
 */

/**
 * swfdec_as_value_set_number:
 * @context: The context to use
 * @val: value to set
 * @number: double value to set
 *
 * Sets @val to the given value. If you are sure the value is a valid
 * integer value, use wfdec_as_value_set_int() instead.
 */
void
swfdec_as_value_set_number (SwfdecAsContext *context, SwfdecAsValue *val,
    double d)
{
  SwfdecAsDoubleValue *dval;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (val != NULL);

  dval = swfdec_as_gcable_new (context, SwfdecAsDoubleValue);
  dval->number = d;
  SWFDEC_AS_GCABLE_SET_NEXT (dval, context->numbers);
  context->numbers = dval;

  val->type = SWFDEC_AS_TYPE_NUMBER;
  val->value.gcable = (SwfdecAsGcable *) dval;
}

/**
 * swfdec_as_str_concat:
 * @cx: a #SwfdecAsContext
 * @s1: first string
 * @s2: second string
 *
 * Convenience function to concatenate two garbage-collected strings. This
 * function is equivalent to g_strconcat ().
 *
 * Returns: A new garbage-collected string
 **/
const char *
swfdec_as_str_concat (SwfdecAsContext *cx, const char * s1, const char *s2)
{
  const char *ret;
  char *s;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (cx), SWFDEC_AS_STR_EMPTY);
  g_return_val_if_fail (s1, SWFDEC_AS_STR_EMPTY);
  g_return_val_if_fail (s2, SWFDEC_AS_STR_EMPTY);

  s = g_strconcat (s1, s2, NULL);
  ret = swfdec_as_context_get_string (cx, s);
  g_free (s);

  return ret;
}

/**
 * swfdec_as_integer_to_string:
 * @context: a #SwfdecAsContext
 * @i: an integer that fits into 32 bits
 *
 * Converts @d into a string using the same conversion algorithm as the 
 * official Flash player.
 *
 * Returns: a garbage-collected string
 **/
const char *
swfdec_as_integer_to_string (SwfdecAsContext *context, int i)
{
  return swfdec_as_context_give_string (context, g_strdup_printf ("%d", i));
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
  gboolean found = FALSE, gotdot = FALSE;
  guint digits = 15;
  char tmp[50], *end, *start, *s;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), SWFDEC_AS_STR_EMPTY);

  if (isnan (d))
    return SWFDEC_AS_STR_NaN;
  if (isinf (d))
    return d < 0 ? SWFDEC_AS_STR__Infinity : SWFDEC_AS_STR_Infinity;
  /* stupid -0.0 */
  if (fabs (d) == 0.0)
    return SWFDEC_AS_STR_0;

  tmp[0] = ' ';
  s = &tmp[1];
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
    char *finish = NULL;
    /* skip all 9s at the end */
    while (start[-1] == '9')
      start--;
    /* if we're before the dot, replace 9s with 0s */
    if (start[-1] == '.') {
      finish = start;
      start--;
    }
    while (start[-1] == '9') {
      start[-1] = '0';
      start--;
    }
    /* write out correct number */
    if (start[-1] == '-') {
      s--;
      start[-2] = '-';
      start[-1] = '1';
    } else if (start[-1] == ' ') {
      s--;
      start[-1] = '1';
    } else {
      start[-1]++;
    }
    /* reposition cursor at end */
    if (finish)
      start = finish;
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

/**
 * swfdec_as_value_to_string:
 * @context: a #SwfdecAsContext
 * @value: value to be expressed as string
 *
 * Converts @value to a string according to the rules of Flash. This might 
 * cause calling back into the script engine if the @value is an object. In
 * that case, the object's valueOf function is called. 
 * <warning>Never use this function for debugging purposes.</warning>
 *
 * Returns: a garbage-collected string representing @value. The value will 
 *          never be %NULL.
 **/
const char *
swfdec_as_value_to_string (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), SWFDEC_AS_STR_EMPTY);

  switch (SWFDEC_AS_VALUE_GET_TYPE (value)) {
    case SWFDEC_AS_TYPE_STRING:
      return SWFDEC_AS_VALUE_GET_STRING (value);
    case SWFDEC_AS_TYPE_UNDEFINED:
      if (context->version > 6)
	return SWFDEC_AS_STR_undefined;
      else
	return SWFDEC_AS_STR_EMPTY;
    case SWFDEC_AS_TYPE_BOOLEAN:
      return SWFDEC_AS_VALUE_GET_BOOLEAN (value) ? SWFDEC_AS_STR_true : SWFDEC_AS_STR_false;
    case SWFDEC_AS_TYPE_NULL:
      return SWFDEC_AS_STR_null;
    case SWFDEC_AS_TYPE_NUMBER:
      return swfdec_as_double_to_string (context, SWFDEC_AS_VALUE_GET_NUMBER (value));
    case SWFDEC_AS_TYPE_OBJECT:
      {
	SwfdecAsObject *object = SWFDEC_AS_VALUE_GET_OBJECT (value);
	if (SWFDEC_IS_AS_STRING (object->relay)) {
	  return SWFDEC_AS_STRING (object->relay)->string;
	} else {
	  SwfdecAsValue ret;
	  swfdec_as_object_call (object, SWFDEC_AS_STR_toString, 0, NULL, &ret);
	  if (SWFDEC_AS_VALUE_IS_STRING (&ret))
	    return SWFDEC_AS_VALUE_GET_STRING (&ret);
	  else if (SWFDEC_IS_AS_SUPER (object->relay))
	    return SWFDEC_AS_STR__type_Object_;
	  else if (SWFDEC_IS_AS_FUNCTION (object->relay))
	    return SWFDEC_AS_STR__type_Function_;
	  else
	    return SWFDEC_AS_STR__type_Object_;
	}
      }
    case SWFDEC_AS_TYPE_MOVIE:
      {
	SwfdecMovie *movie = SWFDEC_AS_VALUE_GET_MOVIE (value);
	char *str;

	if (movie == NULL)
	  return SWFDEC_AS_STR_EMPTY;
	str = swfdec_movie_get_path (movie, TRUE);
	return swfdec_as_context_give_string (context, str);
      }
    case SWFDEC_AS_TYPE_INT:
    default:
      g_assert_not_reached ();
      return SWFDEC_AS_STR_EMPTY;
  }
}

/**
 * swfdec_as_value_to_number:
 * @context: a #SwfdecAsContext
 * @value: a #SwfdecAsValue used by context
 *
 * Converts the value to a number according to Flash's conversion routines and
 * the current Flash version. This conversion routine is similar, but not equal
 * to ECMAScript. For objects, it can call back into the script engine by 
 * calling the object's valueOf function.
 *
 * Returns: a double value. It can be NaN or +-Infinity. It will not be -0.0.
 **/
double
swfdec_as_value_to_number (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  SwfdecAsValue tmp;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), 0.0);

  tmp = *value;
  swfdec_as_value_to_primitive (&tmp);

  switch (SWFDEC_AS_VALUE_GET_TYPE (&tmp)) {
    case SWFDEC_AS_TYPE_UNDEFINED:
    case SWFDEC_AS_TYPE_NULL:
      return (context->version >= 7) ? NAN : 0.0;
    case SWFDEC_AS_TYPE_BOOLEAN:
      return SWFDEC_AS_VALUE_GET_BOOLEAN (&tmp) ? 1 : 0;
    case SWFDEC_AS_TYPE_NUMBER:
      return SWFDEC_AS_VALUE_GET_NUMBER (&tmp);
    case SWFDEC_AS_TYPE_STRING:
      {
	const char *s;
	char *end;
	double d;
	
	// FIXME: We should most likely copy Tamarin's code here (MathUtils.cpp)
	s = SWFDEC_AS_VALUE_GET_STRING (&tmp);
	if (s == SWFDEC_AS_STR_EMPTY)
	  return (context->version >= 5) ? NAN : 0.0;
	if (context->version > 5 && s[0] == '0' &&
	    (s[1] == 'x' || s[1] == 'X')) {
	  d = g_ascii_strtoll (s + 2, &end, 16);
	} else if (context->version > 5 &&
	    (s[0] == '0' || ((s[0] == '+' || s[0] == '-') && s[1] == '0')) &&
	    s[strspn (s+1, "01234567")+1] == '\0') {
	  d = g_ascii_strtoll (s, &end, 8);
	} else {
	  if (strpbrk (s, "xXiI") != NULL)
	    return (context->version >= 5) ? NAN : 0.0;
	  d = g_ascii_strtod (s, &end);
	}
	if (*end == '\0' || context->version < 5)
	  return d == -0.0 ? 0.0 : d;
	else
	  return NAN;
      }
    case SWFDEC_AS_TYPE_OBJECT:
    case SWFDEC_AS_TYPE_MOVIE:
      return (context->version >= 5) ? NAN : 0.0;
    case SWFDEC_AS_TYPE_INT:
    default:
      g_assert_not_reached ();
      return NAN;
  }
}

/**
 * swfdec_as_double_to_integer:
 * @d: any double
 *
 * Converts the given double to an integer using the same rules as the Flash
 * player.
 *
 * Returns: an integer
 **/
int
swfdec_as_double_to_integer (double d)
{
  if (!isfinite (d))
    return 0;
  if (d < 0) {
    d = fmod (-d, 4294967296.0);
    return - (guint) d;
  } else {
    d = fmod (d, 4294967296.0);
    return (guint) d;
  }
}

/**
 * swfdec_as_value_to_integer:
 * @context: a #SwfdecAsContext
 * @value: value to convert
 *
 * Converts the given value to an integer. This is done similar to the 
 * conversion used by swfdec_as_value_to_number().
 *
 * Returns: An Integer that can be represented in 32 bits.
 **/
int
swfdec_as_value_to_integer (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  double d;
  
  d = swfdec_as_value_to_number (context, value);
  return swfdec_as_double_to_integer (d);
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
  SwfdecAsFunction *fun;
  SwfdecAsValue val;
  const char *s;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  switch (SWFDEC_AS_VALUE_GET_TYPE (value)) {
    case SWFDEC_AS_TYPE_UNDEFINED:
    case SWFDEC_AS_TYPE_NULL:
      return NULL;
    case SWFDEC_AS_TYPE_NUMBER:
      s = SWFDEC_AS_STR_Number;
      break;
    case SWFDEC_AS_TYPE_STRING:
      s = SWFDEC_AS_STR_String;
      break;
    case SWFDEC_AS_TYPE_BOOLEAN:
      s = SWFDEC_AS_STR_Boolean;
      break;
    case SWFDEC_AS_TYPE_OBJECT:
    case SWFDEC_AS_TYPE_MOVIE:
      return SWFDEC_AS_VALUE_GET_COMPOSITE (value);
    case SWFDEC_AS_TYPE_INT:
    default:
      g_assert_not_reached ();
      return NULL;
  }

  swfdec_as_object_get_variable (context->global, s, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val) ||
      !SWFDEC_IS_AS_FUNCTION (fun = (SwfdecAsFunction *) (SWFDEC_AS_VALUE_GET_OBJECT (&val)->relay)))
    return NULL;
  swfdec_as_object_create (fun, 1, value, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    return SWFDEC_AS_VALUE_GET_OBJECT (&val);
  } else {
    SWFDEC_ERROR ("did not construct an object");
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

  /* FIXME: what do we do when called in flash 4? */
  switch (SWFDEC_AS_VALUE_GET_TYPE (value)) {
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
    case SWFDEC_AS_TYPE_MOVIE:
      return TRUE;
    case SWFDEC_AS_TYPE_INT:
    default:
      g_assert_not_reached ();
      return FALSE;
  }
}

/**
* swfdec_as_value_to_primitive:
* @value: value to convert
*
* Tries to convert the given @value inline to its primitive value. Primitive 
* values are values that are not objects. If the value is an object, the 
* object's valueOf function is called. If the result of that function is still 
* an object, it is returned nonetheless.
**/
void
swfdec_as_value_to_primitive (SwfdecAsValue *value)
{

  if (SWFDEC_AS_VALUE_IS_OBJECT (value)) {
    swfdec_as_object_call (SWFDEC_AS_VALUE_GET_OBJECT (value), SWFDEC_AS_STR_valueOf,
	0, NULL, value);
  }
}

/**
 * swfdec_as_value_get_variable:
 * @cx: the context
 * @value: the value to get the variable from
 * @name: name of the variable to get
 * @ret: The return value to set. May be identical to the passed in @value.
 *
 * Gets a variable from the given @value. This function is a shortcut for 
 * converting to a #SwfdecAsObject and then calling 
 * swfdec_As_object_get_variable(). When the @value cannot be converted to an
 * object, @ret is set to undefined.
 **/
void
swfdec_as_value_get_variable (SwfdecAsContext *cx, const SwfdecAsValue *value, 
    const char *name, SwfdecAsValue *ret)
{
  SwfdecAsObject *object;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (cx));
  g_return_if_fail (value != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (ret != NULL);

  object = swfdec_as_value_to_object (cx, value);
  if (object == NULL) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (ret);
    return;
  }
  swfdec_as_object_get_variable (object, name, ret);
}

/* from swfdec_internal.h */
gboolean
swfdec_as_value_to_twips (SwfdecAsContext *context, const SwfdecAsValue *val, 
    gboolean is_length, SwfdecTwips *result)
{
  double d;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), FALSE);
  g_return_val_if_fail (val != NULL, FALSE);
  g_return_val_if_fail (result != NULL, FALSE);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (val) || SWFDEC_AS_VALUE_IS_NULL (val))
    return FALSE;

  d = swfdec_as_value_to_number (context, val);
  if (isnan (d))
    return FALSE;
  if (is_length && d < 0)
    return FALSE;

  d *= SWFDEC_TWIPS_SCALE_FACTOR;
  *result = swfdec_as_double_to_integer (d);
  if (is_length)
    *result = ABS (*result);
  return TRUE;
}

