/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_as_frame.h"
#include "swfdec_as_function.h"
#include "swfdec_as_script_function.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_super.h"
#include "swfdec_as_with.h"
#include "swfdec_debug.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include "swfdec_decoder.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "swfdec_swf_instance.h"

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
  frame = GUINT16_FROM_LE (*((guint16 *) data));
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
  val = swfdec_as_stack_peek (cx->frame->stack, 1);
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
  swfdec_as_stack_pop (cx->frame->stack);
}

static void
swfdec_script_skip_actions (SwfdecAsContext *cx, guint jump)
{
  SwfdecScript *script = cx->frame->script;
  guint8 *pc = cx->frame->pc;
  guint8 *endpc = script->buffer->data + script->buffer->length;

  /* jump instructions */
  do {
    if (pc >= endpc)
      break;
    if (*pc & 0x80) {
      if (pc + 2 >= endpc)
	break;
      pc += 3 + GUINT16_FROM_LE (*((guint16 *) (pc + 1)));
    } else {
      pc++;
    }
  } while (jump-- > 0);
  cx->frame->pc = pc;
}

#if 0
static void
swfdec_action_wait_for_frame2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  jsval val;

  if (len != 1) {
    SWFDEC_ERROR ("WaitForFrame2 needs a 1-byte data");
    return JS_FALSE;
  }
  val = cx->fp->sp[-1];
  cx->fp->sp--;
  if (SWFDEC_IS_SPRITE_MOVIE (cx->frame->target))
    SwfdecMovie *movie = SWFDEC_MOVIE (cx->frame->target);
    int frame = swfdec_value_to_frame (cx, movie, val);
    guint jump = data[2];
    guint loaded;
    if (frame < 0)
      return JS_TRUE;
    if (SWFDEC_IS_ROOT_MOVIE (movie)) {
      SwfdecDecoder *dec = SWFDEC_ROOT_MOVIE (movie)->decoder;
      loaded = dec->frames_loaded;
      g_assert (loaded <= movie->n_frames);
    } else {
      loaded = movie->n_frames;
    }
    if (loaded <= (guint) frame)
      swfdec_script_skip_actions (cx, jump);
  } else {
    SWFDEC_ERROR ("no movie to WaitForFrame2 on");
  }
  return JS_TRUE;
}
#endif

static void
swfdec_action_wait_for_frame (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecSpriteMovie *movie;
  guint frame, jump, loaded;

  if (len != 3) {
    SWFDEC_ERROR ("WaitForFrame action length invalid (is %u, should be 3", len);
    return;
  }
  if (!SWFDEC_IS_SPRITE_MOVIE (cx->frame->target)) {
    SWFDEC_ERROR ("no movie for WaitForFrame");
    return;
  }

  movie = SWFDEC_SPRITE_MOVIE (cx->frame->target);
  frame = GUINT16_FROM_LE (*((guint16 *) data));
  jump = data[2];
  if (SWFDEC_MOVIE (movie)->swf->movie == movie) {
    SwfdecDecoder *dec = SWFDEC_MOVIE (movie)->swf->decoder;
    loaded = dec->frames_loaded;
    g_assert (loaded <= movie->n_frames);
  } else {
    loaded = movie->n_frames;
  }
  if (loaded <= frame)
    swfdec_script_skip_actions (cx, jump);
}

static void
swfdec_action_constant_pool (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecConstantPool *pool;
  SwfdecAsFrame *frame;

  frame = cx->frame;
  pool = swfdec_constant_pool_new_from_action (data, len, cx->version);
  if (pool == NULL)
    return;
  swfdec_constant_pool_attach_to_context (pool, cx);
  if (frame->constant_pool)
    swfdec_constant_pool_free (frame->constant_pool);
  frame->constant_pool = pool;
  if (frame->constant_pool_buffer)
    swfdec_buffer_unref (frame->constant_pool_buffer);
  frame->constant_pool_buffer = swfdec_buffer_new_subbuffer (frame->script->buffer,
      data - frame->script->buffer->data, len);
}

static void
swfdec_action_push (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsStack *stack = cx->frame->stack;
  SwfdecBits bits;

  swfdec_bits_init_data (&bits, data, len);
  while (swfdec_bits_left (&bits)) {
    guint type = swfdec_bits_get_u8 (&bits);
    SWFDEC_LOG ("push type %u", type);
    swfdec_as_stack_ensure_free (stack, 1);
    switch (type) {
      case 0: /* string */
	{
	  char *s = swfdec_bits_get_string_with_version (&bits, cx->version);
	  if (s == NULL)
	    return;
	  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_push (stack), 
	      swfdec_as_context_get_string (cx, s));
	  break;
	}
      case 1: /* float */
	SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_push (stack), 
	    swfdec_bits_get_float (&bits));
	break;
      case 2: /* null */
	SWFDEC_AS_VALUE_SET_NULL (swfdec_as_stack_push (stack));
	break;
      case 3: /* undefined */
	SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_push (stack));
	break;
      case 4: /* register */
	{
	  guint regnum = swfdec_bits_get_u8 (&bits);
	  if (!swfdec_action_has_register (cx, regnum)) {
	    SWFDEC_ERROR ("cannot Push register %u: not enough registers", regnum);
	    return;
	  }
	  *swfdec_as_stack_push (stack) = cx->frame->registers[regnum];
	  break;
	}
      case 5: /* boolean */
	SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_push (stack), 
	    swfdec_bits_get_u8 (&bits) ? TRUE : FALSE);
	break;
      case 6: /* double */
	SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_push (stack), 
	    swfdec_bits_get_double (&bits));
	break;
      case 7: /* 32bit int */
	SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_push (stack), 
	    (int) swfdec_bits_get_u32 (&bits));
	break;
      case 8: /* 8bit ConstantPool address */
	{
	  guint i = swfdec_bits_get_u8 (&bits);
	  SwfdecConstantPool *pool = cx->frame->constant_pool;
	  if (pool == NULL) {
	    SWFDEC_ERROR ("no constant pool to push from");
	    return;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    SWFDEC_ERROR ("constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    return;
	  }
	  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_push (stack), 
	      swfdec_constant_pool_get (pool, i));
	  break;
	}
      case 9: /* 16bit ConstantPool address */
	{
	  guint i = swfdec_bits_get_u16 (&bits);
	  SwfdecConstantPool *pool = cx->frame->constant_pool;
	  if (pool == NULL) {
	    SWFDEC_ERROR ("no constant pool to push from");
	    return;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    SWFDEC_ERROR ("constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    return;
	  }
	  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_push (stack), 
	      swfdec_constant_pool_get (pool, i));
	  break;
	}
      default:
	SWFDEC_ERROR ("Push: type %u not implemented", type);
	return;
    }
  }
}

static void
swfdec_action_get_variable (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;

  s = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  swfdec_as_context_eval (cx, NULL, s, swfdec_as_stack_peek (cx->frame->stack, 1));
#ifdef SWFDEC_WARN_MISSING_PROPERTIES
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 1))) {
    SWFDEC_WARNING ("no variable named %s", s);
  }
#endif
}

static void
swfdec_action_set_variable (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;

  s = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
  swfdec_as_context_eval_set (cx, NULL, s, swfdec_as_stack_peek (cx->frame->stack, 1));
  swfdec_as_stack_pop_n (cx->frame->stack, 2);
}

static const char *
swfdec_as_interpret_eval (SwfdecAsContext *cx, SwfdecAsObject *obj, 
    SwfdecAsValue *val)
{
  if (SWFDEC_AS_VALUE_IS_STRING (val)) {
    const char *s = SWFDEC_AS_VALUE_GET_STRING (val);
    if (s != SWFDEC_AS_STR_EMPTY) {
      swfdec_as_context_eval (cx, obj, s, val);
      return s;
    }
  } 
  if (obj != NULL)
    SWFDEC_AS_VALUE_SET_OBJECT (val, obj);
  else
    SWFDEC_AS_VALUE_SET_UNDEFINED (val);
  return SWFDEC_AS_STR_EMPTY;
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
  SwfdecAsValue *val;
  SwfdecAsObject *obj;
  guint id;

  id = swfdec_as_value_to_integer (cx, swfdec_as_stack_pop (cx->frame->stack));
  if (id > (cx->version > 4 ? 21 : 18)) {
    SWFDEC_WARNING ("trying to SetProperty %u, not allowed", id);
    goto out;
  }
  val = swfdec_as_stack_peek (cx->frame->stack, 1);
  swfdec_as_interpret_eval (cx, NULL, val);
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (val)) {
    obj = cx->frame->target;
  } else if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    SWFDEC_WARNING ("not an object, can't GetProperty");
    goto out;
  }
  swfdec_as_object_get_variable (obj, swfdec_movieclip_props[id].name,
      swfdec_as_stack_peek (cx->frame->stack, 1));
  return;

out:
  SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 1));
}

