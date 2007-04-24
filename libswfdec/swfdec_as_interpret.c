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
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_function.h"
#include "swfdec_as_stack.h"
#include "swfdec_debug.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include "swfdec_decoder.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"

/* Define this to get SWFDEC_WARN'd about missing properties of objects.
 * This can be useful to find out about unimplemented native properties,
 * but usually just causes a lot of spam. */
//#define SWFDEC_WARN_MISSING_PROPERTIES

/*** SUPPORT FUNCTIONS ***/

#define swfdec_action_has_register(cx, i) \
  ((i) < (cx)->frame->n_registers)

static SwfdecMovie *
swfdec_action_get_target (SwfdecAsContext *context)
{
  SwfdecAsObject *target = context->frame->target;

  if (target == NULL) {
    target = SWFDEC_AS_OBJECT (context->frame);
    while (SWFDEC_IS_AS_FRAME (target))
      target = SWFDEC_AS_FRAME (target)->scope;
  }
  if (!SWFDEC_IS_MOVIE (target)) {
    SWFDEC_ERROR ("no valid target");
    return NULL;
  }
  return SWFDEC_MOVIE (target);
}

/*** ALL THE ACTION IS HERE ***/

static void
swfdec_action_stop (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  if (movie)
    movie->stopped = TRUE;
  else
    SWFDEC_ERROR ("no movie to stop");
}

static void
swfdec_action_play (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  if (movie)
    movie->stopped = FALSE;
  else
    SWFDEC_ERROR ("no movie to play");
}

static void
swfdec_action_next_frame (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  if (movie) {
    if (movie->frame + 1 < movie->n_frames) {
      swfdec_movie_goto (movie, movie->frame + 1);
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
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  if (movie) {
    if (movie->frame > 0) {
      swfdec_movie_goto (movie, movie->frame - 1);
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
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  guint frame;

  if (len != 2) {
    SWFDEC_ERROR ("GotoFrame action length invalid (is %u, should be 2", len);
    return;
  }
  frame = GUINT16_FROM_LE (*((guint16 *) data));
  if (movie) {
    swfdec_movie_goto (movie, frame);
    movie->stopped = TRUE;
  } else {
    SWFDEC_ERROR ("no movie to goto on");
  }
}

static void
swfdec_action_goto_label (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);

  if (!memchr (data, 0, len)) {
    SWFDEC_ERROR ("GotoLabel action does not specify a string");
    return;
  }

  if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
    int frame = swfdec_sprite_get_frame (SWFDEC_SPRITE_MOVIE (movie)->sprite, (const char *) data);
    if (frame == -1)
      return;
    swfdec_movie_goto (movie, frame);
    movie->stopped = TRUE;
  } else {
    SWFDEC_ERROR ("no movie to goto on");
  }
}

static int
swfdec_value_to_frame (SwfdecAsContext *cx, SwfdecMovie *movie, SwfdecAsValue *val)
{
  int frame;

  if (SWFDEC_AS_VALUE_IS_STRING (val)) {
    const char *name = SWFDEC_AS_VALUE_GET_STRING (val);
    double d;
    if (!SWFDEC_IS_SPRITE_MOVIE (movie))
      return -1;
    if (strchr (name, ':')) {
      SWFDEC_ERROR ("FIXME: handle targets");
    }
    /* treat valid encoded numbers as numbers, otherwise assume it's a frame label */
    d = swfdec_as_value_to_number (cx, val);
    if (isnan (d))
      frame = swfdec_sprite_get_frame (SWFDEC_SPRITE_MOVIE (movie)->sprite, name);
    else
      frame = d - 1;
  } else if (SWFDEC_AS_VALUE_IS_NUMBER (val)) {
    return (int) SWFDEC_AS_VALUE_GET_NUMBER (val) - 1;
  } else {
    SWFDEC_WARNING ("cannot convert value to frame number");
    /* FIXME: how do we treat undefined etc? */
    frame = -1;
  }
  return frame;
}

static void
swfdec_action_goto_frame2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  guint bias;
  gboolean play;
  SwfdecAsValue *val;
  SwfdecMovie *movie;

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
  movie = swfdec_action_get_target (cx);
  /* now set it */
  if (movie) {
    int frame = swfdec_value_to_frame (cx, movie, val);
    if (frame >= 0) {
      frame += bias;
      frame = CLAMP (frame, 0, (int) movie->n_frames - 1);
      swfdec_movie_goto (movie, frame);
      movie->stopped = !play;
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
  SwfdecMovie *movie;

  if (len != 1) {
    SWFDEC_ERROR ("WaitForFrame2 needs a 1-byte data");
    return JS_FALSE;
  }
  val = cx->fp->sp[-1];
  cx->fp->sp--;
  movie = swfdec_action_get_target (cx);
  if (movie) {
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
    if (loaded < (guint) frame)
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
  SwfdecMovie *movie;
  guint frame, jump, loaded;

  if (len != 3) {
    SWFDEC_ERROR ("WaitForFrame action length invalid (is %u, should be 3", len);
    return;
  }
  movie = swfdec_action_get_target (cx);
  if (movie == NULL) {
    SWFDEC_ERROR ("no movie for WaitForFrame");
    return;
  }

  frame = GUINT16_FROM_LE (*((guint16 *) data));
  jump = data[2];
  if (SWFDEC_IS_ROOT_MOVIE (movie)) {
    SwfdecDecoder *dec = SWFDEC_ROOT_MOVIE (movie)->decoder;
    loaded = dec->frames_loaded;
    g_assert (loaded <= movie->n_frames);
  } else {
    loaded = movie->n_frames;
  }
  if (loaded < frame)
    swfdec_script_skip_actions (cx, jump);
}

static void
swfdec_action_constant_pool (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecConstantPool *pool;
  SwfdecAsFrame *frame;

  frame = cx->frame;
  pool = swfdec_constant_pool_new_from_action (data, len);
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
    swfdec_as_stack_ensure_left (stack, 1);
    switch (type) {
      case 0: /* string */
	{
	  const char *s = swfdec_bits_skip_string (&bits);
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
	    swfdec_bits_get_u32 (&bits));
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

#define CONSTANT_INDEX 39
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
    obj = cx->frame->scope;
  } else if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    SWFDEC_WARNING ("not an object, can't GetProperty");
    goto out;
  }
  swfdec_as_object_get_variable (obj, SWFDEC_AS_STR_CONSTANT (CONSTANT_INDEX + id),
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
  val = swfdec_as_stack_peek (cx->frame->stack, 1);
  swfdec_as_interpret_eval (cx, NULL, val);
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (val)) {
    obj = cx->frame->var_object;
  } else if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
    obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
  } else {
    SWFDEC_WARNING ("not an object, can't get SetProperty");
    goto out;
  }
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_CONSTANT (CONSTANT_INDEX + id),
      swfdec_as_stack_peek (cx->frame->stack, 3));
out:
  swfdec_as_stack_pop_n (cx->frame->stack, 3);
}

static void
swfdec_action_get_member (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  /* FIXME: do we need a "convert to object" function here? */
  if (SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (cx->frame->stack, 2))) {
    const char *name;
    SwfdecAsObject *o = SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (cx->frame->stack, 2));
    name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
    swfdec_as_object_get_variable (o, name, swfdec_as_stack_peek (cx->frame->stack, 2));
#ifdef SWFDEC_WARN_MISSING_PROPERTIES
    if (SWFDEC_AS_VALUE_IS_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 2))) {
	SWFDEC_WARNING ("no variable named %s:%s", G_OBJECT_TYPE_NAME (o), s);
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
  const char *s;

  s = swfdec_as_value_to_printable (cx, swfdec_as_stack_pop (cx->frame->stack));
  swfdec_as_context_trace (cx, s);
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

  if (!SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (frame->stack, 1)) ||
      !SWFDEC_AS_VALUE_IS_OBJECT (swfdec_as_stack_peek (frame->stack, 2)))
    goto error;
  fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (frame->stack, 1));
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    goto error;
  thisp = SWFDEC_AS_VALUE_GET_OBJECT (swfdec_as_stack_peek (frame->stack, 2));
  swfdec_as_stack_pop_n (frame->stack, 2);
  /* sanitize argument count */
  if (n_args > swfdec_as_stack_get_size (frame->stack))
    n_args = swfdec_as_stack_get_size (frame->stack);
  /* push return value on stack */
  swfdec_as_stack_push (frame->stack);
  /* swap arguments and return value on the stack */
  /* FIXME: can we somehow keep this order please, it might be interesting for debuggers */
  for (i = 0; i < (n_args + 1) / 2; i++) {
    SwfdecAsValue tmp = *swfdec_as_stack_peek (frame->stack, i + 1);
    *swfdec_as_stack_peek (frame->stack, i + 1) = *swfdec_as_stack_peek (frame->stack, n_args + 1 - i);
    *swfdec_as_stack_peek (frame->stack, n_args + 1 - i) = tmp;
  }
  if (n_args)
    swfdec_as_stack_pop_n (frame->stack, n_args);
  swfdec_as_function_call (fun, thisp, n_args, swfdec_as_stack_peek (frame->stack, 0), 
      swfdec_as_stack_peek (frame->stack, 1));
  return TRUE;

