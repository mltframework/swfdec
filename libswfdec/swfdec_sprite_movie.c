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
#include "swfdec_as_strings.h"
#include "swfdec_audio_event.h"
#include "swfdec_audio_stream.h"
#include "swfdec_debug.h"
#include "swfdec_graphic_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_ringbuffer.h"
#include "swfdec_script.h"
#include "swfdec_sprite.h"
#include "swfdec_swf_instance.h"
#include "swfdec_tag.h"
#include "swfdec_utils.h"

/*** SWFDEC_SPRITE_MOVIE ***/

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

static cairo_operator_t
swfdec_sprite_convert_operator (guint operator)
{
  return CAIRO_OPERATOR_OVER;
}

static int
swfdec_get_clipeventflags (SwfdecMovie *movie, SwfdecBits * bits)
{
  if (SWFDEC_SWF_DECODER (movie->swf->decoder)->version <= 5) {
    return swfdec_bits_get_u16 (bits);
  } else {
    return swfdec_bits_get_u32 (bits);
  }
}

gboolean
swfdec_sprite_movie_perform_place (SwfdecSpriteMovie *movie, SwfdecBits *bits, guint tag)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecMovie *cur;
  gboolean has_clip_actions;
  gboolean has_clip_depth;
  gboolean has_name;
  gboolean has_ratio;
  gboolean has_ctrans;
  gboolean has_transform;
  gboolean has_character;
  gboolean move;
  gboolean depth;
  gboolean cache;
  gboolean has_blend_mode = 0;
  gboolean has_filter = 0;
  int clip_depth;
  cairo_matrix_t transform;
  SwfdecColorTransform ctrans;
  guint ratio, id, version;
  SwfdecEventList *events;
  const char *name;
  SwfdecGraphic *graphic;

  version = SWFDEC_SWF_DECODER (mov->swf->decoder)->version;

  /* 1) check which stuff is set */
  has_clip_actions = swfdec_bits_getbit (bits);
  has_clip_depth = swfdec_bits_getbit (bits);
  has_name = swfdec_bits_getbit (bits);
  has_ratio = swfdec_bits_getbit (bits);
  has_ctrans = swfdec_bits_getbit (bits);
  has_transform = swfdec_bits_getbit (bits);
  has_character = swfdec_bits_getbit (bits);
  move = swfdec_bits_getbit (bits);

  SWFDEC_LOG ("performing PlaceObject%d on movie %s", tag == SWFDEC_TAG_PLACEOBJECT2 ? 2 : 3, mov->name);
  SWFDEC_LOG ("  has_clip_actions = %d", has_clip_actions);
  SWFDEC_LOG ("  has_clip_depth = %d", has_clip_depth);
  SWFDEC_LOG ("  has_name = %d", has_name);
  SWFDEC_LOG ("  has_ratio = %d", has_ratio);
  SWFDEC_LOG ("  has_ctrans = %d", has_ctrans);
  SWFDEC_LOG ("  has_transform = %d", has_transform);
  SWFDEC_LOG ("  has_character = %d", has_character);
  SWFDEC_LOG ("  move = %d", move);

  if (tag == SWFDEC_TAG_PLACEOBJECT3) {
    swfdec_bits_getbits (bits, 5);
    cache = swfdec_bits_getbit (bits);
    has_blend_mode = swfdec_bits_getbit (bits);
    has_filter = swfdec_bits_getbit (bits);
    SWFDEC_LOG ("  cache = %d", cache);
    SWFDEC_LOG ("  has filter = %d", has_filter);
    SWFDEC_LOG ("  has blend mode = %d", has_blend_mode);
  }

  /* 2) read all properties */
  depth = swfdec_bits_get_u16 (bits);
  if (depth >= 16384) {
    SWFDEC_FIXME ("depth of placement too high: %u >= 16384", depth);
  }
  SWFDEC_LOG ("  depth = %d (=> %d)", depth, depth - 16384);
  depth -= 16384;
  if (has_character) {
    id = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  id = %d", id);
  } else {
    id = 0;
  }

  if (has_transform) {
    swfdec_bits_get_matrix (bits, &transform, NULL);
    SWFDEC_LOG ("  matrix = { %g %g, %g %g } + { %g %g }", 
	transform.xx, transform.yx,
	transform.xy, transform.yy,
	transform.x0, transform.y0);
  }
  if (has_ctrans) {
    swfdec_bits_get_color_transform (bits, &ctrans);
    SWFDEC_LOG ("  color transform = %d %d  %d %d  %d %d  %d %d",
	ctrans.ra, ctrans.rb,
	ctrans.ga, ctrans.gb,
	ctrans.ba, ctrans.bb,
	ctrans.aa, ctrans.ab);
  }

  if (has_ratio) {
    ratio = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("  ratio = %d", ratio);
  } else {
    ratio = -1;
  }

  if (has_name) {
    char *s = swfdec_bits_get_string_with_version (bits, version);
    name = swfdec_as_context_give_string (SWFDEC_AS_CONTEXT (player), s);
    SWFDEC_LOG ("  name = %s", name);
  } else {
    name = NULL;
  }

  if (has_clip_depth) {
    clip_depth = swfdec_bits_get_u16 (bits) - 16384;
    SWFDEC_LOG ("  clip_depth = %d (=> %d)", clip_depth + 16384, clip_depth);
  } else {
    clip_depth = 0;
  }

  if (has_filter) {
    SWFDEC_ERROR ("filters aren't implemented, skipping PlaceObject tag!");
    return TRUE;
  }

  if (has_blend_mode) {
    /* FIXME: implement */
    guint operator = swfdec_bits_get_u8 (bits);
    swfdec_sprite_convert_operator (operator);
    SWFDEC_ERROR ("  operator = %u", operator);
  }

  if (has_clip_actions) {
    int reserved, clip_event_flags, event_flags, key_code;
    char *script_name;

    events = swfdec_event_list_new (player);
    reserved = swfdec_bits_get_u16 (bits);
    clip_event_flags = swfdec_get_clipeventflags (mov, bits);

    if (name)
      script_name = g_strdup (name);
    else if (id)
      script_name = g_strdup_printf ("Sprite%u", id);
    else
      script_name = g_strdup ("unknown");
    while ((event_flags = swfdec_get_clipeventflags (mov, bits)) != 0) {
      guint length = swfdec_bits_get_u32 (bits);
      SwfdecBits action_bits;

      swfdec_bits_init_bits (&action_bits, bits, length);
      if (event_flags & SWFDEC_EVENT_KEY_PRESS)
	key_code = swfdec_bits_get_u8 (&action_bits);
      else
	key_code = 0;

      SWFDEC_INFO ("clip event with flags 0x%X, key code %d", event_flags, key_code);
#define SWFDEC_IMPLEMENTED_EVENTS \
  (SWFDEC_EVENT_LOAD | SWFDEC_EVENT_UNLOAD | SWFDEC_EVENT_ENTER | SWFDEC_EVENT_INITIALIZE | SWFDEC_EVENT_CONSTRUCT | \
   SWFDEC_EVENT_MOUSE_DOWN | SWFDEC_EVENT_MOUSE_MOVE | SWFDEC_EVENT_MOUSE_UP)
      if (event_flags & ~SWFDEC_IMPLEMENTED_EVENTS) {
	SWFDEC_ERROR ("using non-implemented clip events %u", event_flags & ~SWFDEC_IMPLEMENTED_EVENTS);
      }
      swfdec_event_list_parse (events, &action_bits, version, 
	  event_flags, key_code, script_name);
      if (swfdec_bits_left (&action_bits)) {
	SWFDEC_ERROR ("not all action data was parsed: %u bytes left",
	    swfdec_bits_left (&action_bits));
      }
    }
    g_free (script_name);
  } else {
    events = NULL;
  }

  /* 3) perform the actions depending on the set properties */
  cur = swfdec_movie_find (mov, depth);
  graphic = swfdec_swf_decoder_get_character (SWFDEC_SWF_DECODER (mov->swf->decoder), id);
  if (move) {
    if (cur == NULL) {
      SWFDEC_INFO ("no movie at depth %d, ignoring move command", depth);
      return TRUE;
    }
    if (graphic) {
      SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (cur);
      if (klass->replace)
	klass->replace (cur, graphic);
    }
    swfdec_movie_set_static_properties (cur, has_transform ? &transform : NULL, 
	has_ctrans ? &ctrans : NULL, ratio, clip_depth, events);
  } else {
    if (cur != NULL && version > 5) {
      SWFDEC_INFO ("depth %d is already occupied by movie %s, not placing", depth, cur->name);
      return TRUE;
    }
    if (!SWFDEC_IS_GRAPHIC (graphic)) {
      SWFDEC_FIXME ("character %u is not a graphic (does it even exist?), aborting", id);
      return FALSE;
    }
    cur = swfdec_movie_new (player, depth, mov, graphic, name);
    swfdec_movie_set_static_properties (cur, has_transform ? &transform : NULL, 
	has_ctrans ? &ctrans : NULL, ratio, clip_depth, events);
    swfdec_movie_initialize (cur);
    if (SWFDEC_IS_SPRITE_MOVIE (cur)) {
      g_queue_push_tail (player->init_queue, cur);
      g_queue_push_tail (player->construct_queue, cur);
      swfdec_movie_queue_script (cur, SWFDEC_EVENT_LOAD);
    }
  }

  return TRUE;
}

