/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_as_internal.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

SWFDEC_AS_NATIVE (5, 0, swfdec_mouse_show)
void
swfdec_mouse_show (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);

  SWFDEC_AS_VALUE_SET_INT (retval, player->priv->mouse_visible ? 1 : 0);
  player->priv->mouse_visible = TRUE;
}

SWFDEC_AS_NATIVE (5, 1, swfdec_mouse_hide)
void
swfdec_mouse_hide (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);

  SWFDEC_AS_VALUE_SET_INT (retval, player->priv->mouse_visible ? 1 : 0);
  player->priv->mouse_visible = FALSE;
}

