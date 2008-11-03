/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_movie.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_bitmap_data.h"
#include "swfdec_bitmap_movie.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_filter.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_sandbox.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_swf_decoder.h"
#include "swfdec_transform_as.h"
#include "swfdec_resource.h"
#include "swfdec_utils.h"
#include "swfdec_as_internal.h"

SWFDEC_AS_NATIVE (900, 200, swfdec_sprite_movie_get_tabIndex)
void
swfdec_sprite_movie_get_tabIndex (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.tabIndex (get)");
}

SWFDEC_AS_NATIVE (900, 201, swfdec_sprite_movie_set_tabIndex)
void
swfdec_sprite_movie_set_tabIndex (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.tabIndex (set)");
}

SWFDEC_AS_NATIVE (900, 300, swfdec_sprite_movie_get__lockroot)
void
swfdec_sprite_movie_get__lockroot (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "");

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, movie->lockroot);
}

SWFDEC_AS_NATIVE (900, 301, swfdec_sprite_movie_set__lockroot)
void
swfdec_sprite_movie_set__lockroot (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  gboolean lockroot;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "b", &lockroot);

  movie->lockroot = lockroot;
}

SWFDEC_AS_NATIVE (900, 401, swfdec_sprite_movie_get_cacheAsBitmap)
void
swfdec_sprite_movie_get_cacheAsBitmap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.cacheAsBitmap (get)");
}

SWFDEC_AS_NATIVE (900, 402, swfdec_sprite_movie_set_cacheAsBitmap)
void
swfdec_sprite_movie_set_cacheAsBitmap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.cacheAsBitmap (set)");
}

SWFDEC_AS_NATIVE (900, 403, swfdec_sprite_movie_get_opaqueBackground)
void
swfdec_sprite_movie_get_opaqueBackground (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.opaqueBackground (get)");
}

SWFDEC_AS_NATIVE (900, 404, swfdec_sprite_movie_set_opaqueBackground)
void
swfdec_sprite_movie_set_opaqueBackground (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.opaqueBackground (set)");
}

SWFDEC_AS_NATIVE (900, 405, swfdec_sprite_movie_get_scrollRect)
void
swfdec_sprite_movie_get_scrollRect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.scrollRect (get)");
}

SWFDEC_AS_NATIVE (900, 406, swfdec_sprite_movie_set_scrollRect)
void
swfdec_sprite_movie_set_scrollRect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.scrollRect (set)");
}

SWFDEC_AS_NATIVE (900, 417, swfdec_sprite_movie_get_filters)
void
swfdec_sprite_movie_get_filters (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.filters (get)");
}

SWFDEC_AS_NATIVE (900, 418, swfdec_sprite_movie_set_filters)
void
swfdec_sprite_movie_set_filters (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsObject *array;
  SwfdecAsValue val;
  SwfdecFilter *filter;
  SwfdecMovie *movie;
  int i, length;
  GSList *list;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "o", &array);

  swfdec_movie_invalidate_next (movie);

  swfdec_as_object_get_variable (array, SWFDEC_AS_STR_length, &val);
  length = swfdec_as_value_to_integer (cx, &val);

  list = NULL;
  for (i = 0; i < length; i++) {
    if (!swfdec_as_object_get_variable (array, 
	  swfdec_as_integer_to_string (cx, i), &val) ||
	!SWFDEC_AS_VALUE_IS_OBJECT (&val) ||
	!SWFDEC_IS_FILTER (SWFDEC_AS_VALUE_GET_OBJECT (&val)->relay))
      continue;
    filter = SWFDEC_FILTER (SWFDEC_AS_VALUE_GET_OBJECT (&val)->relay);
    filter = swfdec_filter_clone (filter);
    list = g_slist_prepend (list, filter);
  }
  g_slist_free (movie->filters);
  movie->filters = list;
}

SWFDEC_AS_NATIVE (900, 419, swfdec_sprite_movie_get_transform)
void
swfdec_sprite_movie_get_transform (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecTransformAs *trans;
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "");

  trans = swfdec_transform_as_new (cx, movie);
  SWFDEC_AS_VALUE_SET_OBJECT (rval, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (trans)));
}

SWFDEC_AS_NATIVE (900, 420, swfdec_sprite_movie_set_transform)
void
swfdec_sprite_movie_set_transform (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.transform (set)");
}

