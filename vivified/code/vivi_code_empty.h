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

#ifndef _VIVI_CODE_EMPTY_H_
#define _VIVI_CODE_EMPTY_H_

#include <vivified/code/vivi_code_statement.h>
#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodeEmpty ViviCodeEmpty;
typedef struct _ViviCodeEmptyClass ViviCodeEmptyClass;

#define VIVI_TYPE_CODE_EMPTY                    (vivi_code_empty_get_type())
#define VIVI_IS_CODE_EMPTY(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_EMPTY))
#define VIVI_IS_CODE_EMPTY_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_EMPTY))
#define VIVI_CODE_EMPTY(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_EMPTY, ViviCodeEmpty))
#define VIVI_CODE_EMPTY_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_EMPTY, ViviCodeEmptyClass))
#define VIVI_CODE_EMPTY_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_EMPTY, ViviCodeEmptyClass))

struct _ViviCodeEmpty
{
  ViviCodeStatement	statement;
};

struct _ViviCodeEmptyClass
{
  ViviCodeStatementClass	statement_class;
};

GType			vivi_code_empty_get_type	(void);

ViviCodeStatement *	vivi_code_empty_new		(void);


G_END_DECLS
#endif
