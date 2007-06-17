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

#include "swfdec_as_string.h"
#include "swfdec_as_array.h"
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

static inline const char *
swfdec_as_str_nth_char (const char *s, guint n)
{
  while (*s && n--)
    s = g_utf8_next_char (s);
  return s;
}

static void
swfdec_as_string_charAt (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  int i;
  const char *s, *t;

  i = swfdec_as_value_to_integer (object->context, &argv[0]);
  if (i < 0) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
    return;
  }
  s = swfdec_as_str_nth_char (string->string, i);
  if (*s == 0) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
    return;
  }
  t = g_utf8_next_char (s);
  s = swfdec_as_context_give_string (cx, g_strndup (s, t - s));
  SWFDEC_AS_VALUE_SET_STRING (ret, s);
}

static void
swfdec_as_string_charCodeAt (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  int i;
  const char *s;
  gunichar c;

  i = swfdec_as_value_to_integer (cx, &argv[0]);
  if (i < 0) {
    SWFDEC_AS_VALUE_SET_NUMBER (ret, NAN);
    return;
  }
  s = swfdec_as_str_nth_char (string->string, i);
  if (*s == 0) {
    if (cx->version > 5) {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, NAN);
    } else {
      SWFDEC_AS_VALUE_SET_INT (ret, 0);
    }
    return;
  }
  c = g_utf8_get_char (s);
  SWFDEC_AS_VALUE_SET_NUMBER (ret, c);
}

static void
swfdec_as_string_fromCharCode_5 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  guint i, c;
  guint8 append;
  GError *error = NULL;
  char *s;
  GByteArray *array = g_byte_array_new ();

  for (i = 0; i < argc; i++) {
    c = ((guint) swfdec_as_value_to_integer (cx, &argv[i])) % 65536;
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
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, s));
    g_free (s);
  } else {
    SWFDEC_ERROR ("%s", error->message);
    g_error_free (error);
  }
  g_byte_array_free (array, TRUE);
}

static void
swfdec_as_string_fromCharCode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
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
    chars[i] = ((guint) swfdec_as_value_to_integer (cx, &argv[i])) % 65536;
  }

  s = g_ucs4_to_utf8 (chars, argc, NULL, NULL, &error);
  if (s) {
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, s));
    g_free (s);
  } else {
    SWFDEC_ERROR ("%s", error->message);
    g_error_free (error);
  }

  if (chars != tmp)
    g_free (chars);
}

static void
swfdec_as_string_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *s;

  if (argc > 0) {
    s = swfdec_as_value_to_string (cx, &argv[0]);
  } else {
    s = SWFDEC_AS_STR_EMPTY;
  }

  if (cx->frame->construct) {
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
swfdec_as_string_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);

  SWFDEC_AS_VALUE_SET_STRING (ret, string->string);
}

static void
swfdec_as_string_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
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

static void
swfdec_as_string_split_5 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsArray *arr;
  SwfdecAsValue val;
  const char *str, *end, *delim;
  int count;

  str = SWFDEC_AS_STRING (object)->string;
  arr = SWFDEC_AS_ARRAY (swfdec_as_array_new (cx));
  if (arr == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (arr));
  /* hi, i'm the special case */
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0])) {
    delim = SWFDEC_AS_STR_COMMA;
  } else {
    delim = swfdec_as_value_to_string (cx, &argv[0]);
  }
  if (delim == SWFDEC_AS_STR_EMPTY) {
    SWFDEC_AS_VALUE_SET_STRING (&val, str);
    swfdec_as_array_push (arr, &val);
    return;
  }
  if (argc > 1) {
    swfdec_as_value_to_string (cx, &argv[0]);
    count = swfdec_as_value_to_integer (cx, &argv[1]);
  } else {
    count = G_MAXINT;
  }
  if (count <= 0)
    return;
  if (str == SWFDEC_AS_STR_EMPTY || delim[1] != 0) {
    SWFDEC_AS_VALUE_SET_STRING (&val, str);
    swfdec_as_array_push (arr, &val);
    return;
  }
  while (*str && count > 0) {
    end = strchr (str, delim[0]);
    if (end == NULL) {
      SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (cx, str));
      swfdec_as_array_push (arr, &val);
      break;
    }
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_give_string (cx, g_strndup (str, end - str)));
    swfdec_as_array_push (arr, &val);
    if (count)
      count--;
    str = end + 1;
  }
}

