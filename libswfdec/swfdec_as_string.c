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

#include "swfdec_as_string.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_native_function.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsString, swfdec_as_string, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_string_do_mark (SwfdecAsObject *object)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);

  swfdec_as_string_mark (string->string);

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_string_parent_class)->mark (object);
}

static void
swfdec_as_string_class_init (SwfdecAsStringClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  asobject_class->mark = swfdec_as_string_do_mark;
}

static void
swfdec_as_string_init (SwfdecAsString *string)
{
  string->string = SWFDEC_AS_STR_EMPTY;
}

/*** AS CODE ***/

static void
swfdec_as_string_fromCharCode_5 (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  guint i, c;
  guint8 append;
  GError *error = NULL;
  char *s;
  GByteArray *array = g_byte_array_new ();

  for (i = 0; i < argc; i++) {
    c = ((guint) swfdec_as_value_to_integer (object->context, &argv[i])) % 65536;
    if (c > 255) {
      append = c / 256;
      g_byte_array_append (array, &append, 1);
    }
    append = c;
    g_byte_array_append (array, &append, 1);
  }

  /* FIXME: are these the correct charset names? */
  s = g_convert ((char *) array->data, array->len, "UTF8", "LATIN1", NULL, NULL, &error);
  if (s) {
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (object->context, s));
    g_free (s);
  } else {
    SWFDEC_ERROR ("%s", error->message);
    g_error_free (error);
  }
  g_byte_array_free (array, TRUE);
}

static void
swfdec_as_string_fromCharCode (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gunichar tmp[8];
  gunichar *chars;
  guint i;
  char *s;
  GError *error = NULL;

  if (argc <= 8)
    chars = tmp;
  else
    chars = g_new (gunichar, argc);

  for (i = 0; i < argc; i++) {
    chars[i] = ((guint) swfdec_as_value_to_integer (object->context, &argv[i])) % 65536;
  }

  s = g_ucs4_to_utf8 (chars, argc, NULL, NULL, &error);
  if (s) {
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (object->context, s));
    g_free (s);
  } else {
    SWFDEC_ERROR ("%s", error->message);
    g_error_free (error);
  }

  if (chars != tmp)
    g_free (chars);
}

static void
swfdec_as_string_construct (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *s;

  if (argc > 0) {
    s = swfdec_as_value_to_string (object->context, &argv[0]);
  } else {
    s = SWFDEC_AS_STR_EMPTY;
  }

  if (object->context->frame->construct) {
    SwfdecAsString *string = SWFDEC_AS_STRING (object);
    SwfdecAsValue val;

    string->string = s;
    SWFDEC_AS_VALUE_SET_INT (&val, g_utf8_strlen (string->string, -1));
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_length, &val);
    SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (ret, s);
  }
}

static void
swfdec_as_string_toString (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);

  SWFDEC_AS_VALUE_SET_STRING (ret, string->string);
}

static void
swfdec_as_string_valueOf (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);

  SWFDEC_AS_VALUE_SET_STRING (ret, string->string);
}

#if 0
charAt(index:Number) : String
charCodeAt(index:Number) : Number
concat(value:Object) : String
indexOf(value:String, [startIndex:Number]) : Number
lastIndexOf(value:String, [startIndex:Number]) : Number
slice(start:Number, end:Number) : String
split(delimiter:String, [limit:Number]) : Array
#endif

static const char *
swfdec_as_str_sub (SwfdecAsContext *cx, const char *str, guint offset, guint len)
{
  const char *end;
  char *dup;

  str = g_utf8_offset_to_pointer (str, offset);
  end = g_utf8_offset_to_pointer (str, len);
  dup = g_strndup (str, end - str);
  str = swfdec_as_context_get_string (cx, dup);
  g_free (dup);
  return str;
}

static void
swfdec_as_string_substr (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  int from, to, len;

  from = swfdec_as_value_to_integer (object->context, &argv[0]);
  len = g_utf8_strlen (string->string, -1);
  if (from < 0)
    from += len; /* from = len - ABS (from) aka start from end */
  
  if (argc > 1) {
    to = swfdec_as_value_to_integer (object->context, &argv[1]);
  } else {
    to = G_MAXINT;
  }
  from = CLAMP (from, 0, len);
  to = CLAMP (to, 0, len - from);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_str_sub (object->context, string->string, from, to));
}

static void
swfdec_as_string_substring (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  int from, to, len;

  len = g_utf8_strlen (string->string, -1);
  from = swfdec_as_value_to_integer (object->context, &argv[0]);
  if (argc > 1) {
    to = swfdec_as_value_to_integer (object->context, &argv[1]);
  } else {
    to = len;
  }
  from = CLAMP (from, 0, len);
  to = CLAMP (to, 0, len);
  if (to < from) {
    int tmp = to;
    to = from;
    from = tmp;
  }
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_str_sub (object->context, string->string, from, to - from));
}

static void
swfdec_as_string_toLowerCase (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  char *s;

  s = g_utf8_strdown (string->string, -1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (object->context, s));
  g_free (s);
}

static void
swfdec_as_string_toUpperCase (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  char *s;

  s = g_utf8_strup (string->string, -1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (object->context, s));
  g_free (s);
}

void
swfdec_as_string_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *string, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  string = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_String, 0, swfdec_as_string_construct, 0));
  swfdec_as_native_function_set_construct_type (SWFDEC_AS_NATIVE_FUNCTION (string), SWFDEC_TYPE_AS_STRING);
  if (!string)
    return;
  proto = swfdec_as_object_new (context);
  /* set the right properties on the String object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (string, SWFDEC_AS_STR_prototype, &val);
  if (version <= 5) {
    swfdec_as_object_add_function (string, SWFDEC_AS_STR_fromCharCode, 0, swfdec_as_string_fromCharCode_5, 0);
  } else {
    swfdec_as_object_add_function (string, SWFDEC_AS_STR_fromCharCode, 0, swfdec_as_string_fromCharCode, 0);
  }

  /* set the right properties on the String.prototype object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, string);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_substr, SWFDEC_TYPE_AS_STRING, swfdec_as_string_substr, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_substring, SWFDEC_TYPE_AS_STRING, swfdec_as_string_substring, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toLowerCase, SWFDEC_TYPE_AS_STRING, swfdec_as_string_toLowerCase, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, SWFDEC_TYPE_AS_STRING, swfdec_as_string_toString, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toUpperCase, SWFDEC_TYPE_AS_STRING, swfdec_as_string_toUpperCase, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_valueOf, SWFDEC_TYPE_AS_STRING, swfdec_as_string_valueOf, 0);
}

