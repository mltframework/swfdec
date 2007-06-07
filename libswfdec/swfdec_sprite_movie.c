/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include <strings.h>

#include "swfdec_sprite_movie.h"
#include "swfdec_as_object.h"
#include "swfdec_audio_event.h"
#include "swfdec_audio_stream.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_ringbuffer.h"
#include "swfdec_script.h"
#include "swfdec_sprite.h"
#include "swfdec_swf_instance.h"

/*** SWFDEC_SPRITE_MOVIE ***/

static SwfdecMovie *
swfdec_sprite_movie_find (GList *movie_list, int depth)
{
  GList *walk;

  for (walk = movie_list; walk; walk = walk->next) {
    SwfdecMovie *movie = walk->data;

    if (movie->depth < depth)
      continue;
    if (movie->depth == depth)
      return movie;
    break;
  }
  return NULL;
}

static gboolean
swfdec_sprite_movie_remove_child (SwfdecMovie *movie, int depth)
{
  SwfdecMovie *child = swfdec_movie_find (movie, depth);

  if (child == NULL)
    return FALSE;

  swfdec_movie_remove (child);
  return TRUE;
}

static void
swfdec_sprite_movie_run_script (gpointer movie, gpointer data)
{
  swfdec_as_object_run (movie, data);
}

static void
swfdec_sprite_movie_perform_one_action (SwfdecSpriteMovie *movie, SwfdecSpriteAction *action,
    gboolean skip_scripts, GList **movie_list)
{
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (mov)->context);
  SwfdecMovie *child;
  SwfdecContent *content;

  switch (action->type) {
    case SWFDEC_SPRITE_ACTION_SCRIPT:
      SWFDEC_LOG ("SCRIPT action");
      if (!skip_scripts) {
	swfdec_player_add_action (player, mov, swfdec_sprite_movie_run_script, action->data);
      }
      break;
    case SWFDEC_SPRITE_ACTION_ADD:
      content = action->data;
      SWFDEC_LOG ("ADD action: depth %d", content->depth);
      if (swfdec_sprite_movie_remove_child (mov, content->depth))
	SWFDEC_DEBUG ("removed a child before adding new one");
      child = swfdec_sprite_movie_find (*movie_list, content->depth);
      if (child == NULL || child->content->sequence != content->sequence) {
	child = swfdec_movie_new (mov, content);
      } else {
	swfdec_movie_set_content (child, content);
	*movie_list = g_list_remove (*movie_list, child);
	mov->list = g_list_insert_sorted (mov->list, child, swfdec_movie_compare_depths);
      }
      break;
    case SWFDEC_SPRITE_ACTION_UPDATE:
      content = action->data;
      SWFDEC_LOG ("UPDATE action: depth %d", content->depth);
      child = swfdec_movie_find (mov, content->depth);
      if (child != NULL && child->content->sequence == content->sequence) {
	swfdec_movie_set_content (child, content);
      } else if (child) {
	SWFDEC_INFO ("supposed to update depth %d, but child is in different sequence", 
	    content->depth);
      } else {
	SWFDEC_INFO ("supposed to update depth %d, but no child", content->depth);
      }
      break;
    case SWFDEC_SPRITE_ACTION_REMOVE:
      SWFDEC_LOG ("REMOVE action: depth %d", GPOINTER_TO_INT (action->data));
      if (!swfdec_sprite_movie_remove_child (mov, GPOINTER_TO_INT (action->data)))
	SWFDEC_INFO ("could not remove, no child at depth %d", GPOINTER_TO_INT (action->data));
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_movie_tell_about_removal (SwfdecMovie *movie)
{
  GList *walk;
  if (movie->will_be_removed)
    return;
  movie->will_be_removed = TRUE;
  for (walk = movie->list; walk; walk = walk->next) {
    swfdec_movie_tell_about_removal (walk->data);
  }
}

void
swfdec_sprite_movie_prepare (SwfdecSpriteMovie *movie)
{
  GList *walk;
  guint frame;

  g_return_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie));

  if (SWFDEC_MOVIE (movie)->stopped ||
      movie->sprite == NULL)
    return;

  frame = swfdec_sprite_get_next_frame (movie->sprite, SWFDEC_MOVIE (movie)->frame);
  /* tell all relevant movies that they won't survive this */
  for (walk = SWFDEC_MOVIE (movie)->list; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (frame < cur->content->sequence->start || 
	frame >= cur->content->sequence->end)
      swfdec_movie_tell_about_removal (cur);
  }
}

