/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_as_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_audio_swf_stream.h"
#include "swfdec_audio_event.h"
#include "swfdec_audio_stream.h"
#include "swfdec_debug.h"
#include "swfdec_filter.h"
#include "swfdec_graphic_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_ringbuffer.h"
#include "swfdec_script_internal.h"
#include "swfdec_sprite.h"
#include "swfdec_resource.h"
#include "swfdec_tag.h"

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

static int
swfdec_get_clipeventflags (SwfdecMovie *movie, SwfdecBits * bits)
{
  if (SWFDEC_SWF_DECODER (movie->resource->decoder)->version <= 5) {
    return swfdec_bits_get_u16 (bits);
  } else {
    return swfdec_bits_get_u32 (bits);
  }
}

static gboolean
swfdec_sprite_movie_perform_old_place (SwfdecSpriteMovie *movie,
    SwfdecBits *bits, guint tag)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecMovie *cur;
  SwfdecSwfDecoder *dec;
  int depth;
  cairo_matrix_t transform;
  gboolean has_ctrans;
  SwfdecColorTransform ctrans;
  guint id;
  SwfdecGraphic *graphic;

  dec = SWFDEC_SWF_DECODER (mov->resource->decoder);

  SWFDEC_LOG ("performing PlaceObject on movie %s", mov->name);

  id = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  depth = swfdec_bits_get_u16 (bits);
  if (depth >= 16384) {
    SWFDEC_FIXME ("depth of placement too high: %u >= 16384", depth);
  }
  SWFDEC_LOG ("  depth = %d (=> %d)", depth, depth - 16384);
  depth -= 16384;

  swfdec_bits_get_matrix (bits, &transform, NULL);
  SWFDEC_LOG ("  matrix = { %g %g, %g %g } + { %g %g }",
      transform.xx, transform.yx,
      transform.xy, transform.yy,
      transform.x0, transform.y0);

  if (swfdec_bits_left (bits)) {
    has_ctrans = TRUE;
    swfdec_bits_get_color_transform (bits, &ctrans);
    SWFDEC_LOG ("  color transform = %d %d  %d %d  %d %d  %d %d",
	ctrans.ra, ctrans.rb,
	ctrans.ga, ctrans.gb,
	ctrans.ba, ctrans.bb,
	ctrans.aa, ctrans.ab);
  } else {
    has_ctrans = FALSE;
  }

  /* 3) perform the actions depending on the set properties */
  cur = swfdec_movie_find (mov, depth);
  graphic = swfdec_swf_decoder_get_character (dec, id);

  if (!SWFDEC_IS_GRAPHIC (graphic)) {
    SWFDEC_FIXME ("character %u is not a graphic (does it even exist?), aborting", id);
    return FALSE;
  }

  cur = swfdec_movie_new (player, depth, mov, mov->resource, graphic, NULL);
  swfdec_movie_set_static_properties (cur, &transform,
      has_ctrans ? &ctrans : NULL, -1, 0, 0, NULL);
  if (SWFDEC_IS_ACTOR (cur)) {
    SwfdecActor *actor = SWFDEC_ACTOR (cur);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_INITIALIZE);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_CONSTRUCT);
    swfdec_actor_queue_script (actor, SWFDEC_EVENT_LOAD);
  }
  swfdec_movie_initialize (cur);

  return TRUE;
}


