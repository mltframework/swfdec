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

#ifndef _VIVI_CODE_ASSEMBLER_H_
#define _VIVI_CODE_ASSEMBLER_H_

#include <vivified/code/vivi_code_asm.h>
#include <vivified/code/vivi_code_statement.h>

G_BEGIN_DECLS


typedef struct _ViviCodeAssembler ViviCodeAssembler;
typedef struct _ViviCodeAssemblerClass ViviCodeAssemblerClass;

#define VIVI_TYPE_CODE_ASSEMBLER                    (vivi_code_assembler_get_type())
#define VIVI_IS_CODE_ASSEMBLER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASSEMBLER))
#define VIVI_IS_CODE_ASSEMBLER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASSEMBLER))
#define VIVI_CODE_ASSEMBLER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASSEMBLER, ViviCodeAssembler))
#define VIVI_CODE_ASSEMBLER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASSEMBLER, ViviCodeAssemblerClass))
#define VIVI_CODE_ASSEMBLER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASSEMBLER, ViviCodeAssemblerClass))

struct _ViviCodeAssembler
{
  ViviCodeStatement	statement;

  GPtrArray *		codes;		/* ViviCodeAsm inside this assembler */
};

struct _ViviCodeAssemblerClass
{
  ViviCodeStatementClass statement_class;
};

GType			vivi_code_assembler_get_type		(void);

ViviCodeStatement *	vivi_code_assembler_new			(void);

guint			vivi_code_assembler_get_n_codes		(ViviCodeAssembler *	assembler);
ViviCodeAsm *		vivi_code_assembler_get_code		(ViviCodeAssembler *	assembler,
								 guint			i);
#define vivi_code_assembler_add_code(assembler, code) vivi_code_assembler_insert_code (assembler, G_MAXUINT, code)
void			vivi_code_assembler_insert_code  	(ViviCodeAssembler *	assembler,
								 guint			i,
								 ViviCodeAsm *		code);
void			vivi_code_assembler_remove_code		(ViviCodeAssembler *	assembler,
								 ViviCodeAsm *		code);

SwfdecScript *		vivi_code_assembler_assemble_script	(ViviCodeAssembler *	assembler,
								 guint			version,
								 GError **		error);


G_END_DECLS
#endif
