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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <swfdec/swfdec_as_interpret.h>

#include "vivi_code_asm_push.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_push_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeAsmPush *push = VIVI_CODE_ASM_PUSH (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);
  gsize len;

  len = swfdec_bots_get_bytes (push->contents);
  if (len > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"Push action too big");
    return FALSE;
  }
  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_PUSH);
  swfdec_bots_put_u16 (emit, len);
  swfdec_bots_put_bots (emit, push->contents);
  return TRUE;
}

static void
vivi_code_asm_push_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_push_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmPush, vivi_code_asm_push, VIVI_TYPE_CODE_ASM_PUSH,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_push_asm_init))

static void
vivi_code_asm_push_dispose (GObject *object)
{
  ViviCodeAsmPush *push = VIVI_CODE_ASM_PUSH (object);

  swfdec_bots_free (push->contents);
  g_ptr_array_free (push->offsets, TRUE);

  G_OBJECT_CLASS (vivi_code_asm_push_parent_class)->dispose (object);
}

static void
vivi_code_asm_push_class_init (ViviCodeAsmPushClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_code_asm_push_dispose;
}

static void
vivi_code_asm_push_init (ViviCodeAsmPush *push)
{
  push->contents = swfdec_bots_open ();
  push->offsets = g_ptr_array_new ();
}

ViviCodeAsm *
vivi_code_asm_push_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_ASM_PUSH, NULL);
}

guint
vivi_code_asm_push_get_n_values (ViviCodeAsmPush *push)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  
  return push->offsets->len;
}

ViviCodeConstantType
vivi_code_asm_push_get_value_type (ViviCodeAsmPush *push, guint i)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  g_return_val_if_fail (i < push->offsets->len, 0);

  return push->contents->ptr[GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, i))];
}

void
vivi_code_asm_push_add_string (ViviCodeAsmPush *push, const char *string)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (string != NULL);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_STRING);
  swfdec_bots_put_string (push->contents, string);
}

void
vivi_code_asm_push_add_float (ViviCodeAsmPush *push, float f)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_FLOAT);
  swfdec_bots_put_float (push->contents, f);
}

void
vivi_code_asm_push_add_null (ViviCodeAsmPush *push)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_NULL);
}

void
vivi_code_asm_push_add_undefined (ViviCodeAsmPush *push)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_UNDEFINED);
}

void
vivi_code_asm_push_add_register (ViviCodeAsmPush *push, guint id)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id < 256);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_REGISTER);
  swfdec_bots_put_u8 (push->contents, id);
}

void
vivi_code_asm_push_add_boolean (ViviCodeAsmPush *push, gboolean b)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_BOOLEAN);
  swfdec_bots_put_u8 (push->contents, b ? 1 : 0);
}

void
vivi_code_asm_push_add_double (ViviCodeAsmPush *push, double d)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_DOUBLE);
  swfdec_bots_put_double (push->contents, d);
}

void
vivi_code_asm_push_add_integer (ViviCodeAsmPush *push, int i)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (i >= G_MININT32 && i <= G_MAXINT32);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_INTEGER);
  swfdec_bots_put_u32 (push->contents, i);
}

void
vivi_code_asm_push_add_pool (ViviCodeAsmPush *push, guint id)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id < 256);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_CONSTANT_POOL);
  swfdec_bots_put_u8 (push->contents, id);
}

void
vivi_code_asm_push_add_pool_big (ViviCodeAsmPush *push, guint id)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id <= G_MAXUINT16);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG);
  swfdec_bots_put_u16 (push->contents, id);
}

