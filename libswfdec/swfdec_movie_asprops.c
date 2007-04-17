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
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_player_internal.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_swf_decoder.h"

static void
mc_x_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  swfdec_movie_update (movie);
  d = SWFDEC_TWIPS_TO_DOUBLE (movie->matrix.x0);
  SWFDEC_AS_VALUE_SET_NUMBER (rval, d);
}

static void
mc_x_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  d = swfdec_as_value_to_number (obj->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to move %s._x to a non-finite value, ignoring", movie->name);
    return;
  }
  movie->modified = TRUE;
  d = SWFDEC_DOUBLE_TO_TWIPS (d);
  if (d != movie->matrix.x0) {
    movie->matrix.x0 = d;
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
  }
}

static void
mc_y_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  swfdec_movie_update (movie);
  d = SWFDEC_TWIPS_TO_DOUBLE (movie->matrix.y0);
  SWFDEC_AS_VALUE_SET_NUMBER (rval, d);
}

static void
mc_y_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  d = swfdec_as_value_to_number (obj->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to move %s._y to a non-finite value, ignoring", movie->name);
    return;
  }
  movie->modified = TRUE;
  d = SWFDEC_DOUBLE_TO_TWIPS (d);
  if (d != movie->matrix.y0) {
    movie->matrix.y0 = d;
    swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
  }
}

static void
mc_xscale_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_NUMBER (rval, movie->xscale);
}

static void
mc_xscale_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  d = swfdec_as_value_to_number (obj->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set xscale to a non-finite value, ignoring");
    return;
  }
  movie->modified = TRUE;
  movie->xscale = d;
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
}

static void
mc_yscale_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_NUMBER (rval, movie->yscale);
}

static void
mc_yscale_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  d = swfdec_as_value_to_number (obj->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set yscale to a non-finite value, ignoring");
    return;
  }
  movie->modified = TRUE;
  movie->yscale = d;
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
}

static void
mc_currentframe (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_NUMBER (rval, movie->frame + 1);
}

static void
mc_framesloaded (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  guint loaded;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  /* only root movies can be partially loaded */
  if (SWFDEC_IS_ROOT_MOVIE (movie)) {
    SwfdecDecoder *dec = SWFDEC_ROOT_MOVIE (movie->root)->decoder;
    loaded = dec->frames_loaded;
    g_assert (loaded <= movie->n_frames);
  } else {
    loaded = movie->n_frames;
  }
  SWFDEC_AS_VALUE_SET_NUMBER (rval, loaded);
}

static void
mc_name_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  const char *string;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  if (movie->has_name) {
    /* FIXME: make names GC'd */
    string = swfdec_as_context_get_string (obj->context, movie->name);
  } else {
    string = SWFDEC_AS_STR_EMPTY;
  }
  SWFDEC_AS_VALUE_SET_STRING (rval, string);
}

static void
mc_name_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  const char *str;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  str = swfdec_as_value_to_string (obj->context, val);
#if 0
  if (!SWFDEC_IS_ROOT_MOVIE (movie))
    swfdec_js_movie_remove_property (movie);
#endif
  g_free (movie->name);
  movie->name = g_strdup (str);
  movie->has_name = TRUE;
#if 0
  if (!SWFDEC_IS_ROOT_MOVIE (movie))
    swfdec_js_movie_add_property (movie);
#endif
}

static void
mc_totalframes (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_NUMBER (rval, movie->n_frames);
}

static void
mc_alpha_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_NUMBER (rval,
      movie->color_transform.aa * 100.0 / 256.0);
}

static void
mc_alpha_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  double d;
  int alpha;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  d = swfdec_as_value_to_number (obj->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set alpha to a non-finite value, ignoring");
    return;
  }
  alpha = d * 256.0 / 100.0;
  if (alpha != movie->color_transform.aa) {
    movie->color_transform.aa = alpha;
    swfdec_movie_invalidate (movie);
  }
}

static void
mc_visible_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, movie->visible);
}

static void
mc_visible_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  gboolean b;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  b = swfdec_as_value_to_boolean (obj->context, val);
  if (b != movie->visible) {
    movie->visible = b;
    swfdec_movie_invalidate (movie);
  }
}

static void
mc_width_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  swfdec_movie_update (movie);
  d = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) (rint (movie->extents.x1 - movie->extents.x0)));
  SWFDEC_AS_VALUE_SET_NUMBER (rval, d);
}

static void
mc_width_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  double d, cur;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  /* property was readonly in Flash 4 and before */
  if (obj->context->version < 5)
    return;
  d = swfdec_as_value_to_number (obj->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set height to a non-finite value, ignoring");
    return;
  }
  swfdec_movie_update (movie);
  movie->modified = TRUE;
  cur = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) (rint (movie->original_extents.x1 - movie->original_extents.x0)));
  if (cur != 0) {
    movie->xscale = 100 * d / cur;
  } else {
    movie->xscale = 0;
    movie->yscale = 0;
  }
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
}