static void
swfdec_action_set_property (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  SwfdecAsObject *obj;
  guint id;

  id = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
  if (id > (cx->version > 4 ? 21 : 18)) {
    SWFDEC_WARNING ("trying to SetProperty %u, not allowed", id);
    goto out;
  }
  val = swfdec_as_stack_peek (cx->frame->stack, 3);
  swfdec_as_interpret_eval (cx, NULL, val);
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (val)) {
    obj = cx->frame->target;
  } else if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    SWFDEC_WARNING ("not an object, can't get SetProperty");
    goto out;
  }
  swfdec_as_object_set_variable (obj, swfdec_movieclip_props[id].name,
      swfdec_as_stack_peek (cx->frame->stack, 1));
out:
  swfdec_as_stack_pop_n (cx->frame->stack, 3);
}

static void
swfdec_action_get_member (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsObject *object = swfdec_as_value_to_object (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
  if (object) {
    const char *name;
    name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
    swfdec_as_object_get_variable (object, name, swfdec_as_stack_peek (cx->frame->stack, 2));
#ifdef SWFDEC_WARN_MISSING_PROPERTIES
    if (SWFDEC_AS_VALUE_IS_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 2))) {
	SWFDEC_WARNING ("no variable named %s:%s", G_OBJECT_TYPE_NAME (object), name);
    }
#endif
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 2));
  }
  swfdec_as_stack_pop (cx->frame->stack);
}

static void
swfdec_action_set_member (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (cx->frame->stack, 3))) {
    const char *name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
    swfdec_as_object_set_variable (SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (cx->frame->stack, 3)),
	name, swfdec_as_stack_peek (cx->frame->stack, 1));
  }
  swfdec_as_stack_pop_n (cx->frame->stack, 3);
}

static void
swfdec_action_trace (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  const char *s;

  val = swfdec_as_stack_pop (cx->frame->stack);
  if (val->type == SWFDEC_AS_TYPE_UNDEFINED)
    s = SWFDEC_AS_STR_undefined;
  else
    s = swfdec_as_value_to_string (cx, val);
  g_signal_emit_by_name (cx, "trace", s);
}

/* stack looks like this: [ function, this, arg1, arg2, ... ] */
/* stack must be at least 2 elements big */
static gboolean
swfdec_action_call (SwfdecAsContext *cx, guint n_args)
{
  SwfdecAsFunction *fun;
  SwfdecAsObject *thisp;
  SwfdecAsFrame *frame = cx->frame;
  guint i;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (frame->stack, 1)))
    goto error;
  fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (frame->stack, 1));
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    goto error;
  if (!SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (frame->stack, 2))) {
    thisp = NULL;
  } else {
    thisp = SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (frame->stack, 2));
  }
  swfdec_as_stack_pop_n (frame->stack, 2);
  /* sanitize argument count */
  if (n_args > swfdec_as_stack_get_size (frame->stack))
    n_args = swfdec_as_stack_get_size (frame->stack);
  /* push return value on stack */
  swfdec_as_stack_push (frame->stack);
  /* swap arguments and return value on the stack */
  /* FIXME: can we somehow keep this order please, it might be interesting for debuggers */
  for (i = 0; i < (n_args + 1) / 2; i++) {
    swfdec_as_stack_swap (frame->stack, i + 1, n_args + 1 - i);
  }
  if (n_args)
    swfdec_as_stack_pop_n (frame->stack, n_args);
  swfdec_as_function_call (fun, thisp, n_args, swfdec_as_stack_peek (frame->stack, 0), 
      swfdec_as_stack_peek (frame->stack, 1));
  if (SWFDEC_IS_AS_SUPER (fun)) {
    SWFDEC_LOG ("replacing super object on frame");
    swfdec_as_super_replace (SWFDEC_AS_SUPER (fun), NULL);
  }
  return TRUE;

error:
  n_args += 2;
  if (n_args > swfdec_as_stack_get_size (frame->stack))
    n_args = swfdec_as_stack_get_size (frame->stack);
  swfdec_as_stack_pop_n (frame->stack, n_args);
  SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_push (frame->stack));
  return FALSE;
}

static void
swfdec_action_call_function (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsFrame *frame = cx->frame;
  SwfdecAsObject *obj;
  guint n_args;
  const char *name;
  SwfdecAsValue *fun, *thisp;
  
  swfdec_as_stack_ensure_size (frame->stack, 2);
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (frame->stack, 2));
  name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (frame->stack, 1));
  thisp = swfdec_as_stack_peek (frame->stack, 2);
  fun = swfdec_as_stack_peek (frame->stack, 1);
  obj = swfdec_as_frame_find_variable (frame, name);
  if (obj) {
    SWFDEC_AS_VALUE_SET_OBJECT (thisp, obj);
    swfdec_as_object_get_variable (obj, name, fun);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (thisp);
    SWFDEC_AS_VALUE_SET_UNDEFINED (fun);
  }
  if (!swfdec_action_call (cx, n_args)) {
    SWFDEC_ERROR ("no function named %s", name);
  }
}

static void
swfdec_action_call_method (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsFrame *frame = cx->frame;
  SwfdecAsValue *val;
  SwfdecAsObject *obj;
  guint n_args;
  const char *name = NULL;
  
  swfdec_as_stack_ensure_size (frame->stack, 3);
  obj = swfdec_as_value_to_object (cx, swfdec_as_stack_peek (frame->stack, 2));
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (frame->stack, 3));
  val = swfdec_as_stack_peek (frame->stack, 1);
  /* FIXME: this is a hack for constructors calling super - is this correct? */
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (val)) {
    SWFDEC_AS_VALUE_SET_STRING (val, SWFDEC_AS_STR_EMPTY);
  }
  if (obj) {
    if (SWFDEC_AS_VALUE_IS_STRING (val) && 
	SWFDEC_AS_VALUE_GET_STRING (val) == SWFDEC_AS_STR_EMPTY) {
      SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (frame->stack, 3));
      SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (frame->stack, 2), obj);
    } else {
      SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (frame->stack, 3), obj);
      name = swfdec_as_value_to_string (cx, val);
      swfdec_as_object_get_variable (obj, name, swfdec_as_stack_peek (frame->stack, 2));
    }
  } else {
    if (SWFDEC_AS_VALUE_IS_STRING (val))
      name = SWFDEC_AS_VALUE_GET_STRING (val);
    SWFDEC_AS_VALUE_SET_NULL (swfdec_as_stack_peek (frame->stack, 3));
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (frame->stack, 2));
  }
  swfdec_as_stack_pop (frame->stack);
  if (swfdec_action_call (cx, n_args)) {
    /* setup super to point to the right prototype */
    frame = cx->frame;
    if (SWFDEC_IS_AS_SUPER (obj)) {
      swfdec_as_super_replace (SWFDEC_AS_SUPER (obj), name);
    } else if (frame->super) {
      SwfdecAsSuper *super = SWFDEC_AS_SUPER (frame->super);
      if (name && 
	  cx->version > 6 && 
	  swfdec_as_object_get_variable_and_flags (frame->thisp, 
	    name, NULL, NULL, &super->object) && 
	  super->object == frame->thisp) {
	super->object = super->object->prototype;
      }
    }
  } else {
    SWFDEC_ERROR ("no function named %s on object %s", name ? name : "unknown", obj ? G_OBJECT_TYPE_NAME(obj) : "unknown");
  }
}

static void
swfdec_action_pop (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  swfdec_as_stack_pop (cx->frame->stack);
}

static void
swfdec_action_binary (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double l, r;

  r = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  l = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
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
	  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx->frame->stack, 1), SWFDEC_AS_STR__ERROR_);
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
  swfdec_as_stack_pop (cx->frame->stack);
  SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1), l);
}

static void
swfdec_action_add2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *rval, *lval, rtmp, ltmp;

  rval = swfdec_as_stack_peek (cx->frame->stack, 1);
  lval = swfdec_as_stack_peek (cx->frame->stack, 2);
  rtmp = *rval;
  ltmp = *lval;
  swfdec_as_value_to_primitive (&rtmp);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&rtmp) || SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (&rtmp)))
    rval = &rtmp;
  swfdec_as_value_to_primitive (&ltmp);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&ltmp) || SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (&ltmp)))
    lval = &ltmp;

  if (SWFDEC_AS_VALUE_IS_STRING (lval) || SWFDEC_AS_VALUE_IS_STRING (rval)) {
    const char *lstr, *rstr;
    lstr = swfdec_as_value_to_string (cx, lval);
    rstr = swfdec_as_value_to_string (cx, rval);
    lstr = swfdec_as_str_concat (cx, lstr, rstr);
    swfdec_as_stack_pop (cx->frame->stack);
    SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx->frame->stack, 1), lstr);
  } else {
    double d, d2;
    d = swfdec_as_value_to_number (cx, lval);
    d2 = swfdec_as_value_to_number (cx, rval);
    d += d2;
    swfdec_as_stack_pop (cx->frame->stack);
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1), d);
  }
}

