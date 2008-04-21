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

#ifndef _VIVI_CODE_ASM_POOL_H_
#define _VIVI_CODE_ASM_POOL_H_

#include <swfdec/swfdec_constant_pool.h>
#include <vivified/code/vivi_code_asm.h>
#include <vivified/code/vivi_code_asm_code.h>

G_BEGIN_DECLS

typedef struct _ViviCodeAsmPool ViviCodeAsmPool;
typedef struct _ViviCodeAsmPoolClass ViviCodeAsmPoolClass;

#define VIVI_TYPE_CODE_ASM_POOL                    (vivi_code_asm_pool_get_type())
#define VIVI_IS_CODE_ASM_POOL(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_POOL))
#define VIVI_IS_CODE_ASM_POOL_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_POOL))
#define VIVI_CODE_ASM_POOL(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_POOL, ViviCodeAsmPool))
#define VIVI_CODE_ASM_POOL_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_POOL, ViviCodeAsmPoolClass))
#define VIVI_CODE_ASM_POOL_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_POOL, ViviCodeAsmPoolClass))

struct _ViviCodeAsmPool
{
  ViviCodeAsmCode	code;

  SwfdecConstantPool *	pool;
};

struct _ViviCodeAsmPoolClass
{
  ViviCodeAsmCodeClass	code_class;
};

GType			vivi_code_asm_pool_get_type	  	(void);

ViviCodeAsm *		vivi_code_asm_pool_new			(SwfdecConstantPool *	pool);

SwfdecConstantPool *	vivi_code_asm_pool_get_pool		(ViviCodeAsmPool *	pool);


G_END_DECLS
#endif
