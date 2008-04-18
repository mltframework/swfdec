/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *               2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_code_builtin_value_statement_default.h"
#include "vivi_code_asm_code_default.h"

#define DEFAULT_BUILTIN_VALUE_STATEMENT(CapsName, underscore_name, name) \
\
G_DEFINE_TYPE (ViviCode ## CapsName, vivi_code_ ## underscore_name, VIVI_TYPE_CODE_BUILTIN_VALUE_STATEMENT) \
\
static void \
vivi_code_ ## underscore_name ## _class_init (ViviCodeBuiltinValueStatementClass *klass) \
{ \
  ViviCodeBuiltinStatementClass *builtin_statement_class = VIVI_CODE_BUILTIN_STATEMENT_CLASS (klass); \
\
  builtin_statement_class->function_name = name; \
  builtin_statement_class->asm_constructor = vivi_code_asm_ ## underscore_name ## _new; \
} \
\
static void \
vivi_code_ ## underscore_name ## _init (ViviCodeBuiltinValueStatement *builtin_statement) \
{ \
} \
\
ViviCodeStatement * \
vivi_code_ ## underscore_name ## _new (ViviCodeValue *value) \
{ \
  ViviCodeBuiltinValueStatement *stmt; \
\
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL); \
\
  stmt = VIVI_CODE_BUILTIN_VALUE_STATEMENT (g_object_new (vivi_code_ ## underscore_name ## _get_type (), NULL)); \
  vivi_code_builtin_value_statement_set_value (stmt, value); \
\
  return VIVI_CODE_STATEMENT (stmt); \
}

#include "vivi_code_defaults.h"
