/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
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

#include "swfdec_as_interpret.h"
#include "swfdec_as_array.h"
#include "swfdec_as_context.h"
#include "swfdec_as_date.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_function.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_script_function.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_string.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_super.h"
#include "swfdec_as_internal.h"
#include "swfdec_debug.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include "swfdec_decoder.h"
#include "swfdec_load_object.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_resource.h"
#include "swfdec_text_field_movie.h" // for typeof

/* Define this to get SWFDEC_WARN'd about missing properties of objects.
 * This can be useful to find out about unimplemented native properties,
 * but usually just causes a lot of spam. */
//#define SWFDEC_WARN_MISSING_PROPERTIES

/*** SUPPORT FUNCTIONS ***/

#define swfdec_action_has_register(cx, i) \
  ((i) < (cx)->frame->n_registers)

/*** ALL THE ACTION IS HERE ***/

static void
swfdec_action_stop (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (SWFDEC_IS_SPRITE_MOVIE (cx->frame->target))
    SWFDEC_SPRITE_MOVIE (cx->frame->target)->playing = FALSE;
  else
    SWFDEC_ERROR ("no movie to stop");
}

static void
swfdec_action_play (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (SWFDEC_IS_SPRITE_MOVIE (cx->frame->target))
    SWFDEC_SPRITE_MOVIE (cx->frame->target)->playing = TRUE;
  else
    SWFDEC_ERROR ("no movie to play");
}

static void
swfdec_action_next_frame (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (SWFDEC_IS_SPRITE_MOVIE (cx->frame->target)) {
    SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (cx->frame->target);
    if (movie->frame < movie->n_frames) {
      swfdec_sprite_movie_goto (movie, movie->frame + 1);
      movie->playing = FALSE;
    } else {
      SWFDEC_INFO ("can't execute nextFrame, already at last frame");
    }
  } else {
    SWFDEC_ERROR ("no movie to nextFrame on");
  }
}

static void
swfdec_action_previous_frame (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (SWFDEC_IS_SPRITE_MOVIE (cx->frame->target)) {
    SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (cx->frame->target);
    if (movie->frame > 1) {
      swfdec_sprite_movie_goto (movie, movie->frame - 1);
      movie->playing = FALSE;
    } else {
      SWFDEC_INFO ("can't execute previousFrame, already at first frame");
    }
  } else {
    SWFDEC_ERROR ("no movie to previousFrame on");
  }
}

static void
swfdec_action_goto_frame (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  guint frame;

  if (len != 2) {
    SWFDEC_ERROR ("GotoFrame action length invalid (is %u, should be 2", len);
    return;
  }
  frame = data[0] | (data[1] << 8);
  if (SWFDEC_IS_SPRITE_MOVIE (cx->frame->target)) {
    SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (cx->frame->target);
    swfdec_sprite_movie_goto (movie, frame + 1);
    movie->playing = FALSE;
  } else {
    SWFDEC_ERROR ("no movie to goto on");
  }
}

static void
swfdec_action_goto_label (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (!memchr (data, 0, len)) {
    SWFDEC_ERROR ("GotoLabel action does not specify a string");
    return;
  }

  if (SWFDEC_IS_SPRITE_MOVIE (cx->frame->target)) {
    SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (cx->frame->target);
    int frame;
    if (movie->sprite == NULL ||
	(frame = swfdec_sprite_get_frame (movie->sprite, (const char *) data)) == -1)
      return;
    swfdec_sprite_movie_goto (movie, frame + 1);
    movie->playing = FALSE;
  } else {
    SWFDEC_ERROR ("no movie to goto on");
  }
}

/* returns: frame to go to or 0 on error */
static guint
swfdec_value_to_frame (SwfdecAsContext *cx, SwfdecSpriteMovie *movie, SwfdecAsValue *val)
{
  int frame;

  if (movie->sprite == NULL)
    return 0;
  if (SWFDEC_AS_VALUE_IS_STRING (val)) {
    const char *name = SWFDEC_AS_VALUE_GET_STRING (val);
    double d;
    if (strchr (name, ':')) {
      SWFDEC_ERROR ("FIXME: handle targets");
    }
    /* treat valid encoded numbers as numbers, otherwise assume it's a frame label */
    d = swfdec_as_value_to_number (cx, val);
    if (isnan (d))
      frame = swfdec_sprite_get_frame (movie->sprite, name) + 1;
    else
      frame = d;
  } else if (SWFDEC_AS_VALUE_IS_NUMBER (val)) {
    frame = swfdec_as_value_to_integer (cx, val);
  } else {
    SWFDEC_WARNING ("cannot convert value to frame number");
    /* FIXME: how do we treat undefined etc? */
    frame = 0;
  }
  return frame <= 0 ? 0 : frame;
}

static void
swfdec_action_goto_frame2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  guint bias;
  gboolean play;
  SwfdecAsValue *val;

  swfdec_bits_init_data (&bits, data, len);
  if (swfdec_bits_getbits (&bits, 6)) {
    SWFDEC_WARNING ("reserved bits in GotoFrame2 aren't 0");
  }
  bias = swfdec_bits_getbit (&bits);
  play = swfdec_bits_getbit (&bits);
  if (bias) {
    bias = swfdec_bits_get_u16 (&bits);
  }
  val = swfdec_as_stack_peek (cx, 1);
  /* now set it */
  if (SWFDEC_IS_SPRITE_MOVIE (cx->frame->target)) {
    SwfdecSpriteMovie *movie = SWFDEC_SPRITE_MOVIE (cx->frame->target);
    guint frame = swfdec_value_to_frame (cx, movie, val);
    if (frame > 0) {
      frame += bias;
      frame = CLAMP (frame, 1, movie->n_frames);
      swfdec_sprite_movie_goto (movie, frame);
      movie->playing = play;
    }
  } else {
    SWFDEC_ERROR ("no movie to GotoFrame2 on");
  }
  swfdec_as_stack_pop (cx);
}

static void
swfdec_script_skip_actions (SwfdecAsContext *cx, guint jump)
{
  SwfdecScript *script = cx->frame->script;
  const guint8 *pc = cx->frame->pc;
  const guint8 *endpc = script->buffer->data + script->buffer->length;

  /* jump instructions */
  do {
    if (pc >= endpc)
      break;
    if (*pc & 0x80) {
      if (pc + 2 >= endpc)
	break;
      pc += 3 + (pc[1] | (pc[2] << 8));
    } else {
      pc++;
    }
  } while (jump-- > 0);
  cx->frame->pc = pc;
}

static void
swfdec_action_wait_for_frame2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecSpriteMovie *movie;
  int frame, loaded;

  if (len < 1) {
    SWFDEC_ERROR ("WaitForFrame2 needs a 1-byte data");
    return;
  }
  if (!SWFDEC_IS_SPRITE_MOVIE (cx->frame->target)) {
    SWFDEC_ERROR ("no movie for WaitForFrame");
    return;
  }

  movie = SWFDEC_SPRITE_MOVIE (cx->frame->target);
  frame = swfdec_value_to_frame (cx, movie, swfdec_as_stack_pop (cx));
  loaded = swfdec_sprite_movie_get_frames_loaded (movie);
  if (loaded < (int) movie->n_frames &&
      loaded < frame - 1)
    swfdec_script_skip_actions (cx, data[0]);
}

static void
swfdec_action_wait_for_frame (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecSpriteMovie *movie;
  guint jump;
  int frame, loaded;

  if (len != 3) {
    SWFDEC_ERROR ("WaitForFrame action length invalid (is %u, should be 3", len);
    return;
  }
  if (!SWFDEC_IS_SPRITE_MOVIE (cx->frame->target)) {
    SWFDEC_ERROR ("no movie for WaitForFrame");
    return;
  }

  movie = SWFDEC_SPRITE_MOVIE (cx->frame->target);
  frame = data[0] | (data[1] << 8);
  jump = data[2];
  loaded = swfdec_sprite_movie_get_frames_loaded (movie);
  if (loaded < (int) movie->n_frames &&
      loaded < frame)
    swfdec_script_skip_actions (cx, jump);
}

static void
swfdec_action_constant_pool (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecConstantPool *pool;
  SwfdecAsFrame *frame;
  SwfdecBuffer *buffer;

  frame = cx->frame;
  /* FIXME: lots of hackery to get at the buffer */
  buffer = frame->script->buffer;
  buffer = swfdec_buffer_new_subbuffer (buffer, data - buffer->data, len);
  pool = swfdec_constant_pool_new (cx, buffer, cx->version);
  swfdec_buffer_unref (buffer);
  if (pool == NULL)
    return;
  if (frame->constant_pool)
    swfdec_constant_pool_unref (frame->constant_pool);
  frame->constant_pool = pool;
}

static void
swfdec_action_push (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;

  swfdec_bits_init_data (&bits, data, len);
  while (swfdec_bits_left (&bits)) {
    guint type = swfdec_bits_get_u8 (&bits);
    SWFDEC_LOG ("push type %u", type);
    swfdec_as_stack_ensure_free (cx, 1);
    switch (type) {
      case 0: /* string */
	{
	  char *s = swfdec_bits_get_string (&bits, cx->version);
	  if (s == NULL)
	    return;
	  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_push (cx), 
	      swfdec_as_context_give_string (cx, s));
	  break;
	}
      case 1: /* float */
	SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_push (cx), 
	    swfdec_bits_get_float (&bits));
	break;
      case 2: /* null */
	SWFDEC_AS_VALUE_SET_NULL (swfdec_as_stack_push (cx));
	break;
      case 3: /* undefined */
	SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_push (cx));
	break;
      case 4: /* register */
	{
	  guint regnum = swfdec_bits_get_u8 (&bits);
	  if (!swfdec_action_has_register (cx, regnum)) {
	    SWFDEC_ERROR ("cannot Push register %u: not enough registers", regnum);
	    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_push (cx));
	  } else {
	    *swfdec_as_stack_push (cx) = cx->frame->registers[regnum];
	  }
	  break;
	}
      case 5: /* boolean */
	SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_push (cx), 
	    swfdec_bits_get_u8 (&bits) ? TRUE : FALSE);
	break;
      case 6: /* double */
	SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_push (cx), 
	    swfdec_bits_get_double (&bits));
	break;
      case 7: /* 32bit int */
	SWFDEC_AS_VALUE_SET_INT (swfdec_as_stack_push (cx), 
	    swfdec_bits_get_s32 (&bits));
	break;
      case 8: /* 8bit ConstantPool address */
      case 9: /* 16bit ConstantPool address */
	{
	  guint i = type == 8 ? swfdec_bits_get_u8 (&bits) : swfdec_bits_get_u16 (&bits);
	  SwfdecConstantPool *pool = cx->frame->constant_pool;
	  if (pool == NULL) {
	    SWFDEC_ERROR ("no constant pool to push from");
	    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_push (cx));
	    break;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    SWFDEC_ERROR ("constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_push (cx));
	    break;
	  }
	  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_push (cx), 
	      swfdec_constant_pool_get (pool, i));
	  break;
	}
      default:
	SWFDEC_ERROR ("Push: unknown type %u, skipping", type);
	break;
    }
  }
}

/* NB: name must be GC'd */
static SwfdecAsObject *
super_special_movie_lookup_magic (SwfdecAsContext *cx, SwfdecAsObject *o, const char *name)
{
  SwfdecAsValue val;

  if (o == NULL) {
    o = swfdec_as_frame_get_variable (cx->frame, name, NULL);
    if (o == NULL)
      return NULL;
  }
  if (SWFDEC_IS_MOVIE (o)) {
    SwfdecMovie *ret = swfdec_movie_get_by_name (SWFDEC_MOVIE (o), name, TRUE);
    if (ret)
      return SWFDEC_AS_OBJECT (ret);
  }
  if (!swfdec_as_object_get_variable (o, name, &val))
    return NULL;
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return NULL;
  return SWFDEC_AS_VALUE_GET_OBJECT (&val);
}

static SwfdecAsObject *
swfdec_action_get_movie_by_slash_path (SwfdecAsContext *cx, const char *path)
{
  SwfdecAsObject *o;

  o = cx->frame->target;
  if (!SWFDEC_IS_MOVIE (o))
    return NULL;
  if (*path == '/') {
    o = SWFDEC_AS_OBJECT (swfdec_movie_get_root (SWFDEC_MOVIE (o)));
    path++;
  }
  while (*path) {
    char *slash = strchr (path, '/');
    const char *name;
    if (slash) {
      if (slash == path)
	return NULL;
      name = swfdec_as_context_give_string (cx, g_strndup (path, slash - path));
      path = slash + 1;
    } else {
      name = swfdec_as_context_get_string (cx, path);
      path += strlen (path);
    }
    o = super_special_movie_lookup_magic (cx, o, name);
    if (!SWFDEC_IS_MOVIE (o))
      return NULL;
  }
  return o;
}