static const char *blend_mode_names[] = {
  SWFDEC_AS_STR_normal,
  SWFDEC_AS_STR_layer,
  SWFDEC_AS_STR_multiply,
  SWFDEC_AS_STR_screen,
  SWFDEC_AS_STR_lighten,
  SWFDEC_AS_STR_darken,
  SWFDEC_AS_STR_difference,
  SWFDEC_AS_STR_add,
  SWFDEC_AS_STR_subtract,
  SWFDEC_AS_STR_invert,
  SWFDEC_AS_STR_alpha,
  SWFDEC_AS_STR_erase,
  SWFDEC_AS_STR_overlay,
  SWFDEC_AS_STR_hardlight
};
static const gsize num_blend_mode_names =
  sizeof (blend_mode_names) / sizeof (blend_mode_names[0]);

SWFDEC_AS_NATIVE (900, 500, swfdec_sprite_movie_get_blendMode)
void
swfdec_sprite_movie_get_blendMode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "");

  if (movie->blend_mode > 0 && movie->blend_mode <= num_blend_mode_names)
    SWFDEC_AS_VALUE_SET_STRING (rval, blend_mode_names[movie->blend_mode - 1]);
}

SWFDEC_AS_NATIVE (900, 501, swfdec_sprite_movie_set_blendMode)
void
swfdec_sprite_movie_set_blendMode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecAsValue val;
  const char *str;
  int blend_mode;
  gsize i;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "v", &val);

  if (SWFDEC_AS_VALUE_IS_NUMBER (&val)) {
    blend_mode = SWFDEC_AS_VALUE_GET_NUMBER (&val);
  } else if (SWFDEC_AS_VALUE_IS_STRING (&val)) {
    blend_mode = 0;
    str = SWFDEC_AS_VALUE_GET_STRING (&val);
    for (i = 0; i < num_blend_mode_names; i++) {
      if (str == blend_mode_names[i]) { // case-sensitive
	blend_mode = i + 1;
	break;
      }
    }
  } else if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    blend_mode = 0;
  } else {
    blend_mode = 1;
  }

  if ((guint)blend_mode != movie->blend_mode) {
    movie->blend_mode = blend_mode;
    swfdec_movie_invalidate_last (movie);
  }
}

SWFDEC_AS_NATIVE (900, 2, swfdec_sprite_movie_localToGlobal)
void
swfdec_sprite_movie_localToGlobal (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecAsObject *o;
  SwfdecAsValue *xv, *yv;
  double x, y;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "O", &o);

  xv = swfdec_as_object_peek_variable (o, SWFDEC_AS_STR_x);
  if (xv == NULL || !SWFDEC_AS_VALUE_IS_NUMBER (xv))
    return;
  yv = swfdec_as_object_peek_variable (o, SWFDEC_AS_STR_y);
  if (yv == NULL || !SWFDEC_AS_VALUE_IS_NUMBER (yv))
    return;

  x = SWFDEC_AS_VALUE_GET_NUMBER (xv);
  y = SWFDEC_AS_VALUE_GET_NUMBER (yv);
  x = swfdec_as_double_to_integer (x * SWFDEC_TWIPS_SCALE_FACTOR);
  y = swfdec_as_double_to_integer (y * SWFDEC_TWIPS_SCALE_FACTOR);
  swfdec_movie_local_to_global (movie, &x, &y);
  swfdec_as_value_set_number (cx, xv, SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) x));
  swfdec_as_value_set_number (cx, yv, SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) y));
}

SWFDEC_AS_NATIVE (900, 3, swfdec_sprite_movie_globalToLocal)
void
swfdec_sprite_movie_globalToLocal (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecAsObject *o;
  SwfdecAsValue *xv, *yv;
  double x, y;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "O", &o);

  xv = swfdec_as_object_peek_variable (o, SWFDEC_AS_STR_x);
  if (xv == NULL || !SWFDEC_AS_VALUE_IS_NUMBER (xv))
    return;
  yv = swfdec_as_object_peek_variable (o, SWFDEC_AS_STR_y);
  if (yv == NULL || !SWFDEC_AS_VALUE_IS_NUMBER (yv))
    return;

  x = SWFDEC_AS_VALUE_GET_NUMBER (xv);
  y = SWFDEC_AS_VALUE_GET_NUMBER (yv);
  x = swfdec_as_double_to_integer (x * SWFDEC_TWIPS_SCALE_FACTOR);
  y = swfdec_as_double_to_integer (y * SWFDEC_TWIPS_SCALE_FACTOR);
  swfdec_movie_global_to_local (movie, &x, &y);
  swfdec_as_value_set_number (cx, xv, SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) x));
  swfdec_as_value_set_number (cx, yv, SWFDEC_TWIPS_TO_DOUBLE ((SwfdecTwips) y));
}