static gboolean
swfdec_sprite_movie_perform_place (SwfdecSpriteMovie *movie, SwfdecBits *bits, guint tag)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecMovie *cur;
  SwfdecSwfDecoder *dec;
  gboolean has_clip_actions;
  gboolean has_clip_depth;
  gboolean has_name;
  gboolean has_ratio;
  gboolean has_ctrans;
  gboolean has_transform;
  gboolean has_character;
  gboolean move;
  int depth;
  gboolean cache;
  gboolean has_blend_mode = 0;
  gboolean has_filter = 0;
  int clip_depth;
  cairo_matrix_t transform;
  SwfdecColorTransform ctrans;
  guint ratio, id, version;
  SwfdecEventList *events;
  const char *name;
  guint blend_mode;
  SwfdecGraphic *graphic;
  GSList *filters;

  dec = SWFDEC_SWF_DECODER (mov->resource->decoder);
  version = dec->version;

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
    char *s = swfdec_bits_get_string (bits, version);
    if (s) {
      name = swfdec_as_context_give_string (SWFDEC_AS_CONTEXT (player), s);
      SWFDEC_LOG ("  name = %s", name);
    } else {
      name = NULL;
    }
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
    filters = swfdec_filter_parse (player, bits);
  } else {
    filters = NULL;
  }

  if (has_blend_mode) {
    blend_mode = swfdec_bits_get_u8 (bits);
    SWFDEC_LOG ("  blend mode = %u", blend_mode);
  } else {
    blend_mode = 0;
  }

  if (has_clip_actions) {
    int reserved, clip_event_flags, event_flags, key_code;
    char *script_name;

    events = swfdec_event_list_new ();
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
      if (event_flags & (1<<SWFDEC_EVENT_KEY_PRESS))
	key_code = swfdec_bits_get_u8 (&action_bits);
      else
	key_code = 0;

      SWFDEC_INFO ("clip event with flags 0x%X, key code %d", event_flags, key_code);
#define SWFDEC_UNIMPLEMENTED_EVENTS \
  ((1<< SWFDEC_EVENT_DATA))
      if (event_flags & SWFDEC_UNIMPLEMENTED_EVENTS) {
	SWFDEC_ERROR ("using non-implemented clip events %u", event_flags & SWFDEC_UNIMPLEMENTED_EVENTS);
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
  graphic = swfdec_swf_decoder_get_character (dec, id);
  if (move) {
    if (cur == NULL) {
      SWFDEC_INFO ("no movie at depth %d, ignoring move command", depth);
      goto out;
    }
    if (graphic) {
      SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (cur);
      if (klass->replace)
	klass->replace (cur, graphic);
    }
    swfdec_movie_set_static_properties (cur, has_transform ? &transform : NULL, 
	has_ctrans ? &ctrans : NULL, ratio, clip_depth, blend_mode, events);
  } else {
    if (cur != NULL && version > 5) {
      SWFDEC_INFO ("depth %d is already occupied by movie %s, not placing", depth, cur->name);
      goto out;
    }
    if (!SWFDEC_IS_GRAPHIC (graphic)) {
      SWFDEC_FIXME ("character %u is not a graphic (does it even exist?), aborting", id);
      if (events)
	swfdec_event_list_free (events);
      g_slist_foreach (filters, (GFunc) g_object_unref, NULL);
      g_slist_free (filters);
      return FALSE;
    }
    cur = swfdec_movie_new (player, depth, mov, mov->resource, graphic, name);
    swfdec_movie_set_static_properties (cur, has_transform ? &transform : NULL, 
	has_ctrans ? &ctrans : NULL, ratio, clip_depth, blend_mode, events);
    if (SWFDEC_IS_ACTOR (cur)) {
      SwfdecActor *actor = SWFDEC_ACTOR (cur);
      swfdec_actor_queue_script (actor, SWFDEC_EVENT_INITIALIZE);
      swfdec_actor_queue_script (actor, SWFDEC_EVENT_CONSTRUCT);
      swfdec_actor_queue_script (actor, SWFDEC_EVENT_LOAD);
    }
    swfdec_movie_initialize (cur);
  }
out:
  if (has_filter) {
    if (cur->filters)
      g_slist_free (cur->filters);
    cur->filters = filters;
  }

  if (events)
    swfdec_event_list_free (events);
  return TRUE;
}

static void
swfdec_sprite_movie_start_sound (SwfdecMovie *movie, SwfdecBits *bits)
{
  SwfdecSoundChunk *chunk;
  int id;

  id = swfdec_bits_get_u16 (bits);
  chunk = swfdec_sound_parse_chunk (SWFDEC_SWF_DECODER (movie->resource->decoder), bits, id);
  if (chunk) {
    SwfdecAudio *audio = swfdec_audio_event_new_from_chunk (SWFDEC_PLAYER (
	  swfdec_gc_object_get_context (movie)), chunk);
    if (audio)
      g_object_unref (audio);
  }
}

static gboolean
swfdec_sprite_movie_perform_one_action (SwfdecSpriteMovie *movie, guint tag, SwfdecBuffer *buffer,
    gboolean fast_forward, gboolean first_time)
{
  SwfdecMovie *mov = SWFDEC_MOVIE (movie);
  SwfdecActor *actor = SWFDEC_ACTOR (movie);
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (mov));
  SwfdecBits bits;

  g_assert (mov->resource);
  swfdec_bits_init (&bits, buffer);

  SWFDEC_LOG ("%p: executing %uth tag %s in frame %u", movie, movie->next_action - 1, 
      swfdec_swf_decoder_get_tag_name (tag), movie->frame);
  switch (tag) {
    case SWFDEC_TAG_SETBACKGROUNDCOLOR:
      swfdec_player_set_background_color (player, swfdec_bits_get_color (&bits));
      return TRUE;
    case SWFDEC_TAG_DOACTION:
      SWFDEC_LOG ("SCRIPT action");
      if (!fast_forward) {
	SwfdecScript *script = swfdec_swf_decoder_get_script (
	    SWFDEC_SWF_DECODER (mov->resource->decoder), buffer->data);
	if (script) {
	  swfdec_player_add_action_script (player, actor, script, 
	      SWFDEC_PLAYER_ACTION_QUEUE_NORMAL);
	} else {
	  SWFDEC_ERROR ("Failed to locate script for DoAction tag");
	}
      }
      return TRUE;
    case SWFDEC_TAG_PLACEOBJECT:
      return swfdec_sprite_movie_perform_old_place (movie, &bits, tag);
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
    case SWFDEC_TAG_STARTSOUND:
      if (!fast_forward) 
	swfdec_sprite_movie_start_sound (mov, &bits);
      return TRUE;
    case SWFDEC_TAG_SHOWFRAME:
      if (movie->frame < movie->n_frames) {
	movie->frame++;
      } else {
	SWFDEC_ERROR ("too many ShowFrame tags");
      }
      return FALSE;
    case SWFDEC_TAG_EXPORTASSETS:
      {
	SwfdecResource *resource = swfdec_movie_get_own_resource (mov);
	guint i, count;

	g_assert (resource); /* must hold, ExportAssets can only be in root movies */
	if (!first_time)
	  return TRUE;
	count = swfdec_bits_get_u16 (&bits);
	SWFDEC_LOG ("exporting %u assets", count);
	for (i = 0; i < count && swfdec_bits_left (&bits); i++) {
	  SwfdecSwfDecoder *s = SWFDEC_SWF_DECODER (resource->decoder);
	  guint id;
	  SwfdecCharacter *object;
	  char *name;

	  id = swfdec_bits_get_u16 (&bits);
	  object = swfdec_swf_decoder_get_character (s, id);
	  name = swfdec_bits_get_string (&bits, s->version);
	  if (object == NULL) {
	    SWFDEC_ERROR ("cannot export id %u as %s, id wasn't found", id, name);
	  } else if (name == NULL) {
	    SWFDEC_ERROR ("cannot export id %u, no name was given", id);
	  } else {
	    SWFDEC_LOG ("exporting %s %u as %s", G_OBJECT_TYPE_NAME (object), id, name);
	    swfdec_resource_add_export (resource, object, name); 
	  }
	  g_free (name);
	}
      }
      return TRUE;
    case SWFDEC_TAG_DOINITACTION:
      if (!first_time)
	return TRUE;
      if (!swfdec_movie_get_own_resource (mov)) {
	SWFDEC_FIXME ("behavior of init actions in DefineSprite untested");
      }
      {
	guint id;
	SwfdecSprite *sprite;

	id = swfdec_bits_get_u16 (&bits);
	SWFDEC_LOG ("InitAction");
	SWFDEC_LOG ("  id = %u", id);
	sprite = swfdec_swf_decoder_get_character (SWFDEC_SWF_DECODER (mov->resource->decoder), id);
	if (!SWFDEC_IS_SPRITE (sprite)) {
	  SWFDEC_ERROR ("character %u is not a sprite", id);
	  return TRUE;
	}
	if (sprite->init_action != NULL) {
	  SWFDEC_ERROR ("sprite %u already has an init action", id);
	  return TRUE;
	}
	sprite->init_action = swfdec_script_ref (swfdec_swf_decoder_get_script (
	    SWFDEC_SWF_DECODER (mov->resource->decoder), buffer->data + 2));
	if (sprite->init_action) {
	  swfdec_player_add_action_script (player, actor, sprite->init_action, 
	      SWFDEC_PLAYER_ACTION_QUEUE_INIT);
	} else {
	  SWFDEC_ERROR ("Failed to locate script for InitAction of Sprite %u", id);
	}
      }
      return TRUE;
    case SWFDEC_TAG_SOUNDSTREAMHEAD:
    case SWFDEC_TAG_SOUNDSTREAMHEAD2:
      /* ignore, those are handled by the sound stream */
      return TRUE;
    case SWFDEC_TAG_SOUNDSTREAMBLOCK:
      if (!fast_forward) {
	if (movie->sound_stream == NULL) {
	  movie->sound_stream = swfdec_audio_swf_stream_new (player, movie->sprite,
	      movie->next_action - 1);
	}
	movie->sound_active = TRUE;
      }
      return TRUE;
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

static GList *
my_g_list_split (GList *list, GList *split)
{
  GList *prev;

  if (split == NULL)
    return list;

  prev = split->prev;
  if (prev == NULL)
    return NULL;
  prev->next = NULL;
  split->prev = NULL;
  return list;
}

void
swfdec_sprite_movie_goto (SwfdecSpriteMovie *movie, guint goto_frame)
{
  SwfdecMovie *mov;
  SwfdecPlayer *player;
  GList *old;
  guint n;
  gboolean remove_audio;

  g_return_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie));

  mov = SWFDEC_MOVIE (movie);
  /* lots of things where we've got nothing to do */
  if (goto_frame == 0 || goto_frame > movie->n_frames || 
      movie->sprite == NULL || mov->state >= SWFDEC_MOVIE_STATE_REMOVED || goto_frame == movie->frame)
    return;

  if (goto_frame > movie->sprite->parse_frame) {
    SWFDEC_WARNING ("jumping to not-yet-loaded frame %u (loaded: %u/%u)",
	goto_frame, movie->sprite->parse_frame, movie->sprite->n_frames);
    return;
  }

  player = SWFDEC_PLAYER (swfdec_gc_object_get_context (movie));
  SWFDEC_LOG ("doing goto %u for %p %d", goto_frame, movie, 
      SWFDEC_CHARACTER (movie->sprite)->id);

  SWFDEC_DEBUG ("performing goto %u -> %u for character %u", 
      movie->frame, goto_frame, SWFDEC_CHARACTER (movie->sprite)->id);
  if (goto_frame < movie->frame) {
    GList *walk;
    movie->frame = 0;
    for (walk = mov->list; walk && 
	swfdec_depth_classify (SWFDEC_MOVIE (walk->data)->depth) != SWFDEC_DEPTH_CLASS_TIMELINE;
	walk = walk->next) {
      /* do nothing */
    }
    old = walk;
    mov->list = my_g_list_split (mov->list, old);
    for (walk = old; walk && 
	swfdec_depth_classify (SWFDEC_MOVIE (walk->data)->depth) == SWFDEC_DEPTH_CLASS_TIMELINE;
	walk = walk->next) {
      /* do nothing */
    }
    old = my_g_list_split (old, walk);
    mov->list = g_list_concat (mov->list, walk);
    n = goto_frame;
    movie->next_action = 0;
    remove_audio = TRUE;
  } else {
    /* NB: this path is also taken on init */
    old = NULL;
    n = goto_frame - movie->frame;
    remove_audio = n > 1;
  }
  /* remove audio after seeks */
  if (remove_audio && movie->sound_stream) {
    swfdec_audio_remove (movie->sound_stream);
    g_object_unref (movie->sound_stream);
    movie->sound_stream = NULL;
  }
  remove_audio = !movie->sound_active;
  movie->sound_active = FALSE;
  while (n) {
    guint tag;
    gboolean first_time;
    SwfdecBuffer *buffer;
    if (!swfdec_sprite_get_action (movie->sprite, movie->next_action, &tag, &buffer))
      break;
    movie->next_action++;
    if (movie->next_action > movie->max_action) {
      first_time = TRUE;
      movie->max_action = movie->next_action;
    } else {
      first_time = FALSE;
    }
    if (!swfdec_sprite_movie_perform_one_action (movie, tag, buffer, n > 1, first_time))
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
	SwfdecMovieClass *klass = SWFDEC_MOVIE_GET_CLASS (prev);
	walk->data = prev;
	/* FIXME: This merging stuff probably needs to be improved a _lot_ */
	if (klass->replace)
	  klass->replace (prev, cur->graphic);
	swfdec_movie_set_static_properties (prev, &cur->original_transform,
	    &cur->color_transform, cur->original_ratio, cur->clip_depth, 
	    cur->blend_mode, SWFDEC_IS_ACTOR (cur) ? SWFDEC_ACTOR (cur)->events : NULL);
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

  /* after two frames without SoundStreamBlock, audio apparently gets removed */
  if (!movie->sound_active && remove_audio && movie->sound_stream != NULL) {
    swfdec_audio_remove (movie->sound_stream);
    g_object_unref (movie->sound_stream);
    movie->sound_stream = NULL;
  }
}

/*** MOVIE ***/

G_DEFINE_TYPE (SwfdecSpriteMovie, swfdec_sprite_movie, SWFDEC_TYPE_ACTOR)

static void
swfdec_sprite_movie_dispose (GObject *object)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (object);

  g_assert (movie->sound_stream == NULL);

  G_OBJECT_CLASS (swfdec_sprite_movie_parent_class)->dispose (object);
}

