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

#include "vivi_code_get_url.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"

G_DEFINE_TYPE (ViviCodeGetUrl, vivi_code_get_url, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_get_url_dispose (GObject *object)
{
  ViviCodeGetUrl *get_url = VIVI_CODE_GET_URL (object);

  g_object_unref (get_url->target);
  g_object_unref (get_url->url);

  G_OBJECT_CLASS (vivi_code_get_url_parent_class)->dispose (object);
}

static void
vivi_code_get_url_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeGetUrl *url = VIVI_CODE_GET_URL (token);

  if (url->variables) {
    vivi_code_printer_print (printer, "loadVariables (");
  } else if (url->internal) {
    vivi_code_printer_print (printer, "loadMovie (");
  } else {
    vivi_code_printer_print (printer, "getURL (");
  }
  vivi_code_printer_print_value (printer, url->url, VIVI_PRECEDENCE_MIN);
  vivi_code_printer_print (printer, ", ");
  vivi_code_printer_print_value (printer, url->target, VIVI_PRECEDENCE_MIN);
  if (url->method != 0) {
    vivi_code_printer_print (printer,
	url->method == 2 ? ", \"POST\"" : ", \"GET\"");
  }
  vivi_code_printer_print (printer, ");");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_get_url_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  g_printerr ("Implement getURL2");
#if 0
  ViviCodeGetUrl *url = VIVI_CODE_GET_URL (token);
  guint bits;

  vivi_code_value_compile (url->url, assembler);
  vivi_code_value_compile (url->target, assembler);

  vivi_code_assembler_take_code (assembler,
      vivi_code_asm_get_url2_new (url->method, url->internal, url->variables));
#endif
}

static void
vivi_code_get_url_class_init (ViviCodeGetUrlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_get_url_dispose;

  token_class->print = vivi_code_get_url_print;
  token_class->compile = vivi_code_get_url_compile;
}

static void
vivi_code_get_url_init (ViviCodeGetUrl *token)
{
}

ViviCodeStatement *
vivi_code_get_url_new (ViviCodeValue *target, ViviCodeValue *url,
    SwfdecLoaderRequest method, gboolean internal, gboolean variables)
{
  ViviCodeGetUrl *ret;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (target), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (url), NULL);
  g_return_val_if_fail (method < 4, NULL);

  ret = g_object_new (VIVI_TYPE_CODE_GET_URL, NULL);
  ret->target = g_object_ref (target);
  ret->url = g_object_ref (url);
  ret->method = method;
  ret->internal = internal;
  ret->variables = variables;

  return VIVI_CODE_STATEMENT (ret);
}

