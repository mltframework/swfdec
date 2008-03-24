/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "vivi_code_constant.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeConstant, vivi_code_constant, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_constant_dispose (GObject *object)
{
  ViviCodeConstant *constant = VIVI_CODE_CONSTANT (object);

  if (SWFDEC_AS_VALUE_IS_STRING (&constant->value))
    g_free ((char *) SWFDEC_AS_VALUE_GET_STRING (&constant->value));

  G_OBJECT_CLASS (vivi_code_constant_parent_class)->dispose (object);
}

static char *
escape_string (const char *s)
{
  GString *str;
  char *next;

  str = g_string_new ("\"");
  while ((next = strpbrk (s, "\"\n"))) {
    g_string_append_len (str, s, next - s);
    switch (*next) {
      case '"':
	g_string_append (str, "\"");
	break;
      case '\n':
	g_string_append (str, "\n");
	break;
      default:
	g_assert_not_reached ();
    }
    s = next + 1;
  }
  g_string_append (str, s);
  g_string_append_c (str, '"');
  return g_string_free (str, FALSE);
}

static void
vivi_code_constant_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeConstant *constant = VIVI_CODE_CONSTANT (token);
  char *s;

  switch (constant->value.type) {
    case SWFDEC_AS_TYPE_UNDEFINED:
      vivi_code_printer_print (printer, "undefined");
      break;
    case SWFDEC_AS_TYPE_NULL:
      vivi_code_printer_print (printer, "null");
      break;
    case SWFDEC_AS_TYPE_BOOLEAN:
      vivi_code_printer_print (printer, SWFDEC_AS_VALUE_GET_BOOLEAN (&constant->value) ? "true" : "false");
      break;
    case SWFDEC_AS_TYPE_NUMBER:
      s = g_strdup_printf ("%.16g", SWFDEC_AS_VALUE_GET_NUMBER (&constant->value));
      vivi_code_printer_print (printer, s);
      g_free (s);
      break;
    case SWFDEC_AS_TYPE_STRING:
      s = escape_string (SWFDEC_AS_VALUE_GET_STRING (&constant->value));
      vivi_code_printer_print (printer, s);
      g_free (s);
      break;
    case SWFDEC_AS_TYPE_INT:
    case SWFDEC_AS_TYPE_OBJECT:
    default:
      g_assert_not_reached ();
  }
}

static gboolean
vivi_code_constant_is_constant (ViviCodeValue *value)
{
  return TRUE;
}

static void
vivi_code_constant_class_init (ViviCodeConstantClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeValueClass *value_class = VIVI_CODE_VALUE_CLASS (klass);

  object_class->dispose = vivi_code_constant_dispose;

  token_class->print = vivi_code_constant_print;

  value_class->is_constant = vivi_code_constant_is_constant;
}

static void
vivi_code_constant_init (ViviCodeConstant *constant)
{
  vivi_code_value_set_precedence (VIVI_CODE_VALUE (constant), VIVI_PRECEDENCE_MAX);
}

static ViviCodeValue *
vivi_code_constant_new (const SwfdecAsValue *value)
{
  ViviCodeConstant *constant;

  constant = g_object_new (VIVI_TYPE_CODE_CONSTANT, NULL);
  constant->value = *value;

  return VIVI_CODE_VALUE (constant);
}

ViviCodeValue *
vivi_code_constant_new_undefined (void)
{
  SwfdecAsValue val = { 0, };
  return vivi_code_constant_new (&val);
}

ViviCodeValue *
vivi_code_constant_new_null (void)
{
  SwfdecAsValue val;

  SWFDEC_AS_VALUE_SET_NULL (&val);
  return vivi_code_constant_new (&val);
}

ViviCodeValue *
vivi_code_constant_new_string (const char *string)
{
  SwfdecAsValue val;

  g_return_val_if_fail (string != NULL, NULL);

  SWFDEC_AS_VALUE_SET_STRING (&val, g_strdup (string));
  return vivi_code_constant_new (&val);
}

ViviCodeValue *
vivi_code_constant_new_number (double number)
{
  SwfdecAsValue val;

  SWFDEC_AS_VALUE_SET_NUMBER (&val, number);
  return vivi_code_constant_new (&val);
}

ViviCodeValue *
vivi_code_constant_new_boolean (gboolean boolean)
{
  SwfdecAsValue val;

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, boolean);
  return vivi_code_constant_new (&val);
}

char *
vivi_code_constant_get_variable_name (ViviCodeConstant *constant)
{
  static const char *accept = G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "_$";

  g_return_val_if_fail (VIVI_IS_CODE_CONSTANT (constant), NULL);

  switch (constant->value.type) {
    case SWFDEC_AS_TYPE_UNDEFINED:
      return g_strdup ("undefined");
    case SWFDEC_AS_TYPE_NULL:
      return g_strdup ("null");
      break;
    case SWFDEC_AS_TYPE_BOOLEAN:
      return g_strdup (SWFDEC_AS_VALUE_GET_BOOLEAN (&constant->value) ? "true" : "false");
      break;
    case SWFDEC_AS_TYPE_STRING:
      {
	const char *s = SWFDEC_AS_VALUE_GET_STRING (&constant->value);
	gsize len = strspn (s, accept);
	if (s[len] == '\0')
	  return g_strdup (s);
	else
	  return NULL;
      }
    case SWFDEC_AS_TYPE_NUMBER:
      return NULL;
    case SWFDEC_AS_TYPE_INT:
    case SWFDEC_AS_TYPE_OBJECT:
    default:
      g_assert_not_reached ();
  }
}

SwfdecAsValueType
vivi_code_constant_get_value_type (ViviCodeConstant *constant)
{
  g_return_val_if_fail (VIVI_IS_CODE_CONSTANT (constant), SWFDEC_AS_TYPE_UNDEFINED);

  return constant->value.type;
}

double
vivi_code_constant_get_number (ViviCodeConstant *constant)
{
  g_return_val_if_fail (VIVI_IS_CODE_CONSTANT (constant), NAN);

  if (SWFDEC_AS_VALUE_IS_NUMBER (&constant->value))
    return SWFDEC_AS_VALUE_GET_NUMBER (&constant->value);
  else
    return NAN;
}

