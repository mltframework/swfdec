/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
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
#include "swfdec_as_strings.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_sandbox.h"
#include "swfdec_button_movie.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_text_field_movie.h"

SWFDEC_AS_NATIVE (600, 0, swfdec_selection_getBeginIndex)
void
swfdec_selection_getBeginIndex (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.getBeginIndex (static)");
}

SWFDEC_AS_NATIVE (600, 1, swfdec_selection_getEndIndex)
void
swfdec_selection_getEndIndex (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.getEndIndex (static)");
}

SWFDEC_AS_NATIVE (600, 2, swfdec_selection_getCaretIndex)
void
swfdec_selection_getCaretIndex (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.getCaretIndex (static)");
}

SWFDEC_AS_NATIVE (600, 3, swfdec_selection_getFocus)
void
swfdec_selection_getFocus (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayerPrivate *priv = SWFDEC_PLAYER (cx)->priv;

  if (priv->focus) {
    char *s = swfdec_movie_get_path (SWFDEC_MOVIE (priv->focus), TRUE);
    SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, s));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static gboolean
swfdec_actor_can_grab_focus (SwfdecActor *actor)
{
  SwfdecAsValue val;

  /* Functions like this just make me love Flash */
  if (SWFDEC_IS_SPRITE_MOVIE (actor) ||
      SWFDEC_IS_BUTTON_MOVIE (actor)) {
    if (SWFDEC_MOVIE (actor)->parent == NULL)
      return FALSE;
    if (!swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (actor),
	SWFDEC_AS_STR_focusEnabled, &val))
      return FALSE;
    return swfdec_as_value_to_boolean (SWFDEC_AS_OBJECT (actor)->context, &val);
  } else if (SWFDEC_IS_TEXT_FIELD_MOVIE (actor)) {
    /* cool that you can select all textfields, eh? */
    return TRUE;
  } else {
    return FALSE;
  }
}

SWFDEC_AS_NATIVE (600, 4, swfdec_selection_setFocus)
void
swfdec_selection_setFocus (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecActor *actor;
  SwfdecSandbox *sandbox;

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
  SWFDEC_AS_CHECK (0, NULL, "O", &actor);

  if (actor != NULL) {
    if (!SWFDEC_IS_ACTOR (actor) ||
	!swfdec_actor_can_grab_focus (actor))
      return;
  }

  /* FIXME: how is security handled here? */
  sandbox = SWFDEC_SANDBOX (cx->global);
  swfdec_sandbox_unuse (sandbox);
  swfdec_player_grab_focus (SWFDEC_PLAYER (cx), actor);
  swfdec_sandbox_use (sandbox);
  if (actor == NULL) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
  }
  return;
}

SWFDEC_AS_NATIVE (600, 5, swfdec_selection_setSelection)
void
swfdec_selection_setSelection (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.setSelection (static)");
}
