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

#ifndef _VIVI_CODE_ASM_CODE_DEFAULT_H_
#define _VIVI_CODE_ASM_CODE_DEFAULT_H_

#include <vivified/code/vivi_code_asm_code.h>
#include <vivified/code/vivi_code_asm.h>

G_BEGIN_DECLS

#define DEFAULT_ASM(CapsName, underscore_name, bytecode) \
\
typedef ViviCodeAsmCode ViviCodeAsm ## CapsName; \
typedef ViviCodeAsmCodeClass ViviCodeAsm ## CapsName ## Class; \
\
GType			vivi_code_asm_ ## underscore_name ## _get_type 	(void); \
\
ViviCodeAsm *		vivi_code_asm_ ## underscore_name ## _new	(void);

#include "vivi_code_defaults.h"

#define VIVI_TYPE_CODE_ASM_GET_MEMBER                    (vivi_code_asm_get_member_get_type())
#define VIVI_IS_CODE_ASM_GET_MEMBER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_GET_MEMBER))
#define VIVI_IS_CODE_ASM_GET_MEMBER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_GET_MEMBER))
#define VIVI_CODE_ASM_GET_MEMBER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_GET_MEMBER, ViviCodeAsmGetMember))
#define VIVI_CODE_ASM_GET_MEMBER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_GET_MEMBER, ViviCodeAsmGetMemberClass))
#define VIVI_CODE_ASM_GET_MEMBER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_GET_MEMBER, ViviCodeAsmGetMemberClass))

#define VIVI_TYPE_CODE_ASM_GET_VARIABLE                    (vivi_code_asm_get_variable_get_type())
#define VIVI_IS_CODE_ASM_GET_VARIABLE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_GET_VARIABLE))
#define VIVI_IS_CODE_ASM_GET_VARIABLE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_GET_VARIABLE))
#define VIVI_CODE_ASM_GET_VARIABLE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_GET_VARIABLE, ViviCodeAsmGetVariable))
#define VIVI_CODE_ASM_GET_VARIABLE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_GET_VARIABLE, ViviCodeAsmGetVariableClass))
#define VIVI_CODE_ASM_GET_VARIABLE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_GET_VARIABLE, ViviCodeAsmGetVariableClass))

#define VIVI_TYPE_CODE_ASM_RANDOM		     (vivi_code_asm_random_get_type())
#define VIVI_IS_CODE_ASM_RANDOM(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_RANDOM))
#define VIVI_IS_CODE_ASM_RANDOM_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_RANDOM))
#define VIVI_CODE_ASM_RANDOM(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_RANDOM, ViviCodeAsmRandom))
#define VIVI_CODE_ASM_RANDOM_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_RANDOM, ViviCodeAsmRandomClass))
#define VIVI_CODE_ASM_RANDOM_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_RANDOM, ViviCodeAsmRandomClass))

#define VIVI_TYPE_CODE_ASM_ENUMERATE		     (vivi_code_asm_enumerate_get_type())
#define VIVI_IS_CODE_ASM_ENUMERATE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_ENUMERATE))
#define VIVI_IS_CODE_ASM_ENUMERATE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_ENUMERATE))
#define VIVI_CODE_ASM_ENUMERATE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_ENUMERATE, ViviCodeAsmEnumerate))
#define VIVI_CODE_ASM_ENUMERATE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_ENUMERATE, ViviCodeAsmEnumerateClass))
#define VIVI_CODE_ASM_ENUMERATE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_ENUMERATE, ViviCodeAsmEnumerateClass))

#define VIVI_TYPE_CODE_ASM_ENUMERATE2		     (vivi_code_asm_enumerate2_get_type())
#define VIVI_IS_CODE_ASM_ENUMERATE2(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_ENUMERATE2))
#define VIVI_IS_CODE_ASM_ENUMERATE2_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_ENUMERATE2))
#define VIVI_CODE_ASM_ENUMERATE2(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_ENUMERATE2, ViviCodeAsmEnumerate2))
#define VIVI_CODE_ASM_ENUMERATE2_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_ENUMERATE2, ViviCodeAsmEnumerate2Class))
#define VIVI_CODE_ASM_ENUMERATE2_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_ENUMERATE2, ViviCodeAsmEnumerate2Class))


G_END_DECLS
#endif
