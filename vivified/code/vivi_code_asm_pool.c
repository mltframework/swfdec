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
#include <swfdec/swfdec_bits.h>

#include "vivi_code_asm_pool.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_pool_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeAsmPool *pool = VIVI_CODE_ASM_POOL (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);
  SwfdecBuffer *buffer;

  buffer = swfdec_constant_pool_get_buffer (pool->pool);
  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_CONSTANT_POOL);
  swfdec_bots_put_u16 (emit, buffer->length);
  swfdec_bots_put_buffer (emit, buffer);

  return TRUE;
}

static void
vivi_code_asm_pool_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_pool_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmPool, vivi_code_asm_pool, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_pool_asm_init))

static void
vivi_code_asm_pool_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmPool *pool = VIVI_CODE_ASM_POOL (token);
  guint i;
  char *s;

  vivi_code_printer_print (printer, "pool");
  for (i = 0; i < swfdec_constant_pool_size (pool->pool); i++) {
    vivi_code_printer_print (printer, i == 0 ? " " : ", ");
    s = vivi_code_escape_string (swfdec_constant_pool_get (pool->pool, i));
    vivi_code_printer_print (printer, s);
    g_free (s);
  }
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_pool_dispose (GObject *object)
{
  ViviCodeAsmPool *pool = VIVI_CODE_ASM_POOL (object);

  swfdec_constant_pool_unref (pool->pool);

  G_OBJECT_CLASS (vivi_code_asm_pool_parent_class)->dispose (object);
}

static void
vivi_code_asm_pool_class_init (ViviCodeAsmPoolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_asm_pool_dispose;

  token_class->print = vivi_code_asm_pool_print;
}

static void
vivi_code_asm_pool_init (ViviCodeAsmPool *pool)
{
}

ViviCodeAsm *
vivi_code_asm_pool_new (SwfdecConstantPool *pool)
{
  ViviCodeAsmPool *ret;

  g_return_val_if_fail (SWFDEC_IS_CONSTANT_POOL (pool), NULL);

  ret = g_object_new (VIVI_TYPE_CODE_ASM_POOL, NULL);
  ret->pool = swfdec_constant_pool_ref (pool);

  return VIVI_CODE_ASM (ret);
}

SwfdecConstantPool *
vivi_code_asm_pool_get_pool (ViviCodeAsmPool *pool)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_POOL (pool), NULL);

  return pool->pool;
}