static void
swfdec_action_new_comparison (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *lval, *rval;
  double l, r;

  rval = swfdec_as_stack_pop (cx->frame->stack);
  lval = swfdec_as_stack_peek (cx->frame->stack, 1);

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
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), FALSE);
    return;
  }
  /* same for the rval */
  swfdec_as_value_to_primitive (rval);
  if (SWFDEC_AS_VALUE_IS_OBJECT (rval) &&
      !SWFDEC_IS_MOVIE (SWFDEC_AS_VALUE_GET_OBJECT (rval))) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), FALSE);
    return;
  }
  /* movieclips are not objects, but they evaluate to NaN, so we can handle them here */
  if (SWFDEC_AS_VALUE_IS_OBJECT (rval) ||
      SWFDEC_AS_VALUE_IS_OBJECT (lval)) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 1));
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
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), cmp < 0);
    return;
  }
  /* convert to numbers and compare those */
  l = swfdec_as_value_to_number (cx, lval);
  r = swfdec_as_value_to_number (cx, rval);
  /* NaN results in undefined */
  if (isnan (l) || isnan (r)) {
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 1));
    return;
  }
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), l < r);
}

static void
swfdec_action_not_4 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double d;

  d = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1), d == 0 ? 1 : 0);
}

static void
swfdec_action_not_5 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), 
      !swfdec_as_value_to_boolean (cx, swfdec_as_stack_peek (cx->frame->stack, 1)));
}

static void
swfdec_action_jump (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (len != 2) {
    SWFDEC_ERROR ("Jump action length invalid (is %u, should be 2", len);
    return;
  }
  cx->frame->pc += 5 + GINT16_FROM_LE (*((gint16*) data)); 
}

static void
swfdec_action_if (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (len != 2) {
    SWFDEC_ERROR ("Jump action length invalid (is %u, should be 2", len);
    return;
  }
  if (swfdec_as_value_to_boolean (cx, swfdec_as_stack_pop (cx->frame->stack)))
    cx->frame->pc += 5 + GINT16_FROM_LE (*((gint16*) data)); 
}

static void
swfdec_action_decrement (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;

  val = swfdec_as_stack_peek (cx->frame->stack, 1);
  SWFDEC_AS_VALUE_SET_NUMBER (val, swfdec_as_value_to_number (cx, val) - 1);
}

static void
swfdec_action_increment (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;

  val = swfdec_as_stack_peek (cx->frame->stack, 1);
  SWFDEC_AS_VALUE_SET_NUMBER (val, swfdec_as_value_to_number (cx, val) + 1);
}

static void
swfdec_action_get_url (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  char *url, *target;

  swfdec_bits_init_data (&bits, data, len);
  url = swfdec_bits_get_string_with_version (&bits, cx->version);
  target = swfdec_bits_get_string_with_version (&bits, cx->version);
  if (url == NULL || target == NULL) {
    SWFDEC_ERROR ("not enough data in GetURL");
    g_free (url);
    g_free (target);
    return;
  }
  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("leftover bytes in GetURL action");
  }
  if (SWFDEC_IS_MOVIE (cx->frame->target))
    swfdec_movie_load (SWFDEC_MOVIE (cx->frame->target), url, target);
  else
    SWFDEC_WARNING ("no movie to load");
  g_free (url);
  g_free (target);
}

static void
swfdec_action_get_url2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *target, *url;
  guint method;

  if (len != 1) {
    SWFDEC_ERROR ("GetURL2 requires 1 byte of data, not %u", len);
    return;
  }
  target = swfdec_as_value_to_string (cx, swfdec_as_stack_pop (cx->frame->stack));
  url = swfdec_as_value_to_string (cx, swfdec_as_stack_pop (cx->frame->stack));
  method = data[0] >> 6;
  if (method == 3) {
    SWFDEC_ERROR ("GetURL method 3 invalid");
    method = 0;
  }
  if (method) {
    SWFDEC_FIXME ("implement encoding variables using %s", method == 1 ? "GET" : "POST");
  }
  if (data[0] & 2) {
    SWFDEC_FIXME ("implement LoadTarget");
  }
  if (data[0] & 1) {
    SWFDEC_FIXME ("implement LoadVariables");
  }
  if (SWFDEC_IS_MOVIE (cx->frame->target))
    swfdec_movie_load (SWFDEC_MOVIE (cx->frame->target), url, target);
  else
    SWFDEC_WARNING ("no movie to load");
}

static void
swfdec_action_string_add (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *lval, *rval;

  rval = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  lval = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
  lval = swfdec_as_str_concat (cx, lval, rval);
  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx->frame->stack, 2), lval);
  swfdec_as_stack_pop (cx->frame->stack);
}

static void
swfdec_action_push_duplicate (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  *swfdec_as_stack_push (cx->frame->stack) = *swfdec_as_stack_peek (cx->frame->stack, 1);
}

static void
swfdec_action_random_number (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  gint32 max;
  SwfdecAsValue *val;

  val = swfdec_as_stack_peek (cx->frame->stack, 1);
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

  l = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
  r = swfdec_as_value_to_number (cx, swfdec_as_stack_pop (cx->frame->stack));
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
  if (cx->version < 5) {
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1), cond ? 1 : 0);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), cond);
  }
}

static void
swfdec_action_string_length (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;
  SwfdecAsValue *v;

  v = swfdec_as_stack_peek (cx->frame->stack, 1);
  s = swfdec_as_value_to_string (cx, v);
  SWFDEC_AS_VALUE_SET_INT (v, g_utf8_strlen (s, -1));  
}

static void
swfdec_action_string_compare (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *l, *r;
  gboolean cond;

  r = swfdec_as_value_to_string (cx, swfdec_as_stack_pop (cx->frame->stack));
  l = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  switch (action) {
    case SWFDEC_AS_ACTION_STRING_EQUALS:
      cond = l == r;
      break;
    case SWFDEC_AS_ACTION_STRING_LESS:
      cond = strcmp (l, r) < 0;
      break;
    default:
      g_assert_not_reached ();
      break;
  }
  if (cx->version < 5) {
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1), cond ? 1 : 0);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), cond);
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

  rval = swfdec_as_stack_peek (cx->frame->stack, 1);
  lval = swfdec_as_stack_peek (cx->frame->stack, 2);
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

    if (!SWFDEC_IS_MOVIE (lo))
      lo = SWFDEC_AS_VALUE_GET_OBJECT (lval);
    if (!SWFDEC_IS_MOVIE (ro))
      ro = SWFDEC_AS_VALUE_GET_OBJECT (rval);

    if (SWFDEC_IS_MOVIE (lo) && SWFDEC_IS_MOVIE (ro)) {
      /* do nothing */
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
  swfdec_as_stack_pop (cx->frame->stack);
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), cond);
}

static void
swfdec_action_equals2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *rval, *lval;
  SwfdecAsValueType ltype, rtype;
  double l, r;
  gboolean cond;

  rval = swfdec_as_stack_peek (cx->frame->stack, 1);
  lval = swfdec_as_stack_peek (cx->frame->stack, 2);
  ltype = lval->type;
  rtype = rval->type;
  
  /* get objects compared */
  if (ltype == SWFDEC_AS_TYPE_OBJECT && rtype == SWFDEC_AS_TYPE_OBJECT) {
    SwfdecAsObject *lo = SWFDEC_AS_VALUE_GET_OBJECT (lval);
    SwfdecAsObject *ro = SWFDEC_AS_VALUE_GET_OBJECT (rval);

    if (SWFDEC_IS_MOVIE (lo) && SWFDEC_IS_MOVIE (ro)) {
      /* do nothing */
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
  swfdec_as_stack_pop (cx->frame->stack);
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), cond);
}

static void
swfdec_action_strict_equals (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *rval, *lval;
  gboolean cond;

  rval = swfdec_as_stack_peek (cx->frame->stack, 1);
  lval = swfdec_as_stack_peek (cx->frame->stack, 2);

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
	cond = SWFDEC_AS_VALUE_GET_OBJECT (rval) == SWFDEC_AS_VALUE_GET_OBJECT (lval);
	break;
      default:
	g_assert_not_reached ();
	cond = FALSE;
    }
  }

  swfdec_as_stack_pop (cx->frame->stack);
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), cond);
}

static void
swfdec_action_set_target (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  if (!memchr (data, 0, len)) {
    SWFDEC_ERROR ("SetTarget action does not specify a string");
    return;
  }
  if (*data == '\0') {
    swfdec_as_frame_set_target (cx->frame, NULL);
  } else {
    SwfdecAsValue target;
    swfdec_as_context_eval (cx, NULL, (const char *) data, &target);
    if (!SWFDEC_AS_VALUE_IS_OBJECT (&target)) {
      SWFDEC_WARNING ("target is not an object");
      return;
    } 
    /* FIXME: allow non-movieclips as targets? */
    swfdec_as_frame_set_target (cx->frame, SWFDEC_AS_VALUE_GET_OBJECT (&target));
  }
}