SWFDEC_AS_NATIVE (900, 8, swfdec_sprite_movie_attachAudio)
void
swfdec_sprite_movie_attachAudio (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.attachAudio");
}

SWFDEC_AS_NATIVE (900, 9, swfdec_sprite_movie_attachVideo)
void
swfdec_sprite_movie_attachVideo (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.attachVideo");
}

SWFDEC_AS_NATIVE (900, 23, swfdec_sprite_movie_getInstanceAtDepth)
void
swfdec_sprite_movie_getInstanceAtDepth (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  int depth;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "i", &depth);

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  movie = swfdec_movie_find (movie, depth);
  if (movie != NULL) {
    if (!swfdec_movie_is_scriptable (movie))
      movie = movie->parent;
    SWFDEC_AS_VALUE_SET_MOVIE (rval, movie);
  }
}

SWFDEC_AS_NATIVE (900, 24, swfdec_sprite_movie_getSWFVersion)
void
swfdec_sprite_movie_getSWFVersion (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  if (object != NULL && SWFDEC_IS_MOVIE (object)) {
    swfdec_as_value_set_integer (cx, rval,
	swfdec_movie_get_version (SWFDEC_MOVIE (object)));
  } else {
    swfdec_as_value_set_integer (cx, rval, -1);
  }
}

SWFDEC_AS_NATIVE (900, 25, swfdec_sprite_movie_attachBitmap)
void
swfdec_sprite_movie_attachBitmap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *parent;
  SwfdecAsObject *bitmap;
  const char *snapping = SWFDEC_AS_STR_auto;
  gboolean smoothing = FALSE;
  int depth;
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SPRITE_MOVIE, &parent, "oi|sb", 
      &bitmap, &depth, &snapping, &smoothing);

  if (!SWFDEC_IS_BITMAP_DATA (bitmap->relay))
    return;
  if (swfdec_depth_classify (depth) == SWFDEC_DEPTH_CLASS_EMPTY)
    return;

  movie = swfdec_movie_find (parent, depth);
  if (movie)
    swfdec_movie_remove (movie);

  swfdec_bitmap_movie_new (parent, SWFDEC_BITMAP_DATA (bitmap->relay), depth);
  SWFDEC_LOG ("created new BitmapMovie to parent %s at depth %d", 
      parent->nameasdf, depth);
}

SWFDEC_AS_NATIVE (900, 26, swfdec_sprite_movie_getRect)
void
swfdec_sprite_movie_getRect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("MovieClip.getRect");
}

SWFDEC_AS_NATIVE (900, 12, swfdec_sprite_movie_play)
void
swfdec_sprite_movie_play (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecSpriteMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SPRITE_MOVIE, (gpointer)&movie, "");

  movie->playing = TRUE;
}

SWFDEC_AS_NATIVE (900, 13, swfdec_sprite_movie_stop)
void
swfdec_sprite_movie_stop (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecSpriteMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SPRITE_MOVIE, (gpointer)&movie, "");

  movie->playing = FALSE;
}

SWFDEC_AS_NATIVE (900, 7, swfdec_sprite_movie_getBytesLoaded)
void
swfdec_sprite_movie_getBytesLoaded (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecResource *resource;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, (gpointer)&movie, "");

  resource = swfdec_movie_get_own_resource (movie);
  if (resource && resource->decoder) {
    swfdec_as_value_set_integer (cx, rval, resource->decoder->bytes_loaded);
  } else {
    swfdec_as_value_set_integer (cx, rval, 0);
  }
}

SWFDEC_AS_NATIVE (900, 6, swfdec_sprite_movie_getBytesTotal)
void
swfdec_sprite_movie_getBytesTotal (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecResource *resource;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, (gpointer)&movie, "");

  resource = swfdec_movie_get_own_resource (movie);
  if (resource) {
    if (resource->decoder) {
      swfdec_as_value_set_integer (cx, rval, resource->decoder->bytes_total);
    } else {
      swfdec_as_value_set_integer (cx, rval, -1);
    }
  } else {
    swfdec_as_value_set_integer (cx, rval, 0);
  }
}