static void
swfdec_as_string_split (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsArray *arr;
  SwfdecAsValue val;
  const char *str, *end, *delim;
  int count;
  guint len;

  str = SWFDEC_AS_STRING (object)->string;
  arr = SWFDEC_AS_ARRAY (swfdec_as_array_new (cx));
  if (arr == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (arr));
  /* hi, i'm the special case */
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0])) {
    SWFDEC_AS_VALUE_SET_STRING (&val, str);
    swfdec_as_array_push (arr, &val);
    return;
  }
  delim = swfdec_as_value_to_string (cx, &argv[0]);
  if (str == SWFDEC_AS_STR_EMPTY) {
    SWFDEC_AS_VALUE_SET_STRING (&val, str);
    swfdec_as_array_push (arr, &val);
    return;
  }
  if (argc > 1)
    count = swfdec_as_value_to_integer (cx, &argv[1]);
  else
    count = G_MAXINT;
  if (count <= 0)
    return;
  len = strlen (delim);
  while (count > 0) {
    if (delim == SWFDEC_AS_STR_EMPTY) {
      if (*str)
	end = str + 1;
      else
	break;
    } else {
      end = strstr (str, delim);
      if (end == NULL) {
	SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (cx, str));
	swfdec_as_array_push (arr, &val);
	break;
      }
    }
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_give_string (cx, g_strndup (str, end - str)));
    swfdec_as_array_push (arr, &val);
    if (count)
      count--;
    str = end + len;
  }
}

static const char *
swfdec_as_str_sub (SwfdecAsContext *cx, const char *str, guint offset, guint len)
{
  const char *end;

  str = g_utf8_offset_to_pointer (str, offset);
  end = g_utf8_offset_to_pointer (str, len);
  str = swfdec_as_context_give_string (cx, g_strndup (str, end - str));
  return str;
}

static void
swfdec_as_string_substr (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  int from, to, len;

  from = swfdec_as_value_to_integer (cx, &argv[0]);
  len = g_utf8_strlen (string->string, -1);
  
  if (argc > 1) {
    to = swfdec_as_value_to_integer (cx, &argv[1]);
    /* FIXME: wtf? */
    if (to < 0) {
      if (-to <= from)
	to = 0;
      else
	to += len;
      if (to < 0)
	to = 0;
      if (from < 0 && to >= -from)
	to = 0;
    }
  } else {
    to = G_MAXINT;
  }
  if (from < 0)
    from += len;
  from = CLAMP (from, 0, len);
  to = CLAMP (to, 0, len - from);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_str_sub (cx, string->string, from, to));
}

static void
swfdec_as_string_substring (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  int from, to, len;

  len = g_utf8_strlen (string->string, -1);
  from = swfdec_as_value_to_integer (cx, &argv[0]);
  if (argc > 1) {
    to = swfdec_as_value_to_integer (cx, &argv[1]);
  } else {
    to = len;
  }
  from = MAX (from, 0);
  if (from >= len) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
    return;
  }
  to = CLAMP (to, 0, len);
  if (to < from) {
    int tmp = to;
    to = from;
    from = tmp;
  }
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_str_sub (cx, string->string, from, to - from));
}

static void
swfdec_as_string_toLowerCase (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  char *s;

  s = g_utf8_strdown (string->string, -1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, s));
  g_free (s);
}

static void
swfdec_as_string_toUpperCase (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);
  char *s;

  s = g_utf8_strup (string->string, -1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, s));
  g_free (s);
}

/* escape and unescape are implemented here so the mad string functions share the same place */

static void
swfdec_as_string_unescape_5 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  GByteArray *array;
  const char *msg;
  char cur = 0; /* currently decoded character */
  char *out, *in, *s;
  guint decoding = 0; /* set if we're decoding a %XY string */

