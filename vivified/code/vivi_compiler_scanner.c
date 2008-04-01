/* Vivified
 * Copyright (C) Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_compiler_scanner.h"

#include "vivi_compiler_scanner_lex.h"
#include "vivi_compiler_scanner_lex_include.h"

G_DEFINE_TYPE (ViviCompilerScanner, vivi_compiler_scanner, G_TYPE_OBJECT)

static void
vivi_compiler_scanner_dispose (GObject *object)
{
  //ViviCompilerScanner *scanner = VIVI_COMPILER_SCANNER (object);

  G_OBJECT_CLASS (vivi_compiler_scanner_parent_class)->dispose (object);
}

static void
vivi_compiler_scanner_class_init (ViviCompilerScannerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_compiler_scanner_dispose;
}

static void
vivi_compiler_scanner_init (ViviCompilerScanner *token)
{
}

ViviCompilerScanner *
vivi_compiler_scanner_new (FILE *file)
{
  ViviCompilerScanner *scanner;

  g_return_val_if_fail (file != NULL, NULL);

  scanner = g_object_new (VIVI_TYPE_COMPILER_SCANNER, NULL);
  scanner->file = file;

  return scanner;
}

static void
vivi_compiler_scanner_advance (ViviCompilerScanner *scanner)
{
  g_return_if_fail (VIVI_IS_COMPILER_SCANNER (scanner));

  scanner->token = scanner->next_token;
  scanner->value = scanner->next_value;

  if (scanner->file == NULL) {
    scanner->next_token = TOKEN_EOF;
    scanner->next_value.v_string = NULL;
  } else {
    scanner->next_token = yylex ();
    scanner->next_value = yylval;
  }
}

ViviCompilerScannerToken
vivi_compiler_scanner_get_next_token (ViviCompilerScanner *scanner)
{
  g_return_val_if_fail (VIVI_IS_COMPILER_SCANNER (scanner), TOKEN_EOF);

  vivi_compiler_scanner_advance (scanner);

  return scanner->token;
}

ViviCompilerScannerToken
vivi_compiler_scanner_peek_next_token (ViviCompilerScanner *scanner)
{
  g_return_val_if_fail (VIVI_IS_COMPILER_SCANNER (scanner), TOKEN_EOF);

  return scanner->next_token;
}
