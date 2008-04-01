/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include <math.h>

#include "swfdec_movie.h"
#include "swfdec_as_strings.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_player_internal.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_resource.h"

static void
mc_x_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double d;

  swfdec_movie_update (movie);
  d = SWFDEC_TWIPS_TO_DOUBLE (movie->matrix.x0);
  SWFDEC_AS_VALUE_SET_NUMBER (rval, d);
}

static void
mc_x_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;

  d = swfdec_as_value_to_number (SWFDEC_AS_OBJECT (movie)->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to move %s._x to a non-finite value, ignoring", movie->name);
    return;
  }
  movie->modified = TRUE;
  d = SWFDEC_DOUBLE_TO_TWIPS (d);
  if (d != movie->matrix.x0) {
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->matrix.x0 = d;
  }
}

static void
mc_y_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double d;

  swfdec_movie_update (movie);
  d = SWFDEC_TWIPS_TO_DOUBLE (movie->matrix.y0);
  SWFDEC_AS_VALUE_SET_NUMBER (rval, d);
}

static void
mc_y_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;

  d = swfdec_as_value_to_number (SWFDEC_AS_OBJECT (movie)->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to move %s._y to a non-finite value, ignoring", movie->name);
    return;
  }
  movie->modified = TRUE;
  d = SWFDEC_DOUBLE_TO_TWIPS (d);
  if (d != movie->matrix.y0) {
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->matrix.y0 = d;
  }
}

static void
mc_xscale_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SWFDEC_AS_VALUE_SET_NUMBER (rval, movie->xscale);
}

static void
mc_xscale_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;

  d = swfdec_as_value_to_number (SWFDEC_AS_OBJECT (movie)->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set xscale to a non-finite value, ignoring");
    return;
  }
  movie->modified = TRUE;
  if (movie->xscale != d) {
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->xscale = d;
  }
}

static void
mc_yscale_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SWFDEC_AS_VALUE_SET_NUMBER (rval, movie->yscale);
}

static void
mc_yscale_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;

  d = swfdec_as_value_to_number (SWFDEC_AS_OBJECT (movie)->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set yscale to a non-finite value, ignoring");
    return;
  }
  movie->modified = TRUE;
  if (movie->yscale != d) {
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->yscale = d;
  }
}

static void
mc_name_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SWFDEC_AS_VALUE_SET_STRING (rval, movie->name);
}

static void
mc_name_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  movie->name = swfdec_as_value_to_string (SWFDEC_AS_OBJECT (movie)->context, val);
}

static void
mc_currentframe (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  g_assert (SWFDEC_IS_SPRITE_MOVIE (movie));
  SWFDEC_AS_VALUE_SET_NUMBER (rval, SWFDEC_SPRITE_MOVIE (movie)->frame);
}

static void
mc_framesloaded (SwfdecMovie *mov, SwfdecAsValue *rval)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);

  SWFDEC_AS_VALUE_SET_INT (rval, 
      swfdec_sprite_movie_get_frames_loaded (movie));
}

static void
mc_totalframes (SwfdecMovie *mov, SwfdecAsValue *rval)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);

  SWFDEC_AS_VALUE_SET_INT (rval, 
      swfdec_sprite_movie_get_frames_total (movie));
}

static void
mc_alpha_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SWFDEC_AS_VALUE_SET_NUMBER (rval,
      movie->color_transform.aa * 100.0 / 256.0);
}

static void
mc_alpha_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;
  int alpha;

  d = swfdec_as_value_to_number (SWFDEC_AS_OBJECT (movie)->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set alpha to a non-finite value, ignoring");
    return;
  }
  alpha = d * 256.0 / 100.0;
  if (alpha != movie->color_transform.aa) {
    movie->color_transform.aa = alpha;
    swfdec_movie_invalidate_last (movie);
  }
}

static void
mc_visible_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, movie->visible);
}

static void
mc_visible_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  gboolean b;

  b = swfdec_as_value_to_boolean (SWFDEC_AS_OBJECT (movie)->context, val);
  if (b != movie->visible) {
    movie->visible = b;
    swfdec_movie_invalidate_last (movie);
  }
}

static void
mc_width_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double d;

  swfdec_movie_update (movie);
  d = rint (movie->extents.x1 - movie->extents.x0);
  d = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) d);
  SWFDEC_AS_VALUE_SET_NUMBER (rval, d);
}

static void
mc_width_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d, cur;

  /* property was readonly in Flash 4 and before */
  if (SWFDEC_AS_OBJECT (movie)->context->version < 5)
    return;
  d = swfdec_as_value_to_number (SWFDEC_AS_OBJECT (movie)->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set width to a non-finite value, ignoring");
    return;
  }
  swfdec_movie_update (movie);
  movie->modified = TRUE;
  cur = rint (movie->original_extents.x1 - movie->original_extents.x0);
  cur = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) cur);
  if (cur != 0) {
    d = 100 * d / cur;
    if (d == movie->xscale)
      return;
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->xscale = d;
  } else {
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->xscale = 0;
    movie->yscale = 0;
  }
}