error:
  n_args += 2;
  if (n_args > swfdec_as_stack_get_size (frame->stack))
    n_args = swfdec_as_stack_get_size (frame->stack);
  swfdec_as_stack_pop_n (frame->stack, n_args);
  return FALSE;
}

static void
swfdec_action_call_function (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsFrame *frame = cx->frame;
  SwfdecAsObject *obj;
  guint n_args;
  const char *name;
  
  swfdec_as_stack_ensure_size (frame->stack, 2);
  n_args = swfdec_as_value_to_integer (cx, swfdec_as_stack_peek (frame->stack, 2));
  name = swfdec_as_value_to_string (cx, swfdec_as_stack_peek (frame->stack, 1));
  obj = swfdec_as_frame_find_variable (frame, name);
  if (obj) {
    SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (frame->stack, 2), obj);
    swfdec_as_object_get_variable (obj, name, swfdec_as_stack_peek (frame->stack, 1));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (swfdec_as_stack_peek (frame->stack, 2));
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (frame->stack, 1));
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
  /* FIXME: this is a hack for constructtors calling super - is this correct? */
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (val)) {
    SWFDEC_AS_VALUE_SET_STRING (val, SWFDEC_AS_STR_EMPTY);
  }
  if (obj) {
    SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (frame->stack, 3), obj);
    if (SWFDEC_AS_VALUE_IS_STRING (val) && 
	SWFDEC_AS_VALUE_GET_STRING (val) == SWFDEC_AS_STR_EMPTY) {
      SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_peek (frame->stack, 2), obj);
    } else {
      name = swfdec_as_value_to_string (cx, val);
      swfdec_as_object_get_variable (obj, name, swfdec_as_stack_peek (frame->stack, 2));
    }
  } else {
    SWFDEC_AS_VALUE_SET_NULL (swfdec_as_stack_peek (frame->stack, 3));
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (frame->stack, 2));
  }
  swfdec_as_stack_pop (frame->stack);
  if (!swfdec_action_call (cx, n_args)) {
    SWFDEC_ERROR ("no function named %s", name ? name : "unknown");
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
    case 0x0a:
      l = l + r;
      break;
    case 0x0b:
      l = l - r;
      break;
    case 0x0c:
      l = l * r;
      break;
    case 0x0d:
      if (cx->version < 5) {
	if (r == 0) {
	  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx->frame->stack, 1), SWFDEC_AS_STR_HASH_ERROR);
	  return;
	}
      } else if (cx->version < 7) {
	if (isnan (r))
	  r = 0;
      }
      l = l / r;
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
  SwfdecAsValue *rval, *lval;

  rval = swfdec_as_stack_peek (cx->frame->stack, 1);
  lval = swfdec_as_stack_peek (cx->frame->stack, 2);
  if (SWFDEC_AS_VALUE_IS_STRING (lval) || SWFDEC_AS_VALUE_IS_STRING (rval)) {
    const char *l, *r;
    char *ret;
    r = swfdec_as_value_to_string (cx, rval);
    l = swfdec_as_value_to_string (cx, lval);
    ret = g_strconcat (l, r, NULL);
    l = swfdec_as_context_get_string (cx, ret);
    swfdec_as_stack_pop (cx->frame->stack);
    SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_peek (cx->frame->stack, 1), l);
  } else {
    double d, d2;
    d2 = swfdec_as_value_to_number (cx, rval);
    d = swfdec_as_value_to_number (cx, lval);
    d += d2;
    swfdec_as_stack_pop (cx->frame->stack);
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1), d);
  }
}