static void
swfdec_sprite_movie_init_movie (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);

  g_assert (movie->frame == (guint) -1);
  movie->frame = 0;
  swfdec_sprite_movie_goto (movie, 1);
}

static GObject *
swfdec_sprite_movie_constructor (GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties)
{
  GObject *object;
  SwfdecMovie *movie;

  object = G_OBJECT_CLASS (swfdec_sprite_movie_parent_class)->constructor (type, 
      n_construct_properties, construct_properties);

  movie = SWFDEC_MOVIE (object);
  if (movie->resource->sandbox) {
    swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (movie), 
	movie->resource->sandbox->MovieClip);
  }

  if (movie->graphic) {
    SwfdecSpriteMovie *smovie = SWFDEC_SPRITE_MOVIE (object);
    smovie->sprite = SWFDEC_SPRITE (movie->graphic);
    smovie->n_frames = smovie->sprite->n_frames;
  }

  return object;
}

static void
swfdec_sprite_movie_iterate (SwfdecActor *actor)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (actor);
  guint goto_frame;

  if (SWFDEC_MOVIE (actor)->state >= SWFDEC_MOVIE_STATE_REMOVED)
    return;

  if (movie->sprite && movie->frame == (guint) -1)
    movie->frame = 0;

  swfdec_actor_queue_script (actor, SWFDEC_EVENT_ENTER);
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

