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

#include "swfdec_movie.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_player_internal.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_swf_decoder.h"
#include "swfdec_swf_instance.h"

static void
swfdec_sprite_movie_play (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_MOVIE (obj)->stopped = FALSE;
}

static void
swfdec_sprite_movie_stop (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_MOVIE (obj)->stopped = TRUE;
}

static void
swfdec_sprite_movie_getBytesLoaded (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);

  if (SWFDEC_MOVIE (movie->swf->movie) == movie) {
    SWFDEC_AS_VALUE_SET_INT (rval, movie->swf->decoder->bytes_loaded);
  } else {
    SWFDEC_AS_VALUE_SET_INT (rval, 0);
  }
}

static void
swfdec_sprite_movie_getBytesTotal (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);

  if (SWFDEC_MOVIE (movie->swf->movie) == movie) {
    SWFDEC_AS_VALUE_SET_INT (rval, movie->swf->decoder->bytes_total);
  } else {
    SWFDEC_AS_VALUE_SET_INT (rval, 0);
  }
}

static void
swfdec_sprite_movie_getNextHighestDepth (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  int depth;

  if (movie->list) {
    depth = SWFDEC_MOVIE (g_list_last (movie->list)->data)->depth + 1;
    if (depth < 0)
      depth = 0;
  } else {
    depth = 0;
  }
  SWFDEC_AS_VALUE_SET_INT (rval, depth);
}

static void
swfdec_sprite_movie_do_goto (SwfdecMovie *movie, SwfdecAsValue *target)
{
  int frame;

  if (SWFDEC_AS_VALUE_IS_STRING (target)) {
    const char *label = SWFDEC_AS_VALUE_GET_STRING (target);
    frame = swfdec_sprite_get_frame (SWFDEC_SPRITE_MOVIE (movie)->sprite, label);
    /* FIXME: nonexisting frames? */
    if (frame == -1)
      return;
    frame++;
  } else {
    frame = swfdec_as_value_to_integer (SWFDEC_AS_OBJECT (movie)->context, target);
  }
  /* FIXME: how to handle overflow? */
  frame = CLAMP (frame, 1, (int) movie->n_frames) - 1;

  swfdec_movie_goto (movie, frame);
}

static void
swfdec_sprite_movie_gotoAndPlay (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  
  swfdec_sprite_movie_do_goto (movie, &argv[0]);
  movie->stopped = FALSE;
}

static void
swfdec_sprite_movie_gotoAndStop (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  
  swfdec_sprite_movie_do_goto (movie, &argv[0]);
  movie->stopped = TRUE;
}

static void
swfdec_sprite_movie_nextFrame (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  
  if (movie->frame + 1 < movie->n_frames)
    swfdec_movie_goto (movie, movie->frame + 1);
  movie->stopped = TRUE;
}

static void
swfdec_sprite_movie_prevFrame (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  
  if (movie->frame > 0)
    swfdec_movie_goto (movie, movie->frame - 1);
  movie->stopped = TRUE;
}

static void
swfdec_sprite_movie_hitTest (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  
  if (argc == 1) {
    SwfdecMovie *other, *tmp;
    SwfdecRect movie_rect, other_rect;
    guint movie_depth, other_depth;
    if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]) ||
        !SWFDEC_IS_MOVIE (other = (SwfdecMovie *) SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]))) {
      SWFDEC_ERROR ("FIXME: what happens now?");
      return;
    }
    swfdec_movie_update (movie);
    swfdec_movie_update (other);
    if (movie->parent != other->parent) {
      movie_rect = movie->original_extents;
      other_rect = other->original_extents;
      swfdec_movie_update (other);
      tmp = movie;
      for (movie_depth = 0; tmp->parent; movie_depth++)
	tmp = tmp->parent;
      tmp = other;
      for (other_depth = 0; tmp->parent; other_depth++)
	tmp = tmp->parent;
      while (movie_depth > other_depth) {
	swfdec_rect_transform (&movie_rect, &movie_rect, &movie->matrix);
	movie = movie->parent;
	movie_depth--;
      }
      while (other_depth > movie_depth) {
	swfdec_rect_transform (&other_rect, &other_rect, &other->matrix);
	other = other->parent;
	other_depth--;
      }
      while (other != movie && other->parent) {
	swfdec_rect_transform (&movie_rect, &movie_rect, &movie->matrix);
	movie = movie->parent;
	swfdec_rect_transform (&other_rect, &other_rect, &other->matrix);
	other = other->parent;
      }
    } else {
      movie_rect = movie->extents;
      other_rect = other->extents;
    }