SWFDEC_AS_NATIVE (900, 22, swfdec_sprite_movie_getNextHighestDepth)
void
swfdec_sprite_movie_getNextHighestDepth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  int depth;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, (gpointer)&movie, "");

  if (movie->list) {
    depth = SWFDEC_MOVIE (g_list_last (movie->list)->data)->depth + 1;
    if (depth < 0)
      depth = 0;
  } else {
    depth = 0;
  }
  swfdec_as_value_set_integer (cx, rval, depth);
}

static void
swfdec_sprite_movie_do_goto (SwfdecSpriteMovie *movie, SwfdecAsValue *target)
{
  int frame;

  g_return_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie));
  g_return_if_fail (SWFDEC_IS_AS_VALUE (target));

  if (SWFDEC_AS_VALUE_IS_STRING (target)) {
    const char *label = SWFDEC_AS_VALUE_GET_STRING (target);
    frame = swfdec_sprite_get_frame (movie->sprite, label);
    /* FIXME: nonexisting frames? */
    if (frame == -1)
      return;
    frame++;
  } else {
    frame = swfdec_as_value_to_integer (swfdec_gc_object_get_context (movie), target);
  }
  /* FIXME: how to handle overflow? */
  frame = CLAMP (frame, 1, (int) movie->n_frames);

  swfdec_sprite_movie_goto (movie, frame);
}

SWFDEC_AS_NATIVE (900, 16, swfdec_sprite_movie_gotoAndPlay)
void
swfdec_sprite_movie_gotoAndPlay (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecSpriteMovie *movie;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SPRITE_MOVIE, (gpointer)&movie, "v", &val);
  
  swfdec_sprite_movie_do_goto (movie, &val);
  movie->playing = TRUE;
}

SWFDEC_AS_NATIVE (900, 17, swfdec_sprite_movie_gotoAndStop)
void
swfdec_sprite_movie_gotoAndStop (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecSpriteMovie *movie;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SPRITE_MOVIE, (gpointer)&movie, "v", &val);
  
  swfdec_sprite_movie_do_goto (movie, &val);
  movie->playing = FALSE;
}

SWFDEC_AS_NATIVE (900, 14, swfdec_sprite_movie_nextFrame)
void
swfdec_sprite_movie_nextFrame (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecSpriteMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SPRITE_MOVIE, (gpointer)&movie, "");
  
  swfdec_sprite_movie_goto (movie, movie->frame + 1);
  movie->playing = FALSE;
}

SWFDEC_AS_NATIVE (900, 15, swfdec_sprite_movie_prevFrame)
void
swfdec_sprite_movie_prevFrame (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecSpriteMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SPRITE_MOVIE, (gpointer)&movie, "");
  
  swfdec_sprite_movie_goto (movie, movie->frame - 1);
  movie->playing = FALSE;
}

SWFDEC_AS_NATIVE (900, 4, swfdec_sprite_movie_hitTest)
void
swfdec_sprite_movie_hitTest (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "");
  
  if (argc == 1) {
    SwfdecMovie *other;
    SwfdecRect movie_rect, other_rect;

    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
    SWFDEC_AS_CHECK (0, NULL, "m", &other);

    swfdec_movie_update (movie);
    swfdec_movie_update (other);
    movie_rect = movie->extents;
    if (movie->parent)
      swfdec_movie_rect_local_to_global (movie->parent, &movie_rect);
    other_rect = other->extents;
    if (other->parent)
      swfdec_movie_rect_local_to_global (other->parent, &other_rect);
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, swfdec_rect_intersect (NULL, &movie_rect, &other_rect));
  } else if (argc >= 2) {
    double x, y;
    gboolean shape = FALSE;
    gboolean ret;

    SWFDEC_AS_CHECK (0, NULL, "nn|b", &x, &y, &shape);
    x *= SWFDEC_TWIPS_SCALE_FACTOR;
    y *= SWFDEC_TWIPS_SCALE_FACTOR;

    if (shape) {
      if (movie->parent)
	swfdec_movie_global_to_local (movie->parent, &x, &y);
      ret = swfdec_movie_contains (movie, x, y);
    } else {
      if (movie->cache_state >= SWFDEC_MOVIE_INVALID_EXTENTS)
	  swfdec_movie_update (movie);
      swfdec_movie_global_to_local (movie, &x, &y);
      ret = swfdec_rect_contains (&movie->original_extents, x, y);
    }
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, ret);
  }
}