static void
swfdec_action_new_comparison_6 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double d, d2;

  d2 = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  d = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 2));
  swfdec_as_stack_pop (cx->frame->stack);
  if (action == 0x48)
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), d < d2);
  else 
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), d > d2);
}

static void
swfdec_action_new_comparison_7 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *lval, *rval;

  rval = swfdec_as_stack_peek (cx->frame->stack, 1);
  lval = swfdec_as_stack_peek (cx->frame->stack, 2);
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (rval) || SWFDEC_AS_VALUE_IS_UNDEFINED (lval)) {
    swfdec_as_stack_pop (cx->frame->stack);
    SWFDEC_AS_VALUE_SET_UNDEFINED (swfdec_as_stack_peek (cx->frame->stack, 1));
  } else if (SWFDEC_AS_VALUE_IS_STRING (rval) || SWFDEC_AS_VALUE_IS_STRING (lval)) {
    int comp = strcmp (SWFDEC_AS_VALUE_GET_STRING (rval), SWFDEC_AS_VALUE_GET_STRING (lval));
    swfdec_as_stack_pop (cx->frame->stack);
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), action == 0x48 ? comp < 0 : comp > 0);
  } else {
    double d, d2;
    d2 = swfdec_as_value_to_number (cx, rval);
    d = swfdec_as_value_to_number (cx, lval);
    swfdec_as_stack_pop (cx->frame->stack);
    if (action == 0x48)
      SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), d < d2);
    else 
      SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), d > d2);
  }
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
  double d;

  d = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), d == 0 ? TRUE : FALSE);
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
  double d;

  if (len != 2) {
    SWFDEC_ERROR ("Jump action length invalid (is %u, should be 2", len);
    return;
  }
  d = swfdec_as_value_to_number (cx, swfdec_as_stack_pop (cx->frame->stack));
  if (d != 0)
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

#if 0
static void
swfdec_action_get_url (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie;
  SwfdecBits bits;
  const char *url, *target;

  swfdec_bits_init_data (&bits, data, len);
  url = swfdec_bits_skip_string (&bits);
  target = swfdec_bits_skip_string (&bits);
  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("leftover bytes in GetURL action");
  }
  movie = swfdec_action_get_target (cx);
  if (movie)
    swfdec_root_movie_load (SWFDEC_ROOT_MOVIE (movie->root), url, target);
  else
    SWFDEC_WARNING ("no movie to load");
  return JS_TRUE;
}

static void
swfdec_action_get_url2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *target, *url;
  guint method;
  SwfdecMovie *movie;

  if (len != 1) {
    SWFDEC_ERROR ("GetURL2 requires 1 byte of data, not %u", len);
    return JS_FALSE;
  }
  target = swfdec_js_to_string (cx, cx->fp->sp[-1]);
  url = swfdec_js_to_string (cx, cx->fp->sp[-2]);
  if (target == NULL || url == NULL)
    return JS_FALSE;
  method = data[0] >> 6;
  if (method == 3) {
    SWFDEC_ERROR ("GetURL method 3 invalid");
    method = 0;
  }
  if (method) {
    SWFDEC_ERROR ("FIXME: implement encoding variables using %s", method == 1 ? "GET" : "POST");
  }
  if (data[0] & 2) {
    SWFDEC_ERROR ("FIXME: implement LoadTarget");
  }
  if (data[0] & 1) {
    SWFDEC_ERROR ("FIXME: implement LoadVariables");
  }
  movie = swfdec_action_get_target (cx);
  if (movie)
    swfdec_root_movie_load (SWFDEC_ROOT_MOVIE (movie->root), url, target);
  else
    SWFDEC_WARNING ("no movie to load");
  cx->fp->sp -= 2;
  return JS_TRUE;
}

static void
swfdec_action_string_add (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  JSString *lval, *rval;

  rval = JS_ValueToString (cx, cx->fp->sp[-1]);
  lval = JS_ValueToString (cx, cx->fp->sp[-2]);
  if (lval == NULL || rval == NULL)
    return FALSE;
  lval = JS_ConcatStrings (cx, lval, rval);
  if (lval == NULL)
    return FALSE;
  cx->fp->sp--;
  cx->fp->sp[-1] = STRING_TO_JSVAL (lval);
  return JS_TRUE;
}
#endif

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
  r = swfdec_as_value_to_number (cx, swfdec_as_stack_peek (cx->frame->stack, 1));
  switch (action) {
    case 0x0e:
      cond = l == r;
      break;
    case 0x0f:
      cond = l < r;
      break;
    default: 
      g_assert_not_reached ();
      return;
  }
  swfdec_as_stack_pop (cx->frame->stack);
  if (cx->version < 5) {
    SWFDEC_AS_VALUE_SET_NUMBER (swfdec_as_stack_peek (cx->frame->stack, 1), cond ? 1 : 0);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (swfdec_as_stack_peek (cx->frame->stack, 1), cond);
  }
}

static void
swfdec_action_equals2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue *rval, *lval;
  SwfdecAsType ltype, rtype;
  gboolean cond;

  rval = swfdec_as_stack_peek (cx->frame->stack, 1);
  lval = swfdec_as_stack_peek (cx->frame->stack, 2);
  ltype = lval->type;
  rtype = rval->type;
  if (ltype == rtype) {
    switch (ltype) {
      case SWFDEC_AS_TYPE_UNDEFINED:
      case SWFDEC_AS_TYPE_NULL:
	cond = TRUE;
	break;
      case SWFDEC_AS_TYPE_BOOLEAN:
	cond = SWFDEC_AS_VALUE_GET_BOOLEAN (lval) == SWFDEC_AS_VALUE_GET_BOOLEAN (rval);
	break;
      case SWFDEC_AS_TYPE_NUMBER:
	cond = SWFDEC_AS_VALUE_GET_NUMBER (lval) == SWFDEC_AS_VALUE_GET_NUMBER (rval);
	break;
      case SWFDEC_AS_TYPE_STRING:
	cond = SWFDEC_AS_VALUE_GET_STRING (lval) == SWFDEC_AS_VALUE_GET_STRING (rval);
	break;
      case SWFDEC_AS_TYPE_OBJECT:
	cond = SWFDEC_AS_VALUE_GET_OBJECT (lval) == SWFDEC_AS_VALUE_GET_OBJECT (rval);
	break;
      default:
	g_assert_not_reached ();
	cond = FALSE;
	break;
    }
  } else {
    if (ltype == SWFDEC_AS_TYPE_UNDEFINED || ltype == SWFDEC_AS_TYPE_NULL) {
      cond = (rtype == SWFDEC_AS_TYPE_UNDEFINED || rtype == SWFDEC_AS_TYPE_NULL);
    } else if (rtype == SWFDEC_AS_TYPE_UNDEFINED || rtype == SWFDEC_AS_TYPE_NULL) {
      cond = FALSE;
    } else {
      SWFDEC_WARNING ("FIXME: test equality operations between non-equal types");
      double l, r;
      r = swfdec_as_value_to_number (cx, rval);
      l = swfdec_as_value_to_number (cx, lval);
      cond = r == l;
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
    return;
  } 
  /* FIXME: allow non-movieclips as targets? */
  swfdec_as_frame_set_target (cx->frame, SWFDEC_AS_VALUE_GET_OBJECT (val));
}

