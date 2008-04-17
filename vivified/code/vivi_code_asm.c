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

#include "vivi_code_asm.h"

static void
vivi_code_asm_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (G_UNLIKELY (!initialized)) {
    initialized = TRUE;
  }
}

GType
vivi_code_asm_get_type (void)
{
  static GType code_asm_type = 0;
  
  if (!code_asm_type) {
    static const GTypeInfo code_asm_info = {
      sizeof (ViviCodeAsmInterface),
      vivi_code_asm_base_init,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
    };
    
    code_asm_type = g_type_register_static (G_TYPE_INTERFACE,
        "ViviCodeAsm", &code_asm_info, 0);
    g_type_interface_add_prerequisite (code_asm_type, G_TYPE_OBJECT);
  }
  
  return code_asm_type;
}

