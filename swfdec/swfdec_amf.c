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

#ifndef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_amf.h"
#include "swfdec_as_array.h"
#include "swfdec_as_date.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

typedef gboolean (* SwfdecAmfParseFunc) (SwfdecAsContext *cx, SwfdecBits *bits, SwfdecAsValue *val);
extern const SwfdecAmfParseFunc parse_funcs[SWFDEC_AMF_N_TYPES];

static gboolean
swfdec_amf_parse_boolean (SwfdecAsContext *context, SwfdecBits *bits, SwfdecAsValue *val)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (val, swfdec_bits_get_u8 (bits) ? TRUE : FALSE);
  return TRUE;
}

static gboolean
swfdec_amf_parse_number (SwfdecAsContext *context, SwfdecBits *bits, SwfdecAsValue *val)
{
  swfdec_as_value_set_number (context, val, swfdec_bits_get_bdouble (bits));
  return TRUE;
}

static gboolean
swfdec_amf_parse_string (SwfdecAsContext *context, SwfdecBits *bits, SwfdecAsValue *val)
{
  guint len = swfdec_bits_get_bu16 (bits);
  char *s;
  
  /* FIXME: the supplied version is likely incorrect */
  s = swfdec_bits_get_string_length (bits, len, context->version);
  if (s == NULL)
    return FALSE;
  SWFDEC_AS_VALUE_SET_STRING (val, swfdec_as_context_give_string (context, s));
  return TRUE;
}

static gboolean
swfdec_amf_parse_properties (SwfdecAsContext *context, SwfdecBits *bits, SwfdecAsObject *object)
{
  guint type;
  SwfdecAmfParseFunc func;

  while (swfdec_bits_left (bits)) {
    SwfdecAsValue val;
    const char *name;

    if (!swfdec_amf_parse_string (context, bits, &val))
      return FALSE;
    name = SWFDEC_AS_VALUE_GET_STRING (&val);
    type = swfdec_bits_get_u8 (bits);
    if (type == SWFDEC_AMF_END_OBJECT)
      break;
    if (type >= SWFDEC_AMF_N_TYPES ||
	(func = parse_funcs[type]) == NULL) {
      SWFDEC_ERROR ("no parse func for AMF type %u", type);
      goto error;
    }
    swfdec_as_object_set_variable (object, name, &val); /* GC... */
    if (!func (context, bits, &val)) {
      goto error;
    }
    swfdec_as_object_set_variable (object, name, &val);
  }
  /* no more bytes seems to end automatically */
  return TRUE;

error:
  return FALSE;
}

static gboolean
swfdec_amf_parse_object (SwfdecAsContext *context, SwfdecBits *bits, SwfdecAsValue *val)
{
  SwfdecAsObject *object;
  
  object = swfdec_as_object_new (context, SWFDEC_AS_STR_Object, NULL);
  if (!swfdec_amf_parse_properties (context, bits, object))
    return FALSE;
  SWFDEC_AS_VALUE_SET_COMPOSITE (val, object);
  return TRUE;
}

static gboolean
swfdec_amf_parse_mixed_array (SwfdecAsContext *context, SwfdecBits *bits, SwfdecAsValue *val)
{
  guint len;
  SwfdecAsObject *array;
  
  len = swfdec_bits_get_bu32 (bits);
  array = swfdec_as_array_new (context);
  if (!swfdec_amf_parse_properties (context, bits, array))
    return FALSE;
  SWFDEC_AS_VALUE_SET_COMPOSITE (val, array);
  return TRUE;
}

static gboolean
swfdec_amf_parse_array (SwfdecAsContext *context, SwfdecBits *bits, SwfdecAsValue *val)
{
  guint i, len;
  SwfdecAsObject *array;
  guint type;
  SwfdecAmfParseFunc func;
  
  len = swfdec_bits_get_bu32 (bits);
  array = swfdec_as_array_new (context);
  for (i = 0; i < len; i++) {
    SwfdecAsValue tmp;
    type = swfdec_bits_get_u8 (bits);
    if (type >= SWFDEC_AMF_N_TYPES ||
	(func = parse_funcs[type]) == NULL) {
      SWFDEC_ERROR ("no parse func for AMF type %u", type);
      goto fail;
    }
    if (!func (context, bits, &tmp))
      goto fail;
    swfdec_as_array_push (array, &tmp);
  }

  SWFDEC_AS_VALUE_SET_COMPOSITE (val, array);
  return TRUE;

fail:
  return FALSE;
}

