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

#ifndef _VIVI_CODE_ASM_CODE_H_
#define _VIVI_CODE_ASM_CODE_H_

#include <swfdec/swfdec_as_interpret.h>
#include <vivified/code/vivi_code_token.h>

G_BEGIN_DECLS


typedef struct _ViviCodeAsmCode ViviCodeAsmCode;
typedef struct _ViviCodeAsmCodeClass ViviCodeAsmCodeClass;

#define VIVI_TYPE_CODE_ASM_CODE                    (vivi_code_asm_code_get_type())
#define VIVI_IS_CODE_ASM_CODE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_CODE))
#define VIVI_IS_CODE_ASM_CODE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_CODE))
#define VIVI_CODE_ASM_CODE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_CODE, ViviCodeAsmCode))
#define VIVI_CODE_ASM_CODE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_CODE, ViviCodeAsmCodeClass))
#define VIVI_CODE_ASM_CODE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_CODE, ViviCodeAsmCodeClass))

struct _ViviCodeAsmCode
{
  ViviCodeToken		token;
};

struct _ViviCodeAsmCodeClass
{
  ViviCodeTokenClass	token_class;

  /*< private >*/
  /* use vivi_code_default.h */
  SwfdecAsAction	bytecode;
};

GType			vivi_code_asm_code_get_type		(void);

SwfdecAsAction		vivi_code_asm_code_get_action		(ViviCodeAsmCode *	code);


G_END_DECLS
#endif