#if 0
static void
swfdec_action_start_drag (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  guint n_args = 1;

  if (!swfdec_script_ensure_stack (cx, 3))
    return JS_FALSE;
  if (!swfdec_eval_jsval (cx, NULL, &fp->sp[-1]))
    return JS_FALSE;
  if (swfdec_value_to_number (cx, fp->sp[-3])) {
    jsval tmp;
    if (!swfdec_script_ensure_stack (cx, 7))
      return JS_FALSE;
    n_args = 5;
    /* yay for order */
    tmp = fp->sp[-4];
    fp->sp[-4] = fp->sp[-7];
    fp->sp[-7] = tmp;
    tmp = fp->sp[-6];
    fp->sp[-6] = fp->sp[-5];
    fp->sp[-5] = tmp;
  }
  if (!JSVAL_IS_OBJECT (fp->sp[-1]) || JSVAL_IS_NULL (fp->sp[-1])) {
    fp->sp -= n_args + 2;
    return JS_TRUE;
  }
  fp->sp[-3] = fp->sp[-2];
  fp->sp[-2] = fp->sp[-1];
  if (!JS_GetProperty (cx, JSVAL_TO_OBJECT (fp->sp[-2]), "startDrag", &fp->sp[-1]))
    return JS_FALSE;
  swfdec_action_call (cx, n_args, 0);
  fp->sp--;
  return JS_TRUE;
}

static void
swfdec_action_end_drag (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  swfdec_player_set_drag_movie (player, NULL, FALSE, NULL);
  return JS_TRUE;
}

static void
swfdec_action_stop_sounds (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);

  swfdec_player_stop_all_sounds (player);
  return JS_TRUE;
}

static void
swfdec_action_new_object (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  jsval constructor;
  JSObject *object;
  guint n_args;
  const char *name;

  if (!swfdec_script_ensure_stack (cx, 2))
    return JS_FALSE;
  constructor = fp->sp[-1];
  name = swfdec_eval_jsval (cx, NULL, &constructor);
  if (name == NULL)
    return JS_FALSE;
  if (!JS_ValueToECMAUint32 (cx, fp->sp[-2], &n_args))
    return JS_FALSE;
  if (constructor == JSVAL_VOID) {
    SWFDEC_WARNING ("no constructor for %s", name);
  }
  fp->sp[-1] = constructor;

  if (!swfdec_js_construct_object (cx, NULL, constructor, &object))
    return JS_FALSE;
  if (object == NULL)
    goto fail;
  fp->sp[-2] = OBJECT_TO_JSVAL (object);
  if (!swfdec_action_call (cx, n_args, JSINVOKE_CONSTRUCT))
    return JS_FALSE;
  fp->sp[-1] = OBJECT_TO_JSVAL (object);
  return JS_TRUE;

fail:
  fp->sp -= n_args + 1;
  fp->sp[-1] = JSVAL_VOID;
  return JS_TRUE;
}

static void
swfdec_action_new_method (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  const char *s;
  guint32 n_args;
  JSObject *object;
  jsval constructor;
  
  if (!swfdec_script_ensure_stack (cx, 3))
    return JS_FALSE;
  s = swfdec_js_to_string (cx, fp->sp[-1]);
  if (s == NULL)
    return JS_FALSE;
  if (!JS_ValueToECMAUint32 (cx, fp->sp[-3], &n_args))
    return JS_FALSE;
  
  if (!JS_ValueToObject (cx, fp->sp[-2], &object))
    return JS_FALSE;
  if (object == NULL)
    goto fail;
  if (s[0] == '\0') {
    constructor = OBJECT_TO_JSVAL (object);
  } else {
    if (!JS_GetProperty (cx, object, s, &constructor))
      return JS_FALSE;
    if (!JSVAL_IS_OBJECT (constructor)) {
      SWFDEC_WARNING ("%s:%s is not a function", JS_GetClass (object)->name, s);
    }
  }
  fp->sp[-1] = OBJECT_TO_JSVAL (constructor);
  if (!swfdec_js_construct_object (cx, NULL, constructor, &object))
    return JS_FALSE;
  if (object == NULL)
    goto fail;
  fp->sp[-2] = OBJECT_TO_JSVAL (object);
  if (!swfdec_action_call (cx, n_args, JSINVOKE_CONSTRUCT))
    return JS_FALSE;
  fp->sp[-1] = OBJECT_TO_JSVAL (object);
  return JS_TRUE;

fail:
  fp->sp -= 2 + n_args;
  fp->sp[-1] = JSVAL_VOID;
  return JS_TRUE;
}

static void
swfdec_action_init_object (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  JSObject *object;
  guint n_args;
  gulong i;

  if (!JS_ValueToECMAUint32 (cx, fp->sp[-1], &n_args))
    return JS_FALSE;
  if (!swfdec_script_ensure_stack (cx, 2 * n_args + 1))
    return JS_FALSE;

  object = JS_NewObject (cx, &js_ObjectClass, NULL, NULL);
  if (object == NULL)
    return JS_FALSE;
  for (i = 0; i < n_args; i++) {
    const char *s = swfdec_js_to_string (cx, fp->sp[-3 - 2 * i]);
    if (s == NULL)
      return JS_FALSE;
    if (!JS_SetProperty (cx, object, s, &fp->sp[-2 - 2 * i]))
      return JS_FALSE;
  }
  fp->sp -= 2 * n_args;
  fp->sp[-1] = OBJECT_TO_JSVAL (object);
  return JS_TRUE;
}