#if 0
    g_print ("%g %g  %g %g --- %g %g  %g %g\n", 
	SWFDEC_OBJECT (movie)->extents.x0, SWFDEC_OBJECT (movie)->extents.y0,
	SWFDEC_OBJECT (movie)->extents.x1, SWFDEC_OBJECT (movie)->extents.y1,
	SWFDEC_OBJECT (other)->extents.x0, SWFDEC_OBJECT (other)->extents.y0,
	SWFDEC_OBJECT (other)->extents.x1, SWFDEC_OBJECT (other)->extents.y1);
#endif
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, swfdec_rect_intersect (NULL, &movie_rect, &other_rect));
  } else {
    SWFDEC_ERROR ("hitTest for x, y coordinate not implemented");
  }
}

static void
swfdec_sprite_movie_startDrag (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  gboolean center = FALSE;

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
    swfdec_player_set_drag_movie (player, movie, center, &rect);
  } else {
    swfdec_player_set_drag_movie (player, movie, center, NULL);
  }
}

static void
swfdec_sprite_movie_stopDrag (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  swfdec_player_set_drag_movie (SWFDEC_PLAYER (cx), NULL, FALSE, NULL);
}

static void
swfdec_sprite_movie_swapDepths (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  SwfdecMovie *other;
  int depth;

  if (SWFDEC_AS_VALUE_IS_OBJECT (&argv[0])) {
    other = (SwfdecMovie *) SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
    if (!SWFDEC_IS_MOVIE (other) ||
	other->parent != movie->parent)
      return;
    swfdec_movie_invalidate (other);
    depth = other->depth;
    other->depth = movie->depth;
  } else {
    depth = swfdec_as_value_to_integer (cx, &argv[0]);
    other = swfdec_movie_find (movie->parent, depth);
    if (other) {
      swfdec_movie_invalidate (other);
      other->depth = movie->depth;
    }
  }
  swfdec_movie_invalidate (movie);
  movie->depth = depth;
  movie->parent->list = g_list_sort (movie->parent->list, swfdec_movie_compare_depths);
}

static void
swfdec_sprite_movie_copy_props (SwfdecMovie *target, SwfdecMovie *src)
{
  target->matrix = src->matrix;
  target->color_transform = src->color_transform;
  swfdec_movie_queue_update (target, SWFDEC_MOVIE_INVALID_MATRIX);
}

static void
swfdec_sprite_movie_init_from_object (SwfdecMovie *movie, SwfdecAsObject *obj)
{
  SwfdecPlayer *player;

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  g_queue_remove (player->init_queue, movie);
}

static void
swfdec_sprite_movie_attachMovie (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  SwfdecMovie *ret;
  const char *name, *export;
  int depth;
  SwfdecContent *content;
  SwfdecGraphic *sprite;

  export = swfdec_as_value_to_string (cx, &argv[0]);
  name = swfdec_as_value_to_string (cx, &argv[1]);
  if (argc > 3) {
    SWFDEC_FIXME ("attachMovie's initObject isn't implemented");
  }
  sprite = swfdec_swf_instance_get_export (movie->swf, export);
  if (!SWFDEC_IS_SPRITE (sprite)) {
    if (sprite == NULL) {
      SWFDEC_WARNING ("no symbol with name %s exported", export);
    } else {
      SWFDEC_WARNING ("can only use attachMovie with sprites");
    }
    return;
  }
  depth = swfdec_as_value_to_integer (cx, &argv[2]);
  if (swfdec_depth_classify (depth) == SWFDEC_DEPTH_CLASS_EMPTY)
    return;
  ret = swfdec_movie_find (movie, depth);
  if (ret)
    swfdec_movie_remove (ret);
  content = swfdec_content_new (depth);
  content->graphic = sprite;
  content->depth = depth;
  content->clip_depth = 0; /* FIXME: check this */
  content->name = g_strdup (name);
  content->sequence = content;
  content->start = 0;
  content->end = G_MAXUINT;
  content->free = TRUE;
  ret = swfdec_movie_new (movie, content);
  SWFDEC_LOG ("attached %s (%u) as %s to depth %u", export, SWFDEC_CHARACTER (sprite)->id,
      ret->name, ret->depth);
  /* run init and construct */
  swfdec_sprite_movie_init_from_object (ret, NULL);
  swfdec_movie_run_construct (ret);
  SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (ret));
}

