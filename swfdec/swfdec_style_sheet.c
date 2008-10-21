/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_style_sheet.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_array.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_text_format.h"
#include "swfdec_text_field_movie.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_player_internal.h"

enum {
  UPDATE,
  LAST_SIGNAL
};

G_DEFINE_TYPE (SwfdecStyleSheet, swfdec_style_sheet, SWFDEC_TYPE_AS_RELAY)
static guint signals[LAST_SIGNAL] = { 0, };

static void
swfdec_style_sheet_class_init (SwfdecStyleSheetClass *klass)
{
  signals[UPDATE] = g_signal_new ("update", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
}

static void
swfdec_style_sheet_init (SwfdecStyleSheet *style_sheet)
{
}

// Note: This overwrites any old object with the same name
static SwfdecAsObject *
swfdec_style_sheet_get_selector_object (SwfdecAsObject *object,
    const char *name)
{
  SwfdecAsValue val;
  SwfdecAsObject *empty;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  empty = swfdec_as_object_new_empty (swfdec_gc_object_get_context (object));
  SWFDEC_AS_VALUE_SET_OBJECT (&val, empty);
  swfdec_as_object_unset_variable_flags (object, name,
      SWFDEC_AS_VARIABLE_CONSTANT);
  swfdec_as_object_set_variable (object, name, &val);

  return empty;
}

static const char *
swfdec_style_sheet_parse_selectors (SwfdecAsContext *cx, const char *p,
    SwfdecAsObject *object, GPtrArray *selectors)
{
  const char *end;
  const char *name;

  g_return_val_if_fail (p != NULL && p != '\0' && !g_ascii_isspace (*p), NULL);
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);
  g_return_val_if_fail (selectors != NULL, NULL);

  p += strspn (p, " \t\r\n,");
  if (*p == '{')
    return NULL;

  while (*p != '\0' && *p != '{') {
    end = p + strcspn (p, " \t\r\n,{");
    g_assert (end > p);

    name = swfdec_as_context_give_string (cx, g_strndup (p, end - p));
    g_ptr_array_add (selectors,
	swfdec_style_sheet_get_selector_object (object, name));


    p = end + strspn (end, " \t\r\n,");
    if (*p != '{') {
      // no , between selectors?
      if (strchr (end, ',') == NULL || strchr (end, ',') > p)
	return NULL;
    }
  }

  if (*p != '{')
    return NULL;

  p++;
  p = p + strspn (p, " \t\r\n");

  // special case: don't allow empty declarations if not totally empty
  // except in version 8
  if (cx->version < 8 && *(p-1) != '{' && (*p == '\0' || *p == '}'))
    return NULL;

  return p;
}

static char *
swfdec_style_sheet_convert_name (char *name)
{
  char *p;

  p = name;
  while ((p = strchr (p, '-')) != NULL && *(p + 1) != '\0') {
    memmove (p, p + 1, strlen (p + 1) + 1); // include NULL-byte
    *p = g_ascii_toupper (*p);
    p++;
  };

  return name;
}

static const char *
swfdec_style_sheet_parse_property (SwfdecAsContext *cx, const char *p,
    const char **name, const char **value)
{
  const char *end;

  *name = NULL;
  *value = NULL;

  g_return_val_if_fail (p != NULL && p != '\0' && !g_ascii_isspace (*p), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (value != NULL, NULL);

  end = p + strcspn (p, ":;");
  if (*end == '\0' || *end == ';')
    return NULL;

  *name = swfdec_as_context_give_string (cx,
      swfdec_style_sheet_convert_name (g_strndup (p, end - p)));

  end++;
  p = end + strspn (end, " \t\r\n");
  if (*p == '\0')
    return NULL;
  end = p + strcspn (p, ";}");
  if (*end == '\0')
    return NULL;

  if (end == p) {
    *value = SWFDEC_AS_STR_EMPTY;
  } else {
    *value = swfdec_as_context_give_string (cx, g_strndup (p, end - p));
  }

  if (*end == '}') {
    p = end;
  } else {
    end++;
    p = end + strspn (end, " \t\r\n");
  }

  return p;
}

static SwfdecAsObject *
swfdec_style_sheet_parse (SwfdecAsContext *cx, const char *css)
{
  guint i;
  const char *p;
  SwfdecAsValue val;
  SwfdecAsObject *object;
  GPtrArray *selectors;

  g_return_val_if_fail (css != NULL, FALSE);

  object = swfdec_as_object_new_empty (cx);
  selectors = g_ptr_array_new ();

  p = css + strspn (css, " \t\r\n");
  while (p != NULL && *p != '\0')
  {
    if (selectors->len == 0) {
      p = swfdec_style_sheet_parse_selectors (cx, p, object, selectors);
    } else {
      if (*p == '}') {
	g_ptr_array_set_size (selectors, 0);
	p++;
	p += strspn (p, " \t\r\n");
      } else {
	const char *name, *value;
	p = swfdec_style_sheet_parse_property (cx, p, &name, &value);
	if (p != NULL) {
	  for (i = 0; i < selectors->len; i++) {
	    SWFDEC_AS_VALUE_SET_STRING (&val, value);
	    swfdec_as_object_set_variable (
		(SwfdecAsObject *)(selectors->pdata[i]), name, &val);
	  }
	}
      }
    }
  }

  g_ptr_array_free (selectors, TRUE);
  if (p == NULL)
    return NULL;

  return object;
}

SWFDEC_AS_NATIVE (113, 100, swfdec_style_sheet_update)
void
swfdec_style_sheet_update (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecStyleSheet *style;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_STYLE_SHEET, &style, "");

  g_signal_emit (style, signals[UPDATE], 0);
}