static void
swfdec_action_init_array (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  JSObject *array;
  int i, j;
  guint n_items;

  if (!JS_ValueToECMAUint32 (cx, fp->sp[-1], &n_items))
    return JS_FALSE;
  if (!swfdec_script_ensure_stack (cx, n_items + 1))
    return JS_FALSE;

  /* items are the wrong order on the stack */
  j = - 1 - n_items;
  for (i = - 2; i > j; i--, j++) {
    jsval tmp = fp->sp[i];
    fp->sp[i] = fp->sp[j];
    fp->sp[j] = tmp;
  }
  array = JS_NewArrayObject (cx, n_items, fp->sp - n_items - 1);
  if (array == NULL)
    return JS_FALSE;
  fp->sp -= n_items;
  fp->sp[-1] = OBJECT_TO_JSVAL (array);
  return JS_TRUE;
}
#endif

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
  function_name = swfdec_bits_skip_string (&bits);
  if (function_name == NULL) {
    SWFDEC_ERROR ("could not parse function name");
    return;
  }
  fun = swfdec_as_function_new (frame->scope);
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
      args[i].name = swfdec_bits_get_string (&bits);
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
  fun->script = script;
  swfdec_script_add_to_context (script, cx);
  /* attach the function */
  if (*function_name == '\0') {
    swfdec_as_stack_ensure_left (frame->stack, 1);
    SWFDEC_AS_VALUE_SET_OBJECT (swfdec_as_stack_push (frame->stack), SWFDEC_AS_OBJECT (fun));
  } else {
    SwfdecAsValue funval;
    swfdec_as_object_root (SWFDEC_AS_OBJECT (fun));
    /* FIXME: really varobj? Not eval or sth like that? */
    function_name = swfdec_as_context_get_string (cx, function_name);
    SWFDEC_AS_VALUE_SET_OBJECT (&funval, SWFDEC_AS_OBJECT (fun));
    swfdec_as_object_set_variable (frame->var_object, function_name, &funval);
    swfdec_as_object_unroot (SWFDEC_AS_OBJECT (fun));
  }

  /* update current context */
  frame->pc += 3 + len + size;
}

#if 0
static void
swfdec_action_bitwise (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  guint32 a, b;
  double d;

  if (!JS_ValueToECMAUint32 (cx, cx->fp->sp[-1], &a) ||
      !JS_ValueToECMAUint32 (cx, cx->fp->sp[-2], &b))
    return JS_FALSE;

  switch (action) {
    case 0x60:
      d = (int) (a & b);
      break;
    case 0x61:
      d = (int) (a | b);
      break;
    case 0x62:
      d = (int) (a ^ b);
      break;
    default:
      g_assert_not_reached ();
      return JS_FALSE;
  }

  cx->fp->sp--;
  return JS_NewNumberValue (cx, d, &cx->fp->sp[-1]);
}

static void
swfdec_action_shift (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  guint32 amount, value;
  double d;

  if (!JS_ValueToECMAUint32 (cx, cx->fp->sp[-1], &amount) ||
      !JS_ValueToECMAUint32 (cx, cx->fp->sp[-2], &value))
    return JS_FALSE;

  amount &= 31;
  switch (action) {
    case 0x63:
      d = value << amount;
      break;
    case 0x64:
      d = ((gint) value) >> amount;
      break;
    case 0x65:
      d = ((guint) value) >> amount;
      break;
    default:
      g_assert_not_reached ();
      return JS_FALSE;
  }

  cx->fp->sp--;
  return JS_NewNumberValue (cx, d, &cx->fp->sp[-1]);
}

static void
swfdec_action_to_integer (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double d = swfdec_value_to_number (cx, cx->fp->sp[-1]);

  return JS_NewNumberValue (cx, (int) d, &cx->fp->sp[-1]);
}

static void
swfdec_action_target_path (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_scriptable_from_jsval (cx, cx->fp->sp[-1], SWFDEC_TYPE_MOVIE);

  if (movie == NULL) {
    cx->fp->sp[-1] = JSVAL_VOID;
  } else {
    char *s = swfdec_movie_get_path (movie);
    JSString *string = JS_NewStringCopyZ (cx, s);
    g_free (s);
    if (string == NULL)
      return JS_FALSE;
    cx->fp->sp[-1] = STRING_TO_JSVAL (string);
  }
  return JS_TRUE;
}

static void
swfdec_action_define_local (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *name;

  g_assert (cx->fp->scopeChain != NULL);
  name = swfdec_js_to_string (cx, cx->fp->sp[-2]);
  if (name == NULL)
    return JS_FALSE;
  if (!JS_SetProperty (cx, cx->fp->scopeChain, name, &cx->fp->sp[-1]))
    return JS_FALSE;
  cx->fp->sp -= 2;
  return JS_TRUE;
}

static void
swfdec_action_define_local2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *name;
  jsval val = JSVAL_VOID;

  g_assert (cx->fp->scopeChain != NULL);
  name = swfdec_js_to_string (cx, cx->fp->sp[-1]);
  if (name == NULL)
    return JS_FALSE;
  if (!JS_SetProperty (cx, cx->fp->scopeChain, name, &val))
    return JS_FALSE;
  cx->fp->sp--;
  return JS_TRUE;
}
#endif

static void
swfdec_action_return (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  cx->frame->return_value = swfdec_as_stack_pop (cx->frame->stack);
  swfdec_as_context_return (cx);
}

#if 0
static void
swfdec_action_delete (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *name;
  
  cx->fp->sp -= 2;
  name = swfdec_js_to_string (cx, cx->fp->sp[1]);
  if (name == NULL)
    return JS_FALSE;
  if (!JSVAL_IS_OBJECT (cx->fp->sp[0]))
    return JS_TRUE;
  return JS_DeleteProperty (cx, JSVAL_TO_OBJECT (cx->fp->sp[0]), name);
}

static void
swfdec_action_delete2 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  const char *name;
  JSObject *obj, *pobj;
  JSProperty *prop;
  JSAtom *atom;
  
  cx->fp->sp -= 1;
  name = swfdec_js_to_string (cx, cx->fp->sp[1]);
  if (name == NULL)
    return JS_FALSE;
  if (!(atom = js_Atomize (cx, name, strlen (name), 0)) ||
      !js_FindProperty (cx, (jsid) atom, &obj, &pobj, &prop))
    return JS_FALSE;
  if (!pobj)
    return JS_TRUE;
  return JS_DeleteProperty (cx, pobj, name);
}
#endif

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

