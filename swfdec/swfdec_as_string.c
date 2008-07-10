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
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsString, swfdec_as_string, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_string_do_mark (SwfdecAsObject *object)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);

  swfdec_as_string_mark (string->string);

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_string_parent_class)->mark (object);
}

static char *
swfdec_as_string_debug (SwfdecAsObject *object)
{
  SwfdecAsString *string = SWFDEC_AS_STRING (object);

  return g_strdup (string->string);
}

static void
swfdec_as_string_class_init (SwfdecAsStringClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  asobject_class->mark = swfdec_as_string_do_mark;
  asobject_class->debug = swfdec_as_string_debug;
}

static void
swfdec_as_string_init (SwfdecAsString *string)
{
  string->string = SWFDEC_AS_STR_EMPTY;
}

/*** AS CODE ***/

static const char *
swfdec_as_string_object_to_string (SwfdecAsContext *context,
    SwfdecAsObject *object)
{
  SwfdecAsValue val;

  g_return_val_if_fail (object == NULL || SWFDEC_IS_AS_OBJECT (object),
      SWFDEC_AS_STR_EMPTY);

  if (object == NULL) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  } else {
    SWFDEC_AS_VALUE_SET_OBJECT (&val, object);
  }

  return swfdec_as_value_to_string (context, &val);
}

static const char *
swfdec_as_str_nth_char (const char *s, guint n)
{
  while (*s && n--)
    s = g_utf8_next_char (s);
  return s;
}

SWFDEC_AS_NATIVE (251, 9, swfdec_as_string_lastIndexOf)
void
swfdec_as_string_lastIndexOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  gsize len;
  const char *s;

  if (argc < 1)
    return;

  s = swfdec_as_value_to_string (cx, &argv[0]);
  if (argc == 2) {
    int offset = swfdec_as_value_to_integer (cx, &argv[1]);
    if (offset < 0) {
      SWFDEC_AS_VALUE_SET_INT (ret, -1);
      return;
    }
    len = g_utf8_offset_to_pointer (string, offset + 1) - string;
  } else {
    len = G_MAXSIZE;
  }
  s = g_strrstr_len (string, len, s);
  if (s) {
    SWFDEC_AS_VALUE_SET_INT (ret, g_utf8_pointer_to_offset (string, s));
  } else {
    SWFDEC_AS_VALUE_SET_INT (ret, -1);
  }
}

SWFDEC_AS_NATIVE (251, 8, swfdec_as_string_indexOf)
void
swfdec_as_string_indexOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  int offset=0, len, i=-1;
  const char *s, *t = NULL;

  if (argc < 1)
    return;

  s = swfdec_as_value_to_string (cx, &argv[0]);
  if (argc == 2)
    offset = swfdec_as_value_to_integer (cx, &argv[1]);
  if (offset < 0)
    offset = 0;
  len = g_utf8_strlen (string, -1);
  if (offset < len) {
    t = strstr (g_utf8_offset_to_pointer (string, offset), s);
  }
  if (t != NULL) {
    i = g_utf8_pointer_to_offset (string, t);
  }

  SWFDEC_AS_VALUE_SET_INT (ret, i);
}

SWFDEC_AS_NATIVE (251, 5, swfdec_as_string_charAt)
void
swfdec_as_string_charAt (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  int i;
  const char *s, *t;

  if (argc < 1)
    return;

  i = swfdec_as_value_to_integer (cx, &argv[0]);
  if (i < 0) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
    return;
  }
  s = swfdec_as_str_nth_char (string, i);
  if (*s == 0) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
    return;
  }
  t = g_utf8_next_char (s);
  s = swfdec_as_context_give_string (cx, g_strndup (s, t - s));
  SWFDEC_AS_VALUE_SET_STRING (ret, s);
}

SWFDEC_AS_NATIVE (251, 6, swfdec_as_string_charCodeAt)
void
swfdec_as_string_charCodeAt (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  int i;
  const char *s;
  gunichar c;

  if (argc < 1)
    return;

  i = swfdec_as_value_to_integer (cx, &argv[0]);
  if (i < 0) {
    SWFDEC_AS_VALUE_SET_NUMBER (ret, NAN);
    return;
  }
  s = swfdec_as_str_nth_char (string, i);
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

  if (argc > 0) {
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
    s = g_convert ((char *) array->data, array->len, "UTF-8", "LATIN1", NULL, NULL, &error);
  } else{
    s = g_strdup ("");
  }

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
swfdec_as_string_fromCharCode_6 (SwfdecAsContext *cx, SwfdecAsObject *object,
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

SWFDEC_AS_NATIVE (251, 14, swfdec_as_string_fromCharCode)
void
swfdec_as_string_fromCharCode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (cx->version <= 5) {
    swfdec_as_string_fromCharCode_5 (cx, object, argc, argv, ret);
  } else {
    swfdec_as_string_fromCharCode_6 (cx, object, argc, argv, ret);
  }
}

SWFDEC_AS_CONSTRUCTOR (251, 0, swfdec_as_string_construct, swfdec_as_string_get_type)
void
swfdec_as_string_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *s;

  if (argc > 0) {
    s = swfdec_as_value_to_string (cx, &argv[0]);
  } else {
    s = SWFDEC_AS_STR_EMPTY;
  }

  if (swfdec_as_context_is_constructing (cx)) {
    SwfdecAsString *string = SWFDEC_AS_STRING (object);
    SwfdecAsValue val;

    string->string = s;
    SWFDEC_AS_VALUE_SET_INT (&val, g_utf8_strlen (string->string, -1));
    swfdec_as_object_set_variable_and_flags (object, SWFDEC_AS_STR_length,
	&val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
    SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (ret, s);
  }
}

SWFDEC_AS_NATIVE (251, 2, swfdec_as_string_toString)
void
swfdec_as_string_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_AS_STRING (object))
    return;

  SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STRING (object)->string);
}