SwfdecAsObject *
swfdec_action_lookup_object (SwfdecAsContext *cx, SwfdecAsObject *o, const char *path, const char *end)
{
  gboolean dot_allowed = TRUE;
  const char *start;

  if (path == end) {
    if (o == NULL)
      o = cx->frame->target;
    if (SWFDEC_IS_MOVIE (o))
      return o;
    else
      return NULL;
  }

  if (path[0] == '/') {
    if (o == NULL)
      o = cx->frame->target;
    if (!SWFDEC_IS_MOVIE (o))
      return NULL;
    o = SWFDEC_AS_OBJECT (swfdec_movie_get_root (SWFDEC_MOVIE (o)));
    path++;
    dot_allowed = FALSE;
  }
  while (path < end) {
    for (start = path; path < end; path++) {
      if (dot_allowed && path[0] == '.') {
	if (end - path >= 2 && path[1] == '.') {
	  dot_allowed = FALSE;
	  continue;
	}
      } else if (path[0] == ':') {
	if (path[1] == '/')
	  continue;
	if (path == start) {
	  start++;
	  continue;
	}
      } else if (path[0] == '/') {
	dot_allowed = FALSE;
      } else if (path - start < 127) {
	continue;
      }

      break;
    }

    /* parse variable */
    if (start[0] == '.' && start[1] == '.' && start + 2 == path) {
      if (o == NULL) {
	GSList *walk;
	for (walk = cx->frame->scope_chain; walk; walk = walk->next) {
	  if (SWFDEC_IS_MOVIE (walk->data)) {
	    o = walk->data;
	    break;
	  }
	}
	if (o == NULL)
	  o = cx->frame->target;
      }
      /* ".." goes back to parent */
      if (!SWFDEC_IS_MOVIE (o))
	return NULL;
      o = SWFDEC_AS_OBJECT (SWFDEC_MOVIE (o)->parent);
      if (o == NULL)
	return NULL;
    } else {
      o = super_special_movie_lookup_magic (cx, o, 
	      swfdec_as_context_give_string (cx, g_strndup (start, path - start)));
      if (o == NULL)
	return NULL;
    }
    if (path - start < 127)
      path++;
  }

  return o;
}

/* FIXME: this function belongs into swfdec_movie.c */
SwfdecMovie *
swfdec_player_get_movie_from_value (SwfdecPlayer *player, SwfdecAsValue *val)
{
  SwfdecAsContext *cx;
  const char *s;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (val), NULL);

  cx = SWFDEC_AS_CONTEXT (player);
  s = swfdec_as_value_to_string (cx, val);
  return swfdec_player_get_movie_from_string (player, s);
}

SwfdecMovie *
swfdec_player_get_movie_from_string (SwfdecPlayer *player, const char *s)
{
  SwfdecAsObject *ret;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (s != NULL, NULL);

  ret = swfdec_action_lookup_object (SWFDEC_AS_CONTEXT (player), NULL, s, s + strlen (s));
  if (!SWFDEC_IS_MOVIE (ret)) {
    SWFDEC_WARNING ("\"%s\" does not reference a movie", s);
    return NULL;
  }
  return SWFDEC_MOVIE (ret);
}

/**
 * swfdec_action_get_movie_by_path:
 * @cx: a #SwfdecAsContext
 * @path: the path to look up
 * @object: pointer that takes the object that was looked up. The object may be 
 *          %NULL.
 * @variable: pointer that takes variable part of the path. The variable will 
 *            be either %NULL or a non-gc'ed variable name.
 *
 * Looks up a Flash4-compatible path using "/", ":" and "." style syntax.
 *
 * Returns: The #SwfdecMovie that was looked up or %NULL if the path does not 
 *          specify a valid movie.
 **/
static gboolean
swfdec_action_get_movie_by_path (SwfdecAsContext *cx, const char *path, 
    SwfdecAsObject **object, const char **variable)
{
  SwfdecAsObject *movie;
  char *end, *s;

  g_assert (path != NULL);
  g_assert (object != NULL); 
  g_assert (variable != NULL);
  g_assert (cx->frame != NULL);

  /* find dot or colon */
  end = strpbrk (path, ".:");

  /* if no dot or colon, look up slash-path */
  if (end == NULL) {
    /* shortcut for the general case */
    if (strchr (path, '/') != NULL) {
      movie = swfdec_action_get_movie_by_slash_path (cx, path);
      if (movie) {
	*object = movie;
	*variable = NULL;
	return TRUE;
      }
    }

    *object = NULL;
    *variable = path;
    return TRUE;
  }
  /* find last dot or colon */
  while ((s = strpbrk (end + 1, ".:")) != NULL)
    end = s;

  /* variable to use is the part after the last dot or colon */
  *variable = end + 1;
  /* look up object for start of path */
  if (path == end)
    movie = NULL;
  else
    movie = swfdec_action_lookup_object (cx, NULL, path, end);
  if (movie) {
    *object = movie;
    return TRUE;
  } else {
    *variable = NULL;
    return FALSE;
  }
}

static void
swfdec_action_get_variable (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  const char *s;
  SwfdecAsObject *object;

  val = swfdec_as_stack_peek (cx, 1);
  s = swfdec_as_value_to_string (cx, val);
  if (swfdec_action_get_movie_by_path (cx, s, &object, &s)) {
    if (object) {
      if (s) {
	swfdec_as_object_get_variable (object, swfdec_as_context_get_string (cx, s), val);
      } else {
	SWFDEC_AS_VALUE_SET_OBJECT (val, object);
      }
    } else {
      swfdec_as_frame_get_variable (cx->frame, swfdec_as_context_get_string (cx, s), val);
    }
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (val);
#ifdef SWFDEC_WARN_MISSING_PROPERTIES
    SWFDEC_WARNING ("no variable named %s", s);
#endif
  }
}

static void
swfdec_action_set_variable (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s, *rest;
  SwfdecAsObject *object;

  s = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 2));
  if (swfdec_action_get_movie_by_path (cx, s, &object, &rest)) {
    if (object && rest) {
      swfdec_as_object_set_variable (object, swfdec_as_context_get_string (cx, rest), 
	  swfdec_as_stack_peek (cx, 1));
    } else {
      if (object)
	rest = s;
      else
	rest = swfdec_as_context_get_string (cx, rest);
      swfdec_as_frame_set_variable (cx->frame, rest,
	  swfdec_as_stack_peek (cx, 1), TRUE, FALSE);
    }
  }
  swfdec_as_stack_pop_n (cx, 2);
}

/* FIXME: this sucks */
extern struct {
  gboolean needs_movie;
  const char * name; /* GC'd */
  void (* get) (SwfdecMovie *movie, SwfdecAsValue *ret);
  void (* set) (SwfdecMovie *movie, const SwfdecAsValue *val);
} swfdec_movieclip_props[];
static void
swfdec_action_get_property (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie;
  guint id;

  id = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 1));
  if (!SWFDEC_IS_PLAYER (cx)) {
    SWFDEC_INFO ("tried using GetProperty in a non-SwfdecPlayer context");
    goto error;
  } else {
    movie = swfdec_player_get_movie_from_value (SWFDEC_PLAYER (cx),
	swfdec_as_stack_peek (cx, 2));
    if (movie == NULL)
      goto error;
  }
  if (id > (cx->version > 4 ? 21 : 18)) {
    SWFDEC_WARNING ("trying to SetProperty %u, doesn't exist", id);
    goto error;
  }
  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (movie), swfdec_movieclip_props[id].name,
      swfdec_as_stack_peek (cx, 2));
  swfdec_as_stack_pop (cx);
  return;

error :
  swfdec_as_stack_pop (cx);
  SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx, 1));
}

static void
swfdec_action_set_property (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie;
  guint id;

  id = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 2));
  if (!SWFDEC_IS_PLAYER (cx)) {
    SWFDEC_INFO ("tried using GetProperty in a non-SwfdecPlayer context");
    goto error;
  } else {
    movie = swfdec_player_get_movie_from_value (SWFDEC_PLAYER (cx),
	swfdec_as_stack_peek (cx, 3));
    if (movie == NULL)
      goto error;
  }
  if (id > (cx->version > 4 ? 21 : 18)) {
    SWFDEC_WARNING ("trying to SetProperty %u, doesn't exist", id);
    goto error;
  }
  swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (movie), swfdec_movieclip_props[id].name,
      swfdec_as_stack_peek (cx, 1));
  swfdec_as_stack_pop_n (cx, 3);
  return;

error :
  swfdec_as_stack_pop_n (cx, 3);
}

static void
swfdec_action_get_member (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsObject *object = swfdec_as_value_to_object (cx, swfdec_as_stack_peek (cx, 2));
  if (object) {
    const char *name;
    name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));
    swfdec_as_object_get_variable (object, name, swfdec_as_stack_peek (cx, 2));
#ifdef SWFDEC_WARN_MISSING_PROPERTIES
    if (SWFDEC_AS_VALUE_IS_UNDEFINED (swfdec_as_stack_peek (cx, 2))) {
	SWFDEC_WARNING ("no variable named %s:%s", G_OBJECT_TYPE_NAME (object), name);
    }
#endif
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx, 2));
  }
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_set_member (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (cx, 3))) {
    const char *name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 2));
    swfdec_as_object_set_variable (SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (cx, 3)),
	name, swfdec_as_stack_peek (cx, 1));
  }
  swfdec_as_stack_pop_n (cx, 3);
}

static void
swfdec_action_trace (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  const char *s;

  val = swfdec_as_stack_peek (cx, 1);
  if (val->type == SWFDEC_AS_TYPE_UNDEFINED) {
    s = SWFDEC_AS_STR_undefined;
  } else {
    s = swfdec_as_value_to_string (cx, val);
  }
  swfdec_as_stack_pop (cx);
  g_signal_emit_by_name (cx, "trace", s);
}

/* stack looks like this: [ function, this, arg1, arg2, ... ] */
/* stack must be at least 2 elements big */
static SwfdecAsFrame *
swfdec_action_call (SwfdecAsContext *cx, guint n_args)
{
  SwfdecAsFrame *frame;
  SwfdecAsFunction *fun;
  SwfdecAsObject *thisp;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (cx, 1)))
    goto error;
  fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (cx, 1));
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    goto error;
  if (!SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (cx, 2))) {
    thisp = NULL;
  } else {
    thisp = SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (cx, 2));
  }
  swfdec_as_stack_pop_n (cx, 2);
  /* sanitize argument count */
  if (n_args >= swfdec_as_stack_get_size (cx))
    n_args = swfdec_as_stack_get_size (cx);
  frame = swfdec_as_function_call_no_preload (fun, thisp, n_args, NULL, NULL);
  if (frame == NULL)
    return NULL;
  if (SWFDEC_IS_AS_SUPER (fun)) {
    SWFDEC_LOG ("replacing super object on frame");
    swfdec_as_super_new_chain (frame, SWFDEC_AS_SUPER (fun), NULL);
  }
  return frame;

error:
  n_args += 2;
  if (n_args > swfdec_as_stack_get_size (cx))
    n_args = swfdec_as_stack_get_size (cx);
  swfdec_as_stack_pop_n (cx, n_args);
  SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_push (cx));
  return NULL;
}

static void
swfdec_action_call_function (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsFrame *frame = cx->frame;
  SwfdecAsObject *obj;
  guint n_args;
  const char *name;
  SwfdecAsValue *fun, *thisp;
  
  swfdec_as_stack_ensure_size (cx, 2);
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 2));
  name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));
  thisp = swfdec_as_stack_peek (cx, 2);
  fun = swfdec_as_stack_peek (cx, 1);
  obj = swfdec_as_frame_get_variable (frame, name, fun);
  if (obj) {
    SWFDEC_AS_VALUE_SET_OBJECT (thisp, obj);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (thisp);
    SWFDEC_AS_VALUE_SET_UNDEFINED (fun);
  }
  frame = swfdec_action_call (cx, n_args);
  if (frame) {
    swfdec_as_frame_preload (frame);
  } else {
    SWFDEC_WARNING ("no function named %s", name);
  }
}

static void
swfdec_action_call_method (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsFrame *frame = cx->frame;
  SwfdecAsValue *val;
  SwfdecAsObject *obj;
  SwfdecAsObject *pobj = NULL;
  guint n_args;
  const char *name;
  
  swfdec_as_stack_ensure_size (cx, 3);
  obj = swfdec_as_value_to_object (cx, swfdec_as_stack_peek (cx, 2));
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 3));
  val = swfdec_as_stack_peek (cx, 1);
  if (obj) {
    name = swfdec_as_value_to_string (cx, val);
    if (SWFDEC_AS_VALUE_IS_UNDEFINED (val) ||
	name == SWFDEC_AS_STR_EMPTY) {
      SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx, 3));
      SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (cx, 2), obj);
      name = "";
      pobj = obj;
    } else {
      SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (cx, 3), obj);
      swfdec_as_object_get_variable_and_flags (obj, name, swfdec_as_stack_peek (cx, 2), NULL, &pobj);
    }
  } else {
    if (SWFDEC_AS_VALUE_IS_STRING (val))
      name = SWFDEC_AS_VALUE_GET_STRING (val);
    else
      name = "???";
    SWFDEC_AS_VALUE_SET_NULL (swfdec_as_stack_peek (cx, 3));
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx, 2));
  }
  swfdec_as_stack_pop (cx);
  frame = swfdec_action_call (cx, n_args);
  if (frame) {
    /* setup super to point to the right prototype */
    if (SWFDEC_IS_AS_SUPER (obj)) {
      swfdec_as_super_new_chain (frame, SWFDEC_AS_SUPER (obj), name);
    } else if (cx->version > 6) {
      swfdec_as_super_new (frame, obj, pobj == obj ? obj->prototype : pobj);
    } else {
      swfdec_as_super_new (frame, obj, obj->prototype);
    }
    swfdec_as_frame_preload (frame);
  } else {
    SWFDEC_WARNING ("no function named \"%s\" on object %s", name, obj ? G_OBJECT_TYPE_NAME(obj) : "unknown");
  }
}

static void
swfdec_action_pop (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_binary (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double l, r;

  r = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 1));
  l = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 2));
  switch (action) {
    case SWFDEC_AS_ACTION_ADD:
      l = l + r;
      break;
    case SWFDEC_AS_ACTION_SUBTRACT:
      l = l - r;
      break;
    case SWFDEC_AS_ACTION_MULTIPLY:
      l = l * r;
      break;
    case SWFDEC_AS_ACTION_DIVIDE:
      if (cx->version < 5) {
	if (r == 0) {
	  swfdec_as_stack_pop (cx);
	  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx, 1), SWFDEC_AS_STR__ERROR_);
	  return;
	}
      }
      if (r == 0) {
	if (l > 0)
	  l = INFINITY;
	else if (l < 0)
	  l = -INFINITY;
	else
	  l = NAN;
      } else {
	l = l / r;
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
  swfdec_as_stack_pop (cx);
  SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx, 1), l);
}