#if 0
static void
swfdec_action_modulo_5 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double x, y;

  x = swfdec_value_to_number (cx, cx->fp->sp[-1]);
  y = swfdec_value_to_number (cx, cx->fp->sp[-2]);
  cx->fp->sp--;
  errno = 0;
  x = fmod (x, y);
  if (errno != 0) {
    cx->fp->sp[-1] = DOUBLE_TO_JSVAL (cx->runtime->jsNaN);
    return JS_TRUE;
  } else {
    return JS_NewNumberValue (cx, x, &cx->fp->sp[-1]);
  }
}

static void
swfdec_action_modulo_7 (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  double x, y;

  if (!swfdec_value_to_number_7 (cx, cx->fp->sp[-1], &x) ||
      !swfdec_value_to_number_7 (cx, cx->fp->sp[-2], &y))
    return JS_FALSE;
  cx->fp->sp--;
  errno = 0;
  x = fmod (x, y);
  if (errno != 0) {
    cx->fp->sp[-1] = DOUBLE_TO_JSVAL (cx->runtime->jsNaN);
    return JS_TRUE;
  } else {
    return JS_NewNumberValue (cx, x, &cx->fp->sp[-1]);
  }
}
#endif

static void
swfdec_action_swap (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecAsValue val;
  val = *swfdec_as_stack_peek (cx->frame->stack, 1);
  *swfdec_as_stack_peek (cx->frame->stack, 1) = *swfdec_as_stack_peek (cx->frame->stack, 2);
  *swfdec_as_stack_peek (cx->frame->stack, 2) = val;
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
      type = SWFDEC_AS_STR_NUMBER;
      break;
    case SWFDEC_AS_TYPE_BOOLEAN:
      type = SWFDEC_AS_STR_BOOLEAN;
      break;
    case SWFDEC_AS_TYPE_STRING:
      type = SWFDEC_AS_STR_STRING;
      break;
    case SWFDEC_AS_TYPE_UNDEFINED:
      type = SWFDEC_AS_STR_UNDEFINED;
      break;
    case SWFDEC_AS_TYPE_NULL:
      type = SWFDEC_AS_STR_NULL;
      break;
    case SWFDEC_AS_TYPE_OBJECT:
      {
	SwfdecAsObject *obj = SWFDEC_AS_VALUE_GET_OBJECT (val);
	if (SWFDEC_IS_MOVIE (obj)) {
	  type = SWFDEC_AS_STR_MOVIECLIP;
	} else if (SWFDEC_IS_AS_FUNCTION (obj)) {
	  type = SWFDEC_AS_STR_FUNCTION;
	} else {
	  type = SWFDEC_AS_STR_OBJECT;
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

#if 0
static void
swfdec_action_get_time (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);

  *cx->fp->sp++ = INT_TO_JSVAL ((int) SWFDEC_TICKS_TO_MSECS (player->time));
  return JS_TRUE;
}

static void
swfdec_action_extends (SwfdecAsContext *cx, guint action, const guint8 *data, guint len)
{
  jsval superclass, subclass, proto;
  JSObject *prototype;

  superclass = cx->fp->sp[-1];
  subclass = cx->fp->sp[-2];
  cx->fp->sp -= 2;
  if (!JSVAL_IS_OBJECT (superclass) || superclass == JSVAL_NULL ||
      !JSVAL_IS_OBJECT (subclass) || subclass == JSVAL_NULL) {
    SWFDEC_ERROR ("superclass or subclass aren't objects");
    return JS_TRUE;
  }
  if (!JS_GetProperty (cx, JSVAL_TO_OBJECT (superclass), "prototype", &proto) ||
      !JSVAL_IS_OBJECT (proto))
    return JS_FALSE;
  prototype = JS_NewObject (cx, NULL, JSVAL_TO_OBJECT (proto), NULL);
  if (prototype == NULL)
    return JS_FALSE;
  proto = OBJECT_TO_JSVAL (prototype);
  if (!JS_SetProperty (cx, prototype, "__constructor__", &superclass) ||
      !JS_SetProperty (cx, JSVAL_TO_OBJECT (subclass), "prototype", &proto))
    return JS_FALSE;
  return JS_TRUE;
}
#endif

static gboolean
swfdec_action_do_enumerate (SwfdecAsObject *object, const char *val,
    SwfdecAsVariable *var, gpointer stackp)
{
  SwfdecAsStack *stack = stackp;

  if (var->flags & SWFDEC_AS_VARIABLE_DONT_ENUM)
    return TRUE;
  swfdec_as_stack_ensure_left (stack, 1);
  SWFDEC_AS_VALUE_SET_STRING (swfdec_as_stack_push (stack), val);
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

/*** PRINT FUNCTIONS ***/

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

#if 0
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
    SWFDEC_ERROR ("FIXME: implement encoding variables using %s", method == 1 ? "GET" : "POST");
  }
  return g_strdup_printf ("GetURL2%s%s%s", method == 0 ? "" : (method == 1 ? " GET" : " POST"),
      data[0] & 2 ? " LoadTarget" : "", data[0] & 1 ? " LoadVariables" : "");
}

static char *
swfdec_action_print_get_url (guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  const char *url, *target;

  swfdec_bits_init_data (&bits, data, len);
  url = swfdec_bits_skip_string (&bits);
  target = swfdec_bits_skip_string (&bits);
  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("leftover bytes in GetURL action");
  }
  return g_strdup_printf ("GetURL %s %s", url, target);
}
#endif

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
	  const char *s = swfdec_bits_skip_string (&bits);
	  if (!s) {
	    g_string_free (string, TRUE);
	    return NULL;
	  }
	  g_string_append_c (string, '"');
	  g_string_append (string, s);
	  g_string_append_c (string, '"');
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

  pool = swfdec_constant_pool_new_from_action (data, len);
  if (pool == NULL)
    return NULL;
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
#if 0
  [0x09] = { "StopSounds", NULL, 0, 0, { swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds } },
#endif
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
  [SWFDEC_AS_ACTION_STRING_EQUALS] = { "StringEquals", NULL },
  [SWFDEC_AS_ACTION_STRING_LENGTH] = { "StringLength", NULL },
  [SWFDEC_AS_ACTION_STRING_EXTRACT] = { "StringExtract", NULL },
  [SWFDEC_AS_ACTION_POP] = { "Pop", NULL, 1, 0, { NULL, swfdec_action_pop, swfdec_action_pop, swfdec_action_pop, swfdec_action_pop } },
