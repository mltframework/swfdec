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

#ifndef _VIVI_CODE_ASM_FUNCTION2_H_
#define _VIVI_CODE_ASM_FUNCTION2_H_

#include <vivified/code/vivi_code_asm.h>
#include <vivified/code/vivi_code_asm_code.h>
#include <vivified/code/vivi_code_label.h>

G_BEGIN_DECLS

typedef struct _ViviCodeAsmFunction2 ViviCodeAsmFunction2;
typedef struct _ViviCodeAsmFunction2Class ViviCodeAsmFunction2Class;

#define VIVI_TYPE_CODE_ASM_FUNCTION2                    (vivi_code_asm_function2_get_type())
#define VIVI_IS_CODE_ASM_FUNCTION2(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_FUNCTION2))
#define VIVI_IS_CODE_ASM_FUNCTION2_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_FUNCTION2))
#define VIVI_CODE_ASM_FUNCTION2(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_FUNCTION2, ViviCodeAsmFunction2))
#define VIVI_CODE_ASM_FUNCTION2_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_FUNCTION2, ViviCodeAsmFunction2Class))
#define VIVI_CODE_ASM_FUNCTION2_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_FUNCTION2, ViviCodeAsmFunction2Class))

struct _ViviCodeAsmFunction2
{
  ViviCodeAsmCode	code;

  char *		name;
  guint			flags;
  guint			n_registers;
  GArray *		args;
  ViviCodeLabel *	label;
};

struct _ViviCodeAsmFunction2Class
{
  ViviCodeAsmCodeClass	code_class;
};

GType		vivi_code_asm_function2_get_type 	(void);

ViviCodeAsm *	vivi_code_asm_function2_new		(ViviCodeLabel *	label,
							 const char *		name,
							 guint			n_registers,
							 guint			flags);

ViviCodeLabel *	vivi_code_asm_function2_get_label	(ViviCodeAsmFunction2 *	fun);
void		vivi_code_asm_function2_set_label	(ViviCodeAsmFunction2 *	fun,
							 ViviCodeLabel *	label);
const char *  	vivi_code_asm_function2_get_name	(ViviCodeAsmFunction2 *	fun);
guint		vivi_code_asm_function2_get_flags	(ViviCodeAsmFunction2 *	fun);
guint		vivi_code_asm_function2_get_n_registers	(ViviCodeAsmFunction2 *	fun);
guint		vivi_code_asm_function2_get_n_arguments	(ViviCodeAsmFunction2 *	fun);
const char *	vivi_code_asm_function2_get_argument_name
							(ViviCodeAsmFunction2 *	fun,
							 guint			i);
guint		vivi_code_asm_function2_get_argument_preload
							(ViviCodeAsmFunction2 *	fun,
							 guint			i);
void		vivi_code_asm_function2_add_argument	(ViviCodeAsmFunction2 *	fun,
							 const char *		name,
							 guint			preload);


G_END_DECLS
#endif
