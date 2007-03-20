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
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "js/jsapi.h"

typedef gboolean (* SwfdecAmfParseFunc) (JSContext *cx, SwfdecBits *bits, jsval *val);
extern const SwfdecAmfParseFunc parse_funcs[SWFDEC_AMF_N_TYPES];

static gboolean
swfdec_amf_parse_boolean (JSContext *cx, SwfdecBits *bits, jsval *val)
{
  *val = swfdec_bits_get_u8 (bits) ? JSVAL_TRUE : JSVAL_FALSE;
  return TRUE;
}

static gboolean
swfdec_amf_parse_number (JSContext *cx, SwfdecBits *bits, jsval *val)
{
  double d = swfdec_bits_get_bdouble (bits);

  if (!JS_NewNumberValue (cx, d, val))
    return FALSE;
  return TRUE;
}

static gboolean
swfdec_amf_parse_string (JSContext *cx, SwfdecBits *bits, jsval *val)
{
  guint len = swfdec_bits_get_bu16 (bits);
  char *s;
  JSString *string;
  
  s = swfdec_bits_get_string_length (bits, len);
  if (s == NULL)
    return FALSE;

  string = JS_NewStringCopyZ (cx, s);
  g_free (s);
  if (!string)
    return FALSE;
  *val = STRING_TO_JSVAL (string);
  return TRUE;
}
static gboolean
swfdec_amf_parse_properties (JSContext *cx, SwfdecBits *bits, jsval *val)
{
  guint type;
  SwfdecAmfParseFunc func;
  JSObject *object;

  g_assert (JSVAL_IS_OBJECT (*val));
  object = JSVAL_TO_OBJECT (*val);

  while (swfdec_bits_left (bits)) {
    jsval id, val;
    if (!swfdec_amf_parse_string (cx, bits, &id))
      return FALSE;
    type = swfdec_bits_get_u8 (bits);
    if (type == SWFDEC_AMF_END_OBJECT)
      return TRUE;
    if (type >= SWFDEC_AMF_N_TYPES ||
	(func = parse_funcs[type]) == NULL) {
      SWFDEC_ERROR ("no parse func for AMF type %u", type);
      return FALSE;
    }
    if (!func (cx, bits, &val))
      return FALSE;
    if (!JS_SetProperty (cx, object, JS_GetStringBytes (JSVAL_TO_STRING (id)), &val))
      return FALSE;
  }
  /* no more bytes seems to end automatically */
  return TRUE;
}

static gboolean
swfdec_amf_parse_object (JSContext *cx, SwfdecBits *bits, jsval *val)
{
  JSObject *object;
  
  object = JS_NewObject (cx, NULL, NULL, NULL);
  if (object == NULL)
    return FALSE;

  *val = OBJECT_TO_JSVAL (object);
  return swfdec_amf_parse_properties (cx, bits, val);
}

static gboolean
swfdec_amf_parse_mixed_array (JSContext *cx, SwfdecBits *bits, jsval *val)
{
  guint len;
  JSObject *object;
  
  len = swfdec_bits_get_bu32 (bits);
  object = JS_NewArrayObject (cx, len, NULL);
  if (object == NULL)
    return FALSE;

  *val = OBJECT_TO_JSVAL (object);
  return swfdec_amf_parse_properties (cx, bits, val);
}

static gboolean
swfdec_amf_parse_array (JSContext *cx, SwfdecBits *bits, jsval *val)
{
  guint i, len;
  JSObject *object;
  jsval *vector;
  guint type;
  SwfdecAmfParseFunc func;
  
  len = swfdec_bits_get_bu32 (bits);
  vector = g_try_new (jsval, len);
  if (vector == NULL)
    return FALSE;
  for (i = 0; i < len; i++) {
    type = swfdec_bits_get_u8 (bits);
    if (type >= SWFDEC_AMF_N_TYPES ||
	(func = parse_funcs[type]) == NULL) {
      SWFDEC_ERROR ("no parse func for AMF type %u", type);
      goto fail;
    }
    if (!func (cx, bits, &vector[i]))
      goto fail;
  }
  object = JS_NewArrayObject (cx, len, vector);
  if (object == NULL)
    goto fail;

  *val = OBJECT_TO_JSVAL (object);
  g_free (vector);
  return TRUE;

fail:
  g_free (vector);
  return FALSE;
}

const SwfdecAmfParseFunc parse_funcs[SWFDEC_AMF_N_TYPES] = {
  [SWFDEC_AMF_NUMBER] = swfdec_amf_parse_number,
  [SWFDEC_AMF_BOOLEAN] = swfdec_amf_parse_boolean,
  [SWFDEC_AMF_STRING] = swfdec_amf_parse_string,
  [SWFDEC_AMF_OBJECT] = swfdec_amf_parse_object,
  [SWFDEC_AMF_MIXED_ARRAY] = swfdec_amf_parse_mixed_array,
  [SWFDEC_AMF_ARRAY] = swfdec_amf_parse_array,
#if 0
  SWFDEC_AMF_MOVIECLIP = 4,
  SWFDEC_AMF_NULL = 5,
  SWFDEC_AMF_UNDEFINED = 6,
  SWFDEC_AMF_REFERENCE = 7,
  SWFDEC_AMF_END_OBJECT = 9,
  SWFDEC_AMF_ARRAY = 10,
  SWFDEC_AMF_DATE = 11,
  SWFDEC_AMF_BIG_STRING = 12,
  SWFDEC_AMF_RECORDSET = 14,
  SWFDEC_AMF_XML = 15,
  SWFDEC_AMF_CLASS = 16,
  SWFDEC_AMF_FLASH9 = 17,
#endif
};

gboolean
swfdec_amf_parse_one (JSContext *cx, SwfdecBits *bits, SwfdecAmfType expected_type, 
    jsval *rval)
{
  SwfdecAmfParseFunc func;
  guint type;

  g_return_val_if_fail (cx != NULL, FALSE);
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
  return func (cx, bits, rval);
}

guint
swfdec_amf_parse (JSContext *cx, SwfdecBits *bits, guint n_items, ...)
{
  va_list args;
  guint i;

  g_return_val_if_fail (cx != NULL, 0);
  g_return_val_if_fail (bits != NULL, 0);

  va_start (args, n_items);
  for (i = 0; i < n_items; i++) {
    SwfdecAmfType type = va_arg (args, SwfdecAmfType);
    jsval *val = va_arg (args, jsval *);
    if (!swfdec_amf_parse_one (cx, bits, type, val))
      break;
  }
  va_end (args);
  return i;
}