/* attention: c is a char* */
#define APPEND(chr) G_STMT_START{ \
  g_byte_array_append (array, (guchar *) chr, 1); \
}G_STMT_END
  array = g_byte_array_new ();
  msg = swfdec_as_value_to_string (cx, &argv[0]);
  in = s = g_convert (msg, -1, "LATIN1", "UTF8", NULL, NULL, NULL);
  if (s == NULL) {
    SWFDEC_FIXME ("%s can not be converted to utf8 - is this Flash 5 or what?", msg);
    return;
  }
  while (*s != 0) {
    if (decoding) {
      decoding++;
      if (*s >= '0' && *s <= '9') {
	cur = cur * 16 + *s - '0';
      } else if (*s >= 'A' && *s <= 'F') {
	cur = cur * 16 + *s - 'A' + 10;
      } else if (*s >= 'a' && *s <= 'f') {
	cur = cur * 16 + *s - 'a' + 10;
      } else {
	cur = 0;
	decoding = 0;
      }
      if (decoding == 3) {
	APPEND (&cur);
	cur = 0;
	decoding = 0;
      }
    } else if (*s == '%') {
      decoding = 1;
    } else if (*s == '+') {
      char tmp = ' ';
      APPEND (&tmp);
    } else {
      APPEND (s);
    }
    s++;
  }
  g_free (in);
  if (array->len == 0) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (ret);
    return;
  }
  cur = 0;
  g_byte_array_append (array, (guchar *) &cur, 1);
  out = g_convert ((char *) array->data, -1, "UTF8", "LATIN1", NULL, NULL, NULL);
  if (out) {
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, out));
    g_free (out);
  } else {
    g_warning ("can't convert %s to UTF-8", msg);
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
  }
  g_byte_array_free (array, TRUE);
#undef APPEND
}

static void
swfdec_as_string_escape (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  GByteArray *array;
  const char *s;
  char *in = NULL;

  array = g_byte_array_new ();
  s = swfdec_as_value_to_string (cx, &argv[0]);
  if (cx->version <= 5) {
    in = g_convert (s, -1, "LATIN1", "UTF8", NULL, NULL, NULL);
    if (s == NULL) {
      SWFDEC_FIXME ("%s can not be converted to utf8 - is this Flash 5 or what?", s);
      return;
    } else {
      s = in;
    }
  }
  while (*s) {
    if ((*s >= '0' && *s <= '9') ||
        (*s >= 'A' && *s <= 'Z') ||
        (*s >= 'a' && *s <= 'z')) {
      g_byte_array_append (array, (guchar *) s, 1);
    } else {
      guchar add[3] = { '%', 0, 0 };
      add[1] = (guchar) *s / 16;
      add[2] = (guchar) *s % 16;
      add[1] += add[1] < 10 ? '0' : ('A' - 10);
      add[2] += add[2] < 10 ? '0' : ('A' - 10);
      g_byte_array_append (array, add, 3);
    }
    s++;
  }
  g_byte_array_append (array, (guchar *) s, 1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, (char *) array->data));
  g_byte_array_free (array, TRUE);
  g_free (in);
}

