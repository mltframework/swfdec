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

#include "vivi_code_goto.h"

G_DEFINE_TYPE (ViviCodeGoto, vivi_code_goto, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_goto_dispose (GObject *object)
{
  ViviCodeGoto *gotoo = VIVI_CODE_GOTO (object);

  g_object_unref (gotoo->label);

  G_OBJECT_CLASS (vivi_code_goto_parent_class)->dispose (object);
}

static char *
vivi_code_goto_to_code (ViviCodeToken *token)
{
  ViviCodeGoto *gotoo = VIVI_CODE_GOTO (token);

  return g_strdup_printf ("  goto %s;\n", vivi_code_label_get_name (gotoo->label));
}

static void
vivi_code_goto_class_init (ViviCodeGotoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_goto_dispose;

  token_class->to_code = vivi_code_goto_to_code;
}

static void
vivi_code_goto_init (ViviCodeGoto *token)
{
}

ViviCodeToken *
vivi_code_goto_new (ViviCodeLabel *label)
{
  ViviCodeGoto *gotoo;

  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), NULL);

  gotoo = g_object_new (VIVI_TYPE_CODE_GOTO, NULL);
  gotoo->label = g_object_ref (label);

  return VIVI_CODE_TOKEN (gotoo);
}