#if 0
  [0x18] = { "ToInteger", NULL, 1, 1, { NULL, swfdec_action_to_integer, swfdec_action_to_integer, swfdec_action_to_integer, swfdec_action_to_integer } },
#endif
  [SWFDEC_AS_ACTION_GET_VARIABLE] = { "GetVariable", NULL, 1, 1, { NULL, swfdec_action_get_variable, swfdec_action_get_variable, swfdec_action_get_variable, swfdec_action_get_variable } },
  [SWFDEC_AS_ACTION_SET_VARIABLE] = { "SetVariable", NULL, 2, 0, { NULL, swfdec_action_set_variable, swfdec_action_set_variable, swfdec_action_set_variable, swfdec_action_set_variable } },
  [SWFDEC_AS_ACTION_SET_TARGET2] = { "SetTarget2", NULL, 1, 0, { swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2 } },
#if 0
  [0x21] = { "StringAdd", NULL, 2, 1, { NULL, swfdec_action_string_add, swfdec_action_string_add, swfdec_action_string_add, swfdec_action_string_add } },
#endif
  [SWFDEC_AS_ACTION_GET_PROPERTY] = { "GetProperty", NULL, 2, 1, { NULL, swfdec_action_get_property, swfdec_action_get_property, swfdec_action_get_property, swfdec_action_get_property } },
  [SWFDEC_AS_ACTION_SET_PROPERTY] = { "SetProperty", NULL, 3, 0, { NULL, swfdec_action_set_property, swfdec_action_set_property, swfdec_action_set_property, swfdec_action_set_property } },
  [SWFDEC_AS_ACTION_CLONE_SPRITE] = { "CloneSprite", NULL },
  [SWFDEC_AS_ACTION_REMOVE_SPRITE] = { "RemoveSprite", NULL },
  [SWFDEC_AS_ACTION_TRACE] = { "Trace", NULL, 1, 0, { NULL, swfdec_action_trace, swfdec_action_trace, swfdec_action_trace, swfdec_action_trace } },
#if 0
  [0x27] = { "StartDrag", NULL, -1, 0, { NULL, swfdec_action_start_drag, swfdec_action_start_drag, swfdec_action_start_drag, swfdec_action_start_drag } },
  [0x28] = { "EndDrag", NULL, 0, 0, { NULL, swfdec_action_end_drag, swfdec_action_end_drag, swfdec_action_end_drag, swfdec_action_end_drag } },
#endif
  [SWFDEC_AS_ACTION_STRING_LESS] = { "StringLess", NULL },
  /* version 7 */
  [SWFDEC_AS_ACTION_THROW] = { "Throw", NULL },
  [SWFDEC_AS_ACTION_CAST] = { "Cast", NULL },
  [SWFDEC_AS_ACTION_IMPLEMENTS] = { "Implements", NULL },
  /* version 4 */
  [0x30] = { "RandomNumber", NULL, 1, 1, { NULL, swfdec_action_random_number, swfdec_action_random_number, swfdec_action_random_number, swfdec_action_random_number } },
  [SWFDEC_AS_ACTION_MB_STRING_LENGTH] = { "MBStringLength", NULL },
  [SWFDEC_AS_ACTION_CHAR_TO_ASCII] = { "CharToAscii", NULL },
  [SWFDEC_AS_ACTION_ASCII_TO_CHAR] = { "AsciiToChar", NULL },
#if 0
  [0x34] = { "GetTime", NULL, 0, 1, { NULL, swfdec_action_get_time, swfdec_action_get_time, swfdec_action_get_time, swfdec_action_get_time } },
#endif
  [SWFDEC_AS_ACTION_MB_STRING_EXTRACT] = { "MBStringExtract", NULL },
  [SWFDEC_AS_ACTION_MB_CHAR_TO_ASCII] = { "MBCharToAscii", NULL },
  [SWFDEC_AS_ACTION_MB_ASCII_TO_CHAR] = { "MBAsciiToChar", NULL },
  /* version 5 */
#if 0
  [0x3a] = { "Delete", NULL, 2, 0, { NULL, NULL, swfdec_action_delete, swfdec_action_delete, swfdec_action_delete } },
  [0x3b] = { "Delete2", NULL, 1, 0, { NULL, NULL, swfdec_action_delete2, swfdec_action_delete2, swfdec_action_delete2 } },
  [0x3c] = { "DefineLocal", NULL, 2, 0, { NULL, NULL, swfdec_action_define_local, swfdec_action_define_local, swfdec_action_define_local } },
#endif
  [SWFDEC_AS_ACTION_CALL_FUNCTION] = { "CallFunction", NULL, -1, 1, { NULL, NULL, swfdec_action_call_function, swfdec_action_call_function, swfdec_action_call_function } },
  [0x3e] = { "Return", NULL, 1, 0, { NULL, NULL, swfdec_action_return, swfdec_action_return, swfdec_action_return } },
#if 0
  [0x3f] = { "Modulo", NULL, 2, 1, { NULL, NULL, swfdec_action_modulo_5, swfdec_action_modulo_5, swfdec_action_modulo_7 } },
  [0x40] = { "NewObject", NULL, -1, 1, { NULL, NULL, swfdec_action_new_object, swfdec_action_new_object, swfdec_action_new_object } },
  [0x41] = { "DefineLocal2", NULL, 1, 0, { NULL, NULL, swfdec_action_define_local2, swfdec_action_define_local2, swfdec_action_define_local2 } },
  [0x42] = { "InitArray", NULL, -1, 1, { NULL, NULL, swfdec_action_init_array, swfdec_action_init_array, swfdec_action_init_array } },
  [0x43] = { "InitObject", NULL, -1, 1, { NULL, NULL, swfdec_action_init_object, swfdec_action_init_object, swfdec_action_init_object } },
#endif
  [SWFDEC_AS_ACTION_TYPE_OF] = { "TypeOf", NULL, 1, 1, { NULL, NULL, swfdec_action_type_of, swfdec_action_type_of, swfdec_action_type_of } },
#if 0
  [0x45] = { "TargetPath", NULL, 1, 1, { NULL, NULL, swfdec_action_target_path, swfdec_action_target_path, swfdec_action_target_path } },