static void
mc_height_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  swfdec_movie_update (movie);
  d = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) (rint (movie->extents.y1 - movie->extents.y0)));
  SWFDEC_AS_VALUE_SET_NUMBER (rval, d);
}

static void
mc_height_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  double d, cur;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  /* property was readonly in Flash 4 and before */
  if (obj->context->version < 5)
    return;
  d = swfdec_as_value_to_number (obj->context, val);
  if (!isfinite (d)) {
    SWFDEC_WARNING ("trying to set height to a non-finite value, ignoring");
    return;
  }
  swfdec_movie_update (movie);
  movie->modified = TRUE;
  cur = SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) (rint (movie->original_extents.y1 - movie->original_extents.y0)));
  if (cur != 0) {
    movie->yscale = 100 * d / cur;
  } else {
    movie->xscale = 0;
    movie->yscale = 0;
  }
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
}

static void
mc_rotation_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_NUMBER (rval, movie->rotation);
}

static void
mc_rotation_set (SwfdecAsObject *obj, const SwfdecAsValue *val)
{
  SwfdecMovie *movie;
  double d;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  /* FIXME: Flash 4 handles this differently */
  d = swfdec_as_value_to_number (obj->context, val);
  d = fmod (d, 360.0);
  if (d > 180.0)
    d -= 360.0;
  if (d < -180.0)
    d += 360.0;
  if (obj->context->version < 5) {
    if (!isfinite (d))
      return;
    SWFDEC_ERROR ("FIXME: implement correct rounding errors here");
  }
  movie->modified = TRUE;
  movie->rotation = d;
  swfdec_movie_queue_update (movie, SWFDEC_MOVIE_INVALID_MATRIX);
}

static void
mc_xmouse_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  double x, y;
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  swfdec_movie_get_mouse (movie, &x, &y);
  x = rint (x * SWFDEC_TWIPS_SCALE_FACTOR) / SWFDEC_TWIPS_SCALE_FACTOR;
  SWFDEC_AS_VALUE_SET_NUMBER (rval, x);
}

static void
mc_ymouse_get (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  double x, y;
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  swfdec_movie_get_mouse (movie, &x, &y);
  y = rint (y * SWFDEC_TWIPS_SCALE_FACTOR) / SWFDEC_TWIPS_SCALE_FACTOR;
  SWFDEC_AS_VALUE_SET_NUMBER (rval, y);
}

static void
mc_parent (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  if (movie->parent) {
    SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (movie->parent));
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (rval);
  }
}

static void
mc_root (SwfdecAsObject *obj, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  if (!SWFDEC_IS_MOVIE (obj)) {
    SWFDEC_WARNING ("not a movie");
    return;
  }
  movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (movie->root));
}

static struct {
  guint				id;
  SwfdecAsVariableGetter	get;
  SwfdecAsVariableSetter	set;
} movieclip_props[] = {
  { 39,		mc_x_get,	    mc_x_set },
  { 40,			mc_y_get,	    mc_y_set },
  { 41,		mc_xscale_get,	    mc_xscale_set },
  { 42,		mc_yscale_get,	    mc_yscale_set },
  { 43,	mc_currentframe,    NULL },
  { 44,		mc_totalframes,	    NULL },
  { 45,		mc_alpha_get,	    mc_alpha_set },
  { 46,		mc_visible_get,	    mc_visible_set },
  { 47,		mc_width_get,	    mc_width_set },
  { 48,		mc_height_get,	    mc_height_set },
  { 49,		mc_rotation_get,    mc_rotation_set },
  //{"_target",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  { 51,      	mc_framesloaded,    NULL},
  { 52,		mc_name_get,	    mc_name_set },
  //{"_droptarget",   -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  //{"_url",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  //{"_highquality",  -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  //{"_focusrect",    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  //{"_soundbuftime", -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  //{"_quality",	    -1,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  { 59,		mc_xmouse_get,	    NULL },
  { 60,		mc_ymouse_get,	    NULL },
  { 73,		mc_parent,	    NULL },
  { 74,		mc_root,	    NULL },
};

void
swfdec_movie_add_asprops (SwfdecMovie *movie)
{
  SwfdecAsObject *object = SWFDEC_AS_OBJECT (movie);
  guint i;
  
  /* FIXME: figure out a way to not add all of these to buttons etc */
  for (i = 0; i < G_N_ELEMENTS (movieclip_props); i++) {
    swfdec_as_object_add_variable (object, SWFDEC_AS_STR_CONSTANT (movieclip_props[i].id),
	movieclip_props[i].set, movieclip_props[i].get);
  }
}
