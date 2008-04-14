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

#ifndef _VIVI_CODE_BUILTIN_VALUE_STATEMENT_DEFAULT_H_
#define _VIVI_CODE_BUILTIN_VALUE_STATEMENT_DEFAULT_H_

#include <vivified/code/vivi_code_builtin_value_statement.h>
#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS

#define DEFAULT_BUILTIN_VALUE_STATEMENT(CapsName, underscore_name, function_name, bytecode) \
\
typedef ViviCodeBuiltinValueStatement ViviCode ## CapsName; \
typedef ViviCodeBuiltinValueStatementClass ViviCode ## CapsName ## Class; \
\
GType			vivi_code_ ## underscore_name ## _get_type   	(void); \
\
ViviCodeStatement *	vivi_code_## underscore_name ## _new		(ViviCodeValue *value);

#include "vivi_code_defaults.h"


G_END_DECLS
#endif
