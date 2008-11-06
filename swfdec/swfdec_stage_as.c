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

#include <string.h>
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

/*** AS CODE ***/

SWFDEC_AS_NATIVE (666, 1, get_scaleMode)
void
get_scaleMode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);

  switch (player->priv->scale_mode) {
    case SWFDEC_SCALE_SHOW_ALL:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_showAll);
      break;
    case SWFDEC_SCALE_NO_BORDER:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_noBorder);
      break;
    case SWFDEC_SCALE_EXACT_FIT:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_exactFit);
      break;
    case SWFDEC_SCALE_NONE:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_noScale);
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

SWFDEC_AS_NATIVE (666, 2, set_scaleMode)
void
set_scaleMode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  const char *s;
  SwfdecScaleMode mode;

  SWFDEC_AS_CHECK (0, NULL, "s", &s);

  if (g_ascii_strcasecmp (s, SWFDEC_AS_STR_noBorder) == 0) {
    mode = SWFDEC_SCALE_NO_BORDER;
  } else if (g_ascii_strcasecmp (s, SWFDEC_AS_STR_exactFit) == 0) {
    mode = SWFDEC_SCALE_EXACT_FIT;
  } else if (g_ascii_strcasecmp (s, SWFDEC_AS_STR_noScale) == 0) {
    mode = SWFDEC_SCALE_NONE;
  } else {
    mode = SWFDEC_SCALE_SHOW_ALL;
  }
  swfdec_player_set_scale_mode (player, mode);
}

SWFDEC_AS_NATIVE (666, 3, get_align)
void
get_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  SwfdecPlayerPrivate *priv = player->priv;
  char s[5];
  guint i = 0;

  if (priv->align_flags & SWFDEC_ALIGN_FLAG_LEFT)
    s[i++] = 'L';
  if (priv->align_flags & SWFDEC_ALIGN_FLAG_TOP)
    s[i++] = 'T';
  if (priv->align_flags & SWFDEC_ALIGN_FLAG_RIGHT)
    s[i++] = 'R';
  if (priv->align_flags & SWFDEC_ALIGN_FLAG_BOTTOM)
    s[i++] = 'B';
  s[i] = 0;
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, s));
}

SWFDEC_AS_NATIVE (666, 4, set_align)
void
set_align (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  guint flags = 0;
  const char *s;

  SWFDEC_AS_CHECK (0, NULL, "s", &s);

  if (strchr (s, 'l') || strchr (s, 'L'))
    flags |= SWFDEC_ALIGN_FLAG_LEFT;
  if (strchr (s, 't') || strchr (s, 'T'))
    flags |= SWFDEC_ALIGN_FLAG_TOP;
  if (strchr (s, 'r') || strchr (s, 'R'))
    flags |= SWFDEC_ALIGN_FLAG_RIGHT;
  if (strchr (s, 'b') || strchr (s, 'B'))
    flags |= SWFDEC_ALIGN_FLAG_BOTTOM;

  if (flags != player->priv->align_flags) {
    player->priv->align_flags = flags;
    g_object_notify (G_OBJECT (player), "alignment");
    swfdec_player_update_scale (player);
  }
}

SWFDEC_AS_NATIVE (666, 5, get_width)
void
get_width (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayerPrivate *priv = SWFDEC_PLAYER (cx)->priv;

  if (priv->scale_mode == SWFDEC_SCALE_NONE)
    *ret = swfdec_as_value_from_integer (cx, priv->internal_width);
  else
    *ret = swfdec_as_value_from_integer (cx, priv->width);
}

SWFDEC_AS_NATIVE (666, 7, get_height)
void
get_height (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayerPrivate *priv = SWFDEC_PLAYER (cx)->priv;

  if (priv->scale_mode == SWFDEC_SCALE_NONE)
    *ret = swfdec_as_value_from_integer (cx, priv->internal_height);
  else
    *ret = swfdec_as_value_from_integer (cx, priv->height);
}

/* FIXME: do this smarter */
SWFDEC_AS_NATIVE (666, 6, set_width)
void
set_width (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
}

SWFDEC_AS_NATIVE (666, 8, set_height)
void
set_height (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
}

SWFDEC_AS_NATIVE (666, 9, swfdec_stage_get_showMenu)
void
swfdec_stage_get_showMenu (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Stage.showMenu (get)");
}

SWFDEC_AS_NATIVE (666, 10, swfdec_stage_set_showMenu)
void
swfdec_stage_set_showMenu (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Stage.showMenu (set)");
}

SWFDEC_AS_NATIVE (666, 11, swfdec_stage_get_displayState)
void
swfdec_stage_get_displayState (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayerPrivate *priv = SWFDEC_PLAYER (cx)->priv;

  if (priv->fullscreen)
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_fullScreen);
  else
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_normal);
}

SWFDEC_AS_NATIVE (666, 12, swfdec_stage_set_displayState)
void
swfdec_stage_set_displayState (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  const char *s;

  SWFDEC_AS_CHECK (0, NULL, "s", &s);

  if (g_ascii_strcasecmp (s, SWFDEC_AS_STR_normal) == 0) {
    swfdec_player_set_fullscreen (player, FALSE);
  } else if (g_ascii_strcasecmp (s, SWFDEC_AS_STR_fullScreen) == 0) {
    swfdec_player_set_fullscreen (player, TRUE);
  }
}

SWFDEC_AS_NATIVE (666, 100, swfdec_stage_get_fullScreenSourceRect)
void
swfdec_stage_get_fullScreenSourceRect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Stage.fullScreenSourceRect (get)");
}

SWFDEC_AS_NATIVE (666, 101, swfdec_stage_set_fullScreenSourceRect)
void
swfdec_stage_set_fullScreenSourceRect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Stage.fullScreenSourceRect (set)");
}

SWFDEC_AS_NATIVE (666, 102, swfdec_stage_get_fullScreenHeight)
void
swfdec_stage_get_fullScreenHeight (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Stage.fullScreenHeight (get)");
}

SWFDEC_AS_NATIVE (666, 103, swfdec_stage_set_fullScreenHeight)
void
swfdec_stage_set_fullScreenHeight (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Stage.fullScreenHeight (set)");
}

SWFDEC_AS_NATIVE (666, 104, swfdec_stage_get_fullScreenWidth)
void
swfdec_stage_get_fullScreenWidth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Stage.fullScreenWidth (get)");
}

SWFDEC_AS_NATIVE (666, 105, swfdec_stage_set_fullScreenWidth)
void
swfdec_stage_set_fullScreenWidth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Stage.fullScreenWidth (set)");
}
