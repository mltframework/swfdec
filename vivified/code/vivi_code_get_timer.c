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

#include "vivi_code_get_timer.h"

G_DEFINE_TYPE (ViviCodeGetTimer, vivi_code_get_timer, VIVI_TYPE_CODE_BUILTIN_CALL)

static void
vivi_code_get_timer_class_init (ViviCodeGetTimerClass *klass)
{
  ViviCodeBuiltinCallClass *builtin_class =
    VIVI_CODE_BUILTIN_CALL_CLASS (klass);

  builtin_class->function_name = "getTimer";
  builtin_class->bytecode = SWFDEC_AS_ACTION_GET_TIME;
}

static void
vivi_code_get_timer_init (ViviCodeBuiltinCall *builtin_statement)
{
}

ViviCodeValue *
vivi_code_get_timer_new (void)
{
  return VIVI_CODE_VALUE (g_object_new (vivi_code_get_timer_get_type (), NULL));
}