static void
swfdec_action_add2_to_primitive (SwfdecAsValue *value)
{
  SwfdecAsObject *object;
  const char *name;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (value))
    return;
  object = SWFDEC_AS_VALUE_GET_OBJECT (value);
  if (SWFDEC_IS_MOVIE (object))
    return;

  if (SWFDEC_IS_AS_DATE (object) && object->context->version > 5)
    name = SWFDEC_AS_STR_toString;
  else
    name = SWFDEC_AS_STR_valueOf;
  swfdec_as_object_call (SWFDEC_AS_VALUE_GET_OBJECT (value), name, 0, NULL, value);
}

static void
swfdec_action_add2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *rval, *lval, rtmp, ltmp;

  rval = swfdec_as_stack_peek (cx, 1);
  lval = swfdec_as_stack_peek (cx, 2);
  rtmp = *rval;
  ltmp = *lval;
  swfdec_action_add2_to_primitive (&rtmp);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&rtmp) || SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (&rtmp)))
    rval = &rtmp;
  swfdec_action_add2_to_primitive (&ltmp);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&ltmp) || SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (&ltmp)))
    lval = &ltmp;

  if (SWFDEC_AS_VALUE_IS_STRING (lval) || SWFDEC_AS_VALUE_IS_STRING (rval)) {
    const char *lstr, *rstr;
    lstr = swfdec_as_value_to_string (cx, lval);
    rstr = swfdec_as_value_to_string (cx, rval);
    lstr = swfdec_as_str_concat (cx, lstr, rstr);
    swfdec_as_stack_pop (cx);
    SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx, 1), lstr);
  } else {
    double d, d2;
    d = swfdec_as_value_to_number (cx, lval);
    d2 = swfdec_as_value_to_number (cx, rval);
    d += d2;
    swfdec_as_stack_pop (cx);
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx, 1), d);
  }
}

static void
swfdec_action_new_comparison (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *lval, *rval;
  double l, r;

  rval = swfdec_as_stack_peek (cx, 1);
  lval = swfdec_as_stack_peek (cx, 2);

  /* swap if we do a greater comparison */
  if (action == SWFDEC_AS_ACTION_GREATER) {
    SwfdecAsValue *tmp = lval;
    lval = rval;
    rval = tmp;
  }
  /* comparison with object is always false */
  swfdec_as_value_to_primitive (lval);
  if (SWFDEC_AS_VALUE_IS_OBJECT (lval) &&
      !SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (lval))) {
    swfdec_as_stack_pop (cx);
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), FALSE);
    return;
  }
  /* same for the rval */
  swfdec_as_value_to_primitive (rval);
  if (SWFDEC_AS_VALUE_IS_OBJECT (rval) &&
      !SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (rval))) {
    swfdec_as_stack_pop (cx);
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), FALSE);
    return;
  }
  /* movieclips are not objects, but they evaluate to NaN, so we can handle them here */
  if (SWFDEC_AS_VALUE_IS_OBJECT (rval) ||
      SWFDEC_AS_VALUE_IS_OBJECT (lval)) {
    swfdec_as_stack_pop (cx);
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx, 1));
    return;
  }
  /* if both are strings, compare strings */
  if (SWFDEC_AS_VALUE_IS_STRING (rval) &&
      SWFDEC_AS_VALUE_IS_STRING (lval)) {
    const char *ls = SWFDEC_AS_VALUE_GET_STRING (lval);
    const char *rs = SWFDEC_AS_VALUE_GET_STRING (rval);
    int cmp;
    if (ls == SWFDEC_AS_STR_EMPTY) {
      cmp = rs == SWFDEC_AS_STR_EMPTY ? 0 : 1;
    } else if (rs == SWFDEC_AS_STR_EMPTY) {
      cmp = -1;
    } else {
      cmp = strcmp (ls, rs);
    }
    swfdec_as_stack_pop (cx);
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), cmp < 0);
    return;
  }
  /* convert to numbers and compare those */
  l = swfdec_as_value_to_number (cx, lval);
  r = swfdec_as_value_to_number (cx, rval);
  swfdec_as_stack_pop (cx);
  /* NaN results in undefined */
  if (isnan (l) || isnan (r)) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx, 1));
    return;
  }
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), l < r);
}

static void
swfdec_action_not (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (cx->version <= 4) {
    double d = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 1));
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx, 1), d == 0 ? 1 : 0);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), 
	!swfdec_as_value_to_boolean (cx, swfdec_as_stack_peek (cx, 1)));
  }
}

static void
swfdec_action_jump (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  gint16 offset;

  if (len != 2) {
    SWFDEC_ERROR ("Jump action length invalid (is %u, should be 2)", len);
    return;
  }
  offset = data[0] | (data[1] << 8);
  cx->frame->pc += 5 + (int) offset;
}

static void
swfdec_action_if (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (len != 2) {
    SWFDEC_ERROR ("If action length invalid (is %u, should be 2)", len);
    return;
  }
  if (swfdec_as_value_to_boolean (cx, swfdec_as_stack_peek (cx, 1))) {
    gint16 offset = data[0] | (data[1] << 8);
    cx->frame->pc += 5 + (int) offset;
  }
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_decrement (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;

  val = swfdec_as_stack_peek (cx, 1);
  SWFDEC_AS_VALUE_SET_NUMBER (val, swfdec_as_value_to_number (cx, val) - 1);
}

static void
swfdec_action_increment (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;

  val = swfdec_as_stack_peek (cx, 1);
  SWFDEC_AS_VALUE_SET_NUMBER (val, swfdec_as_value_to_number (cx, val) + 1);
}

static void
swfdec_action_get_url (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  char *url, *target;

  swfdec_bits_init_data (&bits, data, len);
  url = swfdec_bits_get_string (&bits, cx->version);
  target = swfdec_bits_get_string (&bits, cx->version);
  if (url == NULL || target == NULL) {
    SWFDEC_ERROR ("not enough data in GetURL");
    g_free (url);
    g_free (target);
    return;
  }
  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("leftover bytes in GetURL action");
  }
  if (!SWFDEC_IS_PLAYER (cx)) {
    SWFDEC_ERROR ("GetURL without a SwfdecPlayer");
  } else {
    swfdec_resource_load (SWFDEC_PLAYER (cx), target, url,
	NULL, NULL, FALSE);
  }
  g_free (url);
  g_free (target);
}

static void
swfdec_as_interpret_load_variables_on_finish (SwfdecAsObject *target,
    const char *text)
{
  if (text != NULL)
    swfdec_as_object_decode (target, text);

  // only call onData for sprite movies
  // FIXME: is it called even when loading fails?
  swfdec_actor_queue_script (SWFDEC_ACTOR (target), SWFDEC_EVENT_DATA);
}

static void
swfdec_action_get_url2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *target, *url;
  guint method, internal, variables;

  if (len != 1) {
    SWFDEC_ERROR ("GetURL2 requires 1 byte of data, not %u", len);
    return;
  }

  method = data[0] & 3;
  if (method == 3) {
    SWFDEC_ERROR ("GetURL method 3 invalid");
    method = 0;
  }
  internal = data[0] & 64;
  variables = data[0] & 128;
  if (method == 1 || method == 2) {
    SWFDEC_FIXME ("encode variables");
  }

  target = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));
  url = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 2));

  if (!SWFDEC_IS_PLAYER (cx)) {
    SWFDEC_ERROR ("GetURL2 action requires a SwfdecPlayer");
  } else if (variables) {
    SwfdecMovie *movie;
    
    movie = swfdec_player_get_movie_from_string (SWFDEC_PLAYER (cx), target);
    if (movie != NULL) {
      swfdec_load_object_create (SWFDEC_AS_OBJECT (movie), url, NULL, NULL,
	  swfdec_as_interpret_load_variables_on_finish);
    }
  } else {
    swfdec_resource_load (SWFDEC_PLAYER (cx), target, url, NULL, NULL, internal);
  }

  swfdec_as_stack_pop_n (cx, 2);
}

static void
swfdec_action_string_add (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *lval, *rval;

  rval = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));
  lval = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 2));
  lval = swfdec_as_str_concat (cx, lval, rval);
  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx, 2), lval);
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_push_duplicate (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx, 1);

  *swfdec_as_stack_push (cx) = *val;
}

static void
swfdec_action_random_number (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  gint32 max;
  SwfdecAsValue *val;

  val = swfdec_as_stack_peek (cx, 1);
  max = swfdec_as_value_to_integer (cx, val);
  
  if (max <= 0)
    SWFDEC_AS_VALUE_SET_NUMBER (val, 0);
  else
    SWFDEC_AS_VALUE_SET_NUMBER (val, g_rand_int_range (cx->rand, 0, max));
}

static void
swfdec_action_old_compare (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double l, r;
  gboolean cond;

  l = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 2));
  r = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 1));
  switch (action) {
    case SWFDEC_AS_ACTION_EQUALS:
      cond = l == r;
      break;
    case SWFDEC_AS_ACTION_LESS:
      cond = l < r;
      break;
    default: 
      g_assert_not_reached ();
      return;
  }
  swfdec_as_stack_pop (cx);
  if (cx->version < 5) {
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx, 1), cond ? 1 : 0);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), cond);
  }
}

static void
swfdec_action_string_extract (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  int start, n, left;
  const char *s;

  n = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 1));
  start = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 2));
  s = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 3));
  swfdec_as_stack_pop_n (cx, 2);
  left = g_utf8_strlen (s, -1);
  if (start > left) {
    SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx, 1), SWFDEC_AS_STR_EMPTY);
    return;
  } else if (start < 2) {
    start = 0;
  } else {
    start--;
  }
  left -= start;
  if (n < 0 || n > left)
    n = left;

  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx, 1),
      swfdec_as_str_sub (cx, s, start, n));
}

static void
swfdec_action_string_length (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;
  SwfdecAsValue *v;

  v = swfdec_as_stack_peek (cx, 1);
  s = swfdec_as_value_to_string (cx, v);
  SWFDEC_AS_VALUE_SET_INT (v, g_utf8_strlen (s, -1));  
}

static void
swfdec_action_string_compare (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *l, *r;
  gboolean cond;

  r = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));
  l = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 2));
  switch (action) {
    case SWFDEC_AS_ACTION_STRING_EQUALS:
      cond = l == r;
      break;
    case SWFDEC_AS_ACTION_STRING_LESS:
      cond = strcmp (l, r) < 0;
      break;
    default:
      cond = FALSE;
      g_assert_not_reached ();
      break;
  }
  swfdec_as_stack_pop (cx);
  if (cx->version < 5) {
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx, 1), cond ? 1 : 0);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), cond);
  }
}

static void
swfdec_action_equals2_5 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *rval, *lval;
  SwfdecAsValue rtmp, ltmp;
  SwfdecAsValueType ltype, rtype;
  double l, r;
  gboolean cond;

  rval = swfdec_as_stack_peek (cx, 1);
  lval = swfdec_as_stack_peek (cx, 2);
  rtmp = *rval;
  ltmp = *lval;
  swfdec_as_value_to_primitive (&rtmp);
  swfdec_as_value_to_primitive (&ltmp);
  ltype = ltmp.type;
  rtype = rtmp.type;
  
  /* get objects compared */
  if (ltype == SWFDEC_AS_TYPE_OBJECT && rtype == SWFDEC_AS_TYPE_OBJECT) {
    SwfdecAsObject *lo = SWFDEC_AS_VALUE_GET_OBJECT (&ltmp);
    SwfdecAsObject *ro = SWFDEC_AS_VALUE_GET_OBJECT (&rtmp);

    if (SWFDEC_IS_MOVIE (lo) && SWFDEC_IS_MOVIE (ro)) {
      lo = SWFDEC_AS_OBJECT (swfdec_movie_resolve (SWFDEC_MOVIE (lo)));
      ro = SWFDEC_AS_OBJECT (swfdec_movie_resolve (SWFDEC_MOVIE (ro)));
    } else if (SWFDEC_IS_MOVIE (lo)) {
      swfdec_as_value_to_primitive (rval);
      rtype = rval->type;
      if (rtype != SWFDEC_AS_TYPE_OBJECT) {
	cond = FALSE;
	goto out;
      }
      ro = SWFDEC_AS_VALUE_GET_OBJECT (rval);
    } else if (SWFDEC_IS_MOVIE (ro)) {
      swfdec_as_value_to_primitive (lval);
      ltype = lval->type;
      if (ltype != SWFDEC_AS_TYPE_OBJECT) {
	cond = FALSE;
	goto out;
      }
      lo = SWFDEC_AS_VALUE_GET_OBJECT (lval);
    } else {
      lo = SWFDEC_AS_VALUE_GET_OBJECT (lval);
      ro = SWFDEC_AS_VALUE_GET_OBJECT (rval);
    }
    cond = lo == ro;
    goto out;
  }

  /* compare strings */
  if (ltype == SWFDEC_AS_TYPE_STRING && rtype == SWFDEC_AS_TYPE_STRING) {
    /* FIXME: flash 5 case insensitive? */
    cond = SWFDEC_AS_VALUE_GET_STRING (&ltmp) == SWFDEC_AS_VALUE_GET_STRING (&rtmp);
    goto out;
  }

  /* convert to numbers */
  if (SWFDEC_AS_VALUE_IS_OBJECT (&ltmp) && !SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (&ltmp))) {
    l = swfdec_as_value_to_number (cx, lval);
  } else {
    l = swfdec_as_value_to_number (cx, &ltmp);
  }
  if (SWFDEC_AS_VALUE_IS_OBJECT (&rtmp) && !SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (&rtmp))) {
    r = swfdec_as_value_to_number (cx, rval);
  } else {
    r = swfdec_as_value_to_number (cx, &rtmp);
  }

  /* get rid of undefined and null */
  cond = rtype == SWFDEC_AS_TYPE_UNDEFINED || rtype == SWFDEC_AS_TYPE_NULL;
  if (ltype == SWFDEC_AS_TYPE_UNDEFINED || ltype == SWFDEC_AS_TYPE_NULL) {
    goto out;
  } else if (cond) {
    cond = FALSE;
    goto out;
  }

  /* else compare as numbers */
  if (isnan (l) && isnan (r))
    cond = ltype == rtype;
  else
    cond = l == r;

