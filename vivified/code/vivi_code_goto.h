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

#ifndef _VIVI_CODE_GOTO_H_
#define _VIVI_CODE_GOTO_H_

#include <vivified/code/vivi_code_label.h>
#include <vivified/code/vivi_code_statement.h>

G_BEGIN_DECLS


typedef struct _ViviCodeGoto ViviCodeGoto;
typedef struct _ViviCodeGotoClass ViviCodeGotoClass;

#define VIVI_TYPE_CODE_GOTO                    (vivi_code_goto_get_type())
#define VIVI_IS_CODE_GOTO(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_GOTO))
#define VIVI_IS_CODE_GOTO_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_GOTO))
#define VIVI_CODE_GOTO(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_GOTO, ViviCodeGoto))
#define VIVI_CODE_GOTO_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_GOTO, ViviCodeGotoClass))
#define VIVI_CODE_GOTO_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_GOTO, ViviCodeGotoClass))

struct _ViviCodeGoto
{
  ViviCodeStatement	statement;

  ViviCodeLabel *	label;
};

struct _ViviCodeGotoClass
{
  ViviCodeStatementClass	statement_class;
};

GType			vivi_code_goto_get_type   	(void);

ViviCodeToken *		vivi_code_goto_new		(ViviCodeLabel *	label);


G_END_DECLS
#endif