SWFDEC_AS_NATIVE (251, 1, swfdec_as_string_valueOf)
void
swfdec_as_string_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_AS_STRING (object))
    return;

  SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STRING (object)->string);
}

static void
swfdec_as_string_split_5 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsArray *arr;
  SwfdecAsValue val;
  const char *str, *end, *delim;
  int count;

  str = swfdec_as_string_object_to_string (cx, object);
  arr = SWFDEC_AS_ARRAY (swfdec_as_array_new (cx));
  if (arr == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (arr));
  /* hi, i'm the special case */
  if (argc < 1 || SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0])) {
    delim = SWFDEC_AS_STR_COMMA;
  } else {
    delim = swfdec_as_value_to_string (cx, &argv[0]);
  }
  if (delim == SWFDEC_AS_STR_EMPTY) {
    SWFDEC_AS_VALUE_SET_STRING (&val, str);
    swfdec_as_array_push (arr, &val);
    return;
  }
  if (argc > 1 && !SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[1])) {
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
  while (count > 0) {
    end = strchr (str, delim[0]);
    if (end == NULL) {
      SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (cx, str));
      swfdec_as_array_push (arr, &val);
      break;
    }
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_give_string (cx, g_strndup (str, end - str)));
    swfdec_as_array_push (arr, &val);
    count--;
    str = end + 1;
  }
}

static void
swfdec_as_string_split_6 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsArray *arr;
  SwfdecAsValue val;
  const char *str, *end, *delim;
  int count;
  guint len;

  str = swfdec_as_string_object_to_string (cx, object);
  arr = SWFDEC_AS_ARRAY (swfdec_as_array_new (cx));
  if (arr == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (arr));
  /* hi, i'm the special case */
  if (argc < 1 || SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0])) {
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
  if (argc > 1 && !SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[1]))
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
    count--;
    str = end + len;
  }
}

SWFDEC_AS_NATIVE (251, 12, swfdec_as_string_split)
void
swfdec_as_string_split (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (cx->version <= 5) {
    swfdec_as_string_split_5 (cx, object, argc, argv, ret);
  } else {
    swfdec_as_string_split_6 (cx, object, argc, argv, ret);
  }
}

SWFDEC_AS_NATIVE (251, 10, swfdec_as_string_slice)
void
swfdec_as_string_slice (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  int start, end, length;
  const char *str;

  if (argc == 0)
    return;

  str = swfdec_as_string_object_to_string (cx, object);
  length = strlen (str);

  start = swfdec_as_value_to_integer (cx, &argv[0]);
  if (start < 0)
    start += length;
  start = CLAMP (start, 0, length);

  if (argc > 1) {
    end = swfdec_as_value_to_integer (cx, &argv[1]);
    if (end < 0)
      end += length;
    end = CLAMP (end, start, length);
  } else {
    end = length;
  }

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_as_context_give_string (cx, g_strndup (str + start, end - start)));
}

SWFDEC_AS_NATIVE (251, 7, swfdec_as_string_concat)
void
swfdec_as_string_concat (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  guint i;
  GString *string;

  string = g_string_new (swfdec_as_string_object_to_string (cx, object));

  for (i = 0; i < argc; i++) {
    string = g_string_append (string, swfdec_as_value_to_string (cx, &argv[i]));
  }

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_as_context_give_string (cx, g_string_free (string, FALSE)));
}

const char *
swfdec_as_str_sub (SwfdecAsContext *cx, const char *str, guint offset, guint len)
{
  const char *end;

  str = g_utf8_offset_to_pointer (str, offset);
  end = g_utf8_offset_to_pointer (str, len);
  str = swfdec_as_context_give_string (cx, g_strndup (str, end - str));
  return str;
}

SWFDEC_AS_NATIVE (251, 13, swfdec_as_string_substr)
void
swfdec_as_string_substr (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  int from, to, len;

  if (argc < 1)
    return;

  from = swfdec_as_value_to_integer (cx, &argv[0]);
  len = g_utf8_strlen (string, -1);
  
  if (argc > 1 && !SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[1])) {
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
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_str_sub (cx, string, from, to));
}

