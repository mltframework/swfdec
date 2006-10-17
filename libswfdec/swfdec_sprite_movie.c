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

#include "swfdec_sprite_movie.h"
#include "swfdec_audio.h"
#include "swfdec_debug.h"
#include "swfdec_js.h"
#include "swfdec_player_internal.h"
#include "swfdec_ringbuffer.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite.h"

/*** GOTO HANDLING ***/

static SwfdecMovie *
swfdec_movie_find (GList *movie_list, guint depth)
{
  GList *walk;

  for (walk = movie_list; walk; walk = walk->next) {
    SwfdecMovie *movie = walk->data;

    if (movie->content->depth < depth)
      continue;
    if (movie->content->depth == depth)
      return movie;
    break;
  }
  return NULL;
}

static gboolean
swfdec_sprite_movie_remove_child (SwfdecMovie *movie, guint depth)
{
  SwfdecMovie *child = swfdec_movie_find (movie->list, depth);

  if (child == NULL)
    return FALSE;

  swfdec_movie_remove (child);
  return TRUE;
}

static void
swfdec_sprite_movie_perform_one_action (SwfdecSpriteMovie *movie, SwfdecSpriteAction *action,
    gboolean skip_scripts, GList **movie_list)
{
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecPlayer *player = SWFDEC_ROOT_MOVIE (mov->root)->player;
  SwfdecMovie *child;
  SwfdecContent *content;

  switch (action->type) {
    case SWFDEC_SPRITE_ACTION_SCRIPT:
      SWFDEC_LOG ("SCRIPT action");
      if (!skip_scripts)
	swfdec_js_execute_script (player, mov, action->data, NULL);
      break;
    case SWFDEC_SPRITE_ACTION_ADD:
      content = action->data;
      SWFDEC_LOG ("ADD action: depth %u", content->depth);
      if (swfdec_sprite_movie_remove_child (mov, content->depth))
	SWFDEC_DEBUG ("removed a child before adding new one");
      child = swfdec_movie_find (*movie_list, content->depth);
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
      SWFDEC_LOG ("UPDATE action: depth %u", content->depth);
      child = swfdec_movie_find (mov->list, content->depth);
      if (child != NULL) {
	swfdec_movie_set_content (child, content);
      } else {
	SWFDEC_WARNING ("supposed to update depth %u, but no child", content->depth);
      }
      break;
    case SWFDEC_SPRITE_ACTION_REMOVE:
      SWFDEC_LOG ("REMOVE action: depth %u", GPOINTER_TO_UINT (action->data));
      if (!swfdec_sprite_movie_remove_child (mov, GPOINTER_TO_UINT (action->data)))
	SWFDEC_WARNING ("could not remove, no child at depth %u", GPOINTER_TO_UINT (action->data));
      break;
    default:
      g_assert_not_reached ();
  }
}