static gboolean
swfdec_sprite_movie_perform_one_action (SwfdecSpriteMovie *movie, guint tag, SwfdecBuffer *buffer,
    gboolean skip_scripts)
{
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (mov)->context);
  SwfdecBits bits;

  g_assert (mov->swf);
  swfdec_bits_init (&bits, buffer);

  SWFDEC_LOG ("%p: executing %uth tag %s in frame %u", movie, movie->next_action - 1, 
      swfdec_swf_decoder_get_tag_name (tag), movie->frame);
  switch (tag) {
    case SWFDEC_TAG_DOACTION:
      SWFDEC_LOG ("SCRIPT action");
      if (!skip_scripts) {
	SwfdecScript *script = swfdec_swf_decoder_get_script (
	    SWFDEC_SWF_DECODER (mov->swf->decoder), buffer->data);
	g_assert (script);
	swfdec_player_add_action (player, mov, swfdec_sprite_movie_run_script, script);
      }
      return TRUE;
    case SWFDEC_TAG_PLACEOBJECT2:
    case SWFDEC_TAG_PLACEOBJECT3:
      return swfdec_sprite_movie_perform_place (movie, &bits, tag);
    case SWFDEC_TAG_REMOVEOBJECT:
      /* yes, this code is meant to be like this - the following u16 is the 
       * character id, that we don't care about, the rest is like RemoveObject2
       */
      swfdec_bits_get_u16 (&bits);
      /* fall through */
    case SWFDEC_TAG_REMOVEOBJECT2:
      {
	int depth = swfdec_bits_get_u16 (&bits);
	SWFDEC_LOG ("REMOVE action: depth %d => %d", depth, depth - 16384);
	depth -= 16384;
	if (!swfdec_sprite_movie_remove_child (mov, depth))
	  SWFDEC_INFO ("could not remove, no child at depth %d", depth);
      }
      return TRUE;
    case SWFDEC_TAG_SHOWFRAME:
      if (movie->frame < movie->n_frames) {
	movie->frame++;
      } else {
	SWFDEC_ERROR ("too many ShowFrame tags");
      }
      return FALSE;
    default:
      g_assert_not_reached ();
      return FALSE;
  }
}