static void
mc_height_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double d;

  swfdec_movie_update (movie);
  d = rint (movie->extents.y1 - movie->extents.y0);
  d = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) d);
  SWFDEC_AS_VALUE_SET_NUMBER (rval, d);
}

static void
mc_height_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d, cur;

  /* property was readonly in Flash 4 and before */
  if (SWFDEC_AS_OBJECT (movie)->context->version < 5)
    return;
  d = swfdec_as_value_to_number (SWFDEC_AS_OBJECT (movie)->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set height to a non-finite value, ignoring");
    return;
  }
  swfdec_movie_update (movie);
  movie->modified = TRUE;
  cur = rint (movie->original_extents.y1 - movie->original_extents.y0);
  cur = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) cur);
  if (cur != 0) {
    d = 100 * d / cur;
    if (d == movie->yscale)
      return;
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->yscale = d;
  } else {
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->xscale = 0;
    movie->yscale = 0;
  }
}

static void
mc_rotation_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SWFDEC_AS_VALUE_SET_NUMBER (rval, movie->rotation);
}

static void
mc_rotation_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;

  /* FIXME: Flash 4 handles this differently */
  d = swfdec_as_value_to_number (SWFDEC_AS_OBJECT (movie)->context, val);
  if (isnan (d)) {
    SWFDEC_WARNING ("setting rotation to NaN - not allowed");
    return;
  }
  d = fmod (d, 360.0);
  if (d > 180.0)
    d -= 360.0;
  if (d < -180.0)
    d += 360.0;
  if (SWFDEC_AS_OBJECT (movie)->context->version < 5) {
    if (!isfinite (d))
      return;
    SWFDEC_ERROR ("FIXME: implement correct rounding errors here");
  }
  movie->modified = TRUE;
  if (movie->rotation != d) {
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
    movie->rotation = d;
  }
}

static void
mc_xmouse_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double x, y;

  swfdec_movie_get_mouse (movie, &x, &y);
  x = SWFDEC_TWIPS_TO_DOUBLE (rint (x));
  SWFDEC_AS_VALUE_SET_NUMBER (rval, x);
}

static void
mc_ymouse_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double x, y;

  swfdec_movie_get_mouse (movie, &x, &y);
  y = SWFDEC_TWIPS_TO_DOUBLE (rint (y));
  SWFDEC_AS_VALUE_SET_NUMBER (rval, y);
}

static void
mc_parent (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  if (movie->parent) {
    SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (movie->parent));
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (rval);
  }
}

static void
mc_root (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  movie = swfdec_movie_get_root (movie);
  SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (movie));
}

static void
mc_target_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  GString *s;

  s = g_string_new ("");
  while (movie->parent) {
    g_string_prepend (s, movie->name);
    g_string_prepend_c (s, '/');
    movie = movie->parent;
  }
  if (s->len == 0) {
    SWFDEC_AS_VALUE_SET_STRING (rval, SWFDEC_AS_STR_SLASH);
    g_string_free (s, TRUE);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (rval, swfdec_as_context_give_string (
	  SWFDEC_AS_OBJECT (movie)->context, g_string_free (s, FALSE)));
  }
}

static void
mc_url_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SWFDEC_AS_VALUE_SET_STRING (rval, swfdec_as_context_get_string (
	SWFDEC_AS_OBJECT (movie)->context,
	swfdec_url_get_url (swfdec_loader_get_url (movie->resource->loader))));
}

static void
mc_focusrect_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SwfdecAsContext *cx;
  SwfdecActor *actor;
  
  if (!SWFDEC_IS_ACTOR (movie)) {
    SWFDEC_FIXME ("should not be possible to get _focusrect on non-actors");
    return;
  }
  actor = SWFDEC_ACTOR (movie);
  cx = SWFDEC_AS_OBJECT (actor)->context;

  switch (actor->focusrect) {
    case SWFDEC_FLASH_YES:
      if (cx->version > 5)
	SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
      else
	SWFDEC_AS_VALUE_SET_INT (rval, 1);
      break;
    case SWFDEC_FLASH_NO:
      if (cx->version > 5)
	SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
      else
	SWFDEC_AS_VALUE_SET_INT (rval, 0);
      break;
    case SWFDEC_FLASH_MAYBE:
      SWFDEC_AS_VALUE_SET_NULL (rval);
      break;
    default:
      g_assert_not_reached();
  }
}

