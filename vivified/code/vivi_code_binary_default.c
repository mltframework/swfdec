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

#include <swfdec/swfdec_as_interpret.h>

#include "vivi_code_binary_default.h"
#include "vivi_code_asm_code_default.h"


#define DEFAULT_BINARY(CapsName, underscore_name, operatorname, bytecode, precedence) \
\
G_DEFINE_TYPE (ViviCode ## CapsName, vivi_code_ ## underscore_name, VIVI_TYPE_CODE_BINARY) \
\
static void \
vivi_code_ ## underscore_name ## _class_init (ViviCodeBinaryClass *klass) \
{ \
  ViviCodeBinaryClass *binary_class = VIVI_CODE_BINARY_CLASS (klass); \
\
  binary_class->operator_name = operatorname; \
  binary_class->asm_constructor = vivi_code_asm_ ## underscore_name ## _new; \
} \
\
static void \
vivi_code_ ## underscore_name ## _init (ViviCodeBinary *binary) \
{ \
  vivi_code_value_set_precedence (VIVI_CODE_VALUE (binary), precedence); \
} \
\
ViviCodeValue * \
vivi_code_ ## underscore_name ## _new (ViviCodeValue *left, ViviCodeValue *right) \
{ \
  ViviCodeBinary *binary; \
\
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (left), NULL); \
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (right), NULL); \
\
  binary = g_object_new (vivi_code_ ## underscore_name ## _get_type (), NULL); \
  binary->left = g_object_ref (left); \
  binary->right = g_object_ref (right); \
\
  return VIVI_CODE_VALUE (binary); \
}

#include "vivi_code_defaults.h"