static gboolean
swfdec_movie_is_compatible (SwfdecMovie *movie, SwfdecMovie *with)
{
  g_assert (movie->depth == with->depth);

  if (movie->original_ratio != with->original_ratio)
    return FALSE;

  if (G_OBJECT_TYPE (movie) != G_OBJECT_TYPE (with))
    return FALSE;

  return TRUE;
}

void
swfdec_sprite_movie_goto (SwfdecSpriteMovie *movie, guint goto_frame)
{
  SwfdecMovie *mov;
  SwfdecPlayer *player;
  GList *old;
  guint n;

  g_return_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie));

  mov = SWFDEC_MOVIE (movie);
  /* lots of things where we've got nothing to do */
  if (goto_frame == 0 || goto_frame > movie->n_frames || 
      movie->sprite == NULL || mov->will_be_removed || goto_frame == movie->frame)
    return;

  if (goto_frame > movie->sprite->parse_frame) {
    SWFDEC_WARNING ("jumping to not-yet-loaded frame %u (loaded: %u/%u)",
	goto_frame, movie->sprite->parse_frame, movie->sprite->n_frames);
    return;
  }

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (movie)->context);
  SWFDEC_LOG ("doing goto %u for %p %d", goto_frame, movie, 
      SWFDEC_CHARACTER (movie->sprite)->id);

  SWFDEC_DEBUG ("performing goto %u -> %u for character %u", 
      movie->frame, goto_frame, SWFDEC_CHARACTER (movie->sprite)->id);
  if (goto_frame < movie->frame) {
    movie->frame = 0;
    old = mov->list;
    mov->list = NULL;
    n = goto_frame;
    movie->next_action = 0;
  } else {
    /* NB: this path is also taken on init */
    old = NULL;
    n = goto_frame - movie->frame;
  }
  while (n) {
    guint tag;
    SwfdecBuffer *buffer;
    /* FIXME: These actions should probably just be added to the action queue */
    if (movie == mov->swf->movie &&
	mov->swf->parse_frame <= movie->frame)
      swfdec_swf_instance_advance (mov->swf);
    if (!swfdec_sprite_get_action (movie->sprite, movie->next_action, &tag, &buffer))
      break;
    movie->next_action++;
    if (!swfdec_sprite_movie_perform_one_action (movie, tag, buffer, n > 1))
      n--;
  }
  /* now try to copy eventual movies */
  if (old) {
    SwfdecMovie *prev, *cur;
    GList *old_walk, *walk;
    walk = mov->list;
    old_walk = old;
    if (!walk)
      goto out;
    cur = walk->data;
    for (; old_walk; old_walk = old_walk->next) {
      prev = old_walk->data;
      while (cur->depth < prev->depth) {
	walk = walk->next;
	if (!walk)
	  goto out;
	cur = walk->data;
      }
      if (cur->depth == prev->depth &&
	  swfdec_movie_is_compatible (prev, cur)) {
	walk->data = prev;
	/* FIXME: This merging stuff probably needs to be improved a _lot_ */
	if (SWFDEC_IS_GRAPHIC_MOVIE (cur)) {
	  SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (prev);
	  if (klass->replace)
	    klass->replace (prev, SWFDEC_GRAPHIC_MOVIE (cur)->graphic);
	}
	swfdec_movie_set_static_properties (prev, &cur->original_transform,
	    &cur->original_ctrans, cur->original_ratio, cur->clip_depth, cur->events);
	swfdec_movie_destroy (cur);
	cur = prev;
	continue;
      }
      swfdec_movie_remove (prev);
    }
out:
    for (; old_walk; old_walk = old_walk->next) {
      swfdec_movie_remove (old_walk->data);
    }
    g_list_free (old);
  }
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
swfdec_sprite_movie_do_enter_frame (gpointer movie, gpointer unused)
{
  if (SWFDEC_MOVIE (movie)->will_be_removed)
    return;
  swfdec_movie_execute_script (movie, SWFDEC_EVENT_ENTER);
}