SWFDEC_AS_NATIVE (900, 20, swfdec_sprite_movie_startDrag)
void
swfdec_sprite_movie_startDrag (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecActor *actor;
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  gboolean center = FALSE;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_ACTOR, &actor, "");

  if (argc > 0) {
    center = swfdec_as_value_to_boolean (cx, &argv[0]);
  }
  if (argc >= 5) {
    SwfdecRect rect;
    rect.x0 = swfdec_as_value_to_number (cx, &argv[1]);
    rect.y0 = swfdec_as_value_to_number (cx, &argv[2]);
    rect.x1 = swfdec_as_value_to_number (cx, &argv[3]);
    rect.y1 = swfdec_as_value_to_number (cx, &argv[4]);
    swfdec_rect_scale (&rect, &rect, SWFDEC_TWIPS_SCALE_FACTOR);
    swfdec_player_set_drag_movie (player, actor, center, &rect);
  } else {
    swfdec_player_set_drag_movie (player, actor, center, NULL);
  }
}

SWFDEC_AS_NATIVE (900, 21, swfdec_sprite_movie_stopDrag)
void
swfdec_sprite_movie_stopDrag (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  // FIXME: Should this work when called on non-movie objects or not?

  swfdec_player_set_drag_movie (SWFDEC_PLAYER (cx), NULL, FALSE, NULL);
}

SWFDEC_AS_NATIVE (900, 1, swfdec_sprite_movie_swapDepths)
void
swfdec_sprite_movie_swapDepths (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie, *other;
  SwfdecAsValue value;
  int depth;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "v", &value);

  if (movie->parent == NULL)
    SWFDEC_FIXME ("swapDepths on root movie, should do something weird");

  if (SWFDEC_AS_VALUE_IS_MOVIE (&value)) {
    other = SWFDEC_AS_VALUE_GET_MOVIE (&value);
    if (other == NULL || other->parent != movie->parent)
      return;
    depth = other->depth;
  } else {
    depth = swfdec_as_value_to_integer (cx, &value);
    if (movie->parent) {
      other = swfdec_movie_find (movie->parent, depth);
    } else {
      // special case: if root movie: we won't swap, but just set depth
      other = NULL;
    }
  }

  // FIXME: one different than the reserved range, should the classify function
  // be changed instead?
  if (swfdec_depth_classify (depth) == SWFDEC_DEPTH_CLASS_EMPTY ||
      depth >= 2130690045)
    return;

  if (other)
    swfdec_movie_set_depth (other, movie->depth);
  swfdec_movie_set_depth (movie, depth);
}

SWFDEC_AS_NATIVE (901, 0, swfdec_sprite_movie_createEmptyMovieClip)
void
swfdec_sprite_movie_createEmptyMovieClip (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie, *parent;
  int depth;
  const char *name;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &parent, "si", &name, &depth);

  movie = swfdec_movie_find (parent, depth);
  if (movie)
    swfdec_movie_remove (movie);
  movie = swfdec_movie_new (SWFDEC_PLAYER (cx), depth, parent, parent->resource, NULL, name);

  if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
    SwfdecSandbox *sandbox = swfdec_sandbox_get (SWFDEC_PLAYER (cx));
    SwfdecActor *actor = SWFDEC_ACTOR (movie);
    swfdec_sandbox_unuse (sandbox);
    swfdec_movie_initialize (movie);
    swfdec_actor_execute (actor, SWFDEC_EVENT_CONSTRUCT, 0);
    swfdec_sandbox_use (sandbox);
  } else {
    swfdec_movie_initialize (movie);
  }

  SWFDEC_AS_VALUE_SET_MOVIE (rval, movie);
}

static void
swfdec_sprite_movie_copy_props (SwfdecMovie *target, SwfdecMovie *src)
{
  swfdec_movie_begin_update_matrix (target);
  target->matrix = src->matrix;
  target->modified = src->modified;
  target->xscale = src->xscale;
  target->yscale = src->yscale;
  target->rotation = src->rotation;
  target->lockroot = src->lockroot;
  target->color_transform = src->color_transform;
  swfdec_movie_end_update_matrix (target);
}