static void
swfdec_action_set_target2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  val = swfdec_as_stack_peek (cx->frame->stack, 1);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    SWFDEC_WARNING ("target is not an object");
  } else {
    /* FIXME: allow non-movieclips as targets? */
    swfdec_as_frame_set_target (cx->frame, SWFDEC_AS_VALUE_GET_OBJECT (val));
  }
  swfdec_as_stack_pop (cx->frame->stack);
}

static void
swfdec_action_start_drag (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsStack *stack = cx->frame->stack;
  guint n_args = 1;

  swfdec_as_stack_ensure_size (stack, 3);
  if (swfdec_as_interpret_eval (cx, NULL, swfdec_as_stack_peek (stack, 1)) == SWFDEC_AS_STR_EMPTY) {
    SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (stack, 1), cx->frame->target);
  }
  if (swfdec_as_value_to_number (cx, swfdec_as_stack_peek (stack, 3))) {
    swfdec_as_stack_ensure_size (stack, 7);
    n_args = 5;
    /* yay for order */
    swfdec_as_stack_swap (stack, 4, 7);
    swfdec_as_stack_swap (stack, 5, 6);
  }
  if (!SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (stack, 1))) {
    swfdec_as_stack_pop_n (stack, n_args + 2);
    return;
  }
  *swfdec_as_stack_peek (stack, 3) = *swfdec_as_stack_peek (stack, 2);
  *swfdec_as_stack_peek (stack, 2) = *swfdec_as_stack_peek (stack, 1);
  swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (stack, 2)),
      SWFDEC_AS_STR_startDrag, swfdec_as_stack_peek (stack, 1));
  swfdec_action_call (cx, n_args);
  /* FIXME: the return value will still be written to this position */
  swfdec_as_stack_pop (stack);
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

/* Stack looks like this: [ rval, arg1, arg2, ...] */
static void
swfdec_action_create (SwfdecAsFunction *fun, guint n_args)
{
  SwfdecAsStack *stack = SWFDEC_AS_OBJECT (fun)->context->frame->stack;
  SwfdecAsObject *object;
  guint i;

  /* swap arguments and return value on the stack */
  for (i = 0; i < (n_args + 1) / 2; i++) {
    swfdec_as_stack_swap (stack, i + 1, n_args + 1 - i);
  }
  if (n_args)
    swfdec_as_stack_pop_n (stack, n_args);
  object = swfdec_as_object_create (fun, n_args, swfdec_as_stack_peek (stack, 0), TRUE);
  g_assert (object);
  SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (stack, 1), object);
}

static void
swfdec_action_new_object (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *constructor;
  SwfdecAsFunction *fun;
  guint n_args;
  const char *name;

  swfdec_as_stack_ensure_size (cx->frame->stack, 2);
  constructor = swfdec_as_stack_peek (cx->frame->stack, 1);
  name = swfdec_as_interpret_eval (cx, NULL, constructor);
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
  n_args = MIN (swfdec_as_stack_get_size (cx->frame->stack) - 2, n_args);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (constructor) ||
      !SWFDEC_IS_AS_FUNCTION (fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (constructor))) {
    SWFDEC_WARNING ("%s is not a constructor", name);
    goto fail;
  }

  swfdec_as_stack_pop (cx->frame->stack);
  swfdec_action_create (fun, n_args);
  return;

fail:
  swfdec_as_stack_pop_n (cx->frame->stack, n_args + 1);
  SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 1));
}

static void
swfdec_action_new_method (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *constructor;
  SwfdecAsFunction *fun;
  guint n_args;
  const char *name;

  swfdec_as_stack_ensure_size (cx->frame->stack, 3);
  name = swfdec_as_value_to_string (cx, swfdec_as_stack_pop (cx->frame->stack));

  constructor = swfdec_as_stack_pop (cx->frame->stack);
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  n_args = MIN (swfdec_as_stack_get_size (cx->frame->stack) - 1, n_args);
  if (name != SWFDEC_AS_STR_EMPTY) {
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

  swfdec_action_create (fun, n_args);
  return;

fail:
  swfdec_as_stack_pop_n (cx->frame->stack, n_args);
  SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 1));
}

static void
swfdec_action_init_object (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsStack *stack = cx->frame->stack;
  SwfdecAsObject *object;
  guint i, n_args, size;

  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_pop (stack));
  if (n_args * 2 > swfdec_as_stack_get_size (stack)) {
    size = swfdec_as_stack_get_size (stack);
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
    const char *s = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (stack, 2));
    swfdec_as_object_set_variable (object, s, swfdec_as_stack_peek (stack, 1));
    swfdec_as_stack_pop_n (stack, 2);
  }
  swfdec_as_stack_pop_n (stack, size);
  SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_push (stack), object);
}

static void
swfdec_action_init_array (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsStack *stack = cx->frame->stack;
  int i, n;
  SwfdecAsObject *array;

  swfdec_as_stack_ensure_size (stack, 1);
  n = swfdec_as_value_to_integer (cx, swfdec_as_stack_pop (stack));
  array = swfdec_as_array_new (cx);
  if (array == NULL)
    return;
  /* NB: we can't increase the stack here, as the number can easily be MAXINT */
  for (i = 0; i < n && swfdec_as_stack_get_size (stack) > 0; i++) {
    swfdec_as_array_push (SWFDEC_AS_ARRAY (array), swfdec_as_stack_pop (stack));
  }
  if (i != n) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_INT (&val, i);
    swfdec_as_object_set_variable (array, SWFDEC_AS_STR_length, &val);
  }
  SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_push (stack), array);
  return;
}

static void
swfdec_action_define_function (SwfdecAsContext *cx, guint action,
    const guint8 *data, guint len)
{
  const char *function_name;
  const char *name = NULL;
  guint i, n_args, size, n_registers;
  SwfdecBits bits;
  SwfdecAsFunction *fun;
  SwfdecAsFrame *frame;
  SwfdecScript *script;
  guint flags = 0;
  SwfdecScriptArgument *args;
  gboolean v2 = (action == 0x8e);

  frame = cx->frame;
  swfdec_bits_init_data (&bits, data, len);
  function_name = swfdec_bits_get_string_with_version (&bits, cx->version);
  if (function_name == NULL) {
    SWFDEC_ERROR ("could not parse function name");
    return;
  }
  /* see function-scope tests */
  if (cx->version > 5) {
    /* FIXME: or original target? */
    fun = swfdec_as_script_function_new (frame->scope ? frame->scope : SWFDEC_AS_SCOPE (frame), frame->target);
  } else {
    SwfdecAsScope *scope = frame->scope ? frame->scope : SWFDEC_AS_SCOPE (frame);
    while (scope->next)
      scope = scope->next;
    fun = swfdec_as_script_function_new (scope, frame->target);
  }
  if (fun == NULL)
    return;
  n_args = swfdec_bits_get_u16 (&bits);
  if (v2) {
    n_registers = swfdec_bits_get_u8 (&bits) + 1;
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
      args[i].name = swfdec_bits_get_string_with_version (&bits, cx->version);
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
    return;
  }
  /* create the script */
  SwfdecBuffer *buffer = swfdec_buffer_new_subbuffer (frame->script->buffer, 
      frame->pc + 3 + len - frame->script->buffer->data, size);
  swfdec_bits_init (&bits, buffer);
  if (*function_name) {
    name = function_name;
  } else if (swfdec_as_stack_get_size (frame->stack) > 0) {
    /* This is kind of a hack that uses a feature of the Adobe compiler:
     * foo = function () {} is compiled as these actions:
     * Push "foo", DefineFunction, SetVariable/SetMember
     * With this knowledge we can inspect the topmost stack member, since
     * it will contain the name this function will soon be assigned to.
     */
    if (SWFDEC_AS_VALUE_IS_STRING (swfdec_as_stack_peek (frame->stack, 1)))
      name = SWFDEC_AS_VALUE_GET_STRING (swfdec_as_stack_peek (frame->stack, 1));
  }
  if (name == NULL)
    name = "unnamed_function";
  script = swfdec_script_new (&bits, name, cx->version);
  if (script == NULL) {
    SWFDEC_ERROR ("failed to create script");
    g_free (args);
    return;
  }
  if (frame->constant_pool_buffer)
    script->constant_pool = swfdec_buffer_ref (frame->constant_pool_buffer);
  script->flags = flags;
  script->n_registers = n_registers;
  script->n_arguments = n_args;
  script->arguments = args;
  SWFDEC_AS_SCRIPT_FUNCTION (fun)->script = script;
  swfdec_script_add_to_context (script, cx);
  /* attach the function */
  if (*function_name == '\0') {
    swfdec_as_stack_ensure_free (frame->stack, 1);
    SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_push (frame->stack), SWFDEC_AS_OBJECT (fun));
  } else {
    SwfdecAsValue funval;
    /* FIXME: really varobj? Not eval or sth like that? */
    function_name = swfdec_as_context_get_string (cx, function_name);
    SWFDEC_AS_VALUE_SET_OBJECT (&funval, SWFDEC_AS_OBJECT (fun));
    swfdec_as_object_set_variable (frame->target, function_name, &funval);
  }

  /* update current context */
  frame->pc += 3 + len + size;
}

