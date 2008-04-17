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

#ifndef __VIVI_CODE_ASM_H__
#define __VIVI_CODE_ASM_H__

#include <glib-object.h>

G_BEGIN_DECLS


#define VIVI_TYPE_CODE_ASM                (vivi_code_asm_get_type ())
#define VIVI_CODE_ASM(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM, ViviCodeAsm))
#define VIVI_IS_CODE_ASM(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM))
#define VIVI_CODE_ASM_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), VIVI_TYPE_CODE_ASM, ViviCodeAsmInterface))

typedef struct _ViviCodeAsm ViviCodeAsm; /* dummy object */
typedef struct _ViviCodeAsmInterface ViviCodeAsmInterface;

struct _ViviCodeAsmInterface {
  GTypeInterface	interface;
};

GType			vivi_code_asm_get_type		(void) G_GNUC_CONST;


G_END_DECLS

#endif /* __VIVI_CODE_ASM_H__ */