static void
swfdec_sprite_movie_finish_movie (SwfdecMovie *mov)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (mov));

  swfdec_player_remove_all_actions (player, SWFDEC_ACTOR (mov));
  if (movie->sound_stream) {
    swfdec_audio_remove (movie->sound_stream);
    g_object_unref (movie->sound_stream);
    movie->sound_stream = NULL;
  }
}

static void
swfdec_sprite_movie_mark (SwfdecGcObject *object)
{
  GList *walk;

  for (walk = SWFDEC_MOVIE (object)->list; walk; walk = walk->next) {
    SwfdecAsObject *child = walk->data;
    g_assert (child->properties != NULL);
    swfdec_gc_object_mark (child);
  }

  SWFDEC_GC_OBJECT_CLASS (swfdec_sprite_movie_parent_class)->mark (object);
}

static void
swfdec_sprite_movie_property_get (SwfdecMovie *mov, guint prop_id, SwfdecAsValue *val)
{
  SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (mov);

  switch (prop_id) {
    case SWFDEC_MOVIE_PROPERTY_CURRENTFRAME:
      SWFDEC_AS_VALUE_SET_INT (val, movie->frame);
      break;
    case SWFDEC_MOVIE_PROPERTY_FRAMESLOADED:
      SWFDEC_AS_VALUE_SET_INT (val, swfdec_sprite_movie_get_frames_loaded (movie));
      break;
    case SWFDEC_MOVIE_PROPERTY_TOTALFRAMES:
      SWFDEC_AS_VALUE_SET_INT (val, swfdec_sprite_movie_get_frames_total (movie));
      break;
    default:
      SWFDEC_MOVIE_CLASS (swfdec_sprite_movie_parent_class)->property_get (mov, prop_id, val);
      break;
  }
}