static gboolean
swfdec_sprite_movie_foreach_copy_properties (SwfdecAsObject *object,
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data)
{
  SwfdecAsObject *target = data;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (target), FALSE);

  /* FIXME: We likely need better flag handling here.
   * We might even want to fix swfdec_as_object_foreach() */
  if (flags & SWFDEC_AS_VARIABLE_HIDDEN)
    return TRUE;

  swfdec_as_object_set_variable (target, variable, value);

  return TRUE;
}

static void
swfdec_sprite_movie_init_from_object (SwfdecMovie *movie,
    SwfdecAsObject *initObject)
{
  g_return_if_fail (SWFDEC_IS_MOVIE (movie));
  g_return_if_fail (initObject == NULL || SWFDEC_IS_AS_OBJECT (initObject));

  if (initObject != NULL) {
    SwfdecAsContext *cx = swfdec_gc_object_get_context (movie);
    if (cx->version <= 6) {
      swfdec_as_object_foreach (initObject,
	  swfdec_sprite_movie_foreach_copy_properties, SWFDEC_AS_OBJECT (movie));
      swfdec_movie_initialize (movie);
    } else {
      swfdec_movie_initialize (movie);
      swfdec_as_object_foreach (initObject,
	  swfdec_sprite_movie_foreach_copy_properties, SWFDEC_AS_OBJECT (movie));
    }
  } else {
    swfdec_movie_initialize (movie);
  }

  if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
    SwfdecSandbox *sandbox = movie->resource->sandbox;
    SwfdecActor *actor = SWFDEC_ACTOR (movie);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_INITIALIZE);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_LOAD);
    swfdec_sandbox_unuse (sandbox);
    swfdec_actor_execute (actor, SWFDEC_EVENT_CONSTRUCT, 0);
    swfdec_sandbox_use (sandbox);
  }
}

SWFDEC_AS_NATIVE (900, 0, swfdec_sprite_movie_attachMovie)
void
swfdec_sprite_movie_attachMovie (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecMovie *ret;
  SwfdecAsObject *initObject = NULL, *constructor;
  const char *name, *export;
  int depth;
  SwfdecGraphic *sprite;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "ssi|O", &export, &name, &depth, &initObject);

  sprite = swfdec_resource_get_export (movie->resource, export);
  if (!SWFDEC_IS_SPRITE (sprite)) {
    if (sprite == NULL) {
      SWFDEC_WARNING ("no symbol with name %s exported", export);
    } else {
      SWFDEC_WARNING ("can only use attachMovie with sprites");
    }
    return;
  }
  if (swfdec_depth_classify (depth) == SWFDEC_DEPTH_CLASS_EMPTY)
    return;
  ret = swfdec_movie_find (movie, depth);
  if (ret)
    swfdec_movie_remove (ret);
  ret = swfdec_movie_new (SWFDEC_PLAYER (swfdec_gc_object_get_context (object)),
      depth, movie, movie->resource, sprite, name);
  SWFDEC_LOG ("attached %s (%u) as %s to depth %u", export, SWFDEC_CHARACTER (sprite)->id,
      ret->nameasdf, ret->depth);
  /* run init and construct */
  constructor = swfdec_player_get_export_class (SWFDEC_PLAYER (cx), export);
  if (constructor == NULL) {
    swfdec_as_object_set_constructor_by_name (SWFDEC_AS_OBJECT (ret),
	SWFDEC_AS_STR_MovieClip, NULL);
  } else {
    swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (ret), constructor);
  }

  swfdec_sprite_movie_init_from_object (ret, initObject);
  SWFDEC_AS_VALUE_SET_MOVIE (rval, ret);
}

SWFDEC_AS_NATIVE (900, 18, swfdec_sprite_movie_duplicateMovieClip)
void
swfdec_sprite_movie_duplicateMovieClip (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;
  SwfdecMovie *new;
  const char *name;
  int depth;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, (gpointer)&movie, "si", &name, &depth);

  if (swfdec_depth_classify (depth) == SWFDEC_DEPTH_CLASS_EMPTY)
    return;
  new = swfdec_movie_duplicate (movie, name, depth);
  if (new == NULL)
    return;
  swfdec_sprite_movie_copy_props (new, movie);
  SWFDEC_LOG ("duplicated %s as %s to depth %u", movie->nameasdf, new->nameasdf, new->depth);
  SWFDEC_AS_VALUE_SET_MOVIE (rval, new);
}