out:
  swfdec_as_stack_pop (cx);
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), cond);
}

static void
swfdec_action_equals2_6 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *rval, *lval;
  SwfdecAsValueType ltype, rtype;
  double l, r;
  gboolean cond;

  rval = swfdec_as_stack_peek (cx, 1);
  lval = swfdec_as_stack_peek (cx, 2);
  ltype = lval->type;
  rtype = rval->type;
  
  /* get objects compared */
  if (ltype == SWFDEC_AS_TYPE_OBJECT && rtype == SWFDEC_AS_TYPE_OBJECT) {
    SwfdecAsObject *lo = SWFDEC_AS_VALUE_GET_OBJECT (lval);
    SwfdecAsObject *ro = SWFDEC_AS_VALUE_GET_OBJECT (rval);

    if (SWFDEC_IS_MOVIE (lo) && SWFDEC_IS_MOVIE (ro)) {
      lo = SWFDEC_AS_OBJECT (swfdec_movie_resolve (SWFDEC_MOVIE (lo)));
      ro = SWFDEC_AS_OBJECT (swfdec_movie_resolve (SWFDEC_MOVIE (ro)));
    } else if (SWFDEC_IS_MOVIE (lo)) {
      swfdec_as_value_to_primitive (rval);
      rtype = rval->type;
      if (rtype != SWFDEC_AS_TYPE_OBJECT) {
	cond = FALSE;
	goto out;
      }
      ro = SWFDEC_AS_VALUE_GET_OBJECT (rval);
    } else if (SWFDEC_IS_MOVIE (ro)) {
      swfdec_as_value_to_primitive (lval);
      ltype = lval->type;
      if (ltype != SWFDEC_AS_TYPE_OBJECT) {
	cond = FALSE;
	goto out;
      }
      lo = SWFDEC_AS_VALUE_GET_OBJECT (lval);
    }
    cond = lo == ro;
    goto out;
  }

  /* if one of the values is an object, call valueOf. 
   * If it's still an object, return FALSE */
  swfdec_as_value_to_primitive (lval);
  ltype = lval->type;
  if (ltype == SWFDEC_AS_TYPE_OBJECT) {
    cond = FALSE;
    goto out;
  }
  swfdec_as_value_to_primitive (rval);
  rtype = rval->type;
  if (rtype == SWFDEC_AS_TYPE_OBJECT) {
    cond = FALSE;
    goto out;
  }
  /* now we have a comparison without objects */

  /* get rid of undefined and null */
  cond = rtype == SWFDEC_AS_TYPE_UNDEFINED || rtype == SWFDEC_AS_TYPE_NULL;
  if (ltype == SWFDEC_AS_TYPE_UNDEFINED || ltype == SWFDEC_AS_TYPE_NULL) {
    goto out;
  } else if (cond) {
    cond = FALSE;
    goto out;
  }

  /* compare strings */
  if (ltype == SWFDEC_AS_TYPE_STRING && rtype == SWFDEC_AS_TYPE_STRING) {
    /* FIXME: flash 5 case insensitive? */
    cond = SWFDEC_AS_VALUE_GET_STRING (lval) == SWFDEC_AS_VALUE_GET_STRING (rval);
    goto out;
  }

  /* else compare as numbers */
  l = swfdec_as_value_to_number (cx, lval);
  r = swfdec_as_value_to_number (cx, rval);

  if (isnan (l) && isnan (r))
    cond = ltype == rtype;
  else
    cond = l == r;

out:
  swfdec_as_stack_pop (cx);
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), cond);
}

static void
swfdec_action_equals2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (cx->version <= 5) {
    swfdec_action_equals2_5 (cx, action, data, len);
  } else {
    swfdec_action_equals2_6 (cx, action, data, len);
  }
}

static void
swfdec_action_strict_equals (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *rval, *lval;
  gboolean cond;

  rval = swfdec_as_stack_peek (cx, 1);
  lval = swfdec_as_stack_peek (cx, 2);

  if (rval->type != lval->type) {
    cond = FALSE;
  } else {
    switch (rval->type) {
      case SWFDEC_AS_TYPE_UNDEFINED:
      case SWFDEC_AS_TYPE_NULL:
	cond = TRUE;
	break;
      case SWFDEC_AS_TYPE_BOOLEAN:
	cond = SWFDEC_AS_VALUE_GET_BOOLEAN (rval) == SWFDEC_AS_VALUE_GET_BOOLEAN (lval);
	break;
      case SWFDEC_AS_TYPE_NUMBER:
	{
	  double l, r;
	  r = SWFDEC_AS_VALUE_GET_NUMBER (rval);
	  l = SWFDEC_AS_VALUE_GET_NUMBER (lval);
	  cond = (l == r) || (isnan (l) && isnan (r));
	}
	break;
      case SWFDEC_AS_TYPE_STRING:
	cond = SWFDEC_AS_VALUE_GET_STRING (rval) == SWFDEC_AS_VALUE_GET_STRING (lval);
	break;
      case SWFDEC_AS_TYPE_OBJECT:
	{
	  SwfdecAsObject *lo = SWFDEC_AS_VALUE_GET_OBJECT (lval);
	  SwfdecAsObject *ro = SWFDEC_AS_VALUE_GET_OBJECT (rval);
	  if (SWFDEC_IS_MOVIE (lo) && SWFDEC_IS_MOVIE (ro)) {
	    cond = swfdec_movie_resolve (SWFDEC_MOVIE (lo)) == swfdec_movie_resolve (SWFDEC_MOVIE (ro));
	  } else if (!SWFDEC_IS_MOVIE (lo) && !SWFDEC_IS_MOVIE (ro)) {
	    cond = lo == ro;
	  } else {
	    cond = FALSE;
	  }
	}
	break;
      case SWFDEC_AS_TYPE_INT:
      default:
	g_assert_not_reached ();
	cond = FALSE;
    }
  }

  swfdec_as_stack_pop (cx);
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 1), cond);
}

static void
swfdec_action_do_set_target (SwfdecAsContext *cx, const char *target, const char *end)
{
  swfdec_as_frame_set_target (cx->frame, NULL);

  if (target != end) {
    SwfdecAsObject *o = swfdec_action_lookup_object (cx, NULL, target, end);
    if (o == NULL) {
      SWFDEC_WARNING ("target \"%s\" is not an object", target);
    } else if (!SWFDEC_IS_MOVIE (o)) {
      SWFDEC_FIXME ("target \"%s\" is not a movie, something weird is supposed to happen now", target);
      o = NULL;
    }
    swfdec_as_frame_set_target (cx->frame, o);
  }
}

static void
swfdec_action_set_target (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  char *end;

  end = memchr (data, 0, len);
  if (end == NULL) {
    SWFDEC_ERROR ("SetTarget action does not specify a string");
    return;
  }
  swfdec_action_do_set_target (cx, (const char *) data, end);
}

static void
swfdec_action_set_target2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;

  s = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));
  swfdec_action_do_set_target (cx, s, s + strlen (s));
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_start_drag (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecRect rect, *rectp = NULL;
  SwfdecMovie *movie;
  gboolean center;
  guint stack_size = 3;

  swfdec_as_stack_ensure_size (cx, 3);
  center = swfdec_as_value_to_boolean (cx, swfdec_as_stack_peek (cx, 2));
  if (swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 3))) {
    swfdec_as_stack_ensure_size (cx, 7);
    rect.x0 = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 7));
    rect.y0 = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 6));
    rect.x1 = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 5));
    rect.y1 = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 4));
    swfdec_rect_scale (&rect, &rect, SWFDEC_TWIPS_SCALE_FACTOR);
    stack_size = 7;
    rectp = &rect;
  }
  if (!SWFDEC_IS_PLAYER (cx)) {
    SWFDEC_ERROR ("called startDrag on a non-SwfdecPlayer");
  } else {
    movie = swfdec_player_get_movie_from_value (SWFDEC_PLAYER (cx), swfdec_as_stack_peek (cx, 1));
    if (SWFDEC_IS_ACTOR (movie)) {
      swfdec_player_set_drag_movie (SWFDEC_PLAYER (cx), SWFDEC_ACTOR (movie), center, rectp);
    } else {
      SWFDEC_ERROR ("startDrag on something not an Actor");
    }
  }
  swfdec_as_stack_pop_n (cx, stack_size);
}

static void
swfdec_action_end_drag (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (SWFDEC_IS_PLAYER (cx)) {
    swfdec_player_set_drag_movie (SWFDEC_PLAYER (cx), NULL, FALSE, NULL);
  } else {
    SWFDEC_WARNING ("can't end a drag on non-players");
  }
}

static void
swfdec_action_stop_sounds (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (SWFDEC_IS_PLAYER (cx)) {
    swfdec_player_stop_all_sounds (SWFDEC_PLAYER (cx));
  }
}

static void
swfdec_action_new_object (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *constructor;
  SwfdecAsFunction *fun;
  guint n_args;

  swfdec_as_stack_ensure_size (cx, 2);
  swfdec_action_get_variable (cx, action, data, len);
  constructor = swfdec_as_stack_peek (cx, 1);
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 2));
  n_args = MIN (swfdec_as_stack_get_size (cx) - 2, n_args);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (constructor) ||
      !SWFDEC_IS_AS_FUNCTION (fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (constructor))) {
    SWFDEC_WARNING ("not a constructor");
    goto fail;
  }

  swfdec_as_stack_pop_n (cx, 2);
  swfdec_as_object_create (fun, n_args, NULL, NULL);
  return;

fail:
  swfdec_as_stack_pop_n (cx, n_args + 1);
  SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx, 1));
}

static void
swfdec_action_new_method (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *constructor;
  SwfdecAsFunction *fun;
  guint n_args;
  const char *name;

  swfdec_as_stack_ensure_size (cx, 3);
  name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));

  constructor = swfdec_as_stack_peek (cx, 2);
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 3));
  n_args = MIN (swfdec_as_stack_get_size (cx) - 3, n_args);
  if (name == SWFDEC_AS_STR_EMPTY ||
      SWFDEC_AS_VALUE_IS_UNDEFINED (swfdec_as_stack_peek (cx, 1))) {
  } else {
    if (!SWFDEC_AS_VALUE_IS_OBJECT (constructor)) {
      SWFDEC_WARNING ("NewMethod called without an object to get variable %s from", name);
      goto fail;
    }
    swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (constructor),
	name, constructor);
  }
  if (!SWFDEC_AS_VALUE_IS_OBJECT (constructor) ||
      !SWFDEC_IS_AS_FUNCTION (fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (constructor))) {
    SWFDEC_WARNING ("%s is not a constructor", name);
    goto fail;
  }

  swfdec_as_stack_pop_n (cx, 3);
  swfdec_as_object_create (fun, n_args, NULL, NULL);
  return;

fail:
  swfdec_as_stack_pop_n (cx, n_args + 2);
  SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx, 1));
}

static void
swfdec_action_init_object (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsObject *object;
  guint i, n_args, size;

  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 1));
  swfdec_as_stack_pop (cx);
  if (n_args * 2 > swfdec_as_stack_get_size (cx)) {
    size = swfdec_as_stack_get_size (cx);
    SWFDEC_FIXME ("InitObject action with too small stack, help!");
    n_args = size / 2;
    size &= 1;
  } else {
    size = 0;
  }

  object = swfdec_as_object_new (cx);
  if (object == NULL)
    return;
  for (i = 0; i < n_args; i++) {
    const char *s = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 2));
    swfdec_as_object_set_variable (object, s, swfdec_as_stack_peek (cx, 1));
    swfdec_as_stack_pop_n (cx, 2);
  }
  swfdec_as_stack_pop_n (cx, size);
  SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_push (cx), object);
}

static void
swfdec_action_init_array (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  int i, n;
  SwfdecAsObject *array;

  swfdec_as_stack_ensure_size (cx, 1);
  n = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 1));
  swfdec_as_stack_pop (cx);
  array = swfdec_as_array_new (cx);
  if (array == NULL)
    return;
  /* NB: we can't increase the stack here, as the number can easily be MAXINT */
  for (i = 0; i < n && swfdec_as_stack_get_size (cx) > 0; i++) {
    swfdec_as_stack_ensure_size (cx, 1);
    swfdec_as_array_push (SWFDEC_AS_ARRAY (array), swfdec_as_stack_pop (cx));
  }
  if (i != n) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_INT (&val, n);
    swfdec_as_object_set_variable (array, SWFDEC_AS_STR_length, &val);
  }
  SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_push (cx), array);
}