SWFDEC_AS_NATIVE (251, 11, swfdec_as_string_substring)
void
swfdec_as_string_substring (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  int from, to, len;

  if (argc < 1)
    return;

  len = g_utf8_strlen (string, -1);
  from = swfdec_as_value_to_integer (cx, &argv[0]);
  if (argc > 1 && !SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[1])) {
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
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_str_sub (cx, string, from, to - from));
}

SWFDEC_AS_NATIVE (251, 4, swfdec_as_string_toLowerCase)
void
swfdec_as_string_toLowerCase (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  char *s;

  s = g_utf8_strdown (string, -1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, s));
}

SWFDEC_AS_NATIVE (251, 3, swfdec_as_string_toUpperCase)
void
swfdec_as_string_toUpperCase (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  char *s;

  s = g_utf8_strup (string, -1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, s));
}

// only available as ASnative
SWFDEC_AS_NATIVE (102, 1, swfdec_as_string_old_toLowerCase)
void
swfdec_as_string_old_toLowerCase (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  char *s;

  s = g_ascii_strdown (string, -1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, s));
}

// only available as ASnative
SWFDEC_AS_NATIVE (102, 0, swfdec_as_string_old_toUpperCase)
void
swfdec_as_string_old_toUpperCase (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *string = swfdec_as_string_object_to_string (cx, object);
  char *s;

  s = g_ascii_strup (string, -1);
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, s));
}

/* escape and unescape are implemented here so the mad string functions share the same place */

static char *
swfdec_as_string_unescape_5 (SwfdecAsContext *cx, const char *msg)
{
  GByteArray *array;
  char cur = 0; /* currently decoded character */
  char *out, *in, *s;
  guint decoding = 0; /* set if we're decoding a %XY string */

/* attention: c is a char* */
#define APPEND(chr) G_STMT_START{ \
  g_byte_array_append (array, (guchar *) chr, 1); \
}G_STMT_END
  in = s = g_convert (msg, -1, "LATIN1", "UTF-8", NULL, NULL, NULL);
  if (s == NULL) {
    SWFDEC_FIXME ("%s can not be converted to utf8 - is this Flash 5 or what?", msg);
    return NULL;
  }
  array = g_byte_array_new ();
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
    g_byte_array_free (array, TRUE);
    return NULL;
  }
  cur = 0;
  g_byte_array_append (array, (guchar *) &cur, 1);
  out = g_convert ((char *) array->data, -1, "UTF-8", "LATIN1", NULL, NULL, NULL);
  g_byte_array_free (array, TRUE);
  if (out) {
    return out;
  } else {
    g_warning ("can't convert %s to UTF-8", msg);
    g_free (out);
    return g_strdup ("");
  }
#undef APPEND
}

char *
swfdec_as_string_escape (SwfdecAsContext *cx, const char *s)
{
  GByteArray *array;
  char *in = NULL;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (cx), NULL);
  g_return_val_if_fail (s != NULL, NULL);

  array = g_byte_array_new ();
  if (cx->version <= 5) {
    in = g_convert (s, -1, "LATIN1", "UTF-8", NULL, NULL, NULL);
    if (s == NULL) {
      SWFDEC_FIXME ("%s can not be converted to utf8 - is this Flash 5 or what?", s);
      return NULL;
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
  g_free (in);
  return (char *) g_byte_array_free (array, FALSE);
}

SWFDEC_AS_NATIVE (100, 0, swfdec_as_string_escape_internal)
void
swfdec_as_string_escape_internal (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *s;
  char *result;

  SWFDEC_AS_CHECK (0, NULL, "s", &s);

  result = swfdec_as_string_escape (cx, s);
  if (result != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, result));
    g_free (result);
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (ret);
  }
}

static char *
swfdec_as_string_unescape_6 (SwfdecAsContext *cx, const char *s)
{
  GByteArray *array;
  const char *msg;
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
  msg = s;
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
    return (char *) g_byte_array_free (array, FALSE);
  } else {
    g_warning ("%s unescaped is invalid UTF-8", msg);
    g_byte_array_free (array, TRUE);
    return g_strdup ("");
  }
#undef APPEND
}

char *
swfdec_as_string_unescape (SwfdecAsContext *context, const char *string)
{
  if (context->version < 6) {
    return swfdec_as_string_unescape_5 (context, string);
  } else {
    return swfdec_as_string_unescape_6 (context, string);
  }
}

SWFDEC_AS_NATIVE (100, 1, swfdec_as_string_unescape_internal)
void
swfdec_as_string_unescape_internal (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *s;
  char *result;

  SWFDEC_AS_CHECK (0, NULL, "s", &s);

  result = swfdec_as_string_unescape (cx, s);
  if (result != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, result));
    g_free (result);
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (ret);
  }
}

// only available as ASnative
SWFDEC_AS_NATIVE (3, 0, swfdec_as_string_old_constructor)
void
swfdec_as_string_old_constructor (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("old 'String' function (only available as ASnative)");
}
