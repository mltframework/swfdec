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

  switch (player->scale_mode) {
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

  if (argc == 0)
    return;
  s = swfdec_as_value_to_string (cx, &argv[0]);
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