static void
swfdec_action_define_function (SwfdecAsContext *cx, guint action,
    const guint8 *data, guint len)
{
  char *function_name;
  const char *name = NULL;
  guint i, n_args, size, n_registers;
  SwfdecBits bits;
  SwfdecBuffer *buffer;
  SwfdecAsFunction *fun;
  SwfdecAsFrame *frame;
  SwfdecScript *script;
  guint flags = 0;
  SwfdecScriptArgument *args;
  gboolean v2 = (action == 0x8e);

  frame = cx->frame;
  swfdec_bits_init_data (&bits, data, len);
  function_name = swfdec_bits_get_string (&bits, cx->version);
  if (function_name == NULL) {
    SWFDEC_ERROR ("could not parse function name");
    return;
  }
  n_args = swfdec_bits_get_u16 (&bits);
  if (v2) {
    n_registers = swfdec_bits_get_u8 (&bits);
    if (n_registers == 0)
      n_registers = 4;
    flags = swfdec_bits_get_u16 (&bits);
  } else {
    n_registers = 5;
  }
  if (n_args) {
    args = g_new0 (SwfdecScriptArgument, n_args);
    for (i = 0; i < n_args && swfdec_bits_left (&bits); i++) {
      if (v2) {
	args[i].preload = swfdec_bits_get_u8 (&bits);
	if (args[i].preload && args[i].preload >= n_registers) {
	  SWFDEC_ERROR ("argument %u cannot be preloaded into register %u out of %u", 
	      i, args[i].preload, n_registers);
	  /* FIXME: figure out correct error handling here */
	  args[i].preload = 0;
	}
      }
      args[i].name = swfdec_bits_get_string (&bits, cx->version);
      if (args[i].name == NULL || args[i].name == '\0') {
	SWFDEC_ERROR ("empty argument name not allowed");
	g_free (args);
	return;
      }
      /* FIXME: check duplicate arguments */
    }
  } else {
    args = NULL;
  }
  size = swfdec_bits_get_u16 (&bits);
  /* check the script can be created */
  if (frame->script->buffer->data + frame->script->buffer->length < frame->pc + 3 + len + size) {
    SWFDEC_ERROR ("size of function is too big");
    g_free (args);
    g_free (function_name);
    return;
  }
  /* create the script */
  buffer = swfdec_buffer_new_subbuffer (frame->script->buffer, 
      frame->pc + 3 + len - frame->script->buffer->data, size);
  swfdec_bits_init (&bits, buffer);
  if (*function_name) {
    name = function_name;
  } else if (swfdec_as_stack_get_size (cx) > 0) {
    /* This is kind of a hack that uses a feature of the Adobe compiler:
     * foo = function () {} is compiled as these actions:
     * Push "foo", DefineFunction, SetVariable/SetMember
     * With this knowledge we can inspect the topmost stack member, since
     * it will contain the name this function will soon be assigned to.
     */
    if (SWFDEC_AS_VALUE_IS_STRING (swfdec_as_stack_peek (cx, 1)))
      name = SWFDEC_AS_VALUE_GET_STRING (swfdec_as_stack_peek (cx, 1));
  }
  if (name == NULL)
    name = "unnamed_function";
  script = swfdec_script_new_from_bits (&bits, name, cx->version);
  swfdec_buffer_unref (buffer);
  if (script == NULL) {
    SWFDEC_ERROR ("failed to create script");
    g_free (args);
    g_free (function_name);
    return;
  }
  if (frame->constant_pool)
    script->constant_pool = swfdec_buffer_ref (swfdec_constant_pool_get_buffer (frame->constant_pool));
  script->flags = flags;
  script->n_registers = n_registers;
  script->n_arguments = n_args;
  script->arguments = args;
  /* see function-scope tests */
  if (cx->version > 5) {
    /* FIXME: or original target? */
    fun = swfdec_as_script_function_new (frame->original_target, frame->scope_chain, script);
  } else {
    fun = swfdec_as_script_function_new (frame->original_target, NULL, script);
  }
  if (fun == NULL)
    return;
  /* This is a hack that should only trigger for functions defined in the init scripts.
   * It is supposed to ensure that those functions inherit their target when being 
   * called instead of when being defined */
  if (!SWFDEC_IS_MOVIE (frame->original_target))
    SWFDEC_AS_SCRIPT_FUNCTION (fun)->target = NULL;
  /* attach the function */
  if (*function_name == '\0') {
    swfdec_as_stack_ensure_free (cx, 1);
    SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_push (cx), SWFDEC_AS_OBJECT (fun));
  } else {
    SwfdecAsValue funval;
    /* FIXME: really varobj? Not eval or sth like that? */
    name = swfdec_as_context_get_string (cx, function_name);
    SWFDEC_AS_VALUE_SET_OBJECT (&funval, SWFDEC_AS_OBJECT (fun));
    swfdec_as_object_set_variable (frame->target, name, &funval);
  }

  /* update current context */
  frame->pc += 3 + len + size;
  g_free (function_name);
}

static void
swfdec_action_bitwise (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  int a, b;

  a = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 1));
  b = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 2));

  switch (action) {
    case 0x60:
      a = (int) (a & b);
      break;
    case 0x61:
      a = (int) (a | b);
      break;
    case 0x62:
      a = (int) (a ^ b);
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  swfdec_as_stack_pop (cx);
  SWFDEC_AS_VALUE_SET_INT (swfdec_as_stack_peek (cx, 1), a);
}

static void
swfdec_action_shift (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  int amount, value;

  amount = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 1));
  amount &= 31;
  value = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 2));

  switch (action) {
    case 0x63:
      value = value << amount;
      break;
    case 0x64:
      value = ((gint) value) >> amount;
      break;
    case 0x65:
      value = ((guint) value) >> amount;
      break;
    default:
      g_assert_not_reached ();
  }

  swfdec_as_stack_pop (cx);
  SWFDEC_AS_VALUE_SET_INT (swfdec_as_stack_peek (cx, 1), value);
}

static void
swfdec_action_to_integer (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx, 1);

  SWFDEC_AS_VALUE_SET_INT (val, swfdec_as_value_to_integer (cx, val));
}

static void
swfdec_action_target_path (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  SwfdecMovie *movie;
  char *s;

  val = swfdec_as_stack_peek (cx, 1);

  if (!SWFDEC_AS_VALUE_IS_OBJECT (val) ||
      !SWFDEC_IS_MOVIE (movie = (SwfdecMovie *) SWFDEC_AS_VALUE_GET_OBJECT (val))) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (val);
    return;
  }
  s = swfdec_movie_get_path (movie, TRUE);
  SWFDEC_AS_VALUE_SET_STRING (val, swfdec_as_context_give_string (cx, s));
}

static void
swfdec_action_define_local (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *name;

  name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 2));
  swfdec_as_frame_set_variable (cx->frame, name, swfdec_as_stack_peek (cx, 1),
      TRUE, TRUE);
  swfdec_as_stack_pop_n (cx, 2);
}

static void
swfdec_action_define_local2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue val = { 0, };
  const char *name;

  name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));
  swfdec_as_frame_set_variable (cx->frame, name, &val, FALSE, TRUE);
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_return (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  swfdec_as_frame_return (cx->frame, swfdec_as_stack_pop (cx));
}

static void
swfdec_action_delete (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  const char *name;
  gboolean success = FALSE;
  
  name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1));
  val = swfdec_as_stack_peek (cx, 2);
  if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    success = swfdec_as_object_delete_variable (
	SWFDEC_AS_VALUE_GET_OBJECT (val), name) == SWFDEC_AS_DELETE_DELETED;
  }
  SWFDEC_AS_VALUE_SET_BOOLEAN (val, success);
  swfdec_as_stack_pop_n (cx, 1);
}

static void
swfdec_action_delete2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  const char *name;
  gboolean success = FALSE;
  
  val = swfdec_as_stack_peek (cx, 1);
  name = swfdec_as_value_to_string (cx, val);
  success = swfdec_as_frame_delete_variable (cx->frame, name) == SWFDEC_AS_DELETE_DELETED;
  SWFDEC_AS_VALUE_SET_BOOLEAN (val, success);
}

static void
swfdec_action_store_register (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (len != 1) {
    SWFDEC_ERROR ("StoreRegister action requires a length of 1, but got %u", len);
    return;
  }
  if (!swfdec_action_has_register (cx, *data)) {
    SWFDEC_ERROR ("Cannot store into register %u, not enough registers", (guint) *data);
    return;
  }
  cx->frame->registers[*data] = *swfdec_as_stack_peek (cx, 1);
}

static void
swfdec_action_modulo (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double x, y;

  y = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 1));
  x = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 2));
  /* yay, we're portable! */
  if (y == 0.0) {
    x = NAN;
  } else {
    errno = 0;
    x = fmod (x, y);
    if (errno != 0) {
      SWFDEC_FIXME ("errno set after fmod");
    }
  }
  swfdec_as_stack_pop (cx);
  SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx, 1), x);
}

static void
swfdec_action_swap (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  swfdec_as_stack_swap (cx, 1, 2);
}

static void
swfdec_action_to_number (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx, 1),
      swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 1)));
}

static void
swfdec_action_to_string (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx, 1),
      swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 1)));
}

static void
swfdec_action_type_of (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  const char *type;

  val = swfdec_as_stack_peek (cx, 1);
  switch (val->type) {
    case SWFDEC_AS_TYPE_NUMBER:
      type = SWFDEC_AS_STR_number;
      break;
    case SWFDEC_AS_TYPE_BOOLEAN:
      type = SWFDEC_AS_STR_boolean;
      break;
    case SWFDEC_AS_TYPE_STRING:
      type = SWFDEC_AS_STR_string;
      break;
    case SWFDEC_AS_TYPE_UNDEFINED:
      type = SWFDEC_AS_STR_undefined;
      break;
    case SWFDEC_AS_TYPE_NULL:
      type = SWFDEC_AS_STR_null;
      break;
    case SWFDEC_AS_TYPE_OBJECT:
      {
	SwfdecAsObject *obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
	if (SWFDEC_IS_MOVIE (obj)) {
	  if (SWFDEC_IS_TEXT_FIELD_MOVIE (obj)) {
	    type = SWFDEC_AS_STR_object;
	  } else {
	    type = SWFDEC_AS_STR_movieclip;
	  }
	} else if (SWFDEC_IS_AS_FUNCTION (obj)) {
	  type = SWFDEC_AS_STR_function;
	} else {
	  type = SWFDEC_AS_STR_object;
	}
      }
      break;
    case SWFDEC_AS_TYPE_INT:
    default:
      g_assert_not_reached ();
      type = SWFDEC_AS_STR_EMPTY;
      break;
  }
  SWFDEC_AS_VALUE_SET_STRING (val, type);
}

static void
swfdec_action_get_time (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  GTimeVal tv;
  gulong diff;

  swfdec_as_context_get_time (cx, &tv);
  /* we assume here that swfdec_as_context_get_time always returns a tv > start_time */
  diff = tv.tv_sec - cx->start_time.tv_sec;
  if (diff > G_MAXULONG / 1000 - 1) {
    SWFDEC_ERROR ("FIXME: time overflow");
  }
  diff *= 1000;
  diff = diff + (tv.tv_usec - cx->start_time.tv_usec) / 1000;

  SWFDEC_AS_VALUE_SET_INT (swfdec_as_stack_push (cx), diff);
}

static gboolean
swfdec_action_is_instance_of (SwfdecAsObject *object,
    SwfdecAsObject *constructor)
{
  SwfdecAsValue val;
  SwfdecAsObject *class, *prototype;
  GSList *iter;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (constructor), FALSE);

  // FIXME: propflag tests are wrong, and we shouldn't get __proto__.prototype
  swfdec_as_object_get_variable (constructor, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return FALSE;
  prototype = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  class = object;
  while ((class = swfdec_as_object_get_prototype (class)) != NULL) {
    if (class == prototype)
      return TRUE;
    for (iter = class->interfaces; iter != NULL; iter = iter->next) {
      if (iter->data == prototype)
	return TRUE;
    }
  }

  return FALSE;
}

static void
swfdec_action_instance_of (SwfdecAsContext *cx, guint action,
    const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  SwfdecAsObject *object, *constructor;

  val = swfdec_as_stack_pop (cx);
  if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    constructor = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    constructor = NULL;
  }

  val = swfdec_as_stack_pop (cx);
  if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    object = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    object = NULL;
  }


  if (object == NULL || constructor == NULL) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_push (cx), FALSE);
    return;
  }

  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_push (cx),
      swfdec_action_is_instance_of (object, constructor));
}

static void
swfdec_action_cast (SwfdecAsContext *cx, guint action, const guint8 *data,
    guint len)
{
  SwfdecAsValue *val;
  SwfdecAsObject *object, *constructor;

  val = swfdec_as_stack_pop (cx);
  if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    object = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    object = NULL;
  }
 
  val = swfdec_as_stack_pop (cx);
  if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    constructor = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    constructor = NULL;
  }

  if (object == NULL || constructor == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (swfdec_as_stack_push (cx));
    return;
  }

  if (swfdec_action_is_instance_of (object, constructor)) {
    SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_push (cx), object);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (swfdec_as_stack_push (cx));
  }
}

static void
swfdec_action_implements (SwfdecAsContext *cx, guint action,
    const guint8 *data, guint len)
{
  SwfdecAsValue *val, *argv;
  SwfdecAsObject *object, *proto, *interface;
  int argc, i;

  swfdec_as_stack_ensure_size (cx, 2);

  val = swfdec_as_stack_pop (cx);
  if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    object = SWFDEC_AS_VALUE_GET_OBJECT (val);
    swfdec_as_object_get_variable (object, SWFDEC_AS_STR_prototype, val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
      proto = SWFDEC_AS_VALUE_GET_OBJECT (val);
    } else {
      proto = NULL;
    }
  } else {
    object = NULL;
    proto = NULL;
  }

  val = swfdec_as_stack_pop (cx);
  argc = swfdec_as_value_to_integer (cx, val);

  if (argc > 0) {
    swfdec_as_stack_ensure_size (cx, argc);
    argv = swfdec_as_stack_pop_n (cx, argc);
  } else {
    argv = NULL;
  }

  if (proto == NULL)
    return;

  for (i = 0; i < argc; i++) {
    if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[i]))
      continue;
    interface = SWFDEC_AS_VALUE_GET_OBJECT (&argv[i]);
    swfdec_as_object_get_variable (interface, SWFDEC_AS_STR_prototype, val);
    if (!SWFDEC_AS_VALUE_IS_OBJECT (val))
      continue;
    proto->interfaces =
      g_slist_prepend (proto->interfaces, SWFDEC_AS_VALUE_GET_OBJECT (val));
  }
}

