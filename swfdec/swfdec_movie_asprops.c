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
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_internal.h"
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
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, d);
}

static void
mc_x_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  SwfdecTwips x;

  if (!swfdec_as_value_to_twips (swfdec_gc_object_get_context (movie), val, FALSE, &x))
    return;
  movie->modified = TRUE;
  if (x != movie->matrix.x0) {
    swfdec_movie_begin_update_matrix (movie);
    movie->matrix.x0 = x;
    swfdec_movie_end_update_matrix (movie);
  }
}

static void
mc_y_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double d;

  swfdec_movie_update (movie);
  d = SWFDEC_TWIPS_TO_DOUBLE (movie->matrix.y0);
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, d);
}

static void
mc_y_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  SwfdecTwips y;

  if (!swfdec_as_value_to_twips (swfdec_gc_object_get_context (movie), val, FALSE, &y))
    return;
  movie->modified = TRUE;
  if (y != movie->matrix.y0) {
    swfdec_movie_begin_update_matrix (movie);
    movie->matrix.y0 = y;
    swfdec_movie_end_update_matrix (movie);
  }
}

static void
mc_xscale_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, movie->xscale);
}

static void
mc_xscale_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;

  d = swfdec_as_value_to_number (swfdec_gc_object_get_context (movie), *val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set xscale to a non-finite value, ignoring");
    return;
  }
  movie->modified = TRUE;
  if (movie->xscale != d) {
    swfdec_movie_begin_update_matrix (movie);
    movie->xscale = d;
    swfdec_movie_end_update_matrix (movie);
  }
}

static void
mc_yscale_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, movie->yscale);
}

static void
mc_yscale_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;

  d = swfdec_as_value_to_number (swfdec_gc_object_get_context (movie), *val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set yscale to a non-finite value, ignoring");
    return;
  }
  movie->modified = TRUE;
  if (movie->yscale != d) {
    swfdec_movie_begin_update_matrix (movie);
    movie->yscale = d;
    swfdec_movie_end_update_matrix (movie);
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
  movie->name = swfdec_as_value_to_string (swfdec_gc_object_get_context (movie), *val);
}

static void
mc_alpha_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval,
      movie->color_transform.aa * 100.0 / 256.0);
}

static void
mc_alpha_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;
  int alpha;

  d = swfdec_as_value_to_number (swfdec_gc_object_get_context (movie), *val);
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

  b = swfdec_as_value_to_boolean (swfdec_gc_object_get_context (movie), *val);
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
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, d);
}

static void
mc_width_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d, cur;

  /* property was readonly in Flash 4 and before */
  if (swfdec_gc_object_get_context (movie)->version < 5)
    return;
  d = swfdec_as_value_to_number (swfdec_gc_object_get_context (movie), *val);
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
    swfdec_movie_begin_update_matrix (movie);
    movie->xscale = d;
  } else {
    swfdec_movie_begin_update_matrix (movie);
    movie->xscale = 0;
    movie->yscale = 0;
  }
  swfdec_movie_end_update_matrix (movie);
}

static void
mc_height_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double d;

  swfdec_movie_update (movie);
  d = rint (movie->extents.y1 - movie->extents.y0);
  d = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) d);
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, d);
}

static void
mc_height_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d, cur;

  /* property was readonly in Flash 4 and before */
  if (swfdec_gc_object_get_context (movie)->version < 5)
    return;
  d = swfdec_as_value_to_number (swfdec_gc_object_get_context (movie), *val);
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
    swfdec_movie_begin_update_matrix (movie);
    movie->yscale = d;
  } else {
    swfdec_movie_begin_update_matrix (movie);
    movie->xscale = 0;
    movie->yscale = 0;
  }
  swfdec_movie_end_update_matrix (movie);
}

static void
mc_rotation_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, movie->rotation);
}

static void
mc_rotation_set (SwfdecMovie *movie, const SwfdecAsValue *val)
{
  double d;

  /* FIXME: Flash 4 handles this differently */
  d = swfdec_as_value_to_number (swfdec_gc_object_get_context (movie), *val);
  if (isnan (d)) {
    SWFDEC_WARNING ("setting rotation to NaN - not allowed");
    return;
  }
  d = fmod (d, 360.0);
  if (d > 180.0)
    d -= 360.0;
  if (d < -180.0)
    d += 360.0;
  if (swfdec_gc_object_get_context (movie)->version < 5) {
    if (!isfinite (d))
      return;
    SWFDEC_FIXME ("implement correct rounding errors here");
  }
  movie->modified = TRUE;
  if (movie->rotation != d) {
    swfdec_movie_begin_update_matrix (movie);
    movie->rotation = d;
    swfdec_movie_end_update_matrix (movie);
  }
}

static void
mc_xmouse_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double x, y;

  swfdec_movie_get_mouse (movie, &x, &y);
  x = SWFDEC_TWIPS_TO_DOUBLE (rint (x));
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, x);
}

static void
mc_ymouse_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  double x, y;

  swfdec_movie_get_mouse (movie, &x, &y);
  y = SWFDEC_TWIPS_TO_DOUBLE (rint (y));
  swfdec_as_value_set_number (swfdec_gc_object_get_context (movie), rval, y);
}

static void
mc_parent (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  if (movie->parent) {
    SWFDEC_AS_VALUE_SET_MOVIE (rval, movie->parent);
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (rval);
  }
}