static void
swfdec_sprite_movie_goto (SwfdecMovie *mov, guint goto_frame)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  SwfdecPlayer *player;
  GList *old, *walk;
  guint i, j, start;

  g_assert (goto_frame < mov->n_frames);
  if (movie->sprite == NULL)
    return;
  if (goto_frame >= movie->sprite->parse_frame) {
    SWFDEC_WARNING ("jumping to not-yet-loaded frame %u (loaded: %u/%u)",
	goto_frame, movie->sprite->parse_frame, movie->sprite->n_frames);
    return;
  }

  if (mov->will_be_removed)
    return;
  if (goto_frame == movie->current_frame)
    return;

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (mov)->context);
  SWFDEC_LOG ("doing goto %u for %p %d", goto_frame, mov, 
      SWFDEC_CHARACTER (SWFDEC_SPRITE_MOVIE (mov)->sprite)->id);
  mov->frame = goto_frame;

  if (goto_frame < movie->current_frame) {
    start = 0;
    old = mov->list;
    mov->list = NULL;
  } else {
    start = movie->current_frame + 1;
    old = NULL;
  }
  movie->current_frame = goto_frame;
  SWFDEC_DEBUG ("performing goto %u -> %u for character %u", 
      start, goto_frame, SWFDEC_CHARACTER (movie->sprite)->id);
  for (i = start; i <= movie->current_frame; i++) {
    SwfdecSpriteFrame *frame = &movie->sprite->frames[i];
    if (movie == mov->swf->movie &&
	mov->swf->parse_frame <= i) {
      swfdec_swf_instance_advance (mov->swf);
    }
    if (frame->actions == NULL)
      continue;
    for (j = 0; j < frame->actions->len; j++) {
      swfdec_sprite_movie_perform_one_action (movie,
	  &g_array_index (frame->actions, SwfdecSpriteAction, j),
	  i != movie->current_frame, &old);
    }
  }
  /* FIXME: not sure about the order here, might be relevant for unload events */
  for (walk = old; walk; walk = walk->next) {
    swfdec_movie_remove (walk->data);
  }
  g_list_free (old);
}

/*** MOVIE ***/

G_DEFINE_TYPE (SwfdecSpriteMovie, swfdec_sprite_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_sprite_movie_dispose (GObject *object)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (object);

  g_assert (movie->sound_stream == NULL);

  G_OBJECT_CLASS (swfdec_sprite_movie_parent_class)->dispose (object);
}

static void
swfdec_sprite_movie_iterate (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  guint goto_frame;

  if (mov->will_be_removed)
    return;

  swfdec_movie_queue_script (mov, SWFDEC_EVENT_ENTER);
  if (!mov->stopped && movie->sprite != NULL) {
    goto_frame = swfdec_sprite_get_next_frame (movie->sprite, mov->frame);
    swfdec_sprite_movie_goto (mov, goto_frame);
  }
}

static gboolean
swfdec_sprite_movie_iterate_end (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  SwfdecSpriteFrame *last;
  SwfdecSpriteFrame *current;
  GSList *walk;
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (mov)->context);

  if (!SWFDEC_MOVIE_CLASS (swfdec_sprite_movie_parent_class)->iterate_end (mov)) {
    g_assert (movie->sound_stream == NULL);
    return FALSE;
  }
  
  if (movie->sprite == NULL)
    return TRUE;
  current = &movie->sprite->frames[movie->current_frame];
  /* first start all event sounds */
  /* FIXME: is this correct? */
  if (movie->sound_frame != movie->current_frame) {
    for (walk = current->sound; walk; walk = walk->next) {
      SwfdecAudio *audio = swfdec_audio_event_new (player, walk->data);
      if (audio)
	g_object_unref (audio);
    }
  }

  /* then do the streaming thing */
  if (current->sound_head == NULL ||
      SWFDEC_MOVIE (movie)->stopped) {
    if (movie->sound_stream) {
      swfdec_audio_remove (movie->sound_stream);
      g_object_unref (movie->sound_stream);
      movie->sound_stream = NULL;
    }
    goto exit;
  }
  if (movie->sound_stream == NULL && current->sound_block == NULL)
    goto exit;
  SWFDEC_LOG ("iterating audio (from %u to %u)", movie->sound_frame, movie->current_frame);
  if (movie->sound_frame + 1 != movie->current_frame)
    goto new_decoder;
  if (movie->sound_frame == (guint) -1)
    goto new_decoder;
  if (current->sound_head && movie->sound_stream == NULL)
    goto new_decoder;
  last = &movie->sprite->frames[movie->sound_frame];
  if (last->sound_head != current->sound_head)
    goto new_decoder;