static void
swfdec_action_extends (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *superclass, *subclass, proto;
  SwfdecAsObject *prototype;
  SwfdecAsObject *super;

  superclass = swfdec_as_stack_peek (cx, 1);
  subclass = swfdec_as_stack_peek (cx, 2);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (superclass) ||
      !SWFDEC_IS_AS_FUNCTION (SWFDEC_AS_VALUE_GET_OBJECT (superclass))) {
    SWFDEC_ERROR ("superclass is not a function");
    goto fail;
  }
  if (!SWFDEC_AS_VALUE_IS_OBJECT (subclass)) {
    SWFDEC_ERROR ("subclass is not an object");
    goto fail;
  }
  super = SWFDEC_AS_VALUE_GET_OBJECT (superclass);
  prototype = swfdec_as_object_new_empty (cx);
  if (prototype == NULL)
    goto fail;
  swfdec_as_object_get_variable (super, SWFDEC_AS_STR_prototype, &proto);
  swfdec_as_object_set_variable_and_flags (prototype, SWFDEC_AS_STR___proto__, &proto,
      SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  swfdec_as_object_set_variable_and_flags (prototype, SWFDEC_AS_STR___constructor__,
      superclass, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_VERSION_6_UP);
  SWFDEC_AS_VALUE_SET_OBJECT (&proto, prototype);
  swfdec_as_object_set_variable (SWFDEC_AS_VALUE_GET_OBJECT (subclass),
      SWFDEC_AS_STR_prototype, &proto);
fail:
  swfdec_as_stack_pop_n (cx, 2);
}

static gboolean
swfdec_action_enumerate_foreach (SwfdecAsObject *object, const char *variable,
    SwfdecAsValue *value, guint flags, gpointer listp)
{
  GSList **list = listp;

  if (flags & SWFDEC_AS_VARIABLE_HIDDEN)
    return TRUE;

  *list = g_slist_remove (*list, variable);
  *list = g_slist_prepend (*list, (gpointer) variable);
  return TRUE;
}

static void
swfdec_action_do_enumerate (SwfdecAsContext *cx, SwfdecAsObject *object)
{
  guint i;
  GSList *walk, *list = NULL;
  
  for (i = 0; i < 256 && object; i++) {
    swfdec_as_object_foreach (object, swfdec_action_enumerate_foreach, &list);
    object = swfdec_as_object_get_prototype (object);
  }
  if (i == 256) {
    swfdec_as_context_abort (object->context, "Prototype recursion limit exceeded");
    g_slist_free (list);
    return;
  }
  list = g_slist_reverse (list);
  i = 0;
  for (walk = list; walk; walk = walk->next) {
    /* 8 is an arbitrary value */
    if (i % 8 == 0) {
      swfdec_as_stack_ensure_free (cx, 8);
      i = 0;
    }
    i++;
    SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_push (cx), walk->data);
  }
  g_slist_free (list);
}

static void
swfdec_action_enumerate2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  SwfdecAsObject *obj;

  val = swfdec_as_stack_peek (cx, 1);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    SWFDEC_WARNING ("Enumerate called without an object");
    SWFDEC_AS_VALUE_SET_UNDEFINED (val);
    return;
  }
  obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
  SWFDEC_AS_VALUE_SET_UNDEFINED (val);
  swfdec_action_do_enumerate (cx, obj);
}

static void
swfdec_action_enumerate (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  /* FIXME: make this proper functions */
  swfdec_action_get_variable (cx, action, data, len);
  swfdec_action_enumerate2 (cx, action, data, len);
}

static void
swfdec_action_logical (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  gboolean l, r;

  if (cx->version <= 4) {
    l = (swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 1)) != 0);
    // don't call second parameter if not necessary
    if ((action == SWFDEC_AS_ACTION_AND && !l) ||
	(action != SWFDEC_AS_ACTION_AND && l)) {
      r = (swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx, 2)) != 0);
    } else {
      r = FALSE;
    }
  } else {
    l = swfdec_as_value_to_boolean (cx, swfdec_as_stack_peek (cx, 1));
    r = swfdec_as_value_to_boolean (cx, swfdec_as_stack_peek (cx, 2));
  }

  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx, 2),
    (action == SWFDEC_AS_ACTION_AND) ? (l && r) : (l || r));
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_char_to_ascii (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx, 1);
  const char *s = swfdec_as_value_to_string (cx, val);

  if (cx->version <= 5) {
    char *ascii = g_convert (s, -1, "LATIN1", "UTF-8", NULL, NULL, NULL);

    if (ascii == NULL) {
      /* This can happen if a Flash 5 movie gets loaded into a Flash 7 movie */
      SWFDEC_FIXME ("Someone threw unconvertible text %s at Flash <= 5", s);
      SWFDEC_AS_VALUE_SET_INT (val, 0); /* FIXME: what to return??? */
    } else {
      SWFDEC_AS_VALUE_SET_INT (val, (guchar) ascii[0]);
      g_free (ascii);
    }
  } else {
    gunichar *uni = g_utf8_to_ucs4_fast (s, -1, NULL);

    if (uni == NULL) {
      /* This should never happen, everything is valid UTF-8 in here */
      g_warning ("conversion of character %s failed", s);
      SWFDEC_AS_VALUE_SET_INT (val, 0);
    } else {
      SWFDEC_AS_VALUE_SET_INT (val, uni[0]);
      g_free (uni);
    }
  }
}

static void
swfdec_action_ascii_to_char (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx, 1);

  if (cx->version <= 5) {
    char s[3];
    char *utf8;
    guint i;

    if (action == SWFDEC_AS_ACTION_ASCII_TO_CHAR) {
      s[0] = ((guint) swfdec_as_value_to_integer (cx, val)) % 256;
      s[1] = 0;
    } else {
      g_assert (action == SWFDEC_AS_ACTION_MB_ASCII_TO_CHAR);

      i = ((guint) swfdec_as_value_to_integer (cx, val));
      if (i > 255) {
	s[0] = i / 256;
	s[1] = i % 256;
	s[2] = 0;
      } else {
	s[0] = i;
	s[1] = 0;
      }
    }

    utf8 = g_convert (s, -1, "UTF-8", "LATIN1", NULL, NULL, NULL);
    if (utf8 == NULL) {
      g_warning ("conversion of character %u failed", (guint) s[0]);
      SWFDEC_AS_VALUE_SET_STRING (val, SWFDEC_AS_STR_EMPTY);
    } else {
      SWFDEC_AS_VALUE_SET_STRING (val, swfdec_as_context_get_string (cx, utf8));
      g_free (utf8);
    }
  } else {
    char *s;
    gunichar c = ((guint) swfdec_as_value_to_integer (cx, val)) % 65536;

    s = g_ucs4_to_utf8 (&c, 1, NULL, NULL, NULL);
    if (s == NULL) {
      g_warning ("conversion of character %u failed", (guint) c);
      SWFDEC_AS_VALUE_SET_STRING (val, SWFDEC_AS_STR_EMPTY);
    } else {
      SWFDEC_AS_VALUE_SET_STRING (val, swfdec_as_context_get_string (cx, s));
      g_free (s);
    }
  }
}

static void
swfdec_action_throw (SwfdecAsContext *cx, guint action, const guint8 *data,
    guint len)
{
  swfdec_as_context_throw (cx, swfdec_as_stack_pop (cx));
}

typedef struct {
  const guint8 *	catch_start;
  const guint8 *	finally_start;
  guint			catch_size;
  guint			finally_size;

  gboolean		use_register;
  union {
    guint		register_number;
    char *		variable_name;
  };

  SwfdecAsObject *	scope_object;
} TryData;

static void
swfdec_action_try_data_free (gpointer data)
{
  TryData *try_data = data;

  g_return_if_fail (try_data != NULL);

  if (!try_data->use_register)
    g_free (try_data->variable_name);
  g_free (try_data);
}

static void
swfdec_action_try_end_finally (SwfdecAsFrame *frame, gpointer data)
{
  SwfdecAsValue *exception_value = data;
  SwfdecAsContext *cx;

  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));
  g_return_if_fail (SWFDEC_IS_AS_VALUE (exception_value));

  cx = SWFDEC_AS_OBJECT (frame)->context;

  // finally has ended and we had exception stored, throw it
  if (!cx->exception)
    swfdec_as_context_throw (cx, exception_value);

  g_free (data);
}

static void
swfdec_action_try_end_catch (SwfdecAsFrame *frame, gpointer data)
{
  TryData *try_data = data;
  SwfdecAsContext *cx;
  SwfdecAsValue *exception_value, val;

  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));
  g_return_if_fail (try_data != NULL);

  cx = SWFDEC_AS_OBJECT (frame)->context;

  if (try_data->scope_object) {
    g_assert (frame->scope_chain->data == try_data->scope_object);
    frame->scope_chain =
      g_slist_delete_link (frame->scope_chain, frame->scope_chain);
    try_data->scope_object = NULL;
  }

  if (try_data->finally_start && swfdec_as_context_catch (cx, &val))
  {
    // we got an exception while in catch block:
    // create new block for finally to pass on the exception
    // jump to that block

    exception_value = g_malloc (sizeof (SwfdecAsValue));
    *exception_value = val;

    // FIXME: the exception value is not marked while finally block runs
    swfdec_as_frame_push_block (frame, try_data->finally_start,
	try_data->finally_start + try_data->finally_size,
	swfdec_action_try_end_finally, exception_value);
    frame->pc = try_data->finally_start;
  }

  swfdec_action_try_data_free (try_data);
}

static void
swfdec_action_try_end_try (SwfdecAsFrame *frame, gpointer data)
{
  TryData *try_data = data;
  SwfdecAsContext *cx;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_FRAME (frame));
  g_return_if_fail (try_data != NULL);

  // if we don't have a catch block, we handle try block exactly like it was
  // catch block
  if (!try_data->catch_start) {
    swfdec_action_try_end_catch (frame, try_data);
    return;
  }

  cx = SWFDEC_AS_OBJECT (frame)->context;

  if (swfdec_as_context_catch (cx, &val))
  {
    // we got an exception while in try block:
    // set the exception variable
    // add new block for catch and jump to it
    try_data->scope_object = swfdec_as_object_new_empty (cx);
    frame->scope_chain = g_slist_prepend (frame->scope_chain,
	try_data->scope_object);

    swfdec_as_frame_push_block (frame, try_data->catch_start,
	try_data->catch_start + try_data->catch_size,
	swfdec_action_try_end_catch, try_data);
    frame->pc = try_data->catch_start;

    if (try_data->use_register)
    {
      if (try_data->register_number < frame->n_registers) {
	frame->registers[try_data->register_number] = val;
      } else {
	SWFDEC_ERROR ("cannot set Error to register %u: not enough registers",
	    try_data->register_number);
      }
    }
    else
    {
      swfdec_as_object_set_variable (try_data->scope_object,
	  swfdec_as_context_get_string (cx, try_data->variable_name), &val);
    }
  }
  else
  {
    swfdec_action_try_data_free (try_data);
  }
}

static void
swfdec_action_try (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  TryData *try_data;
  guint try_size;
  gboolean use_finally, use_catch;

  if (len < 8) {
    SWFDEC_ERROR ("With action requires a length of at least 8, but got %u",
	len);
    return;
  }

  try_data = g_malloc0 (sizeof (TryData));

  swfdec_bits_init_data (&bits, data, len);

  swfdec_bits_getbits (&bits, 5); // reserved
  try_data->use_register = swfdec_bits_getbit (&bits);
  use_finally = swfdec_bits_getbit (&bits);
  use_catch = swfdec_bits_getbit (&bits);

  try_size = swfdec_bits_get_u16 (&bits);
  try_data->catch_size = swfdec_bits_get_u16 (&bits);
  try_data->finally_size = swfdec_bits_get_u16 (&bits);
  if (use_catch)
    try_data->catch_start = data + len + try_size;
  if (use_finally) {
    try_data->finally_start = data + len + try_size +
      (use_catch ? try_data->catch_size : 0);
  }

  if (try_data->use_register) {
    try_data->register_number = swfdec_bits_get_u8 (&bits);
  } else {
    try_data->variable_name =
      swfdec_bits_get_string (&bits, cx->version);
  }

  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("leftover bytes in Try action");
  }

  if (try_data->catch_start || try_data->finally_start) {
    swfdec_as_frame_push_block (cx->frame, data + len, data + len + try_size,
	swfdec_action_try_end_try, try_data);
  } else {
    SWFDEC_WARNING ("Try with neither catch nor finally block");
    swfdec_action_try_data_free (try_data);
  }
}

static void
swfdec_action_pop_with (SwfdecAsFrame *frame, gpointer with_object)
{
  g_assert (frame->scope_chain->data == with_object);
  frame->scope_chain = g_slist_delete_link (frame->scope_chain, frame->scope_chain);
}