static void
swfdec_action_bitwise (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  int a, b;

  a = swfdec_as_value_to_integer (cx, swfdec_as_stack_pop (cx->frame->stack));
  b = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx->frame->stack, 1));

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

  SWFDEC_AS_VALUE_SET_INT (swfdec_as_stack_peek (cx->frame->stack, 1), a);
}

static void
swfdec_action_shift (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  int amount, value;

  amount = swfdec_as_value_to_integer (cx, swfdec_as_stack_pop (cx->frame->stack));
  amount &= 31;
  value = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (cx->frame->stack, 1));

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

  SWFDEC_AS_VALUE_SET_INT (swfdec_as_stack_peek (cx->frame->stack, 1), value);
}

static void
swfdec_action_to_integer (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx->frame->stack, 1);

  SWFDEC_AS_VALUE_SET_INT (val, swfdec_as_value_to_integer (cx, val));
}

static void
swfdec_action_target_path (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  SwfdecMovie *movie;
  SwfdecAsStack *stack;
  char *s;

  stack = cx->frame->stack;
  val = swfdec_as_stack_peek (stack, 1);

  if (!SWFDEC_AS_VALUE_IS_OBJECT (val) ||
      !SWFDEC_IS_MOVIE (movie = (SwfdecMovie *) SWFDEC_AS_VALUE_GET_OBJECT (val))) {
    SWFDEC_FIXME ("What's the TargetPath for non-movies?");
    SWFDEC_AS_VALUE_SET_UNDEFINED (val);
    return;
  }
  s = swfdec_movie_get_path (movie);
  SWFDEC_AS_VALUE_SET_STRING (val, swfdec_as_context_give_string (cx, s));
}

static void
swfdec_action_define_local (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsObject *target;
  const char *name;

  name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
  if (cx->frame->is_local) {
    target = SWFDEC_AS_OBJECT (cx->frame);
  } else {
    target = cx->frame->target;
  }
  swfdec_as_object_set_variable (target, name,
      swfdec_as_stack_peek (cx->frame->stack, 1));
  swfdec_as_stack_pop_n (cx->frame->stack, 2);
}

static void
swfdec_action_define_local2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue val = { 0, };
  SwfdecAsObject *target;
  const char *name;

  name = swfdec_as_value_to_string (cx, swfdec_as_stack_pop (cx->frame->stack));
  if (cx->frame->is_local) {
    target = SWFDEC_AS_OBJECT (cx->frame);
  } else {
    target = cx->frame->target;
  }
  swfdec_as_object_set_variable (target, name, &val);
}

static void
swfdec_action_return (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  *cx->frame->return_value = *swfdec_as_stack_pop (cx->frame->stack);
  swfdec_as_frame_return (cx->frame);
}

static void
swfdec_action_delete (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  const char *name;
  
  name = swfdec_as_value_to_string (cx, swfdec_as_stack_pop (cx->frame->stack));
  val = swfdec_as_stack_pop (cx->frame->stack);
  if (SWFDEC_AS_VALUE_IS_OBJECT (val))
    swfdec_as_object_delete_variable (SWFDEC_AS_VALUE_GET_OBJECT (val), name);
}

static void
swfdec_action_delete2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *name;
  
  name = swfdec_as_value_to_string (cx, swfdec_as_stack_pop (cx->frame->stack));
  swfdec_as_frame_delete_variable (cx->frame, name);
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
  cx->frame->registers[*data] = *swfdec_as_stack_peek (cx->frame->stack, 1);
}

static void
swfdec_action_modulo (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double x, y;

  y = swfdec_as_value_to_number (cx, swfdec_as_stack_pop (cx->frame->stack));
  x = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
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
  SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1), x);
}

static void
swfdec_action_swap (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  swfdec_as_stack_swap (cx->frame->stack, 1, 2);
}

static void
swfdec_action_to_number (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1),
      swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 1)));
}

static void
swfdec_action_to_string (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx->frame->stack, 1),
      swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 1)));
}

static void
swfdec_action_type_of (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  const char *type;

  val = swfdec_as_stack_peek (cx->frame->stack, 1);
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
	  type = SWFDEC_AS_STR_movieclip;
	} else if (SWFDEC_IS_AS_FUNCTION (obj)) {
	  type = SWFDEC_AS_STR_function;
	} else {
	  type = SWFDEC_AS_STR_object;
	}
      }
      break;
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

  SWFDEC_AS_VALUE_SET_INT (swfdec_as_stack_push (cx->frame->stack), diff);
}

static void
swfdec_action_extends (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *superclass, *subclass, proto;
  SwfdecAsObject *prototype;
  SwfdecAsObject *super;

  superclass = swfdec_as_stack_pop (cx->frame->stack);
  subclass = swfdec_as_stack_pop (cx->frame->stack);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (superclass) ||
      !SWFDEC_IS_AS_FUNCTION (SWFDEC_AS_VALUE_GET_OBJECT (superclass))) {
    SWFDEC_ERROR ("superclass is not a function");
    return;
  }
  if (!SWFDEC_AS_VALUE_IS_OBJECT (subclass)) {
    SWFDEC_ERROR ("subclass is not an object");
    return;
  }
  super = SWFDEC_AS_VALUE_GET_OBJECT (superclass);
  prototype = swfdec_as_object_new (cx);
  if (prototype == NULL)
    return;
  swfdec_as_object_get_variable (super, SWFDEC_AS_STR_prototype, &proto);
  swfdec_as_object_set_variable (prototype, SWFDEC_AS_STR___proto__, &proto);
  swfdec_as_object_delete_variable (prototype, SWFDEC_AS_STR_constructor);
  swfdec_as_object_set_variable (prototype, SWFDEC_AS_STR___constructor__,
      superclass);
  SWFDEC_AS_VALUE_SET_OBJECT (&proto, prototype);
  swfdec_as_object_set_variable (SWFDEC_AS_VALUE_GET_OBJECT (subclass),
      SWFDEC_AS_STR_prototype, &proto);
}

static gboolean
swfdec_action_do_enumerate (SwfdecAsObject *object, const char *variable,
    SwfdecAsValue *value, guint flags, gpointer stackp)
{
  SwfdecAsStack *stack = stackp;

  if (flags & SWFDEC_AS_VARIABLE_DONT_ENUM)
    return TRUE;
  swfdec_as_stack_ensure_free (stack, 1);
  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_push (stack), variable);
  return TRUE;
}

static void
swfdec_action_enumerate (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  SwfdecAsStack *stack;
  SwfdecAsObject *obj;

  stack = cx->frame->stack;
  val = swfdec_as_stack_peek (stack, 1);

  swfdec_as_interpret_eval (cx, NULL, val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    SWFDEC_ERROR ("Enumerate not pointing to an object");
    SWFDEC_AS_VALUE_SET_NULL (val);
    return;
  }
  obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
  SWFDEC_AS_VALUE_SET_NULL (val);
  swfdec_as_object_foreach (obj, swfdec_action_do_enumerate, stack);
}

static void
swfdec_action_enumerate2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  SwfdecAsStack *stack;
  SwfdecAsObject *obj;

  stack = cx->frame->stack;
  val = swfdec_as_stack_peek (stack, 1);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    SWFDEC_ERROR ("Enumerate2 called without an object");
    SWFDEC_AS_VALUE_SET_NULL (val);
    return;
  }
  obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
  SWFDEC_AS_VALUE_SET_NULL (val);
  swfdec_as_object_foreach (obj, swfdec_action_do_enumerate, stack);
}

static void
swfdec_action_logical (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val;
  gboolean l, r;

  l = swfdec_as_value_to_boolean (cx, swfdec_as_stack_pop (cx->frame->stack));
  val = swfdec_as_stack_peek (cx->frame->stack, 1);
  r = swfdec_as_value_to_boolean (cx, val);

  SWFDEC_AS_VALUE_SET_BOOLEAN (val, (action == 0x10) ? (l && r) : (l || r));
}

static void
swfdec_action_char_to_ascii_5 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx->frame->stack, 1);
  const char *s = swfdec_as_value_to_string (cx, val);

  char *ascii;
  ascii = g_convert (s, -1, "LATIN1", "UTF8", NULL, NULL, NULL);
  if (ascii == NULL) {
    /* This can happen if a Flash 5 movie gets loaded into a Flash 7 movie */
    SWFDEC_FIXME ("Someone threw unconvertible text %s at Flash <= 5", s);
    SWFDEC_AS_VALUE_SET_INT (val, 0); /* FIXME: what to return??? */
  } else {
    SWFDEC_AS_VALUE_SET_INT (val, (guchar) ascii[0]);
    g_free (ascii);
  }
}