#endif
  [SWFDEC_AS_ACTION_ENUMERATE] = { "Enumerate", NULL, 1, -1, { NULL, NULL, swfdec_action_enumerate, swfdec_action_enumerate, swfdec_action_enumerate } },
  [SWFDEC_AS_ACTION_ADD2] = { "Add2", NULL, 2, 1, { NULL, NULL, swfdec_action_add2, swfdec_action_add2, swfdec_action_add2 } },
  [SWFDEC_AS_ACTION_LESS2] = { "Less2", NULL, 2, 1, { NULL, NULL, swfdec_action_new_comparison_6, swfdec_action_new_comparison_6, swfdec_action_new_comparison_7 } },
  [SWFDEC_AS_ACTION_EQUALS2] = { "Equals2", NULL, 2, 1, { NULL, NULL, swfdec_action_equals2, swfdec_action_equals2, swfdec_action_equals2 } },
  [SWFDEC_AS_ACTION_TO_NUMBER] = { "ToNumber", NULL, 1, 1, { NULL, NULL, swfdec_action_to_number, swfdec_action_to_number, swfdec_action_to_number } },
  [SWFDEC_AS_ACTION_TO_STRING] = { "ToString", NULL, 1, 1, { NULL, NULL, swfdec_action_to_string, swfdec_action_to_string, swfdec_action_to_string } },
  [SWFDEC_AS_ACTION_PUSH_DUPLICATE] = { "PushDuplicate", NULL, 1, 2, { NULL, NULL, swfdec_action_push_duplicate, swfdec_action_push_duplicate, swfdec_action_push_duplicate } },
  [SWFDEC_AS_ACTION_SWAP] = { "Swap", NULL, 2, 2, { NULL, NULL, swfdec_action_swap, swfdec_action_swap, swfdec_action_swap } },
  [SWFDEC_AS_ACTION_GET_MEMBER] = { "GetMember", NULL, 2, 1, { NULL, swfdec_action_get_member, swfdec_action_get_member, swfdec_action_get_member, swfdec_action_get_member } },
  [SWFDEC_AS_ACTION_SET_MEMBER] = { "SetMember", NULL, 3, 0, { NULL, swfdec_action_set_member, swfdec_action_set_member, swfdec_action_set_member, swfdec_action_set_member } },
  [SWFDEC_AS_ACTION_INCREMENT] = { "Increment", NULL, 1, 1, { NULL, NULL, swfdec_action_increment, swfdec_action_increment, swfdec_action_increment } },
  [SWFDEC_AS_ACTION_DECREMENT] = { "Decrement", NULL, 1, 1, { NULL, NULL, swfdec_action_decrement, swfdec_action_decrement, swfdec_action_decrement } },
  [SWFDEC_AS_ACTION_CALL_METHOD] = { "CallMethod", NULL, -1, 1, { NULL, NULL, swfdec_action_call_method, swfdec_action_call_method, swfdec_action_call_method } },
#if 0
  [0x53] = { "NewMethod", NULL, -1, 1, { NULL, NULL, swfdec_action_new_method, swfdec_action_new_method, swfdec_action_new_method } },
  /* version 6 */
#endif
  [SWFDEC_AS_ACTION_INSTANCE_OF] = { "InstanceOf", NULL },
  [SWFDEC_AS_ACTION_ENUMERATE2] = { "Enumerate2", NULL, 1, -1, { NULL, NULL, NULL, swfdec_action_enumerate2, swfdec_action_enumerate2 } },
  /* version 5 */
#if 0
  [0x60] = { "BitAnd", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [0x61] = { "BitOr", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [0x62] = { "BitXor", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [0x63] = { "BitLShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
  [0x64] = { "BitRShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
  [0x65] = { "BitURShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
#endif
  /* version 6 */
  [SWFDEC_AS_ACTION_STRICT_EQUALS] = { "StrictEquals", NULL },
  [SWFDEC_AS_ACTION_GREATER] = { "Greater", NULL, 2, 1, { NULL, NULL, NULL, swfdec_action_new_comparison_6, swfdec_action_new_comparison_7 } },
  [SWFDEC_AS_ACTION_STRING_GREATER] = { "StringGreater", NULL },
  /* version 7 */
#if 0
  [0x69] = { "Extends", NULL, 2, 0, { NULL, NULL, NULL, NULL, swfdec_action_extends } },
#endif

  /* version 3 */
  [SWFDEC_AS_ACTION_GOTO_FRAME] = { "GotoFrame", swfdec_action_print_goto_frame, 0, 0, { swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame } },
#if 0
  [0x83] = { "GetURL", swfdec_action_print_get_url, 0, 0, { swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url } },
#endif
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
  [SWFDEC_AS_ACTION_WITH] = { "With", NULL },
  /* version 4 */
  [SWFDEC_AS_ACTION_PUSH] = { "Push", swfdec_action_print_push, 0, -1, { NULL, swfdec_action_push, swfdec_action_push, swfdec_action_push, swfdec_action_push } },
  [SWFDEC_AS_ACTION_JUMP] = { "Jump", swfdec_action_print_jump, 0, 0, { NULL, swfdec_action_jump, swfdec_action_jump, swfdec_action_jump, swfdec_action_jump } },
#if 0
  [0x9a] = { "GetURL2", swfdec_action_print_get_url2, 2, 0, { NULL, swfdec_action_get_url2, swfdec_action_get_url2, swfdec_action_get_url2, swfdec_action_get_url2 } },
#endif
  /* version 5 */
  [SWFDEC_AS_ACTION_DEFINE_FUNCTION] = { "DefineFunction", swfdec_action_print_define_function, 0, -1, { NULL, NULL, swfdec_action_define_function, swfdec_action_define_function, swfdec_action_define_function } },
  /* version 4 */
  [SWFDEC_AS_ACTION_IF] = { "If", swfdec_action_print_if, 1, 0, { NULL, swfdec_action_if, swfdec_action_if, swfdec_action_if, swfdec_action_if } },
  [SWFDEC_AS_ACTION_CALL] = { "Call", NULL },
  [SWFDEC_AS_ACTION_GOTO_FRAME2] = { "GotoFrame2", swfdec_action_print_goto_frame2, 1, 0, { NULL, swfdec_action_goto_frame2, swfdec_action_goto_frame2, swfdec_action_goto_frame2, swfdec_action_goto_frame2 } }
};

