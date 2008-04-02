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

#include "vivi_compiler_get_temporary.h"

#include "vivi_code_get.h"
#include "vivi_code_constant.h"
#include "vivi_code_printer.h"

G_DEFINE_TYPE (ViviCompilerGetTemporary, vivi_compiler_get_temporary, VIVI_TYPE_CODE_GET)

static void
vivi_compiler_get_temporary_class_init (ViviCompilerGetTemporaryClass *klass)
{
}

static void
vivi_compiler_get_temporary_init (ViviCompilerGetTemporary *get)
{
}

ViviCodeValue *
vivi_compiler_get_temporary_new (void)
{
  static int counter = 0;
  ViviCompilerGetTemporary *get;
  char *name = g_strdup_printf ("_%i", ++counter);

  get = g_object_new (VIVI_TYPE_COMPILER_GET_TEMPORARY, NULL);
  VIVI_CODE_GET (get)->name = vivi_code_constant_new_string (name);

  g_free (name);

  return VIVI_CODE_VALUE (get);
}
