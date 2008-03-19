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

#include "vivi_code_comment.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCodeComment, vivi_code_comment, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_comment_dispose (GObject *object)
{
  ViviCodeComment *comment = VIVI_CODE_COMMENT (object);

  g_free (comment->comment);

  G_OBJECT_CLASS (vivi_code_comment_parent_class)->dispose (object);
}

static void
vivi_code_comment_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeComment *comment = VIVI_CODE_COMMENT (token);

  vivi_code_printer_new_line (printer, FALSE);
  vivi_code_printer_print (printer, "/* ");
  vivi_code_printer_print (printer, comment->comment);
  vivi_code_printer_print (printer, " */");
}

static void
vivi_code_comment_class_init (ViviCodeCommentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_comment_dispose;

  token_class->print = vivi_code_comment_print;
}

static void
vivi_code_comment_init (ViviCodeComment *token)
{
}

ViviCodeToken *
vivi_code_comment_new (const char *comment)
{
  ViviCodeComment *ret;

  g_return_val_if_fail (comment != NULL, NULL);

  ret = g_object_new (VIVI_TYPE_CODE_COMMENT, NULL);
  ret->comment = g_strdup (comment);

  return VIVI_CODE_TOKEN (ret);
}

