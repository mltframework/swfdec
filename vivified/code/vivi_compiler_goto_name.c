/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *               2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_compiler_goto_name.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCompilerGotoName, vivi_compiler_goto_name, VIVI_TYPE_CODE_GOTO)

static void
vivi_compiler_goto_name_dispose (GObject *object)
{
  ViviCompilerGotoName *goto_ = VIVI_COMPILER_GOTO_NAME (object);

  g_free (goto_->name);

  G_OBJECT_CLASS (vivi_compiler_goto_name_parent_class)->dispose (object);
}

static void
vivi_compiler_goto_name_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCompilerGotoName *goto_ = VIVI_COMPILER_GOTO_NAME (token);

  if (VIVI_CODE_GOTO (goto_)->label != NULL) {
    VIVI_CODE_TOKEN_CLASS (vivi_compiler_goto_name_parent_class)->print (token,
	printer);
    return;
  }

  vivi_code_printer_print (printer, "goto ");
  vivi_code_printer_print (printer, goto_->name);
  vivi_code_printer_print (printer, ";");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_compiler_goto_name_class_init (ViviCompilerGotoNameClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_compiler_goto_name_dispose;

  token_class->print = vivi_compiler_goto_name_print;
}

static void
vivi_compiler_goto_name_init (ViviCompilerGotoName *token)
{
}

const char *
vivi_compiler_goto_name_get_name (ViviCompilerGotoName *goto_)
{
  g_return_val_if_fail (VIVI_IS_COMPILER_GOTO_NAME (goto_), NULL);

  return goto_->name;
}

void
vivi_compiler_goto_name_set_label (ViviCompilerGotoName *goto_,
    ViviCodeLabel *label)
{
  g_return_if_fail (VIVI_IS_COMPILER_GOTO_NAME (goto_));
  g_return_if_fail (VIVI_IS_CODE_LABEL (label));

  VIVI_CODE_GOTO (goto_)->label = g_object_ref (label);
}

ViviCodeStatement *
vivi_compiler_goto_name_new (const char *name)
{
  ViviCompilerGotoName *goto_;

  g_return_val_if_fail (name != NULL, NULL);

  goto_ = g_object_new (VIVI_TYPE_COMPILER_GOTO_NAME, NULL);
  goto_->name = g_strdup (name);

  return VIVI_CODE_STATEMENT (goto_);
}
