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

#include "swfdec_edittext.h"
#include "swfdec_edittext_movie.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"

static void
swfdec_edittext_movie_get_text (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecEditTextMovie *text;

  if (!SWFDEC_IS_EDIT_TEXT_MOVIE (object))
    return;
  text = SWFDEC_EDIT_TEXT_MOVIE (object);

  SWFDEC_AS_VALUE_SET_STRING (ret, (text->str != NULL ? swfdec_as_context_get_string (cx, text->str) : SWFDEC_AS_STR_EMPTY));
}

static void
swfdec_edittext_movie_do_set_text (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecEditTextMovie *text;

  if (!SWFDEC_IS_EDIT_TEXT_MOVIE (object))
    return;
  text = SWFDEC_EDIT_TEXT_MOVIE (object);

  if (argc < 1)
    return;

  swfdec_edit_text_movie_set_text (text,
      swfdec_as_value_to_string (cx, &argv[0]));
}

SWFDEC_AS_NATIVE (104, 200, swfdec_edittext_movie_createTextField)
void
swfdec_edittext_movie_createTextField (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SwfdecMovie *movie, *parent;
  SwfdecEditText *edittext;
  int depth;
  const char *name;
  SwfdecAsFunction *fun;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, (gpointer)&parent, "si", &name, &depth);

  edittext = g_object_new (SWFDEC_TYPE_EDIT_TEXT, NULL);

  movie = swfdec_movie_find (parent, depth);
  if (movie)
    swfdec_movie_remove (movie);

  movie = swfdec_movie_new (SWFDEC_PLAYER (cx), depth, parent,
      SWFDEC_GRAPHIC (edittext), name);
  g_assert (SWFDEC_IS_EDIT_TEXT_MOVIE (movie));
  swfdec_movie_initialize (movie);

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (&val);
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    return;

  /* set initial variables */
  if (swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (fun),
	SWFDEC_AS_STR_prototype, &val)) {
    swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (movie),
	SWFDEC_AS_STR___proto__, &val,
	SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  }
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (fun));
  if (cx->version < 7) {
    swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (movie),
	SWFDEC_AS_STR_constructor, &val, SWFDEC_AS_VARIABLE_HIDDEN);
  }
  swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (movie),
      SWFDEC_AS_STR___constructor__, &val,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_VERSION_6_UP);

  swfdec_as_function_call (fun, SWFDEC_AS_OBJECT (movie), 0, NULL, rval);
  cx->frame->construct = TRUE;
  swfdec_as_context_run (cx);
}

static void
swfdec_edittext_movie_add_variable (SwfdecAsObject *object,
    const char *variable, SwfdecAsNative get, SwfdecAsNative set)
{
  SwfdecAsFunction *get_func, *set_func;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (get != NULL);

  get_func =
    swfdec_as_native_function_new (object->context, variable, get, 0, NULL);
  if (get_func == NULL)
    return;

  if (set != NULL) {
    set_func =
      swfdec_as_native_function_new (object->context, variable, set, 0, NULL);
  } else {
    set_func = NULL;
  }

  swfdec_as_object_add_variable (object, variable, get_func, set_func, 0);
}

SWFDEC_AS_CONSTRUCTOR (104, 0, swfdec_edittext_movie_construct, swfdec_edit_text_movie_get_type)
void
swfdec_edittext_movie_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!cx->frame->construct) {
    SwfdecAsValue val;
    if (!swfdec_as_context_use_mem (cx, sizeof (SwfdecEditTextMovie)))
      return;
    object = g_object_new (SWFDEC_TYPE_EDIT_TEXT_MOVIE, NULL);
    swfdec_as_object_add (object, cx, sizeof (SwfdecEditTextMovie));
    swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_TextField, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      swfdec_as_object_set_constructor (object, SWFDEC_AS_VALUE_GET_OBJECT (&val));
    } else {
      SWFDEC_INFO ("\"TextField\" is not an object");
    }
  }

  g_return_if_fail (SWFDEC_IS_EDIT_TEXT_MOVIE (object));

  if (!SWFDEC_PLAYER (cx)->edittext_movie_properties_initialized) {
    SwfdecAsValue val;
    SwfdecAsObject *proto;

    swfdec_as_object_get_variable (object, SWFDEC_AS_STR___proto__, &val);
    g_return_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val));
    proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

    swfdec_edittext_movie_add_variable (proto, SWFDEC_AS_STR_text,
	swfdec_edittext_movie_get_text, swfdec_edittext_movie_do_set_text);

    SWFDEC_PLAYER (cx)->edittext_movie_properties_initialized = TRUE;
  }

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}
