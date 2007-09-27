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

#include "swfdec_style_sheet.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_array.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecStyleSheet, swfdec_style_sheet, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_style_sheet_class_init (SwfdecStyleSheetClass *klass)
{
}

static void
swfdec_style_sheet_init (SwfdecStyleSheet *style_sheet)
{
}

static SwfdecAsObject *
swfdec_style_sheet_get_selector_object (SwfdecAsObject *object,
    const char *name)
{
  SwfdecAsValue val;
  SwfdecAsObject *empty;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  swfdec_as_object_get_variable (object, name, &val);

  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return SWFDEC_AS_VALUE_GET_OBJECT (&val);

  empty = swfdec_as_object_new_empty (object->context);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, empty);
  // FIXME: unset flags?
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

  if (*p == '{')
    return NULL;

  while (*p != '\0' && *p != '{') {
    end = p + strcspn (p, " \t\r\n,{");
    g_assert (end > p);

    name = swfdec_as_context_give_string (cx, g_strndup (p, end - p));
    g_ptr_array_add (selectors,
	swfdec_style_sheet_get_selector_object (object, name));

    p = end + strspn (end, " \t\r\n,");
  }

  if (*p != '{')
    return NULL;

  p++;
  p = p + strspn (p, " \t\r\n");

  // special case: don't allow empty declarations if not totally empty
  if (*(p-1) != '{' && (*p == '\0' || *p == '}'))
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

  end = strchr (p, ':');
  if (end == NULL)
    return NULL;

  *name = swfdec_as_context_give_string (cx,
      swfdec_style_sheet_convert_name (g_strndup (p, end - p)));

  end++;
  p = end + strspn (end, " \t\r\n");
  if (p == '\0')
    return NULL;
  end = p + strcspn (p, ";}");
  while (g_ascii_isspace (*(end-1))) {
    end--;
  }

  if (end == p) {
    *value = SWFDEC_AS_STR_EMPTY;
  } else {
    *value = swfdec_as_context_give_string (cx, g_strndup (p, end - p));
  }

  p = end + strspn (end, " \t\r\n");
  p++;
  p += strspn (p, " \t\r\n");

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
	selectors->len = 0;
	p++;
	p += strspn (p, " \t\r\n");
      } else {
	const char *name, *value;
	p = swfdec_style_sheet_parse_property (cx, p, &name, &value);
	for (i = 0; i < selectors->len; i++) {
	  SWFDEC_AS_VALUE_SET_STRING (&val, value);
	  swfdec_as_object_set_variable (
	      (SwfdecAsObject *)(selectors->pdata[i]), name, &val);
	}
      }
    }
  }

  if (p == NULL)
    return NULL;

  return object;
}

SWFDEC_AS_NATIVE (113, 101, swfdec_style_sheet_parseCSSInternal)
void
swfdec_style_sheet_parseCSSInternal (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SwfdecAsObject *values;

  if (argc < 1)
    return;

  values =
    swfdec_style_sheet_parse (cx, swfdec_as_value_to_string (cx, &argv[0]));

  if (values == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (rval);
  } else {
    SWFDEC_AS_VALUE_SET_OBJECT (rval, values);
  }
}

SWFDEC_AS_CONSTRUCTOR (113, 0, swfdec_style_sheet_construct, swfdec_style_sheet_get_type)
void
swfdec_style_sheet_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  g_assert (SWFDEC_IS_STYLESHEET (object));
}