static void
swfdec_action_with (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsObject *object;
  guint offset;

  if (len != 2) {
    SWFDEC_ERROR ("With action requires a length of 2, but got %u", len);
    swfdec_as_stack_pop (cx);
    return;
  }
  offset = data[0] | (data[1] << 8);
  object = swfdec_as_value_to_object (cx, swfdec_as_stack_peek (cx, 1));
  if (object == NULL) {
    SWFDEC_INFO ("With called without an object, skipping");
    cx->frame->pc = (guint8 *) data + len + offset;
  } else {
    cx->frame->scope_chain = g_slist_prepend (cx->frame->scope_chain, object);
    swfdec_as_frame_push_block (cx->frame, data + len, data + len + offset,
	swfdec_action_pop_with, object);
  }
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_remove_sprite (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (!SWFDEC_IS_MOVIE (cx->frame->target)) {
    SWFDEC_FIXME ("target is not a movie in RemoveSprite");
  } else if (!SWFDEC_IS_PLAYER (cx)) {
    SWFDEC_INFO ("tried using RemoveSprite in a non-SwfdecPlayer context");
  } else {
    SwfdecMovie *movie = swfdec_player_get_movie_from_value (SWFDEC_PLAYER (cx),
	swfdec_as_stack_peek (cx, 1));
    if (movie && swfdec_depth_classify (movie->depth) == SWFDEC_DEPTH_CLASS_DYNAMIC) {
      SWFDEC_LOG ("removing clip %s", movie->name);
      swfdec_movie_remove (movie);
    } else {
      SWFDEC_INFO ("cannot remove movie");
    }
  }
  swfdec_as_stack_pop (cx);
}

static void
swfdec_action_clone_sprite (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie, *new_movie;
  const char *new_name;
  int depth;

  depth = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx, 1)) - 16384;
  new_name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx, 2));
  if (!SWFDEC_IS_MOVIE (cx->frame->target)) {
    SWFDEC_FIXME ("target is not a movie in CloneSprite");
  } else if (!SWFDEC_IS_PLAYER (cx)) {
    SWFDEC_INFO ("tried using CloneSprite in a non-SwfdecPlayer context");
  } else {
    movie = swfdec_player_get_movie_from_value (SWFDEC_PLAYER (cx), 
	swfdec_as_stack_peek (cx, 3));
    if (movie == NULL) {
      SWFDEC_ERROR ("Object is not an SwfdecMovie object");
      swfdec_as_stack_pop_n (cx, 3);
      return;
    }
    new_movie = swfdec_movie_duplicate (movie, new_name, depth);
    if (new_movie) {
      SWFDEC_LOG ("duplicated %s as %s to depth %u", movie->name, new_movie->name, new_movie->depth);
    }
  }
  swfdec_as_stack_pop_n (cx, 3);
}

/*** PRINT FUNCTIONS ***/

static char *
swfdec_action_print_with (guint action, const guint8 *data, guint len)
{
  if (len != 2) {
    SWFDEC_ERROR ("With action requires a length of 2, but got %u", len);
    return NULL;
  }
  return g_strdup_printf ("With %u", data[0] | (data[1] << 8));
}

static char *
swfdec_action_print_store_register (guint action, const guint8 *data, guint len)
{
  if (len != 1) {
    SWFDEC_ERROR ("StoreRegister action requires a length of 1, but got %u", len);
    return NULL;
  }
  return g_strdup_printf ("StoreRegister %u", (guint) *data);
}

static char *
swfdec_action_print_set_target (guint action, const guint8 *data, guint len)
{
  if (!memchr (data, 0, len)) {
    SWFDEC_ERROR ("SetTarget action does not specify a string");
    return NULL;
  }
  return g_strconcat ("SetTarget ", data, NULL);
}

static char *
swfdec_action_print_define_function (guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  GString *string;
  const char *function_name;
  guint i, n_args, size;
  gboolean v2 = (action == 0x8e);

  string = g_string_new (v2 ? "DefineFunction2 " : "DefineFunction ");
  swfdec_bits_init_data (&bits, data, len);
  /* FIXME: version! */
  function_name = swfdec_bits_get_string (&bits, 7);
  if (function_name == NULL) {
    SWFDEC_ERROR ("could not parse function name");
    g_string_free (string, TRUE);
    return NULL;
  }
  if (*function_name) {
    g_string_append (string, function_name);
    g_string_append_c (string, ' ');
  }
  n_args = swfdec_bits_get_u16 (&bits);
  g_string_append_c (string, '(');
  if (v2) {
  /* n_regs = */ swfdec_bits_get_u8 (&bits);
  /* flags = */ swfdec_bits_get_u16 (&bits);
  }
 
  for (i = 0; i < n_args; i++) {
    guint preload;
    const char *arg_name;
    if (v2)
      preload = swfdec_bits_get_u8 (&bits);
    else
      preload = 0;
    arg_name = swfdec_bits_get_string (&bits, 7);
    if (preload == 0 && (arg_name == NULL || *arg_name == '\0')) {
      SWFDEC_ERROR ("empty argument name not allowed");
      g_string_free (string, TRUE);
      return NULL;
    }
    if (i)
      g_string_append (string, ", ");
    g_string_append (string, arg_name);
    if (preload)
      g_string_append_printf (string, " (%u)", preload);
  }
  g_string_append_c (string, ')');
  size = swfdec_bits_get_u16 (&bits);
  g_string_append_printf (string, " %u", size);
  return g_string_free (string, FALSE);
}

static char *
swfdec_action_print_get_url2 (guint action, const guint8 *data, guint len)
{
  guint method;

  if (len != 1) {
    SWFDEC_ERROR ("GetURL2 requires 1 byte of data, not %u", len);
    return NULL;
  }
  method = data[0] >> 6;
  if (method == 3) {
    SWFDEC_ERROR ("GetURL method 3 invalid");
    method = 0;
  }
  if (method) {
    SWFDEC_FIXME ("implement encoding variables using %s", method == 1 ? "GET" : "POST");
  }
  return g_strdup_printf ("GetURL2%s%s%s", method == 0 ? "" : (method == 1 ? " GET" : " POST"),
      data[0] & 2 ? " LoadTarget" : "", data[0] & 1 ? " LoadVariables" : "");
}

static char *
swfdec_action_print_get_url (guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  char *url, *target, *ret;

  swfdec_bits_init_data (&bits, data, len);
  url = swfdec_bits_get_string (&bits, 7);
  target = swfdec_bits_get_string (&bits, 7);
  if (url == NULL) {
    SWFDEC_ERROR ("not enough data in GetURL");
    url = g_strdup ("???");
  }
  if (target == NULL) {
    SWFDEC_ERROR ("not enough data in GetURL");
    target = g_strdup ("???");
  }
  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("leftover bytes in GetURL action");
  }
  ret = g_strdup_printf ("GetURL %s %s", url, target);
  g_free (url);
  g_free (target);
  return ret;
}

static char *
swfdec_action_print_if (guint action, const guint8 *data, guint len)
{
  gint16 offset;

  if (len != 2) {
    SWFDEC_ERROR ("If action length invalid (is %u, should be 2)", len);
    return NULL;
  }
  offset = data[0] | (data[1] << 8);
  return g_strdup_printf ("If %d", (int) offset);
}

static char *
swfdec_action_print_jump (guint action, const guint8 *data, guint len)
{
  gint16 offset;

  if (len != 2) {
    SWFDEC_ERROR ("If action length invalid (is %u, should be 2)", len);
    return NULL;
  }
  offset = data[0] | (data[1] << 8);
  return g_strdup_printf ("Jump %d", (int) offset);
}

static char *
swfdec_action_print_push (guint action, const guint8 *data, guint len)
{
  gboolean first = TRUE;
  SwfdecBits bits;
  GString *string = g_string_new ("Push");

  swfdec_bits_init_data (&bits, data, len);
  while (swfdec_bits_left (&bits)) {
    guint type = swfdec_bits_get_u8 (&bits);
    if (first)
      g_string_append (string, " ");
    else
      g_string_append (string, ", ");
    first = FALSE;
    switch (type) {
      case 0: /* string */
	{
	  /* FIXME: need version! */
	  char *s = swfdec_bits_get_string (&bits, 7);
	  if (!s) {
	    g_string_free (string, TRUE);
	    return NULL;
	  }
	  g_string_append_c (string, '"');
	  g_string_append (string, s);
	  g_string_append_c (string, '"');
	  g_free (s);
	  break;
	}
      case 1: /* float */
	g_string_append_printf (string, "%g", swfdec_bits_get_float (&bits));
	break;
      case 2: /* null */
	g_string_append (string, "null");
	break;
      case 3: /* undefined */
	g_string_append (string, "void");
	break;
      case 4: /* register */
	g_string_append_printf (string, "Register %u", swfdec_bits_get_u8 (&bits));
	break;
      case 5: /* boolean */
	g_string_append (string, swfdec_bits_get_u8 (&bits) ? "True" : "False");
	break;
      case 6: /* double */
	g_string_append_printf (string, "%g", swfdec_bits_get_double (&bits));
	break;
      case 7: /* 32bit int */
	g_string_append_printf (string, "%d", swfdec_bits_get_s32 (&bits));
	break;
      case 8: /* 8bit ConstantPool address */
	g_string_append_printf (string, "Pool %u", swfdec_bits_get_u8 (&bits));
	break;
      case 9: /* 16bit ConstantPool address */
	g_string_append_printf (string, "Pool %u", swfdec_bits_get_u16 (&bits));
	break;
      default:
	SWFDEC_ERROR ("Push: unknown type %u, skipping", type);
	break;
    }
  }
  return g_string_free (string, FALSE);
}

/* NB: constant pool actions are special in that they are called at init time */
static char *
swfdec_action_print_constant_pool (guint action, const guint8 *data, guint len)
{
  guint i;
  GString *string;
  SwfdecConstantPool *pool;
  SwfdecBuffer *buffer;

  /* FIXME: version */
  buffer = swfdec_buffer_new_static (data, len);
  pool = swfdec_constant_pool_new (NULL, buffer, 6);
  if (pool == NULL)
    return g_strdup ("ConstantPool (invalid)");
  string = g_string_new ("ConstantPool");
  for (i = 0; i < swfdec_constant_pool_size (pool); i++) {
    g_string_append (string, i ? ", " : " ");
    g_string_append (string, swfdec_constant_pool_get (pool, i));
    g_string_append_printf (string, " (%u)", i);
  }
  swfdec_constant_pool_unref (pool);
  swfdec_buffer_unref (buffer);
  return g_string_free (string, FALSE);
}

static char *
swfdec_action_print_wait_for_frame2 (guint action, const guint8 *data, guint len)
{
  if (len < 1) {
    SWFDEC_ERROR ("WaitForFrame2 needs a 1-byte data");
    return NULL;
  }
  return g_strdup_printf ("WaitForFrame2 %u", (guint) *data);
}

static char *
swfdec_action_print_goto_frame2 (guint action, const guint8 *data, guint len)
{
  gboolean play, bias;
  SwfdecBits bits;

  swfdec_bits_init_data (&bits, data, len);
  if (swfdec_bits_getbits (&bits, 6)) {
    SWFDEC_WARNING ("reserved bits in GotoFrame2 aren't 0");
  }
  bias = swfdec_bits_getbit (&bits);
  play = swfdec_bits_getbit (&bits);
  if (bias) {
    return g_strdup_printf ("GotoFrame2 %s +%u", play ? "play" : "stop",
	swfdec_bits_get_u16 (&bits));
  } else {
    return g_strdup_printf ("GotoFrame2 %s", play ? "play" : "stop");
  }
}

static char *
swfdec_action_print_goto_frame (guint action, const guint8 *data, guint len)
{
  guint frame;

  if (len != 2)
    return NULL;

  frame = data[0] | (data[1] << 8);
  return g_strdup_printf ("GotoFrame %u", frame);
}

static char *
swfdec_action_print_goto_label (guint action, const guint8 *data, guint len)
{
  if (!memchr (data, 0, len)) {
    SWFDEC_ERROR ("GotoLabel action does not specify a string");
    return NULL;
  }

  return g_strdup_printf ("GotoLabel %s", data);
}

static char *
swfdec_action_print_wait_for_frame (guint action, const guint8 *data, guint len)
{
  guint frame, jump;

  if (len != 3)
    return NULL;

  frame = data[0] | (data[1] << 8);
  jump = data[2];
  return g_strdup_printf ("WaitForFrame %u %u", frame, jump);
}

/*** BIG FUNCTION TABLE ***/