SWFDEC_AS_NATIVE (113, 101, swfdec_style_sheet_parseCSSInternal)
void
swfdec_style_sheet_parseCSSInternal (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SwfdecAsObject *values;
  const char *s;

  SWFDEC_AS_CHECK (0, NULL, "s", &s);

  values = swfdec_style_sheet_parse (cx, s);

  if (values == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (rval);
  } else {
    SWFDEC_AS_VALUE_SET_OBJECT (rval, values);
  }
}

SWFDEC_AS_NATIVE (113, 102, swfdec_style_sheet_parseCSSFontFamily)
void
swfdec_style_sheet_parseCSSFontFamily (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  const char *name;

  SWFDEC_AS_CHECK (0, NULL, "s", &name);

  if (!g_ascii_strcasecmp (name, "mono")) {
    SWFDEC_AS_VALUE_SET_STRING (rval, SWFDEC_AS_STR__typewriter);
  } else if (!g_ascii_strcasecmp (name, "sans-serif")) {
    SWFDEC_AS_VALUE_SET_STRING (rval, SWFDEC_AS_STR__sans);
  } else if (!g_ascii_strcasecmp (name, "serif")) {
    SWFDEC_AS_VALUE_SET_STRING (rval, SWFDEC_AS_STR__serif);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (rval, name);
  }
}

SWFDEC_AS_NATIVE (113, 103, swfdec_style_sheet_parseColor)
void
swfdec_style_sheet_parseColor (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *value;
  char *tail;
  gint64 result;

  SWFDEC_AS_VALUE_SET_NULL (rval);

  SWFDEC_AS_CHECK (0, NULL, "s", &value);

  if (strlen(value) != 7)
    return;

  if (value[0] != '#')
    return;

  result = g_ascii_strtoll (value + 1, &tail, 16);
  if (*tail != '\0')
    return;

  swfdec_as_value_set_integer (cx, rval, result);
}

SWFDEC_AS_NATIVE (113, 0, swfdec_style_sheet_construct)
void
swfdec_style_sheet_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecStyleSheet *sheet;

  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  sheet = g_object_new (SWFDEC_TYPE_STYLE_SHEET, "context", cx, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (sheet));
  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

static SwfdecTextFormat *
swfdec_style_sheet_get_format (SwfdecStyleSheet *style, const char *name)
{
  SwfdecAsObject *styles;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_STYLE_SHEET (style), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (style),
      SWFDEC_AS_STR__styles, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return NULL;
  styles = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_get_variable (styles, name, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return NULL;
  if (!SWFDEC_IS_TEXT_FORMAT (SWFDEC_AS_VALUE_GET_OBJECT (&val)))
    return NULL;

  return SWFDEC_TEXT_FORMAT (SWFDEC_AS_VALUE_GET_OBJECT (&val));
}

SwfdecTextFormat *
swfdec_style_sheet_get_tag_format (SwfdecStyleSheet *style, const char *name)
{
  return swfdec_style_sheet_get_format (style, name);
}

SwfdecTextFormat *
swfdec_style_sheet_get_class_format (SwfdecStyleSheet *style, const char *name)
{
  char *name_full;

  g_return_val_if_fail (SWFDEC_IS_STYLE_SHEET (style), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  name_full = g_malloc (1 + strlen (name) + 1);
  name_full[0] = '.';
  memcpy (name_full + 1, name, strlen (name) + 1);

  return swfdec_style_sheet_get_format (style, swfdec_as_context_give_string (
	swfdec_gc_object_get_context (style), name_full));
}
