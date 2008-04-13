/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_code_play.h"

G_DEFINE_TYPE (ViviCodePlay, vivi_code_play, VIVI_TYPE_CODE_SPECIAL_STATEMENT)

static void
vivi_code_play_class_init (ViviCodePlayClass *klass)
{
}

static void
vivi_code_play_init (ViviCodePlay *token)
{
  ViviCodeSpecialStatement *stmt = VIVI_CODE_SPECIAL_STATEMENT (token);

  stmt->name = "play";
  stmt->action = SWFDEC_AS_ACTION_PLAY;
}

ViviCodeStatement *
vivi_code_play_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_PLAY, NULL);
}