static void
swfdec_action_char_to_ascii (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx->frame->stack, 1);
  const char *s = swfdec_as_value_to_string(cx, val);
  gunichar *uni;
  
  uni = g_utf8_to_ucs4_fast (s, -1, NULL);
  if (uni == NULL) {
    /* This should never happen, everything is valid UTF-8 in here */
    g_warning ("conversion of character %s failed", s);
    SWFDEC_AS_VALUE_SET_INT (val, 0);
  } else {
    SWFDEC_AS_VALUE_SET_INT (val, uni[0]);
    g_free (uni);
  }
}

static void
swfdec_action_ascii_to_char (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  char *s;
  SwfdecAsValue *val = swfdec_as_stack_peek (cx->frame->stack, 1);
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

static void
swfdec_action_ascii_to_char_5 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx->frame->stack, 1);
  char s[2];
  char *utf8;
  
  s[0] = ((guint) swfdec_as_value_to_integer (cx, val)) % 256;
  s[1] = 0;

  utf8 = g_convert (s, -1, "UTF8", "LATIN1", NULL, NULL, NULL);
  if (utf8 == NULL) {
    g_warning ("conversion of character %u failed", (guint) s[0]);
    SWFDEC_AS_VALUE_SET_STRING (val, SWFDEC_AS_STR_EMPTY);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (val, swfdec_as_context_get_string (cx, utf8));
    g_free (utf8);
  }
}

static void
swfdec_action_mb_ascii_to_char_5 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_peek (cx->frame->stack, 1);
  char s[3];
  char *utf8;
  guint i;
  
  i = ((guint) swfdec_as_value_to_integer (cx, val));
  if (i > 255) {
    s[0] = i / 256;
    s[1] = i % 256;
    s[2] = 0;
  } else {
    s[0] = i;
    s[1] = 0;
  }
  utf8 = g_convert (s, -1, "UTF8", "LATIN1", NULL, NULL, NULL);
  if (utf8 == NULL) {
    g_warning ("conversion of character %u failed", i);
    SWFDEC_AS_VALUE_SET_STRING (val, SWFDEC_AS_STR_EMPTY);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (val, swfdec_as_context_get_string (cx, utf8));
    g_free (utf8);
  }
}

static void
swfdec_action_with (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsObject *object;

  if (len != 2) {
    SWFDEC_ERROR ("With action requires a length of 2, but got %u", len);
    return;
  }
  object = swfdec_as_value_to_object (cx, swfdec_as_stack_pop (cx->frame->stack));
  if (object == NULL) {
    SWFDEC_ERROR ("With called without an object");
    return;
  }
  swfdec_as_with_new (object, data + len, GUINT16_FROM_LE (*(guint16 *) data));
}

static void
swfdec_action_remove_sprite (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *val = swfdec_as_stack_pop (cx->frame->stack);
  SwfdecAsObject *sprite;
  SwfdecMovie *movie;

  if (SWFDEC_AS_VALUE_IS_STRING (val)) {
    const char *name = SWFDEC_AS_VALUE_GET_STRING (val);

    swfdec_as_context_eval (cx, NULL, name, val);
  }
  if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    sprite = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    SWFDEC_FIXME ("unknown type in RemoveSprite");
    return;
  }
  if (!SWFDEC_IS_MOVIE (sprite)) {
    SWFDEC_FIXME ("cannot remove non movieclip objects");
    return;
  }
  movie = SWFDEC_MOVIE (sprite);
  if (swfdec_depth_classify (movie->depth) == SWFDEC_DEPTH_CLASS_DYNAMIC) {
    SWFDEC_LOG ("removing clip %s", movie->name);
    swfdec_movie_remove (movie);
  }
}

/*** PRINT FUNCTIONS ***/

static char *
swfdec_action_print_with (guint action, const guint8 *data, guint len)
{
  if (len != 2) {
    SWFDEC_ERROR ("With action requires a length of 2, but got %u", len);
    return NULL;
  }
  return g_strdup_printf ("With %u", GUINT16_FROM_LE (*(guint16 *) data));
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
  function_name = swfdec_bits_get_string (&bits);
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
    arg_name = swfdec_bits_get_string (&bits);
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
  url = swfdec_bits_get_string (&bits);
  target = swfdec_bits_get_string (&bits);
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
  if (len != 2) {
    SWFDEC_ERROR ("If action length invalid (is %u, should be 2", len);
    return NULL;
  }
  return g_strdup_printf ("If %d", GINT16_FROM_LE (*((gint16*) data)));
}

static char *
swfdec_action_print_jump (guint action, const guint8 *data, guint len)
{
  if (len != 2) {
    SWFDEC_ERROR ("Jump action length invalid (is %u, should be 2", len);
    return NULL;
  }
  return g_strdup_printf ("Jump %d", GINT16_FROM_LE (*((gint16*) data)));
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
	  char *s = swfdec_bits_get_string (&bits);
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
	g_string_append_printf (string, "%d", swfdec_bits_get_u32 (&bits));
	break;
      case 8: /* 8bit ConstantPool address */
	g_string_append_printf (string, "Pool %u", swfdec_bits_get_u8 (&bits));
	break;
      case 9: /* 16bit ConstantPool address */
	g_string_append_printf (string, "Pool %u", swfdec_bits_get_u16 (&bits));
	break;
      default:
	SWFDEC_ERROR ("Push: type %u not implemented", type);
	return NULL;
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

  /* FIXME: version */
  pool = swfdec_constant_pool_new_from_action (data, len, 6);
  if (pool == NULL)
    return g_strdup ("ConstantPool (invalid)");
  string = g_string_new ("ConstantPool");
  for (i = 0; i < swfdec_constant_pool_size (pool); i++) {
    g_string_append (string, i ? ", " : " ");
    g_string_append (string, swfdec_constant_pool_get (pool, i));
    g_string_append_printf (string, " (%u)", i);
  }
  return g_string_free (string, FALSE);
}

#if 0
static char *
swfdec_action_print_wait_for_frame2 (guint action, const guint8 *data, guint len)
{
  if (len != 1) {
    SWFDEC_ERROR ("WaitForFrame2 needs a 1-byte data");
    return NULL;
  }
  return g_strdup_printf ("WaitForFrame2 %u", (guint) *data);
}
#endif

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

  frame = GUINT16_FROM_LE (*((guint16 *) data));
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

  frame = GUINT16_FROM_LE (*((guint16 *) data));
  jump = data[2];
  return g_strdup_printf ("WaitForFrame %u %u", frame, jump);
}

/*** BIG FUNCTION TABLE ***/

