/* Swfdec
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

#include "swfdec_as_internal.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

SWFDEC_AS_NATIVE (800, 0, swfdec_key_getAscii)
void
swfdec_key_getAscii (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);

  SWFDEC_AS_VALUE_SET_INT (retval, player->priv->last_character);
}

SWFDEC_AS_NATIVE (800, 1, swfdec_key_getCode)
void
swfdec_key_getCode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);

  SWFDEC_AS_VALUE_SET_INT (retval, player->priv->last_keycode);
}

SWFDEC_AS_NATIVE (800, 2, swfdec_key_isDown)
void
swfdec_key_isDown (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  guint id;
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);

  if (argc < 1)
    return;

  id = swfdec_as_value_to_integer (cx, &argv[0]);
  if (id >= 256) {
    SWFDEC_FIXME ("id %u too big for a keycode", id);
    id %= 256;
  }
  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, (player->priv->key_pressed[id / 8] & (1 << (id % 8))) ? TRUE : FALSE);
}

SWFDEC_AS_NATIVE (800, 3, swfdec_key_isToggled)
void
swfdec_key_isToggled (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SWFDEC_STUB ("Key.isToggled (static)");
}

SWFDEC_AS_NATIVE (800, 4, swfdec_key_isAccessible)
void
swfdec_key_isAccessible (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SWFDEC_STUB ("Key.isAccessible (static)");
}
