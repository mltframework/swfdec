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

#include "vivi_code_trace.h"

G_DEFINE_TYPE (ViviCodeTrace, vivi_code_trace, VIVI_TYPE_CODE_BUILTIN_VALUE_STATEMENT)

static void
vivi_code_trace_class_init (ViviCodeTraceClass *klass)
{
  ViviCodeBuiltinStatementClass *stmt_class =
    VIVI_CODE_BUILTIN_STATEMENT_CLASS (klass);

  stmt_class->function_name = "trace";
  stmt_class->bytecode = SWFDEC_AS_ACTION_TRACE;
}

static void
vivi_code_trace_init (ViviCodeTrace *token)
{
}

ViviCodeStatement *
vivi_code_trace_new (ViviCodeValue *value)
{
  ViviCodeBuiltinValueStatement *stmt;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  stmt = VIVI_CODE_BUILTIN_VALUE_STATEMENT (
      g_object_new (VIVI_TYPE_CODE_TRACE, NULL));
  vivi_code_builtin_value_statement_set_value (stmt, g_object_ref (value));

  return VIVI_CODE_STATEMENT (stmt);
}