const SwfdecActionSpec swfdec_as_actions[256] = {
  /* version 1 */
  [SWFDEC_AS_ACTION_NEXT_FRAME] = { "NextFrame", NULL, 0, 0, swfdec_action_next_frame, 1 },
  [SWFDEC_AS_ACTION_PREVIOUS_FRAME] = { "PreviousFrame", NULL, 0, 0, swfdec_action_previous_frame, 1 },
  [SWFDEC_AS_ACTION_PLAY] = { "Play", NULL, 0, 0, swfdec_action_play, 1 },
  [SWFDEC_AS_ACTION_STOP] = { "Stop", NULL, 0, 0, swfdec_action_stop, 1 },
  [SWFDEC_AS_ACTION_TOGGLE_QUALITY] = { "ToggleQuality", NULL, -1, -1, NULL, 1 },
  /* version 2 */
  [SWFDEC_AS_ACTION_STOP_SOUNDS] = { "StopSounds", NULL, 0, 0, swfdec_action_stop_sounds, 2 },
  /* version 4 */
  [SWFDEC_AS_ACTION_ADD] = { "Add", NULL, 2, 1, swfdec_action_binary, 4 },
  [SWFDEC_AS_ACTION_SUBTRACT] = { "Subtract", NULL, 2, 1, swfdec_action_binary, 4 },
  [SWFDEC_AS_ACTION_MULTIPLY] = { "Multiply", NULL, 2, 1, swfdec_action_binary, 4 },
  [SWFDEC_AS_ACTION_DIVIDE] = { "Divide", NULL, 2, 1, swfdec_action_binary, 4 },
  [SWFDEC_AS_ACTION_EQUALS] = { "Equals", NULL, 2, 1, swfdec_action_old_compare, 4 },
  [SWFDEC_AS_ACTION_LESS] = { "Less", NULL, 2, 1, swfdec_action_old_compare, 4 },
  [SWFDEC_AS_ACTION_AND] = { "And", NULL, 2, 1, swfdec_action_logical, 4 },
  [SWFDEC_AS_ACTION_OR] = { "Or", NULL, 2, 1, swfdec_action_logical, 4 },
  [SWFDEC_AS_ACTION_NOT] = { "Not", NULL, 1, 1, swfdec_action_not, 4 },
  [SWFDEC_AS_ACTION_STRING_EQUALS] = { "StringEquals", NULL, 2, 1, swfdec_action_string_compare, 4 },
  [SWFDEC_AS_ACTION_STRING_LENGTH] = { "StringLength", NULL, 1, 1, swfdec_action_string_length, 4 },
  [SWFDEC_AS_ACTION_STRING_EXTRACT] = { "StringExtract", NULL, 3, 1, swfdec_action_string_extract, 4 },
  [SWFDEC_AS_ACTION_POP] = { "Pop", NULL, 1, 0, swfdec_action_pop, 4 },
  [SWFDEC_AS_ACTION_TO_INTEGER] = { "ToInteger", NULL, 1, 1, swfdec_action_to_integer, 4 },
  [SWFDEC_AS_ACTION_GET_VARIABLE] = { "GetVariable", NULL, 1, 1, swfdec_action_get_variable, 4 },
  [SWFDEC_AS_ACTION_SET_VARIABLE] = { "SetVariable", NULL, 2, 0, swfdec_action_set_variable, 4 },
  /* version 3 */
  [SWFDEC_AS_ACTION_SET_TARGET2] = { "SetTarget2", NULL, 1, 0, swfdec_action_set_target2, 3 },
  /* version 4 */
  [SWFDEC_AS_ACTION_STRING_ADD] = { "StringAdd", NULL, 2, 1, swfdec_action_string_add, 4 },
  [SWFDEC_AS_ACTION_GET_PROPERTY] = { "GetProperty", NULL, 2, 1, swfdec_action_get_property, 4 },
  [SWFDEC_AS_ACTION_SET_PROPERTY] = { "SetProperty", NULL, 3, 0, swfdec_action_set_property, 4 },
  [SWFDEC_AS_ACTION_CLONE_SPRITE] = { "CloneSprite", NULL, 3, 0, swfdec_action_clone_sprite, 4 },
  [SWFDEC_AS_ACTION_REMOVE_SPRITE] = { "RemoveSprite", NULL, 1, 0, swfdec_action_remove_sprite, 4 },
  [SWFDEC_AS_ACTION_TRACE] = { "Trace", NULL, 1, 0, swfdec_action_trace, 4 },
  [SWFDEC_AS_ACTION_START_DRAG] = { "StartDrag", NULL, -1, 0, swfdec_action_start_drag, 4 },
  [SWFDEC_AS_ACTION_END_DRAG] = { "EndDrag", NULL, 0, 0, swfdec_action_end_drag, 4 },
  [SWFDEC_AS_ACTION_STRING_LESS] = { "StringLess", NULL, 2, 1, swfdec_action_string_compare, 4 },
  /* version 7 */
  [SWFDEC_AS_ACTION_THROW] = { "Throw", NULL, 1, 0, swfdec_action_throw, 7 },
  [SWFDEC_AS_ACTION_CAST] = { "Cast", NULL, 2, 1, swfdec_action_cast, 7 },
  [SWFDEC_AS_ACTION_IMPLEMENTS] = { "Implements", NULL, -1, 0, swfdec_action_implements, 7 },
  /* version 4 */
  [SWFDEC_AS_ACTION_RANDOM] = { "RandomNumber", NULL, 1, 1, swfdec_action_random_number, 4 },
  [SWFDEC_AS_ACTION_MB_STRING_LENGTH] = { "MBStringLength", NULL, -1, -1, NULL, 4 },
  [SWFDEC_AS_ACTION_CHAR_TO_ASCII] = { "CharToAscii", NULL, 1, 1, swfdec_action_char_to_ascii, 4 },
  [SWFDEC_AS_ACTION_ASCII_TO_CHAR] = { "AsciiToChar", NULL, 1, 1, swfdec_action_ascii_to_char, 4 },
  [SWFDEC_AS_ACTION_GET_TIME] = { "GetTime", NULL, 0, 1, swfdec_action_get_time, 4 },
  [SWFDEC_AS_ACTION_MB_STRING_EXTRACT] = { "MBStringExtract", NULL, 3, 1, swfdec_action_string_extract, 4 },
  [SWFDEC_AS_ACTION_MB_CHAR_TO_ASCII] = { "MBCharToAscii", NULL, -1, -1, NULL, 4 },
  [SWFDEC_AS_ACTION_MB_ASCII_TO_CHAR] = { "MBAsciiToChar", NULL, 1, 1, swfdec_action_ascii_to_char, 4 },
  /* version 5 */
  [SWFDEC_AS_ACTION_DELETE] = { "Delete", NULL, 2, 1, swfdec_action_delete, 5 },
  [SWFDEC_AS_ACTION_DELETE2] = { "Delete2", NULL, 1, 1, swfdec_action_delete2, 5 },
  [SWFDEC_AS_ACTION_DEFINE_LOCAL] = { "DefineLocal", NULL, 2, 0, swfdec_action_define_local, 5 },
  [SWFDEC_AS_ACTION_CALL_FUNCTION] = { "CallFunction", NULL, -1, 1, swfdec_action_call_function, 5 },
  [SWFDEC_AS_ACTION_RETURN] = { "Return", NULL, 1, 0, swfdec_action_return, 5 },
  [SWFDEC_AS_ACTION_MODULO] = { "Modulo", NULL, 2, 1, swfdec_action_modulo, 5 },
  [SWFDEC_AS_ACTION_NEW_OBJECT] = { "NewObject", NULL, -1, 1, swfdec_action_new_object, 5 },
  [SWFDEC_AS_ACTION_DEFINE_LOCAL2] = { "DefineLocal2", NULL, 1, 0, swfdec_action_define_local2, 5 },
  [SWFDEC_AS_ACTION_INIT_ARRAY] = { "InitArray", NULL, -1, 1, swfdec_action_init_array, 5 },
  [SWFDEC_AS_ACTION_INIT_OBJECT] = { "InitObject", NULL, -1, 1, swfdec_action_init_object, 5 },
  [SWFDEC_AS_ACTION_TYPE_OF] = { "TypeOf", NULL, 1, 1, swfdec_action_type_of, 5 },
  [SWFDEC_AS_ACTION_TARGET_PATH] = { "TargetPath", NULL, 1, 1, swfdec_action_target_path, 5 },
  [SWFDEC_AS_ACTION_ENUMERATE] = { "Enumerate", NULL, 1, -1, swfdec_action_enumerate, 5 },
  [SWFDEC_AS_ACTION_ADD2] = { "Add2", NULL, 2, 1, swfdec_action_add2, 5 },
  [SWFDEC_AS_ACTION_LESS2] = { "Less2", NULL, 2, 1, swfdec_action_new_comparison, 5 },
  [SWFDEC_AS_ACTION_EQUALS2] = { "Equals2", NULL, 2, 1, swfdec_action_equals2, 5 },
  [SWFDEC_AS_ACTION_TO_NUMBER] = { "ToNumber", NULL, 1, 1, swfdec_action_to_number, 5 },
  [SWFDEC_AS_ACTION_TO_STRING] = { "ToString", NULL, 1, 1, swfdec_action_to_string, 5 },
  [SWFDEC_AS_ACTION_PUSH_DUPLICATE] = { "PushDuplicate", NULL, 1, 2, swfdec_action_push_duplicate, 5 },
  [SWFDEC_AS_ACTION_SWAP] = { "Swap", NULL, 2, 2, swfdec_action_swap, 5 },
  /* version 4 */
  [SWFDEC_AS_ACTION_GET_MEMBER] = { "GetMember", NULL, 2, 1, swfdec_action_get_member, 4 },
  [SWFDEC_AS_ACTION_SET_MEMBER] = { "SetMember", NULL, 3, 0, swfdec_action_set_member, 4 },
  /* version 5 */
  [SWFDEC_AS_ACTION_INCREMENT] = { "Increment", NULL, 1, 1, swfdec_action_increment, 5 },
  [SWFDEC_AS_ACTION_DECREMENT] = { "Decrement", NULL, 1, 1, swfdec_action_decrement, 5 },
  [SWFDEC_AS_ACTION_CALL_METHOD] = { "CallMethod", NULL, -1, 1, swfdec_action_call_method, 5 },
  [SWFDEC_AS_ACTION_NEW_METHOD] = { "NewMethod", NULL, -1, 1, swfdec_action_new_method, 5 },
  /* version 6 */
  [SWFDEC_AS_ACTION_INSTANCE_OF] = { "InstanceOf", NULL, 2, 1, swfdec_action_instance_of, 6 },
  [SWFDEC_AS_ACTION_ENUMERATE2] = { "Enumerate2", NULL, 1, -1, swfdec_action_enumerate2, 6 },
  [SWFDEC_AS_ACTION_BREAKPOINT] = { "Breakpoint", NULL, -1, -1, NULL, 6 },
  /* version 5 */
  [SWFDEC_AS_ACTION_BIT_AND] = { "BitAnd", NULL, 2, 1, swfdec_action_bitwise, 5 },
  [SWFDEC_AS_ACTION_BIT_OR] = { "BitOr", NULL, 2, 1, swfdec_action_bitwise, 5 },
  [SWFDEC_AS_ACTION_BIT_XOR] = { "BitXor", NULL, 2, 1, swfdec_action_bitwise, 5 },
  [SWFDEC_AS_ACTION_BIT_LSHIFT] = { "BitLShift", NULL, 2, 1, swfdec_action_shift, 5 },
  [SWFDEC_AS_ACTION_BIT_RSHIFT] = { "BitRShift", NULL, 2, 1, swfdec_action_shift, 5 },
  [SWFDEC_AS_ACTION_BIT_URSHIFT] = { "BitURShift", NULL, 2, 1, swfdec_action_shift, 5 },
  /* version 6 */
  [SWFDEC_AS_ACTION_STRICT_EQUALS] = { "StrictEquals", NULL, 2, 1, swfdec_action_strict_equals, 6 },
  [SWFDEC_AS_ACTION_GREATER] = { "Greater", NULL, 2, 1, swfdec_action_new_comparison, 6 },
  [SWFDEC_AS_ACTION_STRING_GREATER] = { "StringGreater", NULL, -1, -1, NULL, 6 },
  /* version 7 */
  [SWFDEC_AS_ACTION_EXTENDS] = { "Extends", NULL, 2, 0, swfdec_action_extends, 7 },
  /* version 1 */
  [SWFDEC_AS_ACTION_GOTO_FRAME] = { "GotoFrame", swfdec_action_print_goto_frame, 0, 0, swfdec_action_goto_frame, 1 },
  [SWFDEC_AS_ACTION_GET_URL] = { "GetURL", swfdec_action_print_get_url, 0, 0, swfdec_action_get_url, 1 },
  /* version 5 */
  [SWFDEC_AS_ACTION_STORE_REGISTER] = { "StoreRegister", swfdec_action_print_store_register, 1, 1, swfdec_action_store_register, 5 },
  [SWFDEC_AS_ACTION_CONSTANT_POOL] = { "ConstantPool", swfdec_action_print_constant_pool, 0, 0, swfdec_action_constant_pool, 5 },
  [SWFDEC_AS_ACTION_STRICT_MODE] = { "StrictMode", NULL, -1, -1, NULL, 5 },
  /* version 1 */
  [SWFDEC_AS_ACTION_WAIT_FOR_FRAME] = { "WaitForFrame", swfdec_action_print_wait_for_frame, 0, 0, swfdec_action_wait_for_frame, 1 },
  [SWFDEC_AS_ACTION_SET_TARGET] = { "SetTarget", swfdec_action_print_set_target, 0, 0, swfdec_action_set_target, 1 },
  /* version 3 */
  [SWFDEC_AS_ACTION_GOTO_LABEL] = { "GotoLabel", swfdec_action_print_goto_label, 0, 0, swfdec_action_goto_label, 3 },
  /* version 4 */
  [SWFDEC_AS_ACTION_WAIT_FOR_FRAME2] = { "WaitForFrame2", swfdec_action_print_wait_for_frame2, 1, 0, swfdec_action_wait_for_frame2, 4 },
  /* version 7 */
  [SWFDEC_AS_ACTION_DEFINE_FUNCTION2] = { "DefineFunction2", swfdec_action_print_define_function, 0, -1, swfdec_action_define_function, 7 },
  [SWFDEC_AS_ACTION_TRY] = { "Try", NULL, 0, 0, swfdec_action_try, 7 },
  /* version 5 */
  [SWFDEC_AS_ACTION_WITH] = { "With", swfdec_action_print_with, 1, 0, swfdec_action_with, 5 },
  /* version 4 */
  [SWFDEC_AS_ACTION_PUSH] = { "Push", swfdec_action_print_push, 0, -1, swfdec_action_push, 4 },
  [SWFDEC_AS_ACTION_JUMP] = { "Jump", swfdec_action_print_jump, 0, 0, swfdec_action_jump, 4 },
  [SWFDEC_AS_ACTION_GET_URL2] = { "GetURL2", swfdec_action_print_get_url2, 2, 0, swfdec_action_get_url2, 4 },
  /* version 5 */
  [SWFDEC_AS_ACTION_DEFINE_FUNCTION] = { "DefineFunction", swfdec_action_print_define_function, 0, -1, swfdec_action_define_function, 5 },
  /* version 4 */
  [SWFDEC_AS_ACTION_IF] = { "If", swfdec_action_print_if, 1, 0, swfdec_action_if, 4 },
  [SWFDEC_AS_ACTION_CALL] = { "Call", NULL, -1, -1, NULL, 4 },
  [SWFDEC_AS_ACTION_GOTO_FRAME2] = { "GotoFrame2", swfdec_action_print_goto_frame2, 1, 0, swfdec_action_goto_frame2, 4 }
};