static void
swfdec_as_string_unescape (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  GByteArray *array;
  const char *s, *msg;
  char cur = 0; /* currently decoded character */
  guint decoding = 0; /* set if we're decoding a %XY string */
  guint utf8left = 0; /* how many valid utf8 chars are still required */
  const guchar invalid[3] = { 0xEF, 0xBF, 0xBD };

/* attention: c is a char* */
#define APPEND(chr) G_STMT_START{ \
  guchar c = *chr; \
  if (utf8left) { \
    if ((c & 0xC0) == 0x80) { \
      g_byte_array_append (array, &c, 1); \
      utf8left--; \
    } else { \
      guint __len = array->len - 1; \
      while ((array->data[__len] & 0xC0) != 0xC0) \
	__len--; \
      g_byte_array_set_size (array, __len); \
      g_byte_array_append (array, invalid, 3); \
      utf8left = 0; \
    } \
  } else { \
    if (c < 0x80) { \
      g_byte_array_append (array, &c, 1); \
    } else if (c < 0xC0) { \
      guchar __foo = 0xC2; \
      g_byte_array_append (array, &__foo, 1); \
      g_byte_array_append (array, &c, 1); \
    } else if (c > 0xF7) { \
      break; \
    } else { \
      g_byte_array_append (array, &c, 1); \
      utf8left = (c < 0xE0) ? 1 : ((c < 0xF0) ? 2 : 3); \
    } \
  } \
}G_STMT_END
  array = g_byte_array_new ();
  msg = s = swfdec_as_value_to_string (cx, &argv[0]);
  while (*s != 0) {
    if (decoding) {
      decoding++;
      if (*s >= '0' && *s <= '9') {
	cur = cur * 16 + *s - '0';
      } else if (*s >= 'A' && *s <= 'F') {
	cur = cur * 16 + *s - 'A' + 10;
      } else if (*s >= 'a' && *s <= 'f') {
	cur = cur * 16 + *s - 'a' + 10;
      } else {
	cur = 0;
	decoding = 0;
	if ((guchar) *s > 0x7F) {
	  APPEND (s);
	}
      }
      if (decoding == 3) {
	APPEND (&cur);
	cur = 0;
	decoding = 0;
      }
    } else if (*s == '%') {
      decoding = 1;
    } else if (*s == '+') {
      char tmp = ' ';
      APPEND (&tmp);
    } else {
      APPEND (s);
    }
    s++;
  }
  cur = 0;
  /* loop for break statement in APPEND macro */
  if (utf8left) {
    guint __len = array->len - 1;
    while ((array->data[__len] & 0xC0) != 0xC0)
      __len--;
    g_byte_array_set_size (array, __len);
  }
  g_byte_array_append (array, (guchar *) &cur, 1);
  if (g_utf8_validate ((char *) array->data, -1, NULL)) {
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, (char *) array->data));
  } else {
    g_warning ("%s unescaped is invalid UTF-8", msg);
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
  }
  g_byte_array_free (array, TRUE);
#undef APPEND
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
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_charAt, SWFDEC_TYPE_AS_STRING, swfdec_as_string_charAt, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_charCodeAt, SWFDEC_TYPE_AS_STRING, swfdec_as_string_charCodeAt, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_substr, SWFDEC_TYPE_AS_STRING, swfdec_as_string_substr, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_substring, SWFDEC_TYPE_AS_STRING, swfdec_as_string_substring, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toLowerCase, SWFDEC_TYPE_AS_STRING, swfdec_as_string_toLowerCase, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, SWFDEC_TYPE_AS_STRING, swfdec_as_string_toString, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toUpperCase, SWFDEC_TYPE_AS_STRING, swfdec_as_string_toUpperCase, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_valueOf, SWFDEC_TYPE_AS_STRING, swfdec_as_string_valueOf, 0);
  if (context->version < 6) {
    swfdec_as_object_add_function (proto, SWFDEC_AS_STR_split, SWFDEC_TYPE_AS_STRING, swfdec_as_string_split_5, 1);
  } else {
    swfdec_as_object_add_function (proto, SWFDEC_AS_STR_split, SWFDEC_TYPE_AS_STRING, swfdec_as_string_split, 1);
  }

  /* add properties to global object */
  if (version <= 5) {
    swfdec_as_object_add_function (context->global, SWFDEC_AS_STR_escape, 0, swfdec_as_string_escape, 1);
    swfdec_as_object_add_function (context->global, SWFDEC_AS_STR_unescape, 0, swfdec_as_string_unescape_5, 1);
  } else {
    swfdec_as_object_add_function (context->global, SWFDEC_AS_STR_escape, 0, swfdec_as_string_escape, 1);
    swfdec_as_object_add_function (context->global, SWFDEC_AS_STR_unescape, 0, swfdec_as_string_unescape, 1);
  }
}