static void
swfdec_sprite_movie_iterate (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (mov)->context);
  guint goto_frame;

  if (mov->will_be_removed)
    return;

  swfdec_player_add_action (player, movie, swfdec_sprite_movie_do_enter_frame, NULL);
  if (movie->playing && movie->sprite != NULL) {
    if (movie->frame == movie->n_frames)
      goto_frame = 1;
    else if (movie->sprite && movie->frame == movie->sprite->parse_frame)
      goto_frame = movie->frame;
    else
      goto_frame = movie->frame + 1;
    swfdec_sprite_movie_goto (movie, goto_frame);
  }
}

/* FIXME: This function is a mess */
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
  g_assert (movie->frame <= movie->n_frames);
  current = &movie->sprite->frames[movie->frame - 1];
  /* first start all event sounds */
  /* FIXME: is this correct? */
  if (movie->sound_frame != movie->frame) {
    for (walk = current->sound; walk; walk = walk->next) {
      SwfdecAudio *audio = swfdec_audio_event_new (player, walk->data);
      if (audio)
	g_object_unref (audio);
    }
  }

  /* then do the streaming thing */
  if (current->sound_head == NULL ||
      !movie->playing) {
    if (movie->sound_stream) {
      swfdec_audio_remove (movie->sound_stream);
      g_object_unref (movie->sound_stream);
      movie->sound_stream = NULL;
    }
    goto exit;
  }
  if (movie->sound_stream == NULL && current->sound_block == NULL)
    goto exit;
  SWFDEC_LOG ("iterating audio (from %u to %u)", movie->sound_frame, movie->frame);
  if (movie->sound_frame + 1 != movie->frame)
    goto new_decoder;
  if (movie->sound_frame == (guint) -1)
    goto new_decoder;
  if (current->sound_head && movie->sound_stream == NULL)
    goto new_decoder;
  last = &movie->sprite->frames[movie->sound_frame];
  if (last->sound_head != current->sound_head)
    goto new_decoder;