SWFDEC_AS_NATIVE (900, 19, swfdec_sprite_movie_removeMovieClip)
void
swfdec_sprite_movie_removeMovieClip (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, (gpointer)&movie, "");

  if (swfdec_depth_classify (movie->depth) == SWFDEC_DEPTH_CLASS_DYNAMIC)
    swfdec_movie_remove (movie);
}

SWFDEC_AS_NATIVE (900, 10, swfdec_sprite_movie_getDepth)
void
swfdec_sprite_movie_getDepth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, (gpointer)&movie, "");

  swfdec_as_value_set_integer (cx, rval, movie->depth);
}

SWFDEC_AS_NATIVE (900, 5, swfdec_sprite_movie_getBounds)
void
swfdec_sprite_movie_getBounds (SwfdecAsContext *cx, SwfdecAsObject *object,
        guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  int x0, x1, y0, y1;
  SwfdecAsValue val;
  SwfdecAsObject *obj;
  SwfdecMovie *movie;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "");

  obj= swfdec_as_object_new_empty (cx);

  swfdec_movie_update (movie);
  if (swfdec_rect_is_empty (&movie->extents)) {
    x0 = x1 = y0 = y1 = 0x7FFFFFF;
  } else {
    SwfdecRect rect = movie->extents;
    SwfdecMovie *other;

    if (argc > 0) {
      other =
	swfdec_player_get_movie_from_value (SWFDEC_PLAYER (cx), &argv[0]);
      if (!other)
	return;
    } else {
      other = movie;
    }

    if (movie->parent)
      swfdec_movie_rect_local_to_global (movie->parent, &rect);
    swfdec_movie_rect_global_to_local (other, &rect);

    x0 = rect.x0;
    y0 = rect.y0;
    x1 = rect.x1;
    y1 = rect.y1;
  }
  swfdec_as_value_set_number (cx, &val, SWFDEC_TWIPS_TO_DOUBLE (x0));
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_xMin, &val);
  swfdec_as_value_set_number (cx, &val, SWFDEC_TWIPS_TO_DOUBLE (y0));
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_yMin, &val);
  swfdec_as_value_set_number (cx, &val, SWFDEC_TWIPS_TO_DOUBLE (x1));
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_xMax, &val);
  swfdec_as_value_set_number (cx, &val, SWFDEC_TWIPS_TO_DOUBLE (y1));
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_yMax, &val);

  SWFDEC_AS_VALUE_SET_OBJECT (rval, obj);
}

SWFDEC_AS_NATIVE (900, 11, swfdec_sprite_movie_setMask)
void
swfdec_sprite_movie_setMask (SwfdecAsContext *cx, SwfdecAsObject *object,
        guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie, *mask;

  /* yes, this works with regular movies */
  SWFDEC_AS_CHECK (SWFDEC_TYPE_MOVIE, &movie, "O", &mask);

  if (mask != NULL && !SWFDEC_IS_MOVIE (mask)) {
    SWFDEC_FIXME ("mask is not a movie, what now?");
    mask = NULL;
  }
  if (movie->masked_by)
    movie->masked_by->mask_of = NULL;
  if (movie->mask_of)
    movie->mask_of->masked_by = NULL;
  movie->masked_by = mask;
  movie->mask_of = NULL;
  if (movie->clip_depth) {
    g_assert (movie->parent);
    swfdec_movie_invalidate_last (movie->parent);
    movie->clip_depth = 0;
  } else {
    swfdec_movie_invalidate_last (movie);
  }
  if (mask) {
    if (mask->masked_by)
      mask->masked_by->mask_of = NULL;
    if (mask->mask_of)
      mask->mask_of->masked_by = NULL;
    mask->masked_by = NULL;
    mask->mask_of = movie;
    swfdec_movie_invalidate_last (mask);
    if (mask->clip_depth) {
      g_assert (mask->parent);
      swfdec_movie_invalidate_last (mask->parent);
      mask->clip_depth = 0;
    } else {
      swfdec_movie_invalidate_last (mask);
    }
  }
}

