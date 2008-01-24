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
#include <swfdec-gtk/swfdec-gtk.h>

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

VIVI_FUNCTION ("player_filename_get", vivi_player_as_filename_get)
void
vivi_player_as_filename_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  const char *s = vivi_application_get_filename (app);

  if (s)
    SWFDEC_AS_VALUE_SET_STRING (retval, swfdec_as_context_get_string (cx, s));
}

VIVI_FUNCTION ("player_filename_set", vivi_player_as_filename_set)
void
vivi_player_as_filename_set (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  const char *s;
  
  if (argc == 0)
    return;
  s = swfdec_as_value_to_string (cx, &argv[0]);
  
  vivi_application_set_filename (app, s);
}

VIVI_FUNCTION ("player_variables_get", vivi_player_as_variables_get)
void
vivi_player_as_variables_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  const char *s = vivi_application_get_variables (app);

  if (s)
    SWFDEC_AS_VALUE_SET_STRING (retval, swfdec_as_context_get_string (cx, s));
}

VIVI_FUNCTION ("player_variables_set", vivi_player_as_variables_set)
void
vivi_player_as_variables_set (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  const char *s;
  
  if (argc == 0)
    return;
  s = swfdec_as_value_to_string (cx, &argv[0]);
  
  vivi_application_set_variables (app, s);
}

VIVI_FUNCTION ("player_global_get", vivi_player_as_global_get)
void
vivi_player_as_global_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  
  if (SWFDEC_AS_CONTEXT (app->player)->global) {
    SWFDEC_AS_VALUE_SET_OBJECT (retval, vivi_wrap_object (app, 
	  SWFDEC_AS_CONTEXT (app->player)->global));
  }
}

VIVI_FUNCTION ("player_sound_get", vivi_player_as_sound_get)
void
vivi_player_as_sound_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  
  SWFDEC_AS_VALUE_SET_BOOLEAN (retval,
      swfdec_gtk_player_get_audio_enabled (SWFDEC_GTK_PLAYER (app->player)));
}

VIVI_FUNCTION ("player_sound_set", vivi_player_as_sound_set)
void
vivi_player_as_sound_set (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  
  if (argc == 0)
    return;
  swfdec_gtk_player_set_audio_enabled (SWFDEC_GTK_PLAYER (app->player),
      swfdec_as_value_to_boolean (cx, &argv[0]));
}

