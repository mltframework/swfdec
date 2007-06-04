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

#include "swfdec_as_object.h"
#include "swfdec_debug.h"
#include "swfdec_listener.h"
#include "swfdec_player_internal.h"

static void
swfdec_mouse_addListener (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *return_value)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (object->context);

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    return;
  swfdec_listener_add (player->mouse_listener, SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]));
}

static void
swfdec_mouse_removeListener (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *return_value)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (object->context);

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    return;
  swfdec_listener_remove (player->mouse_listener, SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]));
}

static void
swfdec_mouse_show (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *return_value)
{
  SWFDEC_PLAYER (object->context)->mouse_visible = TRUE;
}

static void
swfdec_mouse_hide (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *return_value)
{
  SWFDEC_PLAYER (object->context)->mouse_visible = FALSE;
}

static void
swfdec_mouse_construct (SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *return_value)
{
}

void
swfdec_mouse_init_context (SwfdecPlayer *player, guint version)
{
  SwfdecAsObject *mouse;
  
  mouse = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (SWFDEC_AS_CONTEXT (player)->global,
      SWFDEC_AS_STR_Mouse, 0, swfdec_mouse_construct, 0));
  if (!mouse)
    return;

  swfdec_as_object_add_function (mouse, SWFDEC_AS_STR_addListener, 0, swfdec_mouse_addListener, 1);
  swfdec_as_object_add_function (mouse, SWFDEC_AS_STR_removeListener, 0, swfdec_mouse_removeListener, 1);
  swfdec_as_object_add_function (mouse, SWFDEC_AS_STR_hide, 0, swfdec_mouse_hide, 0);
  swfdec_as_object_add_function (mouse, SWFDEC_AS_STR_show, 0, swfdec_mouse_show, 0);
}

