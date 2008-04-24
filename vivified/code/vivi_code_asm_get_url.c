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

#include <string.h>

#include <swfdec/swfdec_as_interpret.h>
#include <swfdec/swfdec_bots.h>

#include "vivi_code_asm_get_url.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_get_url_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeAsmGetUrl *url = VIVI_CODE_ASM_GET_URL (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);
  gsize len;

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_GET_URL);
  len = strlen (url->url) + strlen (url->target) + 2;
  if (len > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"url and target exceed maximum size");
    return FALSE;
  }
  swfdec_bots_put_u16 (emit, len);
  swfdec_bots_put_string (emit, url->url);
  swfdec_bots_put_string (emit, url->target);

  return TRUE;
}

static void
vivi_code_asm_get_url_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_get_url_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmGetUrl, vivi_code_asm_get_url, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_get_url_asm_init))

static void
vivi_code_asm_get_url_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmGetUrl *url = VIVI_CODE_ASM_GET_URL (token);
  char *s;

  vivi_code_printer_print (printer, "geturl ");
  s = vivi_code_escape_string (url->url);
  vivi_code_printer_print (printer, s);
  g_free (s);
  s = vivi_code_escape_string (url->target);
  vivi_code_printer_print (printer, s);
  g_free (s);
}

static void
vivi_code_asm_get_url_dispose (GObject *object)
{
  ViviCodeAsmGetUrl *url = VIVI_CODE_ASM_GET_URL (object);

  g_free (url->url);
  url->url = NULL;
  g_free (url->target);
  url->target = NULL;

  G_OBJECT_CLASS (vivi_code_asm_get_url_parent_class)->dispose (object);
}

static void
vivi_code_asm_get_url_class_init (ViviCodeAsmGetUrlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  object_class->dispose = vivi_code_asm_get_url_dispose;

  token_class->print = vivi_code_asm_get_url_print;

  code_class->bytecode = SWFDEC_AS_ACTION_GET_URL;
}

static void
vivi_code_asm_get_url_init (ViviCodeAsmGetUrl *get_url)
{
}

ViviCodeAsm *
vivi_code_asm_get_url_new (const char *url, const char *target)
{
  ViviCodeAsmGetUrl *ret;

  g_return_val_if_fail (url != NULL, NULL);
  g_return_val_if_fail (target != NULL, NULL);

  ret = g_object_new (VIVI_TYPE_CODE_ASM_GET_URL, NULL);
  ret->url = g_strdup (url);
  ret->target = g_strdup (target);

  return VIVI_CODE_ASM (ret);
}

const char *
vivi_code_asm_get_url_get_url (ViviCodeAsmGetUrl *url)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_GET_URL (url), NULL);

  return url->url;
}

const char *
vivi_code_asm_get_url_get_target (ViviCodeAsmGetUrl *url)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_GET_URL (url), NULL);

  return url->target;
}