static void
swfdec_sprite_movie_class_init (SwfdecSpriteMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);
  SwfdecActorClass *actor_class = SWFDEC_ACTOR_CLASS (g_class);

  object_class->dispose = swfdec_sprite_movie_dispose;
  object_class->constructor = swfdec_sprite_movie_constructor;

  gc_class->mark = swfdec_sprite_movie_mark;

  movie_class->init_movie = swfdec_sprite_movie_init_movie;
  movie_class->finish_movie = swfdec_sprite_movie_finish_movie;
  movie_class->property_get = swfdec_sprite_movie_property_get;
  
  actor_class->iterate_start = swfdec_sprite_movie_iterate;
}

static void
swfdec_sprite_movie_init (SwfdecSpriteMovie * movie)
{
  movie->playing = TRUE;
  movie->frame = (guint) -1;
}

/* cute little hack */
extern void
swfdec_sprite_movie_clear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval);
/**
 * swfdec_sprite_movie_unload:
 * @movie: a #SwfdecMovie
 *
 * Clears all contents from the given movie. This means deleting all
 * variables and removing all children movie clips.
 **/
void
swfdec_sprite_movie_unload (SwfdecSpriteMovie *movie)
{
  SwfdecMovie *mov;
  SwfdecAsValue hack;

  g_return_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie));

  mov = SWFDEC_MOVIE (movie);
  swfdec_sprite_movie_clear (swfdec_gc_object_get_context (movie), 
      SWFDEC_AS_OBJECT (movie), 0, NULL, &hack);
  /* FIXME: destroy or unload? */
  while (mov->list)
    swfdec_movie_remove (mov->list->data);
  swfdec_as_object_delete_all_variables (SWFDEC_AS_OBJECT (movie));
  movie->frame = (guint) -1;
  movie->n_frames = 0;
  movie->next_action = 0;
  movie->max_action = 0;
  movie->sprite = NULL;
  swfdec_movie_queue_update (SWFDEC_MOVIE (movie), SWFDEC_MOVIE_INVALID_EXTENTS);
}