static void
mc_focusrect_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  SwfdecAsContext *cx;
  SwfdecActor *actor;
  SwfdecFlashBool b;

  if (!SWFDEC_IS_ACTOR (movie)) {
    SWFDEC_FIXME ("should not be possible to get _focusrect on non-actors");
    return;
  }
  cx = SWFDEC_AS_OBJECT (movie)->context;
  actor = SWFDEC_ACTOR (movie);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (val) ||
      SWFDEC_AS_VALUE_IS_NULL (val)) {
    if (movie->parent == NULL)
      return;
    b = SWFDEC_FLASH_MAYBE;
  } else {
    if (movie->parent == NULL) {
      double d = swfdec_as_value_to_number (cx, val);
      if (isnan (d))
	return;
      b = d ? SWFDEC_FLASH_YES : SWFDEC_FLASH_NO;
    } else {
      b = swfdec_as_value_to_boolean (cx, val) ? SWFDEC_FLASH_YES : SWFDEC_FLASH_NO;
    }
  }

  if (b != actor->focusrect) {
    SwfdecPlayerPrivate *priv = SWFDEC_PLAYER (cx)->priv;
    gboolean had_focusrect = swfdec_actor_has_focusrect (priv->focus);
    actor->focusrect = b;
    if (had_focusrect != swfdec_actor_has_focusrect (priv->focus))
      swfdec_movie_invalidate_last (SWFDEC_MOVIE (priv->focus));
  }
}

struct {
  gboolean needs_movie;
  const char *name;
  void (* get) (SwfdecMovie *movie, SwfdecAsValue *ret);
  void (* set) (SwfdecMovie *movie, const SwfdecAsValue *val);
} swfdec_movieclip_props[] = {
  { 0, SWFDEC_AS_STR__x,		mc_x_get,	    mc_x_set },
  { 0, SWFDEC_AS_STR__y,		mc_y_get,	    mc_y_set },
  { 0, SWFDEC_AS_STR__xscale,		mc_xscale_get,	    mc_xscale_set },
  { 0, SWFDEC_AS_STR__yscale,		mc_yscale_get,	    mc_yscale_set },
  { 1, SWFDEC_AS_STR__currentframe,	mc_currentframe,    NULL },
  { 1, SWFDEC_AS_STR__totalframes,	mc_totalframes,	    NULL },
  { 0, SWFDEC_AS_STR__alpha,		mc_alpha_get,	    mc_alpha_set },
  { 0, SWFDEC_AS_STR__visible,		mc_visible_get,	    mc_visible_set },
  { 0, SWFDEC_AS_STR__width,		mc_width_get,	    mc_width_set },
  { 0, SWFDEC_AS_STR__height,		mc_height_get,	    mc_height_set },
  { 0, SWFDEC_AS_STR__rotation,		mc_rotation_get,    mc_rotation_set },
  { 1, SWFDEC_AS_STR__target,		mc_target_get,	    NULL },
  { 1, SWFDEC_AS_STR__framesloaded,	mc_framesloaded,    NULL},
  { 0, SWFDEC_AS_STR__name,		mc_name_get,	    mc_name_set },
  { 1, SWFDEC_AS_STR__droptarget,	NULL,		    NULL }, //"_droptarget"
  { 0, SWFDEC_AS_STR__url,		mc_url_get,	    NULL },
  { 0, SWFDEC_AS_STR__highquality,	NULL,		    NULL }, //"_highquality"
  { 0, SWFDEC_AS_STR__focusrect,	mc_focusrect_get,   mc_focusrect_set }, //"_focusrect"
  { 0, SWFDEC_AS_STR__soundbuftime,	NULL,		    NULL }, //"_soundbuftime"
  { 0, SWFDEC_AS_STR__quality,		NULL,		    NULL }, //"_quality"
  { 0, SWFDEC_AS_STR__xmouse,		mc_xmouse_get,	    NULL },
  { 0, SWFDEC_AS_STR__ymouse,		mc_ymouse_get,	    NULL },
  { 0, SWFDEC_AS_STR__parent,		mc_parent,	    NULL },
  { 0, SWFDEC_AS_STR__root,		mc_root,	    NULL },
};

static int
swfdec_movie_get_asprop_index (SwfdecMovie *movie, const char *name)
{
  guint i;

  if (name < SWFDEC_AS_STR__x || name > SWFDEC_AS_STR__root)
    return -1;

  for (i = 0; i < G_N_ELEMENTS (swfdec_movieclip_props); i++) {
    if (swfdec_movieclip_props[i].name == name) {
      if (swfdec_movieclip_props[i].needs_movie && !SWFDEC_IS_SPRITE_MOVIE (movie))
	return -1;
      if (swfdec_movieclip_props[i].get == NULL) {
	SWFDEC_ERROR ("property %s not implemented", name);
      }
      return i;
    }
  }
  g_assert_not_reached ();
  return -1;
}

gboolean
swfdec_movie_set_asprop (SwfdecMovie *movie, const char *name, const SwfdecAsValue *val)
{
  int i;
  
  i = swfdec_movie_get_asprop_index (movie, name);
  if (i == -1)
    return FALSE;
  if (swfdec_movieclip_props[i].set != NULL) {
    swfdec_movieclip_props[i].set (movie, val);
  }
  return TRUE;
}

gboolean
swfdec_movie_get_asprop (SwfdecMovie *movie, const char *name, SwfdecAsValue *val)
{
  int i;
  
  i = swfdec_movie_get_asprop_index (movie, name);
  if (i == -1)
    return FALSE;
  if (swfdec_movieclip_props[i].get != NULL) {
    swfdec_movieclip_props[i].get (movie, val);
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (val);
  }
  return TRUE;
}

