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
#include <swfdec/swfdec_bots.h>

#include "vivi_code_asm_get_url2.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_get_url2_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter,
    GError **error)
{
  ViviCodeAsmGetUrl2 *get_url = VIVI_CODE_ASM_GET_URL2 (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_GET_URL2);
  swfdec_bots_put_u8 (emit, get_url->flags);

  return TRUE;
}

static void
vivi_code_asm_get_url2_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_get_url2_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmGetUrl2, vivi_code_asm_get_url2, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_get_url2_asm_init))


/* FIXME: export for compiler */
static const char *flag_names[16] = {
  "get",
  "post",
  "reserved1",
  "reserved2",
  "reserved3",
  "reserved4",
  "internal",
  "variables"
};

static void
vivi_code_asm_get_url2_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeAsmGetUrl2 *get_url = VIVI_CODE_ASM_GET_URL2 (token);
  guint i;

  vivi_code_printer_print (printer, "get_url2");
  for (i = 0; i < G_N_ELEMENTS (flag_names); i++) {
    if (get_url->flags & (1 << i)) {
      vivi_code_printer_print (printer, " ");
      vivi_code_printer_print (printer, flag_names[i]);
    }
  }
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_get_url2_class_init (ViviCodeAsmGetUrl2Class *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  token_class->print = vivi_code_asm_get_url2_print;

  code_class->bytecode = SWFDEC_AS_ACTION_GET_URL2;
}

static void
vivi_code_asm_get_url2_init (ViviCodeAsmGetUrl2 *get_url)
{
}

ViviCodeAsm *
vivi_code_asm_get_url2_new_from_flags (guint flags)
{
  ViviCodeAsmGetUrl2 *get_url;

  g_return_val_if_fail (flags < G_MAXUINT8, NULL);

  get_url = g_object_new (VIVI_TYPE_CODE_ASM_GET_URL2, NULL);
  get_url->flags = flags;

  return VIVI_CODE_ASM (get_url);
}

ViviCodeAsm *
vivi_code_asm_get_url2_new (SwfdecLoaderRequest method, gboolean internal,
    gboolean variables)
{
  guint flags = 0;

  if (method == SWFDEC_LOADER_REQUEST_GET) {
    flags |= (1 << 0);
  } else if (method == SWFDEC_LOADER_REQUEST_POST) {
    flags |= (1 << 1);
  }
  if (internal)
    flags |= (1 << 6);
  if (variables)
    flags |= (1 << 7);

  return vivi_code_asm_get_url2_new_from_flags (flags);
}