exit:
  movie->sound_frame = movie->current_frame;
  return TRUE;

new_decoder:
  if (movie->sound_stream) {
    swfdec_audio_remove (movie->sound_stream);
    g_object_unref (movie->sound_stream);
  }

  if (current->sound_block) {
    movie->sound_stream = swfdec_audio_stream_new (player, 
	movie->sprite, movie->current_frame);
    movie->sound_frame = movie->current_frame;
  }
  return TRUE;
}

static void
swfdec_sprite_movie_init_movie (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);

  mov->n_frames = movie->sprite->n_frames;
  g_assert (movie->sprite->parse_frame > 0);
  swfdec_sprite_movie_goto (mov, 0);
  if (!swfdec_sprite_movie_iterate_end (mov)) {
    g_assert_not_reached ();
  }
}

static void
swfdec_sprite_movie_finish_movie (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (mov)->context);

  swfdec_player_remove_all_actions (player, mov);
  if (movie->sound_stream) {
    swfdec_audio_remove (movie->sound_stream);
    g_object_unref (movie->sound_stream);
    movie->sound_stream = NULL;
  }
}

static void
swfdec_sprite_movie_add (SwfdecAsObject *object)
{
  const char *name;
  SwfdecAsObject *constructor;
  SwfdecSpriteMovie *movie;

  movie = SWFDEC_SPRITE_MOVIE (object);
  if (!movie->sprite)
    return;

  name = swfdec_swf_instance_get_export_name (SWFDEC_MOVIE (movie)->swf,
      SWFDEC_CHARACTER (movie->sprite));
  if (name != NULL) {
    name = swfdec_as_context_get_string (object->context, name);
    constructor = swfdec_player_get_export_class (SWFDEC_PLAYER (object->context),
      name);
  } else {
    constructor = SWFDEC_PLAYER (object->context)->MovieClip;
  }
  swfdec_as_object_set_constructor (object, constructor, FALSE);
  SWFDEC_AS_OBJECT_CLASS (swfdec_sprite_movie_parent_class)->add (object);
}

static SwfdecMovie *
swfdec_sprite_movie_get_by_name (SwfdecMovie *movie, const char *name)
{
  GList *walk;
  guint version = SWFDEC_AS_OBJECT (movie)->context->version;

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (!cur->has_name)
      continue;
    /* FIXME: make the name string GC'd */
    if ((version >= 7 && g_str_equal (cur->name, name)) ||
	strcasecmp (cur->name, name) == 0)
      return cur;
  }
  return NULL;
}

static gboolean
swfdec_sprite_movie_get_variable (SwfdecAsObject *object, const char *variable,
    SwfdecAsValue *val, guint *flags)
{
  SwfdecMovie *movie;

  if (SWFDEC_AS_OBJECT_CLASS (swfdec_sprite_movie_parent_class)->get (object, variable, val, flags))
    return TRUE;
  
  movie = swfdec_sprite_movie_get_by_name (SWFDEC_MOVIE (object), variable);
  if (movie == NULL)
    return FALSE;

  SWFDEC_AS_VALUE_SET_OBJECT (val, SWFDEC_AS_OBJECT (movie));
  *flags = 0;
  return TRUE;
}

static void
swfdec_sprite_movie_mark (SwfdecAsObject *object)
{
  GList *walk;

  for (walk = SWFDEC_MOVIE (object)->list; walk; walk = walk->next) {
    SwfdecAsObject *child = walk->data;
    g_assert (SWFDEC_AS_OBJECT_HAS_CONTEXT (child));
    swfdec_as_object_mark (child);
  }

  SWFDEC_AS_OBJECT_CLASS (swfdec_sprite_movie_parent_class)->mark (object);
}

static void
swfdec_sprite_movie_class_init (SwfdecSpriteMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_sprite_movie_dispose;

  asobject_class->add = swfdec_sprite_movie_add;
  asobject_class->get = swfdec_sprite_movie_get_variable;
  asobject_class->mark = swfdec_sprite_movie_mark;

  movie_class->init_movie = swfdec_sprite_movie_init_movie;
  movie_class->finish_movie = swfdec_sprite_movie_finish_movie;
  movie_class->goto_frame = swfdec_sprite_movie_goto;
  movie_class->iterate_start = swfdec_sprite_movie_iterate;
  movie_class->iterate_end = swfdec_sprite_movie_iterate_end;
}

static void
swfdec_sprite_movie_init (SwfdecSpriteMovie * movie)
{
  movie->current_frame = (guint) -1;
  movie->sound_frame = (guint) -1;
}