static void
mc_root (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  movie = swfdec_movie_get_root (movie);
  SWFDEC_AS_VALUE_SET_MOVIE (rval, movie);
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
	  swfdec_gc_object_get_context (movie), g_string_free (s, FALSE)));
  }
}

static void
mc_url_get (SwfdecMovie *movie, SwfdecAsValue *rval)
{
  SWFDEC_AS_VALUE_SET_STRING (rval, swfdec_as_context_get_string (
	swfdec_gc_object_get_context (movie),
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
  cx = swfdec_gc_object_get_context (actor);

  switch (actor->focusrect) {
    case SWFDEC_FLASH_YES:
      if (cx->version > 5)
	SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
      else
	swfdec_as_value_set_integer (cx, rval, 1);
      break;
    case SWFDEC_FLASH_NO:
      if (cx->version > 5)
	SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
      else
	swfdec_as_value_set_integer (cx, rval, 0);
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
  cx = swfdec_gc_object_get_context (movie);
  actor = SWFDEC_ACTOR (movie);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (*val) ||
      SWFDEC_AS_VALUE_IS_NULL (*val)) {
    if (movie->parent == NULL)
      return;
    b = SWFDEC_FLASH_MAYBE;
  } else {
    if (movie->parent == NULL) {
      double d = swfdec_as_value_to_number (cx, *val);
      if (isnan (d))
	return;
      b = d ? SWFDEC_FLASH_YES : SWFDEC_FLASH_NO;
    } else {
      b = swfdec_as_value_to_boolean (cx, *val) ? SWFDEC_FLASH_YES : SWFDEC_FLASH_NO;
    }
  }

  if (b != actor->focusrect) {
    SwfdecPlayerPrivate *priv = SWFDEC_PLAYER (cx)->priv;
    gboolean had_focusrect = priv->focus ? swfdec_actor_has_focusrect (priv->focus) : FALSE;
    actor->focusrect = b;
    if (priv->focus && had_focusrect != swfdec_actor_has_focusrect (priv->focus))
      swfdec_player_invalidate_focusrect (SWFDEC_PLAYER (cx));
  }
}

struct {
  const char *name;
  void (* get) (SwfdecMovie *movie, SwfdecAsValue *ret);
  void (* set) (SwfdecMovie *movie, const SwfdecAsValue *val);
} swfdec_movieclip_props[] = {
  { SWFDEC_AS_STR__x,		mc_x_get,	    mc_x_set },
  { SWFDEC_AS_STR__y,		mc_y_get,	    mc_y_set },
  { SWFDEC_AS_STR__xscale,	mc_xscale_get,	    mc_xscale_set },
  { SWFDEC_AS_STR__yscale,	mc_yscale_get,	    mc_yscale_set },
  { SWFDEC_AS_STR__currentframe,NULL,		    NULL },
  { SWFDEC_AS_STR__totalframes,	NULL,	  	    NULL },
  { SWFDEC_AS_STR__alpha,	mc_alpha_get,	    mc_alpha_set },
  { SWFDEC_AS_STR__visible,	mc_visible_get,	    mc_visible_set },
  { SWFDEC_AS_STR__width,	mc_width_get,	    mc_width_set },
  { SWFDEC_AS_STR__height,	mc_height_get,	    mc_height_set },
  { SWFDEC_AS_STR__rotation,	mc_rotation_get,    mc_rotation_set },
  { SWFDEC_AS_STR__target,	mc_target_get,	    NULL },
  { SWFDEC_AS_STR__framesloaded,NULL,		    NULL},
  { SWFDEC_AS_STR__name,	mc_name_get,	    mc_name_set },
  { SWFDEC_AS_STR__droptarget,	NULL,		    NULL }, //"_droptarget"
  { SWFDEC_AS_STR__url,		mc_url_get,	    NULL },
  { SWFDEC_AS_STR__highquality,	NULL,		    NULL }, //"_highquality"
  { SWFDEC_AS_STR__focusrect,	mc_focusrect_get,   mc_focusrect_set }, //"_focusrect"
  { SWFDEC_AS_STR__soundbuftime,NULL,		    NULL }, //"_soundbuftime"
  { SWFDEC_AS_STR__quality,	NULL,		    NULL }, //"_quality"
  { SWFDEC_AS_STR__xmouse,	mc_xmouse_get,	    NULL },
  { SWFDEC_AS_STR__ymouse,	mc_ymouse_get,	    NULL },
  { SWFDEC_AS_STR__parent,	mc_parent,	    NULL },
  { SWFDEC_AS_STR__root,	mc_root,	    NULL },
};

guint
swfdec_movie_property_lookup (const char *name)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (swfdec_movieclip_props); i++) {
    if (swfdec_movieclip_props[i].name == name)
      return i;
  }
  return G_MAXUINT;
}

void
swfdec_movie_property_do_get (SwfdecMovie *movie, guint id, 
    SwfdecAsValue *val)
{
  if (id >= G_N_ELEMENTS (swfdec_movieclip_props) ||
      swfdec_movieclip_props[id].get == NULL) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (val);
  } else {
    swfdec_movieclip_props[id].get (movie, val);
  }
}

void
swfdec_movie_property_do_set (SwfdecMovie *movie, guint id, 
    const SwfdecAsValue *val)
{
  if (id < G_N_ELEMENTS (swfdec_movieclip_props) &&
      swfdec_movieclip_props[id].set != NULL) {
    swfdec_movieclip_props[id].set (movie, val);
  }
}

