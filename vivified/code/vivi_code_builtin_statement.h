/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _VIVI_CODE_BUILTIN_STATEMENT_H_
#define _VIVI_CODE_BUILTIN_STATEMENT_H_

#include <swfdec/swfdec_as_interpret.h>

#include <vivified/code/vivi_code_statement.h>

G_BEGIN_DECLS


typedef struct _ViviCodeBuiltinStatement ViviCodeBuiltinStatement;
typedef struct _ViviCodeBuiltinStatementClass ViviCodeBuiltinStatementClass;

#define VIVI_TYPE_CODE_BUILTIN_STATEMENT                    (vivi_code_builtin_statement_get_type())
#define VIVI_IS_CODE_BUILTIN_STATEMENT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_BUILTIN_STATEMENT))
#define VIVI_IS_CODE_BUILTIN_STATEMENT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_BUILTIN_STATEMENT))
#define VIVI_CODE_BUILTIN_STATEMENT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_BUILTIN_STATEMENT, ViviCodeBuiltinStatement))
#define VIVI_CODE_BUILTIN_STATEMENT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_BUILTIN_STATEMENT, ViviCodeBuiltinStatementClass))
#define VIVI_CODE_BUILTIN_STATEMENT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_BUILTIN_STATEMENT, ViviCodeBuiltinStatementClass))

struct _ViviCodeBuiltinStatement
{
  ViviCodeStatement	statement;

  const char		*name;
  SwfdecAsAction	action;
};

struct _ViviCodeBuiltinStatementClass
{
  ViviCodeStatementClass	statement_class;
};

GType		vivi_code_builtin_statement_get_type		(void);


G_END_DECLS
#endif
