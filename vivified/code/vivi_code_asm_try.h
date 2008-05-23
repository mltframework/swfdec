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

#ifndef _VIVI_CODE_ASM_TRY_H_
#define _VIVI_CODE_ASM_TRY_H_

#include <vivified/code/vivi_code_asm.h>
#include <vivified/code/vivi_code_asm_code.h>
#include <vivified/code/vivi_code_label.h>

G_BEGIN_DECLS

typedef struct _ViviCodeAsmTry ViviCodeAsmTry;
typedef struct _ViviCodeAsmTryClass ViviCodeAsmTryClass;

#define VIVI_TYPE_CODE_ASM_TRY                    (vivi_code_asm_try_get_type())
#define VIVI_IS_CODE_ASM_TRY(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_TRY))
#define VIVI_IS_CODE_ASM_TRY_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_TRY))
#define VIVI_CODE_ASM_TRY(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_TRY, ViviCodeAsmTry))
#define VIVI_CODE_ASM_TRY_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_TRY, ViviCodeAsmTryClass))
#define VIVI_CODE_ASM_TRY_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_TRTRY ViviCodeAsmTryClass))

struct _ViviCodeAsmTry
{
  ViviCodeAsmCode	code;

  gboolean		has_catch;
  gboolean		has_finally;
  gboolean		use_register;
  guint			reserved_flags;
  union {
    guint		register_number;
    char *		variable_name;
  };
  ViviCodeLabel *	catch_start;
  ViviCodeLabel *	finally_start;
  ViviCodeLabel *	end;
};

struct _ViviCodeAsmTryClass
{
  ViviCodeAsmCodeClass	code_class;
};

GType			vivi_code_asm_try_get_type	  	(void);

ViviCodeAsm *		vivi_code_asm_try_new			(ViviCodeLabel *	catch_start,
								 ViviCodeLabel *	finally_start,
								 ViviCodeLabel *	end,
								 const char *		name);
ViviCodeAsm *		vivi_code_asm_try_new_register		(ViviCodeLabel *	catch_start,
								 ViviCodeLabel *	finally_start,
								 ViviCodeLabel *	end,
								 guint			id);

void			vivi_code_asm_try_set_has_catch		(ViviCodeAsmTry *	try_,
								 gboolean		has_catch);
void			vivi_code_asm_try_set_has_finally	(ViviCodeAsmTry *	try_,
								 gboolean		has_finally);
void			vivi_code_asm_try_set_reserved_flags	(ViviCodeAsmTry *	try_,
								 guint			flags);

void			vivi_code_asm_try_set_variable_name	(ViviCodeAsmTry *	try_,
								 const char *		name);


G_END_DECLS
#endif
