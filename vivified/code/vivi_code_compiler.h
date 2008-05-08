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

#ifndef _VIVI_CODE_COMPILER_H_
#define _VIVI_CODE_COMPILER_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_bots.h>
#include <vivified/code/vivi_code_assembler.h>
#include <vivified/code/vivi_code_asm.h>
#include <vivified/code/vivi_code_label.h>

G_BEGIN_DECLS

typedef struct _ViviCodeCompilerClass ViviCodeCompilerClass;

#define VIVI_TYPE_CODE_COMPILER                    (vivi_code_compiler_get_type())
#define VIVI_IS_CODE_COMPILER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_COMPILER))
#define VIVI_IS_CODE_COMPILER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_COMPILER))
#define VIVI_CODE_COMPILER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_COMPILER, ViviCodeCompiler))
#define VIVI_CODE_COMPILER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_COMPILER, ViviCodeCompilerClass))
#define VIVI_CODE_COMPILER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_COMPILER, ViviCodeCompilerClass))

struct _ViviCodeCompiler
{
  GObject		object;

  guint			version;
  ViviCodeAssembler *	assembler;

  guint			num_labels;
};

struct _ViviCodeCompilerClass
{
  GObjectClass		object_class;
};

GType			vivi_code_compiler_get_type   		(void);

ViviCodeCompiler *	vivi_code_compiler_new			(guint			version);

#define vivi_code_compiler_compile_value(c,v) vivi_code_compiler_compile_token((c), VIVI_CODE_TOKEN((v)))
#define vivi_code_compiler_compile_statement(c,s) vivi_code_compiler_compile_token((c), VIVI_CODE_TOKEN((s)))
void			vivi_code_compiler_compile_token	(ViviCodeCompiler *	compiler,
								 ViviCodeToken *	token);

void			vivi_code_compiler_add_code		(ViviCodeCompiler	*compiler,
								 ViviCodeAsm		*code);

ViviCodeAssembler *	vivi_code_compiler_get_assembler	(ViviCodeCompiler *	compiler);
guint			vivi_code_compiler_get_version		(ViviCodeCompiler *	compiler);
ViviCodeLabel *		vivi_code_compiler_create_label		(ViviCodeCompiler *	compiler,
								 const char *		prefix);

G_END_DECLS
#endif
