/* Vivified
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "vivi_wrap.h"
#include "vivi_application.h"
#include "vivi_function.h"

VIVI_FUNCTION ("player_frame_get", vivi_player_as_frame_get)
void
vivi_player_as_frame_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  SwfdecAsObject *obj;

  obj = SWFDEC_AS_OBJECT (swfdec_as_context_get_frame (SWFDEC_AS_CONTEXT (app->player)));
  if (obj)
    SWFDEC_AS_VALUE_SET_OBJECT (retval, vivi_wrap_object (app, obj));
}