// FIXME: untested
static gboolean
swfdec_amf_parse_date (SwfdecAsContext *context, SwfdecBits *bits, SwfdecAsValue *val)
{
  SwfdecAsDate *date;
  SwfdecAsObject *object;

  object = swfdec_as_object_new (context, SWFDEC_AS_STR_Date, NULL);
  date = SWFDEC_AS_DATE (object->relay);
  date->milliseconds = swfdec_bits_get_bdouble (bits);
  date->utc_offset = swfdec_bits_get_bu16 (bits);
  if (date->utc_offset > 12 * 60)
    date->utc_offset -= 12 * 60;
  SWFDEC_AS_VALUE_SET_COMPOSITE (val, object);

  return TRUE;
}

const SwfdecAmfParseFunc parse_funcs[SWFDEC_AMF_N_TYPES] = {
  [SWFDEC_AMF_NUMBER] = swfdec_amf_parse_number,
  [SWFDEC_AMF_BOOLEAN] = swfdec_amf_parse_boolean,
  [SWFDEC_AMF_STRING] = swfdec_amf_parse_string,
  [SWFDEC_AMF_OBJECT] = swfdec_amf_parse_object,
  [SWFDEC_AMF_MIXED_ARRAY] = swfdec_amf_parse_mixed_array,
  [SWFDEC_AMF_ARRAY] = swfdec_amf_parse_array,
  [SWFDEC_AMF_DATE] = swfdec_amf_parse_date,
#if 0
  SWFDEC_AMF_MOVIECLIP = 4,
  SWFDEC_AMF_NULL = 5,
  SWFDEC_AMF_UNDEFINED = 6,
  SWFDEC_AMF_REFERENCE = 7,
  SWFDEC_AMF_END_OBJECT = 9,
  SWFDEC_AMF_BIG_STRING = 12,
  SWFDEC_AMF_RECORDSET = 14,
  SWFDEC_AMF_XML = 15,
  SWFDEC_AMF_CLASS = 16,
  SWFDEC_AMF_FLASH9 = 17,
#endif
};

gboolean
swfdec_amf_parse_one (SwfdecAsContext *context, SwfdecBits *bits, 
    SwfdecAmfType expected_type, SwfdecAsValue *rval)
{
  SwfdecAmfParseFunc func;
  guint type;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), 0);
  g_return_val_if_fail (bits != NULL, FALSE);
  g_return_val_if_fail (rval != NULL, FALSE);
  g_return_val_if_fail (expected_type < SWFDEC_AMF_N_TYPES, FALSE);

  type = swfdec_bits_get_u8 (bits);
  if (type != expected_type) {
    SWFDEC_ERROR ("parse object should be type %u, but is %u", 
	expected_type, type);
    return FALSE;
  }
  if (type >= SWFDEC_AMF_N_TYPES ||
      (func = parse_funcs[type]) == NULL) {
    SWFDEC_ERROR ("no parse func for AMF type %u", type);
    return FALSE;
  }
  return func (context, bits, rval);
}

guint
swfdec_amf_parse (SwfdecAsContext *context, SwfdecBits *bits, guint n_items, ...)
{
  va_list args;
  guint i;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), 0);
  g_return_val_if_fail (bits != NULL, 0);

  va_start (args, n_items);
  for (i = 0; i < n_items; i++) {
    SwfdecAmfType type = va_arg (args, SwfdecAmfType);
    SwfdecAsValue *val = va_arg (args, SwfdecAsValue *);
    if (!swfdec_amf_parse_one (context, bits, type, val))
      break;
  }
  va_end (args);
  return i;
}