const SwfdecActionSpec swfdec_as_actions[256] = {
  /* version 3 */
  [SWFDEC_AS_ACTION_NEXT_FRAME] = { "NextFrame", NULL, 0, 0, { swfdec_action_next_frame, swfdec_action_next_frame, swfdec_action_next_frame, swfdec_action_next_frame, swfdec_action_next_frame } },
  [SWFDEC_AS_ACTION_PREVIOUS_FRAME] = { "PreviousFrame", NULL, 0, 0, { swfdec_action_previous_frame, swfdec_action_previous_frame, swfdec_action_previous_frame, swfdec_action_previous_frame, swfdec_action_previous_frame } },
  [SWFDEC_AS_ACTION_PLAY] = { "Play", NULL, 0, 0, { swfdec_action_play, swfdec_action_play, swfdec_action_play, swfdec_action_play, swfdec_action_play } },
  [SWFDEC_AS_ACTION_STOP] = { "Stop", NULL, 0, 0, { swfdec_action_stop, swfdec_action_stop, swfdec_action_stop, swfdec_action_stop, swfdec_action_stop } },
  [SWFDEC_AS_ACTION_TOGGLE_QUALITY] = { "ToggleQuality", NULL },
  [SWFDEC_AS_ACTION_STOP_SOUNDS] = { "StopSounds", NULL, 0, 0, { swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds } },
  /* version 4 */
  [SWFDEC_AS_ACTION_ADD] = { "Add", NULL, 2, 1, { NULL, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary } },
  [SWFDEC_AS_ACTION_SUBTRACT] = { "Subtract", NULL, 2, 1, { NULL, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary } },
  [SWFDEC_AS_ACTION_MULTIPLY] = { "Multiply", NULL, 2, 1, { NULL, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary } },
  [SWFDEC_AS_ACTION_DIVIDE] = { "Divide", NULL, 2, 1, { NULL, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary } },
  [SWFDEC_AS_ACTION_EQUALS] = { "Equals", NULL, 2, 1, { NULL, swfdec_action_old_compare, swfdec_action_old_compare, swfdec_action_old_compare, swfdec_action_old_compare } },
  [SWFDEC_AS_ACTION_LESS] = { "Less", NULL, 2, 1, { NULL, swfdec_action_old_compare, swfdec_action_old_compare, swfdec_action_old_compare, swfdec_action_old_compare } },
  [SWFDEC_AS_ACTION_AND] = { "And", NULL, 2, 1, { NULL, /* FIXME */NULL, swfdec_action_logical, swfdec_action_logical, swfdec_action_logical } },
  [SWFDEC_AS_ACTION_OR] = { "Or", NULL, 2, 1, { NULL, /* FIXME */NULL, swfdec_action_logical, swfdec_action_logical, swfdec_action_logical } },
  [SWFDEC_AS_ACTION_NOT] = { "Not", NULL, 1, 1, { NULL, swfdec_action_not_4, swfdec_action_not_5, swfdec_action_not_5, swfdec_action_not_5 } },
  [SWFDEC_AS_ACTION_STRING_EQUALS] = { "StringEquals", NULL, 2, 1, { NULL, swfdec_action_string_compare, swfdec_action_string_compare, swfdec_action_string_compare, swfdec_action_string_compare } },
  [SWFDEC_AS_ACTION_STRING_LENGTH] = { "StringLength", NULL, 1, 1, { NULL, swfdec_action_string_length, swfdec_action_string_length, swfdec_action_string_length, swfdec_action_string_length } },
  [SWFDEC_AS_ACTION_STRING_EXTRACT] = { "StringExtract", NULL },
  [SWFDEC_AS_ACTION_POP] = { "Pop", NULL, 1, 0, { NULL, swfdec_action_pop, swfdec_action_pop, swfdec_action_pop, swfdec_action_pop } },
  [SWFDEC_AS_ACTION_TO_INTEGER] = { "ToInteger", NULL, 1, 1, { NULL, swfdec_action_to_integer, swfdec_action_to_integer, swfdec_action_to_integer, swfdec_action_to_integer } },
  [SWFDEC_AS_ACTION_GET_VARIABLE] = { "GetVariable", NULL, 1, 1, { NULL, swfdec_action_get_variable, swfdec_action_get_variable, swfdec_action_get_variable, swfdec_action_get_variable } },
  [SWFDEC_AS_ACTION_SET_VARIABLE] = { "SetVariable", NULL, 2, 0, { NULL, swfdec_action_set_variable, swfdec_action_set_variable, swfdec_action_set_variable, swfdec_action_set_variable } },
  [SWFDEC_AS_ACTION_SET_TARGET2] = { "SetTarget2", NULL, 1, 0, { swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2 } },
  [0x21] = { "StringAdd", NULL, 2, 1, { NULL, swfdec_action_string_add, swfdec_action_string_add, swfdec_action_string_add, swfdec_action_string_add } },
  [SWFDEC_AS_ACTION_GET_PROPERTY] = { "GetProperty", NULL, 2, 1, { NULL, swfdec_action_get_property, swfdec_action_get_property, swfdec_action_get_property, swfdec_action_get_property } },
  [SWFDEC_AS_ACTION_SET_PROPERTY] = { "SetProperty", NULL, 3, 0, { NULL, swfdec_action_set_property, swfdec_action_set_property, swfdec_action_set_property, swfdec_action_set_property } },
  [SWFDEC_AS_ACTION_CLONE_SPRITE] = { "CloneSprite", NULL },
  [SWFDEC_AS_ACTION_REMOVE_SPRITE] = { "RemoveSprite", NULL, 1, 0, { NULL, swfdec_action_remove_sprite, swfdec_action_remove_sprite, swfdec_action_remove_sprite, swfdec_action_remove_sprite } },
  [SWFDEC_AS_ACTION_TRACE] = { "Trace", NULL, 1, 0, { NULL, swfdec_action_trace, swfdec_action_trace, swfdec_action_trace, swfdec_action_trace } },
  [SWFDEC_AS_ACTION_START_DRAG] = { "StartDrag", NULL, -1, 0, { NULL, swfdec_action_start_drag, swfdec_action_start_drag, swfdec_action_start_drag, swfdec_action_start_drag } },
  [SWFDEC_AS_ACTION_END_DRAG] = { "EndDrag", NULL, 0, 0, { NULL, swfdec_action_end_drag, swfdec_action_end_drag, swfdec_action_end_drag, swfdec_action_end_drag } },
  [SWFDEC_AS_ACTION_STRING_LESS] = { "StringLess", NULL, 2, 1, { NULL, swfdec_action_string_compare, swfdec_action_string_compare, swfdec_action_string_compare, swfdec_action_string_compare } },
  /* version 7 */
  [SWFDEC_AS_ACTION_THROW] = { "Throw", NULL },
  [SWFDEC_AS_ACTION_CAST] = { "Cast", NULL },
  [SWFDEC_AS_ACTION_IMPLEMENTS] = { "Implements", NULL },
  /* version 4 */
  [0x30] = { "RandomNumber", NULL, 1, 1, { NULL, swfdec_action_random_number, swfdec_action_random_number, swfdec_action_random_number, swfdec_action_random_number } },
  [SWFDEC_AS_ACTION_MB_STRING_LENGTH] = { "MBStringLength", NULL },
  [SWFDEC_AS_ACTION_CHAR_TO_ASCII] = { "CharToAscii", NULL, 1, 1, { NULL, swfdec_action_char_to_ascii_5, swfdec_action_char_to_ascii_5, swfdec_action_char_to_ascii, swfdec_action_char_to_ascii } },
  [SWFDEC_AS_ACTION_ASCII_TO_CHAR] = { "AsciiToChar", NULL, 1, 1, { NULL, swfdec_action_ascii_to_char_5, swfdec_action_ascii_to_char_5, swfdec_action_ascii_to_char, swfdec_action_ascii_to_char } },
  [SWFDEC_AS_ACTION_GET_TIME] = { "GetTime", NULL, 0, 1, { NULL, swfdec_action_get_time, swfdec_action_get_time, swfdec_action_get_time, swfdec_action_get_time } },
  [SWFDEC_AS_ACTION_MB_STRING_EXTRACT] = { "MBStringExtract", NULL },
  [SWFDEC_AS_ACTION_MB_CHAR_TO_ASCII] = { "MBCharToAscii", NULL },
  [SWFDEC_AS_ACTION_MB_ASCII_TO_CHAR] = { "MBAsciiToChar", NULL, 1, 1, { NULL, swfdec_action_mb_ascii_to_char_5, swfdec_action_mb_ascii_to_char_5, swfdec_action_ascii_to_char, swfdec_action_ascii_to_char }  },
  /* version 5 */
  [SWFDEC_AS_ACTION_DELETE] = { "Delete", NULL, 2, 0, { NULL, NULL, swfdec_action_delete, swfdec_action_delete, swfdec_action_delete } },
  [SWFDEC_AS_ACTION_DELETE2] = { "Delete2", NULL, 1, 0, { NULL, NULL, swfdec_action_delete2, swfdec_action_delete2, swfdec_action_delete2 } },
  [SWFDEC_AS_ACTION_DEFINE_LOCAL] = { "DefineLocal", NULL, 2, 0, { NULL, NULL, swfdec_action_define_local, swfdec_action_define_local, swfdec_action_define_local } },
  [SWFDEC_AS_ACTION_CALL_FUNCTION] = { "CallFunction", NULL, -1, 1, { NULL, NULL, swfdec_action_call_function, swfdec_action_call_function, swfdec_action_call_function } },
  [SWFDEC_AS_ACTION_RETURN] = { "Return", NULL, 1, 0, { NULL, NULL, swfdec_action_return, swfdec_action_return, swfdec_action_return } },
  [SWFDEC_AS_ACTION_MODULO] = { "Modulo", NULL, 2, 1, { NULL, NULL, swfdec_action_modulo, swfdec_action_modulo, swfdec_action_modulo } },
  [SWFDEC_AS_ACTION_NEW_OBJECT] = { "NewObject", NULL, -1, 1, { NULL, NULL, swfdec_action_new_object, swfdec_action_new_object, swfdec_action_new_object } },
  [SWFDEC_AS_ACTION_DEFINE_LOCAL2] = { "DefineLocal2", NULL, 1, 0, { NULL, NULL, swfdec_action_define_local2, swfdec_action_define_local2, swfdec_action_define_local2 } },
  [SWFDEC_AS_ACTION_INIT_ARRAY] = { "InitArray", NULL, -1, 1, { NULL, NULL, swfdec_action_init_array, swfdec_action_init_array, swfdec_action_init_array } },
  [SWFDEC_AS_ACTION_INIT_OBJECT] = { "InitObject", NULL, -1, 1, { NULL, NULL, swfdec_action_init_object, swfdec_action_init_object, swfdec_action_init_object } },
  [SWFDEC_AS_ACTION_TYPE_OF] = { "TypeOf", NULL, 1, 1, { NULL, NULL, swfdec_action_type_of, swfdec_action_type_of, swfdec_action_type_of } },
  [SWFDEC_AS_ACTION_TARGET_PATH] = { "TargetPath", NULL, 1, 1, { NULL, NULL, swfdec_action_target_path, swfdec_action_target_path, swfdec_action_target_path } },
  [SWFDEC_AS_ACTION_ENUMERATE] = { "Enumerate", NULL, 1, -1, { NULL, NULL, swfdec_action_enumerate, swfdec_action_enumerate, swfdec_action_enumerate } },
  [SWFDEC_AS_ACTION_ADD2] = { "Add2", NULL, 2, 1, { NULL, NULL, swfdec_action_add2, swfdec_action_add2, swfdec_action_add2 } },
  [SWFDEC_AS_ACTION_LESS2] = { "Less2", NULL, 2, 1, { NULL, NULL, swfdec_action_new_comparison, swfdec_action_new_comparison, swfdec_action_new_comparison } },
  [SWFDEC_AS_ACTION_EQUALS2] = { "Equals2", NULL, 2, 1, { NULL, NULL, swfdec_action_equals2_5, swfdec_action_equals2, swfdec_action_equals2 } },
  [SWFDEC_AS_ACTION_TO_NUMBER] = { "ToNumber", NULL, 1, 1, { NULL, NULL, swfdec_action_to_number, swfdec_action_to_number, swfdec_action_to_number } },
  [SWFDEC_AS_ACTION_TO_STRING] = { "ToString", NULL, 1, 1, { NULL, NULL, swfdec_action_to_string, swfdec_action_to_string, swfdec_action_to_string } },
  [SWFDEC_AS_ACTION_PUSH_DUPLICATE] = { "PushDuplicate", NULL, 1, 2, { NULL, NULL, swfdec_action_push_duplicate, swfdec_action_push_duplicate, swfdec_action_push_duplicate } },
  [SWFDEC_AS_ACTION_SWAP] = { "Swap", NULL, 2, 2, { NULL, NULL, swfdec_action_swap, swfdec_action_swap, swfdec_action_swap } },
  [SWFDEC_AS_ACTION_GET_MEMBER] = { "GetMember", NULL, 2, 1, { NULL, swfdec_action_get_member, swfdec_action_get_member, swfdec_action_get_member, swfdec_action_get_member } },
  [SWFDEC_AS_ACTION_SET_MEMBER] = { "SetMember", NULL, 3, 0, { NULL, swfdec_action_set_member, swfdec_action_set_member, swfdec_action_set_member, swfdec_action_set_member } },
  [SWFDEC_AS_ACTION_INCREMENT] = { "Increment", NULL, 1, 1, { NULL, NULL, swfdec_action_increment, swfdec_action_increment, swfdec_action_increment } },
  [SWFDEC_AS_ACTION_DECREMENT] = { "Decrement", NULL, 1, 1, { NULL, NULL, swfdec_action_decrement, swfdec_action_decrement, swfdec_action_decrement } },
  [SWFDEC_AS_ACTION_CALL_METHOD] = { "CallMethod", NULL, -1, 1, { NULL, NULL, swfdec_action_call_method, swfdec_action_call_method, swfdec_action_call_method } },
  [SWFDEC_AS_ACTION_NEW_METHOD] = { "NewMethod", NULL, -1, 1, { NULL, NULL, swfdec_action_new_method, swfdec_action_new_method, swfdec_action_new_method } },
  /* version 6 */
  [SWFDEC_AS_ACTION_INSTANCE_OF] = { "InstanceOf", NULL },
  [SWFDEC_AS_ACTION_ENUMERATE2] = { "Enumerate2", NULL, 1, -1, { NULL, NULL, NULL, swfdec_action_enumerate2, swfdec_action_enumerate2 } },
  /* version 5 */
  [SWFDEC_AS_ACTION_BIT_AND] = { "BitAnd", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [SWFDEC_AS_ACTION_BIT_OR] = { "BitOr", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [SWFDEC_AS_ACTION_BIT_XOR] = { "BitXor", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [SWFDEC_AS_ACTION_BIT_LSHIFT] = { "BitLShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
  [SWFDEC_AS_ACTION_BIT_RSHIFT] = { "BitRShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
  [SWFDEC_AS_ACTION_BIT_URSHIFT] = { "BitURShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
  /* version 6 */
  [SWFDEC_AS_ACTION_STRICT_EQUALS] = { "StrictEquals", NULL, 2, 1, { NULL, NULL, NULL, swfdec_action_strict_equals, swfdec_action_strict_equals } },
  [SWFDEC_AS_ACTION_GREATER] = { "Greater", NULL, 2, 1, { NULL, NULL, NULL, swfdec_action_new_comparison, swfdec_action_new_comparison } },
  [SWFDEC_AS_ACTION_STRING_GREATER] = { "StringGreater", NULL },
  /* version 7 */
  [SWFDEC_AS_ACTION_EXTENDS] = { "Extends", NULL, 2, 0, { NULL, NULL, NULL, swfdec_action_extends, swfdec_action_extends } },
  /* version 3 */
  [SWFDEC_AS_ACTION_GOTO_FRAME] = { "GotoFrame", swfdec_action_print_goto_frame, 0, 0, { swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame } },
  [SWFDEC_AS_ACTION_GET_URL] = { "GetURL", swfdec_action_print_get_url, 0, 0, { swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url } },
  /* version 5 */
  [SWFDEC_AS_ACTION_STORE_REGISTER] = { "StoreRegister", swfdec_action_print_store_register, 1, 1, { NULL, NULL, swfdec_action_store_register, swfdec_action_store_register, swfdec_action_store_register } },
  [SWFDEC_AS_ACTION_CONSTANT_POOL] = { "ConstantPool", swfdec_action_print_constant_pool, 0, 0, { NULL, NULL, swfdec_action_constant_pool, swfdec_action_constant_pool, swfdec_action_constant_pool } },
  /* version 3 */
  [SWFDEC_AS_ACTION_WAIT_FOR_FRAME] = { "WaitForFrame", swfdec_action_print_wait_for_frame, 0, 0, { swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame } },
  [SWFDEC_AS_ACTION_SET_TARGET] = { "SetTarget", swfdec_action_print_set_target, 0, 0, { swfdec_action_set_target, swfdec_action_set_target, swfdec_action_set_target, swfdec_action_set_target, swfdec_action_set_target } },
  [SWFDEC_AS_ACTION_GOTO_LABEL] = { "GotoLabel", swfdec_action_print_goto_label, 0, 0, { swfdec_action_goto_label, swfdec_action_goto_label, swfdec_action_goto_label, swfdec_action_goto_label, swfdec_action_goto_label } },
#if 0
  /* version 4 */
  [0x8d] = { "WaitForFrame2", swfdec_action_print_wait_for_frame2, 1, 0, { NULL, swfdec_action_wait_for_frame2, swfdec_action_wait_for_frame2, swfdec_action_wait_for_frame2, swfdec_action_wait_for_frame2 } },
#endif
  /* version 7 */
  [SWFDEC_AS_ACTION_DEFINE_FUNCTION2] = { "DefineFunction2", swfdec_action_print_define_function, 0, -1, { NULL, NULL, NULL, swfdec_action_define_function, swfdec_action_define_function } },
  [SWFDEC_AS_ACTION_TRY] = { "Try", NULL },
  /* version 5 */
  [SWFDEC_AS_ACTION_WITH] = { "With", swfdec_action_print_with, 1, 0, { NULL, NULL, swfdec_action_with, swfdec_action_with, swfdec_action_with } },
  /* version 4 */
  [SWFDEC_AS_ACTION_PUSH] = { "Push", swfdec_action_print_push, 0, -1, { NULL, swfdec_action_push, swfdec_action_push, swfdec_action_push, swfdec_action_push } },
  [SWFDEC_AS_ACTION_JUMP] = { "Jump", swfdec_action_print_jump, 0, 0, { NULL, swfdec_action_jump, swfdec_action_jump, swfdec_action_jump, swfdec_action_jump } },
  [SWFDEC_AS_ACTION_GET_URL2] = { "GetURL2", swfdec_action_print_get_url2, 2, 0, { NULL, swfdec_action_get_url2, swfdec_action_get_url2, swfdec_action_get_url2, swfdec_action_get_url2 } },
  /* version 5 */
  [SWFDEC_AS_ACTION_DEFINE_FUNCTION] = { "DefineFunction", swfdec_action_print_define_function, 0, -1, { NULL, NULL, swfdec_action_define_function, swfdec_action_define_function, swfdec_action_define_function } },
  /* version 4 */
  [SWFDEC_AS_ACTION_IF] = { "If", swfdec_action_print_if, 1, 0, { NULL, swfdec_action_if, swfdec_action_if, swfdec_action_if, swfdec_action_if } },
  [SWFDEC_AS_ACTION_CALL] = { "Call", NULL },
  [SWFDEC_AS_ACTION_GOTO_FRAME2] = { "GotoFrame2", swfdec_action_print_goto_frame2, 1, 0, { NULL, swfdec_action_goto_frame2, swfdec_action_goto_frame2, swfdec_action_goto_frame2, swfdec_action_goto_frame2 } }
};