exit:
  movie->sound_frame = movie->frame;
  return TRUE;

new_decoder:
  if (movie->sound_stream) {
    swfdec_audio_remove (movie->sound_stream);
    g_object_unref (movie->sound_stream);
  }

  if (current->sound_block) {
    movie->sound_stream = swfdec_audio_stream_new (player, 
	movie->sprite, movie->frame);
    movie->sound_frame = movie->frame;
  }
  return TRUE;
}

static void
swfdec_sprite_movie_init_movie (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  SwfdecAsContext *context = SWFDEC_AS_OBJECT (movie)->context;
  SwfdecAsObject *constructor = NULL;

  g_assert (mov->swf != NULL);

  if (movie->sprite) {
    const char *name;

    g_assert (movie->sprite->parse_frame > 0);
    movie->n_frames = movie->sprite->n_frames;
    name = swfdec_swf_instance_get_export_name (mov->swf,
	SWFDEC_CHARACTER (movie->sprite));
    if (name != NULL) {
      name = swfdec_as_context_get_string (context, name);
      constructor = swfdec_player_get_export_class (SWFDEC_PLAYER (context),
	  name);
    }
  }
  if (constructor == NULL)
    constructor = SWFDEC_PLAYER (context)->MovieClip;

  swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie), constructor, FALSE);
  swfdec_sprite_movie_goto (movie, 1);
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

static SwfdecMovie *
swfdec_sprite_movie_get_by_name (SwfdecMovie *movie, const char *name)
{
  GList *walk;
  guint version = SWFDEC_AS_OBJECT (movie)->context->version;

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovie *cur = walk->data;
    if (cur->original_name == SWFDEC_AS_STR_EMPTY)
      continue;
    if ((version >= 7 && cur->name == name) ||
	swfdec_str_case_equal (cur->name, name))
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

  asobject_class->get = swfdec_sprite_movie_get_variable;
  asobject_class->mark = swfdec_sprite_movie_mark;

  movie_class->init_movie = swfdec_sprite_movie_init_movie;
  movie_class->finish_movie = swfdec_sprite_movie_finish_movie;
  movie_class->iterate_start = swfdec_sprite_movie_iterate;
  movie_class->iterate_end = swfdec_sprite_movie_iterate_end;
}

static void
swfdec_sprite_movie_init (SwfdecSpriteMovie * movie)
{
  movie->playing = TRUE;
}