/**
 * swfdec_sprite_movie_get_frames_loaded:
 * @movie: a #SwfdecSpriteMovie
 *
 * Computes the number of loaded frames as used by the _framesloaded property
 * or the WaitForFrame actions. If the @movie is fully loaded, this is the 
 * amount of total frames of the sprite it displays, or 0 if it has no sprite.
 * If the movie is not fully loaded, it is the amount of frames that are 
 * completely loaded minus one. Welcome to the world of Flash.
 *
 * Returns: The number of loaded frames as reported by ActionScript.
 **/
int
swfdec_sprite_movie_get_frames_loaded (SwfdecSpriteMovie *movie)
{
  SwfdecResource *resource;
  SwfdecDecoder *dec;

  g_return_val_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie), 0);

  resource = swfdec_movie_get_own_resource (SWFDEC_MOVIE (movie));
  if (resource == NULL) {
    /* FIXME: can we set n_frames to 1 for movies without sprites instead? */
    if (movie->sprite)
      return movie->n_frames;
    else
      return 1;
  }
  dec = resource->decoder;
  if (dec == NULL)
    return -1;
  if (dec->frames_loaded < dec->frames_total)
    return dec->frames_loaded - 1;
  return dec->frames_total;
}

int
swfdec_sprite_movie_get_frames_total (SwfdecSpriteMovie *movie)
{
  SwfdecResource *resource;
  SwfdecDecoder *dec;

  g_return_val_if_fail (SWFDEC_IS_SPRITE_MOVIE (movie), 0);

  resource = swfdec_movie_get_own_resource (SWFDEC_MOVIE (movie));
  if (resource == NULL) {
    /* FIXME: can we set n_frames to 1 for movies without sprites instead? */
    if (movie->sprite)
      return movie->n_frames;
    else
      return 1;
  }
  dec = resource->decoder;
  if (dec == NULL)
    return 0;
  return dec->frames_total;
}
