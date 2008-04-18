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


G_END_DECLS
#endif
