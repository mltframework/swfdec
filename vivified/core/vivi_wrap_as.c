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

VIVI_FUNCTION ("wrap_toString", vivi_wrap_toString)
void
vivi_wrap_toString (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviWrap *wrap;
  char *s;

  if (!VIVI_IS_WRAP (this))
    return;
  
  wrap = VIVI_WRAP (this);
  if (wrap->wrap == NULL)
    return;
  
  s = swfdec_as_object_get_debug (wrap->wrap);
  SWFDEC_AS_VALUE_SET_STRING (retval, swfdec_as_context_give_string (cx, s));
}

VIVI_FUNCTION ("wrap_get", vivi_wrap_get)
void
vivi_wrap_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  ViviWrap *wrap;
  SwfdecAsValue val;
  const char *name;

  if (!VIVI_IS_WRAP (this) || argc == 0)
    return;
  wrap = VIVI_WRAP (this);
  if (wrap->wrap == NULL)
    return;
  
  name = swfdec_as_value_to_string (cx, &argv[0]);
  swfdec_as_object_get_variable (wrap->wrap, 
      swfdec_as_context_get_string (SWFDEC_AS_CONTEXT (app->player), name), 
      &val);
  vivi_wrap_value (app, retval, &val);
}

/*** FRAME specific code ***/

VIVI_FUNCTION ("frame_name_get", vivi_wrap_name_get)
void
vivi_wrap_name_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
#if 0
  ViviWrap *wrap;
  char *s;

  if (!VIVI_IS_WRAP (this))
    return;
  
  wrap = VIVI_WRAP (this);
  
  /* FIXME: improve */
  s = swfdec_as_object_get_debug (wrap->wrap);
  SWFDEC_AS_VALUE_SET_STRING (retval, swfdec_as_context_give_string (cx, s));
#endif
}

VIVI_FUNCTION ("frame_code_get", vivi_wrap_code_get)
void
vivi_wrap_code_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
#if 0
  ViviWrap *wrap;
  SwfdecScript *script;

  if (!VIVI_IS_WRAP (this))
    return;
  
  wrap = VIVI_WRAP (this);
  if (!SWFDEC_IS_AS_FRAME (wrap->wrap))
    return;
  
  script = swfdec_as_frame_get_script (SWFDEC_AS_FRAME (wrap->wrap));
  /* FIXME: wrap scripts */
  if (script)
    SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);
#endif
}

VIVI_FUNCTION ("frame_next_get", vivi_wrap_next_get)
void
vivi_wrap_next_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
#if 0
  ViviWrap *wrap;
  SwfdecAsObject *obj;

  if (!VIVI_IS_WRAP (this))
    return;
  
  wrap = VIVI_WRAP (this);
  if (!SWFDEC_IS_AS_FRAME (wrap->wrap))
    return;
  
  obj = SWFDEC_AS_OBJECT (swfdec_as_frame_get_next (SWFDEC_AS_FRAME (wrap->wrap)));
  if (obj)
    SWFDEC_AS_VALUE_SET_OBJECT (retval, vivi_wrap_object (VIVI_APPLICATION (cx), obj));
#endif
}

VIVI_FUNCTION ("frame_this_get", vivi_wrap_this_get)
void
vivi_wrap_this_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
#if 0
  ViviWrap *wrap;
  SwfdecAsObject *obj;

  if (!VIVI_IS_WRAP (this))
    return;
  
  wrap = VIVI_WRAP (this);
  if (!SWFDEC_IS_AS_FRAME (wrap->wrap))
    return;
  
  obj = SWFDEC_AS_OBJECT (swfdec_as_frame_get_this (SWFDEC_AS_FRAME (wrap->wrap)));
  if (obj)
    SWFDEC_AS_VALUE_SET_OBJECT (retval, vivi_wrap_object (VIVI_APPLICATION (cx), obj));
#endif
}


