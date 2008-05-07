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

#ifndef _VIVI_CODE_ASM_PUSH_H_
#define _VIVI_CODE_ASM_PUSH_H_

#include <swfdec/swfdec_bots.h>
#include <vivified/code/vivi_code_asm.h>
#include <vivified/code/vivi_code_asm_code.h>

G_BEGIN_DECLS

typedef enum {
  VIVI_CODE_CONSTANT_STRING = 0,
  VIVI_CODE_CONSTANT_FLOAT = 1,
  VIVI_CODE_CONSTANT_NULL = 2,
  VIVI_CODE_CONSTANT_UNDEFINED = 3,
  VIVI_CODE_CONSTANT_REGISTER = 4,
  VIVI_CODE_CONSTANT_BOOLEAN = 5,
  VIVI_CODE_CONSTANT_DOUBLE = 6,
  VIVI_CODE_CONSTANT_INTEGER = 7,
  VIVI_CODE_CONSTANT_CONSTANT_POOL = 8,
  VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG = 9
} ViviCodeConstantType;

typedef struct _ViviCodeAsmPush ViviCodeAsmPush;
typedef struct _ViviCodeAsmPushClass ViviCodeAsmPushClass;

#define VIVI_TYPE_CODE_ASM_PUSH                    (vivi_code_asm_push_get_type())
#define VIVI_IS_CODE_ASM_PUSH(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_ASM_PUSH))
#define VIVI_IS_CODE_ASM_PUSH_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_ASM_PUSH))
#define VIVI_CODE_ASM_PUSH(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_ASM_PUSH, ViviCodeAsmPush))
#define VIVI_CODE_ASM_PUSH_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_ASM_PUSH, ViviCodeAsmPushClass))
#define VIVI_CODE_ASM_PUSH_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_ASM_PUSH, ViviCodeAsmPushClass))

struct _ViviCodeAsmPush
{
  ViviCodeAsmCode	code;

  SwfdecBots *		contents;
  GPtrArray *		offsets;
};

struct _ViviCodeAsmPushClass
{
  ViviCodeAsmCodeClass	code_class;
};

GType			vivi_code_asm_push_get_type	  	(void);

ViviCodeAsm *		vivi_code_asm_push_new			(void);

guint			vivi_code_asm_push_get_n_values		(const ViviCodeAsmPush *	push);
ViviCodeConstantType	vivi_code_asm_push_get_value_type	(const ViviCodeAsmPush *	push,
								 guint			i);
const char *		vivi_code_asm_push_get_string		(const ViviCodeAsmPush *	push,
								 guint			id);
float			vivi_code_asm_push_get_float		(const ViviCodeAsmPush *	push,
								 guint			id);
guint			vivi_code_asm_push_get_register		(const ViviCodeAsmPush *	push,
								 guint			id);
double			vivi_code_asm_push_get_double		(const ViviCodeAsmPush *	push,
								 guint			id);
int			vivi_code_asm_push_get_integer		(const ViviCodeAsmPush *	push,
								 guint			id);
gboolean		vivi_code_asm_push_get_boolean		(const ViviCodeAsmPush *	push,
								 guint			id);
guint			vivi_code_asm_push_get_pool		(const ViviCodeAsmPush *	push,
								 guint			id);

void			vivi_code_asm_push_add_string		(ViviCodeAsmPush *	push,
								 const char *		string);
void			vivi_code_asm_push_add_float		(ViviCodeAsmPush *	push,
								 float			f);
void			vivi_code_asm_push_add_null		(ViviCodeAsmPush *	push);
void			vivi_code_asm_push_add_undefined	(ViviCodeAsmPush *	push);
void			vivi_code_asm_push_add_register		(ViviCodeAsmPush *	push,
								 guint			id);
void			vivi_code_asm_push_add_boolean		(ViviCodeAsmPush *	push,
								 gboolean		b);
void			vivi_code_asm_push_add_double		(ViviCodeAsmPush *	push,
								 double			d);
void			vivi_code_asm_push_add_integer		(ViviCodeAsmPush *	push,
								 int			i);
void			vivi_code_asm_push_add_pool		(ViviCodeAsmPush *	push,
								 guint			id);
void			vivi_code_asm_push_add_pool_big		(ViviCodeAsmPush *	push,
								 guint			id);
void			vivi_code_asm_push_copy_value		(ViviCodeAsmPush *	push,
								 const ViviCodeAsmPush *other,
								 guint id);


G_END_DECLS
#endif