static void
swfdec_sprite_movie_duplicateMovieClip (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  SwfdecMovie *ret;
  const char *name;
  int depth;
  SwfdecContent *content;

  name = swfdec_as_value_to_string (cx, &argv[0]);
  depth = swfdec_as_value_to_integer (cx, &argv[1]);
  if (swfdec_depth_classify (depth) == SWFDEC_DEPTH_CLASS_EMPTY)
    return;
  if (!movie->parent) {
    SWFDEC_ERROR ("don't know how to duplicate root movies");
    return;
  }
  ret = swfdec_movie_find (movie->parent, depth);
  if (ret)
    swfdec_movie_remove (ret);
  content = swfdec_content_new (depth);
  *content = *movie->content;
  if (content->events)
    content->events = swfdec_event_list_copy (content->events);
  content->depth = depth;
  content->clip_depth = 0; /* FIXME: check this */
  content->name = g_strdup (name);
  content->sequence = content;
  content->start = 0;
  content->end = G_MAXUINT;
  content->free = TRUE;
  ret = swfdec_movie_new (movie->parent, content);
  /* must be set by now, the movie has a name */
  swfdec_sprite_movie_copy_props (ret, movie);
  SWFDEC_LOG ("duplicated %s as %s to depth %u", movie->name, ret->name, ret->depth);
  SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (ret));
}

static void
swfdec_sprite_movie_removeMovieClip (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);

  if (swfdec_depth_classify (movie->depth) == SWFDEC_DEPTH_CLASS_DYNAMIC)
    swfdec_movie_remove (movie);
}

static void
swfdec_sprite_movie_getURL (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);
  const char *url;
  const char *target;

  url = swfdec_as_value_to_string (cx, &argv[0]);
  if (argc > 1) {
    target = swfdec_as_value_to_string (cx, &argv[1]);
  } else {
    SWFDEC_ERROR ("what's the default target?");
    target = NULL;
  }
  if (argc > 2) {
    SWFDEC_ERROR ("passing variables is not implemented");
  }
  swfdec_movie_load (movie, url, target);
  /* FIXME: does this function return something */
}

static void
swfdec_sprite_movie_getDepth (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (obj);

  SWFDEC_AS_VALUE_SET_INT (rval, movie->depth);
}

void
swfdec_sprite_movie_init_context (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context = SWFDEC_AS_CONTEXT (player);
  SwfdecAsValue val;
  SwfdecAsObject *proto;

  player->MovieClip = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global, 
      SWFDEC_AS_STR_MovieClip, 0, NULL, 0));
  if (player->MovieClip == NULL)
    return;
  proto = swfdec_as_object_new (context);
  if (!proto)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (player->MovieClip, SWFDEC_AS_STR_prototype, &val);
  /* now add all the functions */
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_attachMovie, SWFDEC_TYPE_SPRITE_MOVIE,
      swfdec_sprite_movie_attachMovie, 3);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_duplicateMovieClip, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_duplicateMovieClip, 2);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getBytesLoaded, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_getBytesLoaded, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getBytesTotal, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_getBytesTotal, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getDepth, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_getDepth, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getNextHighestDepth, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_getNextHighestDepth, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getURL, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_getURL, 2);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_gotoAndPlay, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_gotoAndPlay, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_gotoAndStop, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_gotoAndStop, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_hitTest, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_hitTest, 1);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_nextFrame, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_nextFrame, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_play, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_play,	0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_prevFrame, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_prevFrame, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_removeMovieClip, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_removeMovieClip, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_startDrag, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_startDrag, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_stop, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_stop,	0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_stopDrag, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_stopDrag, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_swapDepths, SWFDEC_TYPE_SPRITE_MOVIE, 
      swfdec_sprite_movie_swapDepths, 1);
};