static void
swfdec_sprite_movie_do_goto_frame (SwfdecSpriteMovie *movie, unsigned int goto_frame, 
    gboolean do_enter_frame)
{
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  GList *old, *walk;
  guint i, j, start;

  if (do_enter_frame)
    swfdec_movie_execute (mov, SWFDEC_EVENT_ENTER);

  if (goto_frame == movie->current_frame)
    return;

  if (goto_frame < movie->current_frame) {
    start = 0;
    old = mov->list;
    mov->list = NULL;
  } else {
    start = movie->current_frame + 1;
    old = NULL;
  }
  if (movie->current_frame == (guint) -1 || 
      movie->sprite->frames[goto_frame].bg_color != 
      movie->sprite->frames[movie->current_frame].bg_color) {
    swfdec_movie_invalidate (mov);
  }
  movie->current_frame = goto_frame;
  SWFDEC_DEBUG ("iterating from %u to %u", start, movie->current_frame);
  for (i = start; i <= movie->current_frame; i++) {
    SwfdecSpriteFrame *frame = &movie->sprite->frames[i];
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

static void
swfdec_sprite_movie_goto_func (SwfdecMovie *mov, gpointer data)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  gint i = GPOINTER_TO_INT (data);

  /* decode frame info */
  if (i < 0) {
    swfdec_sprite_movie_do_goto_frame (movie, -i - 1, TRUE);
  } else {
    swfdec_sprite_movie_do_goto_frame (movie, i, FALSE);
  }
}

void
swfdec_sprite_movie_goto (SwfdecSpriteMovie *movie, guint frame, gboolean do_enter_frame)
{
  SwfdecPlayer *player;
  int data;

  g_assert (SWFDEC_IS_SPRITE_MOVIE (movie));
  g_assert (frame < SWFDEC_MOVIE (movie)->n_frames);

  player = SWFDEC_ROOT_MOVIE (SWFDEC_MOVIE (movie)->root)->player;
  SWFDEC_LOG ("queueing goto %u for %p %d", frame, movie, SWFDEC_CHARACTER (movie->sprite)->id);
  
  g_assert (frame <= G_MAXINT);

  /* encode frame info into an int */
  data = frame;
  if (do_enter_frame)
    data = -data - 1;

  swfdec_player_add_action (player, SWFDEC_MOVIE (movie), 
      swfdec_sprite_movie_goto_func, GINT_TO_POINTER (data));
  SWFDEC_MOVIE (movie)->frame = frame;
}

/*** MOVIE ***/

G_DEFINE_TYPE (SwfdecSpriteMovie, swfdec_sprite_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_sprite_movie_dispose (GObject *object)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (object);

  g_assert (movie->sound_stream == 0);

  G_OBJECT_CLASS (swfdec_sprite_movie_parent_class)->dispose (object);
}

static void
swfdec_sprite_movie_goto_frame (SwfdecMovie *movie, guint frame)
{
  swfdec_sprite_movie_goto (SWFDEC_SPRITE_MOVIE (movie), frame, FALSE);
}

static gboolean
swfdec_sprite_movie_should_iterate (SwfdecSpriteMovie *movie)
{
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecMovie *parent;
  guint parent_frame;

  /* movies that are scheduled for removal this frame don't do anything */
  /* FIXME: This function is complicated. It doesn't seem to be as stupid as
   * Flash normally is. I suppose there's a simpler solution */

  parent = mov->parent;
  /* root movie always gets executed */
  if (!SWFDEC_IS_SPRITE_MOVIE (parent))
    return TRUE;

  if (parent->stopped) {
    parent_frame = parent->frame;
  } else {
    parent_frame = swfdec_sprite_get_next_frame (SWFDEC_SPRITE_MOVIE (parent)->sprite, parent->frame);
  }
  /* check if we'll get removed until parent's next frame */
  if (parent_frame < mov->content->sequence->start ||
      parent_frame >= mov->content->sequence->end)
    return FALSE;

  /* check if parent will get removed */
  return swfdec_sprite_movie_should_iterate (SWFDEC_SPRITE_MOVIE (parent));
}

static void
swfdec_sprite_movie_iterate (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  unsigned int goto_frame;

  if (!swfdec_sprite_movie_should_iterate (movie))
    return;
  if (mov->stopped) {
    goto_frame = mov->frame;
  } else {
    goto_frame = swfdec_sprite_get_next_frame (movie->sprite, mov->frame);
  }
  swfdec_sprite_movie_goto (movie, goto_frame, TRUE);
}

static void
swfdec_sprite_movie_iterate_audio (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  SwfdecSpriteFrame *last;
  SwfdecSpriteFrame *current;
  GSList *walk;
  SwfdecPlayer *player = SWFDEC_ROOT_MOVIE (SWFDEC_MOVIE (movie)->root)->player;

  current = &movie->sprite->frames[movie->current_frame];
  
  /* first start all event sounds */
  for (walk = current->sound; walk; walk = walk->next) {
    swfdec_audio_event_init (player, walk->data);
  }

  /* then do the streaming thing */
  if (current->sound_head == NULL) {
    if (movie->sound_stream) {
      swfdec_audio_stream_stop (player, movie->sound_stream);
      movie->sound_stream = 0;
    }
    return;
  }
  SWFDEC_LOG ("iterating audio (from %u to %u)", movie->sound_frame, movie->current_frame);
  if (swfdec_sprite_get_next_frame (movie->sprite, movie->sound_frame) != movie->current_frame)
    goto new_decoder;
  if (movie->sound_frame == (guint) -1)
    goto new_decoder;
  if (current->sound_head && movie->sound_stream == 0)
    goto new_decoder;
  last = &movie->sprite->frames[movie->sound_frame];
  if (last->sound_head != current->sound_head)
    goto new_decoder;
  movie->sound_frame = movie->current_frame;
  return;

new_decoder:
  if (movie->sound_stream) {
    swfdec_audio_stream_stop (player, movie->sound_stream);
    movie->sound_stream = 0;
  }

  movie->sound_stream = swfdec_audio_stream_new (player, 
      movie->sprite, movie->current_frame);
  movie->sound_frame = movie->current_frame;
}

static void
swfdec_sprite_movie_set_parent (SwfdecMovie *mov, SwfdecMovie *parent)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  SwfdecPlayer *player = SWFDEC_ROOT_MOVIE (mov->root)->player;

  if (parent) {
    /* set */
    mov->n_frames = movie->sprite->n_frames;
    swfdec_sprite_movie_do_goto_frame (movie, 0, FALSE);
    swfdec_sprite_movie_iterate_audio (mov);
  } else {
    /* unset */
    swfdec_player_remove_all_actions (player, mov);
    if (movie->sound_stream) {
      swfdec_audio_stream_stop (player, movie->sound_stream);
      movie->sound_stream = 0;
    }
  }
}

static void
swfdec_sprite_movie_class_init (SwfdecSpriteMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_sprite_movie_dispose;

  movie_class->set_parent = swfdec_sprite_movie_set_parent;
  movie_class->goto_frame = swfdec_sprite_movie_goto_frame;
  movie_class->iterate_start = swfdec_sprite_movie_iterate;
  movie_class->iterate_end = swfdec_sprite_movie_iterate_audio;
}

static void
swfdec_sprite_movie_init (SwfdecSpriteMovie * movie)
{
  movie->current_frame = (guint) -1;
}

void
swfdec_sprite_movie_paint_background (SwfdecSpriteMovie *movie, cairo_t *cr)
{
  SwfdecSpriteFrame *frame = &movie->sprite->frames[movie->current_frame];

  swfdec_color_set_source (cr, frame->bg_color);
  cairo_paint (cr);
}
