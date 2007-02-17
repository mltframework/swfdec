/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_script.h"
#include "swfdec_debug.h"
#include "swfdec_debugger.h"
#include "swfdec_scriptable.h"
#include "js/jscntxt.h"
#include "js/jsinterp.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include "swfdec_decoder.h"
#include "swfdec_js.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"
#include "js/jsfun.h"
#include "js/jsscope.h"

/*** CONSTANT POOLS ***/

typedef GPtrArray SwfdecConstantPool;

static SwfdecConstantPool *
swfdec_constant_pool_new_from_action (const guint8 *data, guint len)
{
  guint8 *next;
  guint i, n;
  GPtrArray *pool;

  if (len < 2) {
    SWFDEC_ERROR ("constant pool too small");
    return NULL;
  }
  n = GUINT16_FROM_LE (*((guint16*) data));
  data += 2;
  len -= 2;
  pool = g_ptr_array_sized_new (n);
  g_ptr_array_set_size (pool, n);
  for (i = 0; i < n; i++) {
    next = memchr (data, 0, len);
    if (next == NULL) {
      SWFDEC_ERROR ("not enough strings available");
      g_ptr_array_free (pool, TRUE);
      return NULL;
    }
    next++;
    g_ptr_array_index (pool, i) = (gpointer) data;
    len -= next - data;
    data = next;
  }
  if (len != 0) {
    SWFDEC_WARNING ("constant pool didn't consume whole buffer (%u bytes leftover)", len);
  }
  return pool;
}

static guint
swfdec_constant_pool_size (SwfdecConstantPool *pool)
{
  return pool->len;
}

static const char *
swfdec_constant_pool_get (SwfdecConstantPool *pool, guint i)
{
  g_assert (i < pool->len);
  return g_ptr_array_index (pool, i);
}

static void
swfdec_constant_pool_free (SwfdecConstantPool *pool)
{
  g_ptr_array_free (pool, TRUE);
}

/* FIXME: this is a bit hacky */
static SwfdecBuffer *
swfdec_constant_pool_get_area (SwfdecScript *script, SwfdecConstantPool *pool)
{
  guint8 *start;
  SwfdecBuffer *buffer;
  guint len;

  if (pool->len == 0)
    return NULL;
  start = (guint8 *) g_ptr_array_index (pool, 0) - 5;
  buffer = script->buffer;
  if (start < buffer->data) {
    /* DefineFunction inside DefineFunction */
    g_assert (buffer->parent != NULL);
    buffer = buffer->parent;
    g_assert (start >= buffer->data);
  }
  g_assert (start + 3 < buffer->data + buffer->length);
  g_assert (*start == 0x88);
  len = 3 + (start[1] | start[2] << 8);
  g_assert (start + len < buffer->data + buffer->length);
  return swfdec_buffer_new_subbuffer (buffer, start - buffer->data, len);
}

/*** SUPPORT FUNCTIONS ***/

static gboolean
swfdec_action_has_register (JSContext *cx, guint i)
{
  if (cx->fp->fun == NULL)
    return i < 4;
  else
    return i < cx->fp->fun->nvars;
}

static SwfdecMovie *
swfdec_action_get_target (JSContext *cx)
{
  JSObject *object = cx->fp->scopeChain;
  object = OBJ_THIS_OBJECT (cx, object);
  return swfdec_scriptable_from_jsval (cx, OBJECT_TO_JSVAL (object), SWFDEC_TYPE_MOVIE);
}

static JSBool
swfdec_action_push_string (JSContext *cx, const char *s)
{
  JSString *string = JS_NewStringCopyZ (cx, s);
  if (string == NULL)
    return JS_FALSE;
  *cx->fp->sp++ = STRING_TO_JSVAL (string);
  return JS_TRUE;
}

static double
swfdec_action_to_number (JSContext *cx, jsval val)
{
  if (JSVAL_IS_INT (val)) {
    return JSVAL_TO_INT (val);
  } else if (JSVAL_IS_DOUBLE (val)) {
    return *JSVAL_TO_DOUBLE (val);
  } else if (JSVAL_IS_BOOLEAN (val)) {
    return JSVAL_TO_BOOLEAN (val);
  } else if (JSVAL_IS_STRING (val)) {
    double d;
    if (!JS_ValueToNumber (cx, val, &d))
      return 0;
    return isnan (d) ? 0 : d;
  } else if (JSVAL_IS_OBJECT(val) && (((SwfdecScript *) cx->fp->swf)->version >= 6)) {
    return JSVAL_IS_NULL (val) ? 0 : *cx->runtime->jsNaN;
  } else {
    return 0;
  }
}

static JSBool
swfdec_value_to_number_7 (JSContext *cx, jsval val, double *d)
{
  if (JSVAL_IS_OBJECT (val)) {
    *d = *cx->runtime->jsNaN;
    return JS_TRUE;
  } else {
    return JS_ValueToNumber (cx, val, d);
  }
}

/*** ALL THE ACTION IS HERE ***/

static JSBool
swfdec_action_stop (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  if (movie)
    movie->stopped = TRUE;
  else
    SWFDEC_ERROR ("no movie to stop");
  return JS_TRUE;
}

static JSBool
swfdec_action_play (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  if (movie)
    movie->stopped = FALSE;
  else
    SWFDEC_ERROR ("no movie to play");
  return JS_TRUE;
}

static JSBool
swfdec_action_next_frame (JSContext *cx, guint action, const guint8 *data, guint len)
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
  return JS_TRUE;
}

static JSBool
swfdec_action_previous_frame (JSContext *cx, guint action, const guint8 *data, guint len)
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
  return JS_TRUE;
}

static JSBool
swfdec_action_goto_frame (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  guint frame;

  if (len != 2) {
    SWFDEC_ERROR ("GotoFrame action length invalid (is %u, should be 2", len);
    return JS_FALSE;
  }
  frame = GUINT16_FROM_LE (*((guint16 *) data));
  if (movie) {
    swfdec_movie_goto (movie, frame);
    movie->stopped = TRUE;
  } else {
    SWFDEC_ERROR ("no movie to goto on");
  }
  return JS_TRUE;
}

static JSBool
swfdec_action_goto_label (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);

  if (!memchr (data, 0, len)) {
    SWFDEC_ERROR ("GotoLabel action does not specify a string");
    return JS_FALSE;
  }

  if (SWFDEC_IS_SPRITE_MOVIE (movie)) {
    int frame = swfdec_sprite_get_frame (SWFDEC_SPRITE_MOVIE (movie)->sprite, (const char *) data);
    if (frame == -1)
      return JS_TRUE;
    swfdec_movie_goto (movie, frame);
    movie->stopped = TRUE;
  } else {
    SWFDEC_ERROR ("no movie to goto on");
  }
  return JS_TRUE;
}

static int
swfdec_value_to_frame (JSContext *cx, SwfdecMovie *movie, jsval val)
{
  int frame;

  if (JSVAL_IS_STRING (val)) {
    const char *name = swfdec_js_to_string (cx, val);
    if (name == NULL ||
        !SWFDEC_IS_SPRITE_MOVIE (movie))
      return -1;
    if (strchr (name, ':')) {
      SWFDEC_ERROR ("FIXME: handle targets");
    }
    frame = swfdec_sprite_get_frame (SWFDEC_SPRITE_MOVIE (movie)->sprite, name);
  } else {
    /* FIXME: how do we treat undefined etc? */
    frame = swfdec_action_to_number (cx, val);
  }
  return frame;
}

static JSBool
swfdec_action_goto_frame2 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecBits bits;
  guint bias;
  gboolean play;
  jsval val;
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
  val = cx->fp->sp[-1];
  cx->fp->sp--;
  movie = swfdec_action_get_target (cx);
  /* now set it */
  if (movie) {
    int frame = swfdec_value_to_frame (cx, movie, val);
    if (frame < 0)
      return JS_TRUE;
    frame += bias;
    frame = CLAMP (frame, 0, (int) movie->n_frames - 1);
    swfdec_movie_goto (movie, frame);
    movie->stopped = !play;
  } else {
    SWFDEC_ERROR ("no movie to GotoFrame2 on");
  }
  return JS_TRUE;
}

static void
swfdec_script_skip_actions (JSContext *cx, guint jump)
{
  SwfdecScript *script = cx->fp->swf;
  guint8 *pc = cx->fp->pc;
  guint8 *endpc = script->buffer->data + script->buffer->length;

  /* jump instructions */
  g_assert (script);
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
    cx->fp->pc = pc;
}

static JSBool
swfdec_action_wait_for_frame2 (JSContext *cx, guint action, const guint8 *data, guint len)
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

static JSBool
swfdec_action_wait_for_frame (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  guint frame, jump, loaded;

  if (len != 3) {
    SWFDEC_ERROR ("WaitForFrame action length invalid (is %u, should be 3", len);
    return JS_TRUE;
  }
  if (movie == NULL) {
    SWFDEC_ERROR ("no movie for WaitForFrame");
    return JS_TRUE;
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
  return JS_TRUE;
}

static JSBool
swfdec_action_constant_pool (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecConstantPool *pool;

  pool = swfdec_constant_pool_new_from_action (data, len);
  if (pool == NULL)
    return JS_FALSE;
  if (cx->fp->constant_pool)
    swfdec_constant_pool_free (cx->fp->constant_pool);
  cx->fp->constant_pool = pool;
  return JS_TRUE;
}

static JSBool
swfdec_action_push (JSContext *cx, guint stackspace, const guint8 *data, guint len)
{
  /* FIXME: supply API for this */
  SwfdecBits bits;

  swfdec_bits_init_data (&bits, data, len);
  while (swfdec_bits_left (&bits) && stackspace-- > 0) {
    guint type = swfdec_bits_get_u8 (&bits);
    SWFDEC_LOG ("push type %u", type);
    switch (type) {
      case 0: /* string */
	{
	  const char *s = swfdec_bits_skip_string (&bits);
	  if (!swfdec_action_push_string (cx, s))
	    return JS_FALSE;
	  break;
	}
      case 1: /* float */
	{
	  double d = swfdec_bits_get_float (&bits);
	  if (!JS_NewDoubleValue (cx, d, cx->fp->sp))
	    return JS_FALSE;
	  cx->fp->sp++;
	  break;
	}
      case 2: /* null */
	*cx->fp->sp++ = JSVAL_NULL;
	break;
      case 3: /* undefined */
	*cx->fp->sp++ = JSVAL_VOID;
	break;
      case 4: /* register */
	{
	  guint regnum = swfdec_bits_get_u8 (&bits);
	  if (!swfdec_action_has_register (cx, regnum)) {
	    SWFDEC_ERROR ("cannot Push register %u: not enough registers", regnum);
	    return JS_FALSE;
	  }
	  *cx->fp->sp++ = cx->fp->vars[regnum];
	  break;
	}
      case 5: /* boolean */
	*cx->fp->sp++ = swfdec_bits_get_u8 (&bits) ? JSVAL_TRUE : JSVAL_FALSE;
	break;
      case 6: /* double */
	{
	  double d = swfdec_bits_get_double (&bits);
	  if (!JS_NewDoubleValue (cx, d, cx->fp->sp))
	    return JS_FALSE;
	  cx->fp->sp++;
	  break;
	}
      case 7: /* 32bit int */
	{
	  int i = swfdec_bits_get_u32 (&bits);
	  *cx->fp->sp++ = INT_TO_JSVAL (i);
	  break;
	}
      case 8: /* 8bit ConstantPool address */
	{
	  guint i = swfdec_bits_get_u8 (&bits);
	  SwfdecConstantPool *pool = cx->fp->constant_pool;
	  if (pool == NULL) {
	    SWFDEC_ERROR ("no constant pool to push from");
	    return JS_FALSE;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    SWFDEC_ERROR ("constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    return JS_FALSE;
	  }
	  if (!swfdec_action_push_string (cx, swfdec_constant_pool_get (pool, i)))
	    return JS_FALSE;
	  break;
	}
      case 9: /* 16bit ConstantPool address */
	{
	  guint i = swfdec_bits_get_u16 (&bits);
	  SwfdecConstantPool *pool = cx->fp->constant_pool;
	  if (pool == NULL) {
	    SWFDEC_ERROR ("no constant pool to push from");
	    return JS_FALSE;
	  }
	  if (i >= swfdec_constant_pool_size (pool)) {
	    SWFDEC_ERROR ("constant pool index %u too high - only %u elements",
		i, swfdec_constant_pool_size (pool));
	    return JS_FALSE;
	  }
	  if (!swfdec_action_push_string (cx, swfdec_constant_pool_get (pool, i)))
	    return JS_FALSE;
	  break;
	}
      default:
	SWFDEC_ERROR ("Push: type %u not implemented", type);
	return JS_FALSE;
    }
  }
  return swfdec_bits_left (&bits) ? JS_FALSE : JS_TRUE;
}

static JSBool
swfdec_action_get_variable (JSContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;

  s = swfdec_js_to_string (cx, cx->fp->sp[-1]);
  if (s == NULL)
    return JS_FALSE;
  cx->fp->sp[-1] = swfdec_js_eval (cx, NULL, s);
  return JS_TRUE;
}

static JSBool
swfdec_action_set_variable (JSContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;

  s = swfdec_js_to_string (cx, cx->fp->sp[-2]);
  if (s == NULL)
    return JS_FALSE;

  swfdec_js_eval_set (cx, NULL, s, cx->fp->sp[-1]);
  cx->fp->sp -= 2;
  return JS_TRUE;
}

static JSBool
swfdec_action_trace (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  const char *bytes;

  bytes = swfdec_js_to_string (cx, cx->fp->sp[-1]);
  cx->fp->sp--;
  if (bytes == NULL)
    return JS_TRUE;

  swfdec_player_trace (player, bytes);
  return JS_TRUE;
}

/**
 * swfdec_action_invoke:
 * @cx: the #JSContext
 * @n_args: number of arguments
 *
 * This function is similar to js_Invoke, however it uses a reversed stack
 * order. sp[-1] has to be the function to call, sp[-2] will be the object the 
 * function is called on, sp[-3] is the first argument, followed by the rest of
 * the arguments. The function removes all of these argumends from the stack 
 * and pushes the return value on top.
 *
 * Returns: JS_TRUE on success, JS_FALSE on failure.
 **/
static JSBool
swfdec_action_call (JSContext *cx, guint n_args, guint flags)
{
  JSStackFrame *fp = cx->fp;
  int i, j;
  jsval tmp;

  g_assert ((guint) (fp->sp - fp->spbase) >= n_args + 2);

  j = -1;
  i = - (n_args + 2);
  while (i < j) {
    tmp = fp->sp[j];
    fp->sp[j] = fp->sp[i];
    fp->sp[i] = tmp;
    j--;
    i++;
  }
  return js_Invoke (cx, n_args, flags);
}

/* FIXME: lots of overlap with swfdec_action_call_method */
static JSBool
swfdec_action_call_function (JSContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  const char *s;
  guint32 n_args;
  JSObject *obj, *pobj;
  JSProperty *prop;
  jsval fun;
  JSAtom *atom;
  
  s = swfdec_js_to_string (cx, fp->sp[-1]);
  if (s == NULL)
    return JS_FALSE;
  if (!JS_ValueToECMAUint32 (cx, fp->sp[-2], &n_args))
    return JS_FALSE;
  if (n_args + 2 > (guint) (fp->sp - fp->spbase))
    return JS_FALSE;
  
  if (!(atom = js_Atomize (cx, s, strlen (s), 0)) ||
      !js_FindProperty (cx, (jsid) atom, &obj, &pobj, &prop))
    return JS_FALSE;
  if (!JS_GetProperty (cx, obj, s, &fun))
    return JS_FALSE;
  fp->sp[-1] = fun;
  fp->sp[-2] = OBJECT_TO_JSVAL (obj);
  swfdec_action_call (cx, n_args, 0);
  return JS_TRUE;
}

static JSBool
swfdec_action_call_method (JSContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  const char *s;
  guint32 n_args;
  JSObject *obj;
  jsval fun;
  
  s = swfdec_js_to_string (cx, fp->sp[-1]);
  if (s == NULL)
    return JS_FALSE;
  if (!JS_ValueToECMAUint32 (cx, fp->sp[-3], &n_args))
    return JS_FALSE;
  if (n_args + 3 > (guint) (fp->sp - fp->spbase))
    return JS_FALSE;
  
  if (!JS_ValueToObject (cx, fp->sp[-2], &obj))
    return JS_FALSE;
  if (obj == NULL)
    goto fail;
  if (s[0] == '\0') {
    fun = OBJECT_TO_JSVAL (obj);
  } else {
    if (!JS_GetProperty (cx, obj, s, &fun))
      return JS_FALSE;
  }
  fp->sp--;
  fp->sp[-1] = fun;
  fp->sp[-2] = OBJECT_TO_JSVAL (obj);
  swfdec_action_call (cx, n_args, 0);
  return JS_TRUE;

fail:
  fp->sp -= 2 + n_args;
  fp->sp[-1] = JSVAL_VOID;
  return JS_TRUE;
}

static JSBool
swfdec_action_pop (JSContext *cx, guint action, const guint8 *data, guint len)
{
  cx->fp->sp--;
  return JS_TRUE;
}

static const char *properties[22] = {
  "_x", "_y", "_xscale", "_yscale", "_currentframe",
  "_totalframes", "_alpha", "_visible", "_width", "_height",
  "_rotation", "_target", "_framesloaded", "_name", "_droptarget",
  "_url", "_highquality", "_focusrect", "_soundbuftime", "_quality",
  "_xmouse", "_ymouse"
};

static JSBool
swfdec_eval_jsval (JSContext *cx, JSObject *obj, jsval *val)
{
  if (JSVAL_IS_STRING (*val)) {
    const char *bytes = swfdec_js_to_string (cx, *val);
    if (bytes == NULL)
      return JS_FALSE;
    *val = swfdec_js_eval (cx, obj, bytes);
  } else {
    if (obj == NULL)
      obj = OBJ_THIS_OBJECT (cx, cx->fp->scopeChain);
    *val = OBJECT_TO_JSVAL (obj);
  }
  return JS_TRUE;
}

static JSBool
swfdec_action_get_property (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval val;
  SwfdecMovie *movie;
  JSObject *jsobj;
  guint32 id;

  val = cx->fp->sp[-2];
  if (!swfdec_eval_jsval (cx, NULL, &val))
    return JS_FALSE;
  movie = swfdec_scriptable_from_jsval (cx, val, SWFDEC_TYPE_MOVIE);
  val = JSVAL_VOID;
  if (movie == NULL) {
    SWFDEC_WARNING ("specified target does not reference a movie clip");
    goto out;
  }
  if (!JS_ValueToECMAUint32 (cx,  cx->fp->sp[-1], &id))
    return JS_FALSE;

  if (id > (((SwfdecScript *) cx->fp->swf)->version > 4 ? 21 : 18))
    goto out;

  if (!(jsobj = swfdec_scriptable_get_object (SWFDEC_SCRIPTABLE (movie))))
    return JS_FALSE;

  if (!JS_GetProperty (cx, jsobj, properties[id], &val))
    return JS_FALSE;

out:
  cx->fp->sp -= 1;
  cx->fp->sp[-1] = val;
  return JS_TRUE;
}

static JSBool
swfdec_action_set_property (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval val;
  SwfdecMovie *movie;
  JSObject *jsobj;
  guint32 id;

  val = cx->fp->sp[-3];
  if (!swfdec_eval_jsval (cx, NULL, &val))
    return JS_FALSE;
  movie = swfdec_scriptable_from_jsval (cx, val, SWFDEC_TYPE_MOVIE);
  if (movie == NULL) {
    SWFDEC_WARNING ("specified target does not reference a movie clip");
    goto out;
  }
  if (!JS_ValueToECMAUint32 (cx,  cx->fp->sp[-2], &id))
    return JS_FALSE;

  if (id > (((SwfdecScript *) cx->fp->swf)->version > 4 ? 21 : 18))
    goto out;

  if (!(jsobj = swfdec_scriptable_get_object (SWFDEC_SCRIPTABLE (movie))))
    return JS_FALSE;

  if (!JS_SetProperty (cx, jsobj, properties[id], &cx->fp->sp[-1]))
    return JS_FALSE;

out:
  cx->fp->sp -= 3;
  return JS_TRUE;
}

static JSBool
swfdec_action_binary (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval lval, rval;
  double l, r;

  rval = cx->fp->sp[-1];
  lval = cx->fp->sp[-2];
  if (((SwfdecScript *) cx->fp->swf)->version < 7) {
    l = swfdec_action_to_number (cx, lval);
    r = swfdec_action_to_number (cx, rval);
  } else {
    if (!swfdec_value_to_number_7 (cx, lval, &l) ||
        !swfdec_value_to_number_7 (cx, rval, &r))
      return JS_FALSE;
  }
  cx->fp->sp--;
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
      if (isnan (r))
	r = 0;
      if (r == 0 && ((SwfdecScript *) cx->fp->swf)->version < 5) {
	JSString *str = JS_InternString (cx, "#ERROR#");
	if (str == NULL)
	  return JS_FALSE;
	cx->fp->sp[-1] = STRING_TO_JSVAL (str);
	return JS_TRUE;
      } else if (((SwfdecScript *) cx->fp->swf)->version >= 7 &&
	  r == 0) {
	cx->fp->sp[-1] = DOUBLE_TO_JSVAL (r < 0 ? 
	    cx->runtime->jsNegativeInfinity :
	    cx->runtime->jsPositiveInfinity);
	return JS_TRUE;
      }
      l = l / r;
      break;
    default:
      g_assert_not_reached ();
      return r;
  }
  return JS_NewNumberValue (cx, l, &cx->fp->sp[-1]);
}

static JSString *
swfdec_action_to_string_5 (JSContext *cx, jsval val)
{
  if (JSVAL_IS_VOID (val) || JSVAL_IS_NULL (val))
    return cx->runtime->emptyString;
  return js_ValueToString (cx, val);
}

static JSBool
swfdec_action_add2_5 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval rval, lval;
  gboolean cond;

  rval = cx->fp->sp[-1];
  lval = cx->fp->sp[-2];
  if ((cond = JSVAL_IS_STRING (lval)) || JSVAL_IS_STRING (rval)) {
    JSString *str, *str2;
    if (cond) {
      str = JSVAL_TO_STRING (lval);
      if ((str2 = swfdec_action_to_string_5 (cx, rval)) == NULL)
	return JS_FALSE;
    } else {
      str2 = JSVAL_TO_STRING (rval);
      if ((str = swfdec_action_to_string_5 (cx, lval)) == NULL)
	return JS_FALSE;
    }
    str = js_ConcatStrings (cx, str, str2);
    if (!str)
      return JS_FALSE;
    cx->fp->sp--;
    cx->fp->sp[-1] = STRING_TO_JSVAL (str);
  } else {
    double d, d2;
    d = swfdec_action_to_number (cx, lval);
    d2 = swfdec_action_to_number (cx, rval);
    d += d2;
    cx->fp->sp--;
    return JS_NewNumberValue(cx, d, &cx->fp->sp[-1]);
  }
  return JS_TRUE;
}

static JSBool
swfdec_action_add2_7 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval rval, lval;
  gboolean cond;

  rval = cx->fp->sp[-1];
  lval = cx->fp->sp[-2];
  if ((cond = JSVAL_IS_STRING (lval)) || JSVAL_IS_STRING (rval)) {
    JSString *str, *str2;
    if (cond) {
      str = JSVAL_TO_STRING (lval);
      if ((str2 = js_ValueToString (cx, rval)) == NULL)
	return JS_FALSE;
    } else {
      str2 = JSVAL_TO_STRING (rval);
      if ((str = js_ValueToString (cx, lval)) == NULL)
	return JS_FALSE;
    }
    str = js_ConcatStrings (cx, str, str2);
    if (!str)
      return JS_FALSE;
    cx->fp->sp--;
    cx->fp->sp[-1] = STRING_TO_JSVAL (str);
  } else {
    double d, d2;
    if (!swfdec_value_to_number_7 (cx, lval, &d) ||
        !swfdec_value_to_number_7 (cx, rval, &d2))
	return JS_FALSE;
    d += d2;
    cx->fp->sp--;
    return JS_NewNumberValue(cx, d, &cx->fp->sp[-1]);
  }
  return JS_TRUE;
}

static JSBool
swfdec_action_get_member (JSContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;

  s = swfdec_js_to_string (cx, cx->fp->sp[-1]);
  if (s == NULL)
    return JS_FALSE;

  if (JSVAL_IS_OBJECT (cx->fp->sp[-2]) && !JSVAL_IS_NULL (cx->fp->sp[-2])) {
    if (!JS_GetProperty (cx, JSVAL_TO_OBJECT (cx->fp->sp[-2]), s, &cx->fp->sp[-2]))
      return JS_FALSE;
  } else {
    cx->fp->sp[-2] = JSVAL_VOID;
  }
  cx->fp->sp--;
  return JS_TRUE;
}

static JSBool
swfdec_action_set_member (JSContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;

  s = swfdec_js_to_string (cx, cx->fp->sp[-2]);
  if (s == NULL)
    return JS_FALSE;

  if (JSVAL_IS_OBJECT (cx->fp->sp[-3]) && !JSVAL_IS_NULL (cx->fp->sp[-3])) {
    if (!JS_SetProperty (cx, JSVAL_TO_OBJECT (cx->fp->sp[-3]), s, &cx->fp->sp[-1]))
      return JS_FALSE;
  }
  cx->fp->sp -= 3;
  return JS_TRUE;
}

static JSBool
swfdec_action_new_comparison_6 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval lval, rval;
  double d, d2;

  rval = cx->fp->sp[-1];
  lval = cx->fp->sp[-2];
  cx->fp->sp--;
  d = swfdec_action_to_number (cx, lval);
  d2 = swfdec_action_to_number (cx, rval);
  if (action == 0x48)
    cx->fp->sp[-1] = BOOLEAN_TO_JSVAL (d < d2);
  else 
    cx->fp->sp[-1] = BOOLEAN_TO_JSVAL (d > d2);
  return JS_TRUE;
}

static JSBool
swfdec_action_new_comparison_7 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval lval, rval;

  rval = cx->fp->sp[-1];
  lval = cx->fp->sp[-2];
  cx->fp->sp--;
  if (JSVAL_IS_VOID (rval) || JSVAL_IS_VOID (lval)) {
    cx->fp->sp[-1] = JSVAL_VOID;
  } else if (JSVAL_IS_STRING(lval) && JSVAL_IS_STRING(rval)) {
    int comp = JS_CompareStrings (JSVAL_TO_STRING (lval), JSVAL_TO_STRING (rval));
    cx->fp->sp[-1] = BOOLEAN_TO_JSVAL (action == 0x48 ? comp < 0 : comp > 0);
  } else {
    double d, d2;
    if (!JS_ValueToNumber(cx, lval, &d) ||
        !JS_ValueToNumber(cx, rval, &d2))
	return JS_FALSE;
    cx->fp->sp[-1] = BOOLEAN_TO_JSVAL (action == 0x48 ? d < d2 : d > d2);
  }
  return JS_TRUE;
}

static JSBool
swfdec_action_not_4 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  double d;

  d = swfdec_action_to_number (cx, cx->fp->sp[-1]);
  cx->fp->sp[-1] = INT_TO_JSVAL (d == 0 ? 1 : 0);
  return JS_TRUE;
}

static JSBool
swfdec_action_not_5 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  double d;

  d = swfdec_action_to_number (cx, cx->fp->sp[-1]);
  cx->fp->sp[-1] = d == 0 ? JSVAL_TRUE : JSVAL_FALSE;
  return JS_TRUE;
}

static JSBool
swfdec_action_jump (JSContext *cx, guint action, const guint8 *data, guint len)
{
  if (len != 2) {
    SWFDEC_ERROR ("Jump action length invalid (is %u, should be 2", len);
    return JS_FALSE;
  }
  cx->fp->pc += 5 + GINT16_FROM_LE (*((gint16*) data)); 
  return JS_TRUE;
}

static JSBool
swfdec_action_if (JSContext *cx, guint action, const guint8 *data, guint len)
{
  double d;

  if (len != 2) {
    SWFDEC_ERROR ("Jump action length invalid (is %u, should be 2", len);
    return JS_FALSE;
  }
  d = swfdec_action_to_number (cx, cx->fp->sp[-1]);
  cx->fp->sp--;
  if (d != 0)
    cx->fp->pc += 5 + GINT16_FROM_LE (*((gint16*) data)); 
  return JS_TRUE;
}

static JSBool
swfdec_action_decrement (JSContext *cx, guint action, const guint8 *data, guint len)
{
  double d;

  d = swfdec_action_to_number (cx, cx->fp->sp[-1]);
  d--;
  return JS_NewNumberValue (cx, d, &cx->fp->sp[-1]);
}

static JSBool
swfdec_action_increment (JSContext *cx, guint action, const guint8 *data, guint len)
{
  double d;

  d = swfdec_action_to_number (cx, cx->fp->sp[-1]);
  d++;
  return JS_NewNumberValue (cx, d, &cx->fp->sp[-1]);
}

static JSBool
swfdec_action_get_url (JSContext *cx, guint action, const guint8 *data, guint len)
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

static JSBool
swfdec_action_get_url2 (JSContext *cx, guint action, const guint8 *data, guint len)
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

static JSBool
swfdec_action_string_add (JSContext *cx, guint action, const guint8 *data, guint len)
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

static JSBool
swfdec_action_push_duplicate (JSContext *cx, guint action, const guint8 *data, guint len)
{
  cx->fp->sp++;
  cx->fp->sp[-1] = cx->fp->sp[-2];
  return JS_TRUE;
}

static JSBool
swfdec_action_random_number (JSContext *cx, guint action, const guint8 *data, guint len)
{
  gint32 max, result;

  if (!JS_ValueToECMAInt32 (cx, cx->fp->sp[-1], &max))
    return JS_FALSE;
  
  if (max <= 0)
    result = 0;
  else
    result = g_random_int_range (0, max);

  return JS_NewNumberValue(cx, result, &cx->fp->sp[-1]);
}

static JSBool
swfdec_action_old_compare (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval rval, lval;
  double l, r;
  JSBool cond;

  rval = cx->fp->sp[-1];
  lval = cx->fp->sp[-2];
  l = swfdec_action_to_number (cx, lval);
  r = swfdec_action_to_number (cx, rval);
  switch (action) {
    case 0x0e:
      cond = l == r;
      break;
    case 0x0f:
      cond = l < r;
      break;
    default: 
      g_assert_not_reached ();
      return JS_FALSE;
  }
  cx->fp->sp--;
  if (((SwfdecScript *) cx->fp->swf)->version < 5) {
    cx->fp->sp[-1] = INT_TO_JSVAL (cond ? 1 : 0);
  } else {
    cx->fp->sp[-1] = BOOLEAN_TO_JSVAL (cond);
  }
  return JS_TRUE;
}

static JSBool
swfdec_action_equals2 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval rval, lval;
  int ltag, rtag;
  JSBool cond;

  rval = cx->fp->sp[-1];
  lval = cx->fp->sp[-2];
  ltag = JSVAL_TAG(lval);
  rtag = JSVAL_TAG(rval);
  if (ltag == rtag) {
    if (ltag == JSVAL_STRING) {
      cond = js_CompareStrings (JSVAL_TO_STRING (lval), JSVAL_TO_STRING (rval)) == 0;
    } else if (ltag == JSVAL_DOUBLE) {
      cond = *JSVAL_TO_DOUBLE(lval) == *JSVAL_TO_DOUBLE(rval);
    } else {
      cond = lval == rval;
    }
  } else {
    if (JSVAL_IS_NULL(lval) || JSVAL_IS_VOID(lval)) {
      cond = (JSVAL_IS_NULL(rval) || JSVAL_IS_VOID(rval));
    } else if (JSVAL_IS_NULL(rval) || JSVAL_IS_VOID(rval)) {
      cond = JS_FALSE;
    } else {
      if (ltag == JSVAL_OBJECT) {
	if (!OBJ_DEFAULT_VALUE (cx, JSVAL_TO_OBJECT(lval), 0, &lval))
	  return JS_FALSE;
	ltag = JSVAL_TAG(lval);
      } else if (rtag == JSVAL_OBJECT) {
	if (!OBJ_DEFAULT_VALUE (cx, JSVAL_TO_OBJECT(rval), 0, &rval))
	  return JS_FALSE;
	rtag = JSVAL_TAG(rval);
      }
      if (ltag == JSVAL_STRING && rtag == JSVAL_STRING) {
	cond = js_CompareStrings (JSVAL_TO_STRING (lval), JSVAL_TO_STRING (rval)) == 0;
      } else {
	double d, d2;
	if (!JS_ValueToNumber (cx, lval, &d) ||
	    !JS_ValueToNumber (cx, rval, &d2))
	  return JS_FALSE;
	cond = d == d2;
      }
    }
  }
  cx->fp->sp--;
  cx->fp->sp[-1] = BOOLEAN_TO_JSVAL (cond);
  return JS_TRUE;
}

static JSBool
swfdec_action_do_set_target (JSContext *cx, JSObject *target)
{
  JSObject *with;
  
  /* FIXME: this whole function stops working the moment it's used together 
   * with With */
  if (target == cx->fp->scopeChain)
    return JS_TRUE;
  if (target == cx->fp->thisp) {
    /* FIXME: will probably break once SetTarget is called inside DefineFunctions */
    cx->fp->scopeChain = cx->fp->thisp;
    return JS_TRUE;
  }
  with = js_NewObject(cx, &js_WithClass, target, cx->fp->scopeChain);
  if (!with)
    return JS_FALSE;
  cx->fp->scopeChain = with;
  return JS_TRUE;
}

static JSBool
swfdec_action_set_target (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval target;

  if (!memchr (data, 0, len)) {
    SWFDEC_ERROR ("SetTarget action does not specify a string");
    return JS_FALSE;
  }
  /* evaluate relative to this to not get trapped by previous SetTarget calls */
  target = swfdec_js_eval (cx, cx->fp->thisp, (const char *) data);
  if (!JSVAL_IS_OBJECT (target) || JSVAL_IS_NULL (target)) {
    SWFDEC_WARNING ("target is not an object");
    return JS_TRUE;
  }
  return swfdec_action_do_set_target (cx, JSVAL_TO_OBJECT (target));
}

static JSBool
swfdec_action_set_target2 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  jsval val;
  
  val = cx->fp->sp[-1];
  cx->fp->sp--;
  if (!JSVAL_IS_OBJECT (val) || JSVAL_IS_NULL (val)) {
    SWFDEC_WARNING ("target is not an object");
    return JS_TRUE;
  }
  return swfdec_action_do_set_target (cx, JSVAL_TO_OBJECT (val));
}

static JSBool
swfdec_action_start_drag (JSContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  guint stack_size = fp->sp - fp->spbase;
  guint n_args = 1;

  if (stack_size < 3)
    return JS_FALSE;
  if (!swfdec_eval_jsval (cx, NULL, &fp->sp[-1]))
    return JS_FALSE;
  if (swfdec_action_to_number (cx, fp->sp[-3])) {
    jsval tmp;
    if (stack_size < 7)
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

static JSBool
swfdec_action_end_drag (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  swfdec_player_set_drag_movie (player, NULL, FALSE, NULL);
  return JS_TRUE;
}

static JSBool
swfdec_action_stop_sounds (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);

  swfdec_player_stop_all_sounds (player);
  return JS_TRUE;
}

static JSBool
swfdec_action_new_object (JSContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  jsval constructor;
  JSObject *object;
  const JSClass *clasp;
  guint n_args;

  constructor = fp->sp[-1];
  if (!swfdec_eval_jsval (cx, NULL, &constructor))
    return JS_FALSE;
  if (!JS_ValueToECMAUint32 (cx, fp->sp[-2], &n_args))
    return JS_FALSE;
  if ((guint) (fp->sp - fp->spbase) < n_args + 2) {
    SWFDEC_ERROR ("not enough stack space");
    return JS_FALSE;
  }
  fp->sp[-1] = constructor;

  if (!JSVAL_IS_OBJECT (constructor) || JSVAL_IS_NULL (constructor))
    goto fail;
  object = JSVAL_TO_OBJECT (constructor);
  if (JS_GetClass (object) != &js_FunctionClass)
    goto fail;
  clasp = ((JSFunction *) JS_GetPrivate (cx, object))->clasp;
  object = JS_NewObject (cx, clasp, NULL, NULL);
  if (object == NULL)
    return JS_FALSE;
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

static JSBool
swfdec_action_init_object (JSContext *cx, guint action, const guint8 *data, guint len)
{
  JSStackFrame *fp = cx->fp;
  JSObject *object;
  guint i, n_args;

  if (!JS_ValueToECMAUint32 (cx, fp->sp[-1], &n_args))
    return JS_FALSE;
  if ((guint) (fp->sp - fp->spbase) < 2 * n_args + 1) {
    SWFDEC_ERROR ("not enough stack space");
    return JS_FALSE;
  }

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

static JSBool
swfdec_action_do_define_function (JSContext *cx, guint action,
    const guint8 *data, guint len, gboolean v2)
{
  const char *function_name;
  guint i, n_args, size;
  SwfdecBits bits;
  JSFunction *fun;
  SwfdecScript *script;
  gboolean has_preloads = FALSE;
  guint flags = 0;
  guint8 *preloads = NULL;

  swfdec_bits_init_data (&bits, data, len);
  function_name = swfdec_bits_get_string (&bits);
  if (function_name == NULL) {
    SWFDEC_ERROR ("could not parse function name");
    return JS_FALSE;
  }
  n_args = swfdec_bits_get_u16 (&bits);
  if (*function_name == '\0') {
    /* anonymous function */
    fun = JS_NewFunction (cx, NULL, n_args, JSFUN_LAMBDA, cx->fp->thisp, NULL);
  } else {
    /* named function */
    fun = JS_NewFunction (cx, NULL, n_args, 0, cx->fp->thisp, function_name);
  }
  if (fun == NULL)
    return JS_FALSE;
  if (v2) {
    fun->nvars = swfdec_bits_get_u8 (&bits);
    flags = swfdec_bits_get_u16 (&bits);
    preloads = g_new0 (guint8, n_args);
  } else {
    fun->nvars = 4;
  }
  for (i = 0; i < n_args; i++) {
    JSAtom *atom;
    const char *arg_name;
    if (v2) {
      guint preload = swfdec_bits_get_u8 (&bits);
      if (preload && preload >= fun->nvars) {
	SWFDEC_ERROR ("argument %u is preloaded into register %u out of %u", 
	    i, preload, fun->nvars);
	return JS_FALSE;
      }
      if (preload != 0) {
	preloads[i] = preload;
	swfdec_bits_skip_string (&bits);
	has_preloads = TRUE;
	continue;
      }
    }
    arg_name = swfdec_bits_skip_string (&bits);
    if (arg_name == NULL || *arg_name == '\0') {
      SWFDEC_ERROR ("empty argument name not allowed");
      return JS_FALSE;
    }
    /* FIXME: check duplicate arguments */
    atom = js_Atomize (cx, arg_name, strlen (arg_name), 0);
    if (atom == NULL)
      return JS_FALSE;
    if (!js_AddNativeProperty (cx, fun->object, (jsid) atom,
	js_GetArgument, js_SetArgument, SPROP_INVALID_SLOT,
	JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED,
	SPROP_HAS_SHORTID, i)) {
      return JS_FALSE;
    }
  }
  if (preloads && !has_preloads) {
    g_free (preloads);
    preloads = NULL;
  }
  size = swfdec_bits_get_u16 (&bits);
  /* check the script can be created */
  script = cx->fp->swf;
  if (script->buffer->data + script->buffer->length < cx->fp->pc + 3 + len + size) {
    SWFDEC_ERROR ("size of function is too big");
    return FALSE;
  } else {
    /* create the script */
    SwfdecBuffer *buffer = swfdec_buffer_new_subbuffer (script->buffer, 
	cx->fp->pc + 3 + len - script->buffer->data, size);
    swfdec_bits_init (&bits, buffer);
    script = swfdec_script_new_for_player (JS_GetContextPrivate (cx),
	&bits, *function_name ? function_name : "<lambda>", 
	((SwfdecScript *) cx->fp->swf)->version);
    swfdec_buffer_unref (buffer);
    if (cx->fp->constant_pool) {
      script->constant_pool = swfdec_constant_pool_get_area (cx->fp->swf,
	  cx->fp->constant_pool);
    }
  }
  if (script == NULL) {
    SWFDEC_ERROR ("failed to create script");
    g_free (preloads);
    return JS_FALSE;
  }
  script->flags = flags;
  script->preloads = preloads;
  fun->swf = script;
  /* attach the function */
  if (*function_name == '\0') {
    if (action == 0) {
      SWFDEC_ERROR ("not enough stack space available");
      return JS_FALSE;
    }
    *cx->fp->sp++ = OBJECT_TO_JSVAL (fun->object);
  } else {
    jsval val = OBJECT_TO_JSVAL (fun->object);
    if (!JS_SetProperty (cx, OBJ_THIS_OBJECT (cx, cx->fp->scopeChain), function_name, &val))
      return JS_FALSE;
  }

  /* update current context */
  cx->fp->pc += 3 + len + size;
  return JS_TRUE;
}

static JSBool
swfdec_action_define_function (JSContext *cx, guint action, const guint8 *data, guint len)
{
  return swfdec_action_do_define_function (cx, action, data, len, FALSE);
}

static JSBool
swfdec_action_define_function2 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  return swfdec_action_do_define_function (cx, action, data, len, TRUE);
}

static JSBool
swfdec_action_bitwise (JSContext *cx, guint action, const guint8 *data, guint len)
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

static JSBool
swfdec_action_shift (JSContext *cx, guint action, const guint8 *data, guint len)
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

static JSBool
swfdec_action_to_integer (JSContext *cx, guint action, const guint8 *data, guint len)
{
  double d = swfdec_action_to_number (cx, cx->fp->sp[-1]);

  return JS_NewNumberValue (cx, (int) d, &cx->fp->sp[-1]);
}

static JSBool
swfdec_action_target_path (JSContext *cx, guint action, const guint8 *data, guint len)
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

static JSBool
swfdec_action_define_local (JSContext *cx, guint action, const guint8 *data, guint len)
{
  const char *name;

  if (cx->fp->callobj == NULL) {
    SWFDEC_ERROR ("FIXME: no local scope");
    return JS_FALSE;
  }
  name = swfdec_js_to_string (cx, cx->fp->sp[-2]);
  if (name == NULL)
    return JS_FALSE;
  if (!JS_SetProperty (cx, cx->fp->callobj, name, &cx->fp->sp[-1]))
    return JS_FALSE;
  cx->fp->sp -= 2;
  return JS_TRUE;
}

static JSBool
swfdec_action_define_local2 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  const char *name;
  jsval val = JSVAL_VOID;

  if (cx->fp->callobj == NULL) {
    SWFDEC_ERROR ("FIXME: no local scope");
    return JS_FALSE;
  }
  name = swfdec_js_to_string (cx, cx->fp->sp[-1]);
  if (name == NULL)
    return JS_FALSE;
  if (!JS_SetProperty (cx, cx->fp->callobj, name, &val))
    return JS_FALSE;
  cx->fp->sp--;
  return JS_TRUE;
}

static JSBool
swfdec_action_return (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecScript *script = cx->fp->swf;

  cx->fp->rval = cx->fp->sp[-1];
  cx->fp->pc = script->buffer->data + script->buffer->length;
  cx->fp->sp--;
  return JS_TRUE;
}

static JSBool
swfdec_action_delete (JSContext *cx, guint action, const guint8 *data, guint len)
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

static JSBool
swfdec_action_store_register (JSContext *cx, guint action, const guint8 *data, guint len)
{
  if (len != 1) {
    SWFDEC_ERROR ("StoreRegister action requires a length of 1, but got %u", len);
    return JS_FALSE;
  }
  if (!swfdec_action_has_register (cx, *data)) {
    SWFDEC_ERROR ("Cannot store into register %u, not enough registers", (guint) *data);
    return JS_FALSE;
  }
  cx->fp->vars[*data] = cx->fp->sp[-1];
  return JS_TRUE;
}

static JSBool
swfdec_action_modulo_5 (JSContext *cx, guint action, const guint8 *data, guint len)
{
  double x, y;

  x = swfdec_action_to_number (cx, cx->fp->sp[-1]);
  y = swfdec_action_to_number (cx, cx->fp->sp[-2]);
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

static JSBool
swfdec_action_modulo_7 (JSContext *cx, guint action, const guint8 *data, guint len)
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
    return JS_FALSE;
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
    if (preload)
      g_string_append_printf (string, "PRELOAD %u", preload);
    else
      g_string_append (string, arg_name);
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
	return JS_FALSE;
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
    return JS_FALSE;
  string = g_string_new ("ConstantPool");
  for (i = 0; i < swfdec_constant_pool_size (pool); i++) {
    g_string_append (string, i ? ", " : " ");
    g_string_append (string, swfdec_constant_pool_get (pool, i));
  }
  return g_string_free (string, FALSE);
}

static char *
swfdec_action_print_wait_for_frame2 (guint action, const guint8 *data, guint len)
{
  if (len != 1) {
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

/* defines minimum and maximum versions for which we have seperate scripts */
#define MINSCRIPTVERSION 3
#define MAXSCRIPTVERSION 7
#define EXTRACT_VERSION(v) MIN ((v) - MINSCRIPTVERSION, MAXSCRIPTVERSION - MINSCRIPTVERSION)

typedef JSBool (* SwfdecActionExec) (JSContext *cx, guint action, const guint8 *data, guint len);
typedef struct {
  const char *		name;		/* name identifying the action */
  char *		(* print)	(guint action, const guint8 *data, guint len);
  int			remove;		/* values removed from stack or -1 for dynamic */
  int			add;		/* values added to the stack or -1 for dynamic */
  SwfdecActionExec	exec[MAXSCRIPTVERSION - MINSCRIPTVERSION + 1];
					/* array is for version 3, 4, 5, 6, 7+ */
} SwfdecActionSpec;

static const SwfdecActionSpec actions[256] = {
  /* version 3 */
  [0x04] = { "NextFrame", NULL, 0, 0, { swfdec_action_next_frame, swfdec_action_next_frame, swfdec_action_next_frame, swfdec_action_next_frame, swfdec_action_next_frame } },
  [0x05] = { "PreviousFrame", NULL, 0, 0, { swfdec_action_previous_frame, swfdec_action_previous_frame, swfdec_action_previous_frame, swfdec_action_previous_frame, swfdec_action_previous_frame } },
  [0x06] = { "Play", NULL, 0, 0, { swfdec_action_play, swfdec_action_play, swfdec_action_play, swfdec_action_play, swfdec_action_play } },
  [0x07] = { "Stop", NULL, 0, 0, { swfdec_action_stop, swfdec_action_stop, swfdec_action_stop, swfdec_action_stop, swfdec_action_stop } },
  [0x08] = { "ToggleQuality", NULL },
  [0x09] = { "StopSounds", NULL, 0, 0, { swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds, swfdec_action_stop_sounds } },
  /* version 4 */
  [0x0a] = { "Add", NULL, 2, 1, { NULL, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary } },
  [0x0b] = { "Subtract", NULL, 2, 1, { NULL, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary } },
  [0x0c] = { "Multiply", NULL, 2, 1, { NULL, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary } },
  [0x0d] = { "Divide", NULL, 2, 1, { NULL, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary, swfdec_action_binary } },
  [0x0e] = { "Equals", NULL, 2, 1, { NULL, swfdec_action_old_compare, swfdec_action_old_compare, swfdec_action_old_compare, swfdec_action_old_compare } },
  [0x0f] = { "Less", NULL, 2, 1, { NULL, swfdec_action_old_compare, swfdec_action_old_compare, swfdec_action_old_compare, swfdec_action_old_compare } },
  [0x10] = { "And", NULL },
  [0x11] = { "Or", NULL },
  [0x12] = { "Not", NULL, 1, 1, { NULL, swfdec_action_not_4, swfdec_action_not_5, swfdec_action_not_5, swfdec_action_not_5 } },
  [0x13] = { "StringEquals", NULL },
  [0x14] = { "StringLength", NULL },
  [0x15] = { "StringExtract", NULL },
  [0x17] = { "Pop", NULL, 1, 0, { NULL, swfdec_action_pop, swfdec_action_pop, swfdec_action_pop, swfdec_action_pop } },
  [0x18] = { "ToInteger", NULL, 1, 1, { NULL, swfdec_action_to_integer, swfdec_action_to_integer, swfdec_action_to_integer, swfdec_action_to_integer } },
  [0x1c] = { "GetVariable", NULL, 1, 1, { NULL, swfdec_action_get_variable, swfdec_action_get_variable, swfdec_action_get_variable, swfdec_action_get_variable } },
  [0x1d] = { "SetVariable", NULL, 2, 0, { NULL, swfdec_action_set_variable, swfdec_action_set_variable, swfdec_action_set_variable, swfdec_action_set_variable } },
  [0x20] = { "SetTarget2", NULL, 1, 0, { swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2, swfdec_action_set_target2 } },
  [0x21] = { "StringAdd", NULL, 2, 1, { NULL, swfdec_action_string_add, swfdec_action_string_add, swfdec_action_string_add, swfdec_action_string_add } },
  [0x22] = { "GetProperty", NULL, 2, 1, { NULL, swfdec_action_get_property, swfdec_action_get_property, swfdec_action_get_property, swfdec_action_get_property } },
  [0x23] = { "SetProperty", NULL, 3, 0, { NULL, swfdec_action_set_property, swfdec_action_set_property, swfdec_action_set_property, swfdec_action_set_property } },
  [0x24] = { "CloneSprite", NULL },
  [0x25] = { "RemoveSprite", NULL },
  [0x26] = { "Trace", NULL, 1, 0, { NULL, swfdec_action_trace, swfdec_action_trace, swfdec_action_trace, swfdec_action_trace } },
  [0x27] = { "StartDrag", NULL, -1, 0, { NULL, swfdec_action_start_drag, swfdec_action_start_drag, swfdec_action_start_drag, swfdec_action_start_drag } },
  [0x28] = { "EndDrag", NULL, 0, 0, { NULL, swfdec_action_end_drag, swfdec_action_end_drag, swfdec_action_end_drag, swfdec_action_end_drag } },
  [0x29] = { "StringLess", NULL },
  /* version 7 */
  [0x2a] = { "Throw", NULL },
  [0x2b] = { "Cast", NULL },
  [0x2c] = { "Implements", NULL },
  /* version 4 */
  [0x30] = { "RandomNumber", NULL, 1, 1, { NULL, swfdec_action_random_number, swfdec_action_random_number, swfdec_action_random_number, swfdec_action_random_number } },
  [0x31] = { "MBStringLength", NULL },
  [0x32] = { "CharToAscii", NULL },
  [0x33] = { "AsciiToChar", NULL },
  [0x34] = { "GetTime", NULL },
  [0x35] = { "MBStringExtract", NULL },
  [0x36] = { "MBCharToAscii", NULL },
  [0x37] = { "MVAsciiToChar", NULL },
  /* version 5 */
  [0x3a] = { "Delete", NULL, 2, 0, { NULL, NULL, swfdec_action_delete, swfdec_action_delete, swfdec_action_delete } },
  [0x3b] = { "Delete2", NULL },
  [0x3c] = { "DefineLocal", NULL, 2, 0, { NULL, NULL, swfdec_action_define_local, swfdec_action_define_local, swfdec_action_define_local } },
  [0x3d] = { "CallFunction", NULL, -1, 1, { NULL, NULL, swfdec_action_call_function, swfdec_action_call_function, swfdec_action_call_function } },
  [0x3e] = { "Return", NULL, 1, 0, { NULL, NULL, swfdec_action_return, swfdec_action_return, swfdec_action_return } },
  [0x3f] = { "Modulo", NULL, 2, 1, { NULL, NULL, swfdec_action_modulo_5, swfdec_action_modulo_5, swfdec_action_modulo_7 } },
  [0x40] = { "NewObject", NULL, -1, 1, { NULL, NULL, swfdec_action_new_object, swfdec_action_new_object, swfdec_action_new_object } },
  [0x41] = { "DefineLocal2", NULL, 1, 0, { NULL, NULL, swfdec_action_define_local2, swfdec_action_define_local2, swfdec_action_define_local2 } },
  [0x42] = { "InitArray", NULL },
  [0x43] = { "InitObject", NULL, -1, 1, { NULL, NULL, swfdec_action_init_object, swfdec_action_init_object, swfdec_action_init_object } },
  [0x44] = { "Typeof", NULL },
  [0x45] = { "TargetPath", NULL, 1, 1, { NULL, NULL, swfdec_action_target_path, swfdec_action_target_path, swfdec_action_target_path } },
  [0x46] = { "Enumerate", NULL },
  [0x47] = { "Add2", NULL, 2, 1, { NULL, NULL, swfdec_action_add2_5, swfdec_action_add2_5, swfdec_action_add2_7 } },
  [0x48] = { "Less2", NULL, 2, 1, { NULL, NULL, swfdec_action_new_comparison_6, swfdec_action_new_comparison_6, swfdec_action_new_comparison_7 } },
  [0x49] = { "Equals2", NULL, 2, 1, { NULL, NULL, swfdec_action_equals2, swfdec_action_equals2, swfdec_action_equals2 } },
  [0x4a] = { "ToNumber", NULL },
  [0x4b] = { "ToString", NULL },
  [0x4c] = { "PushDuplicate", NULL, 1, 2, { NULL, NULL, swfdec_action_push_duplicate, swfdec_action_push_duplicate, swfdec_action_push_duplicate } },
  [0x4d] = { "Swap", NULL },
  [0x4e] = { "GetMember", NULL, 2, 1, { NULL, swfdec_action_get_member, swfdec_action_get_member, swfdec_action_get_member, swfdec_action_get_member } },
  [0x4f] = { "SetMember", NULL, 3, 0, { NULL, swfdec_action_set_member, swfdec_action_set_member, swfdec_action_set_member, swfdec_action_set_member } },
  [0x50] = { "Increment", NULL, 1, 1, { NULL, NULL, swfdec_action_increment, swfdec_action_increment, swfdec_action_increment } },
  [0x51] = { "Decrement", NULL, 1, 1, { NULL, NULL, swfdec_action_decrement, swfdec_action_decrement, swfdec_action_decrement } },
  [0x52] = { "CallMethod", NULL, -1, 1, { NULL, NULL, swfdec_action_call_method, swfdec_action_call_method, swfdec_action_call_method } },
  [0x53] = { "NewMethod", NULL },
  /* version 6 */
  [0x54] = { "InstanceOf", NULL },
  [0x55] = { "Enumerate2", NULL },
  /* version 5 */
  [0x60] = { "BitAnd", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [0x61] = { "BitOr", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [0x62] = { "BitXor", NULL, 2, 1, { NULL, NULL, swfdec_action_bitwise, swfdec_action_bitwise, swfdec_action_bitwise } },
  [0x63] = { "BitLShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
  [0x64] = { "BitRShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
  [0x65] = { "BitURShift", NULL, 2, 1, { NULL, NULL, swfdec_action_shift, swfdec_action_shift, swfdec_action_shift } },
  /* version 6 */
  [0x66] = { "StrictEquals", NULL },
  [0x67] = { "Greater", NULL, 2, 1, { NULL, NULL, NULL, swfdec_action_new_comparison_6, swfdec_action_new_comparison_7 } },
  [0x68] = { "StringGreater", NULL },
  /* version 7 */
  [0x69] = { "Extends", NULL },

  /* version 3 */
  [0x81] = { "GotoFrame", swfdec_action_print_goto_frame, 0, 0, { swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame } },
  [0x83] = { "GetURL", swfdec_action_print_get_url, 0, 0, { swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url, swfdec_action_get_url } },
  /* version 5 */
  [0x87] = { "StoreRegister", swfdec_action_print_store_register, 1, 1, { NULL, NULL, swfdec_action_store_register, swfdec_action_store_register, swfdec_action_store_register } },
  [0x88] = { "ConstantPool", swfdec_action_print_constant_pool, 0, 0, { NULL, NULL, swfdec_action_constant_pool, swfdec_action_constant_pool, swfdec_action_constant_pool } },
  /* version 3 */
  [0x8a] = { "WaitForFrame", swfdec_action_print_wait_for_frame, 0, 0, { swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame } },
  [0x8b] = { "SetTarget", swfdec_action_print_set_target, 0, 0, { swfdec_action_set_target, swfdec_action_set_target, swfdec_action_set_target, swfdec_action_set_target, swfdec_action_set_target } },
  [0x8c] = { "GotoLabel", swfdec_action_print_goto_label, 0, 0, { swfdec_action_goto_label, swfdec_action_goto_label, swfdec_action_goto_label, swfdec_action_goto_label, swfdec_action_goto_label } },
  /* version 4 */
  [0x8d] = { "WaitForFrame2", swfdec_action_print_wait_for_frame2, 1, 0, { NULL, swfdec_action_wait_for_frame2, swfdec_action_wait_for_frame2, swfdec_action_wait_for_frame2, swfdec_action_wait_for_frame2 } },
  /* version 7 */
  [0x8e] = { "DefineFunction2", swfdec_action_print_define_function, 0, -1, { NULL, NULL, NULL, NULL, swfdec_action_define_function2 } },
  [0x8f] = { "Try", NULL },
  /* version 5 */
  [0x94] = { "With", NULL },
  /* version 4 */
  [0x96] = { "Push", swfdec_action_print_push, 0, -1, { NULL, swfdec_action_push, swfdec_action_push, swfdec_action_push, swfdec_action_push } },
  [0x99] = { "Jump", swfdec_action_print_jump, 0, 0, { NULL, swfdec_action_jump, swfdec_action_jump, swfdec_action_jump, swfdec_action_jump } },
  [0x9a] = { "GetURL2", swfdec_action_print_get_url2, 2, 0, { NULL, swfdec_action_get_url2, swfdec_action_get_url2, swfdec_action_get_url2, swfdec_action_get_url2 } },
  /* version 5 */
  [0x9b] = { "DefineFunction", swfdec_action_print_define_function, 0, -1, { NULL, NULL, swfdec_action_define_function, swfdec_action_define_function, swfdec_action_define_function } },
  /* version 4 */
  [0x9d] = { "If", swfdec_action_print_if, 1, 0, { NULL, swfdec_action_if, swfdec_action_if, swfdec_action_if, swfdec_action_if } },
  [0x9e] = { "Call", NULL },
  [0x9f] = { "GotoFrame2", swfdec_action_print_goto_frame2, 1, 0, { NULL, swfdec_action_goto_frame2, swfdec_action_goto_frame2, swfdec_action_goto_frame2, swfdec_action_goto_frame2 } }
};

char *
swfdec_script_print_action (guint action, const guint8 *data, guint len)
{
  const SwfdecActionSpec *spec = actions + action;

  if (action & 0x80) {
    if (spec->print == NULL) {
      SWFDEC_ERROR ("action %u %s has no print statement",
	  action, spec->name ? spec->name : "Unknown");
      return NULL;
    }
    return spec->print (action, data, len);
  } else {
    if (spec->name == NULL) {
      SWFDEC_ERROR ("action %u is unknown", action);
      return NULL;
    }
    return g_strdup (spec->name);
  }
}

static gboolean
swfdec_script_foreach_internal (SwfdecBits *bits, SwfdecScriptForeachFunc func, gpointer user_data)
{
  guint action, len;
  const guint8 *data;
  gconstpointer bytecode;

  bytecode = bits->ptr;
  while (swfdec_bits_left (bits) && (action = swfdec_bits_get_u8 (bits))) {
    if (action & 0x80) {
      len = swfdec_bits_get_u16 (bits);
      data = bits->ptr;
    } else {
      len = 0;
      data = NULL;
    }
    if (swfdec_bits_skip_bytes (bits, len) != len) {
      SWFDEC_ERROR ("script too short");
      return FALSE;
    }
    if (!func (bytecode, action, data, len, user_data))
      return FALSE;
    bytecode = bits->ptr;
  }
  return TRUE;
}

/*** PUBLIC API ***/

gboolean
swfdec_script_foreach (SwfdecScript *script, SwfdecScriptForeachFunc func, gpointer user_data)
{
  SwfdecBits bits;

  g_return_val_if_fail (script != NULL, FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  swfdec_bits_init (&bits, script->buffer);
  return swfdec_script_foreach_internal (&bits, func, user_data);
}

static gboolean
validate_action (gconstpointer bytecode, guint action, const guint8 *data, guint len, gpointer scriptp)
{
  SwfdecScript *script = scriptp;
  int version = EXTRACT_VERSION (script->version);

  /* ensure there's a function to execute this opcode, otherwise fail */
  if (actions[action].exec[version] == NULL) {
    SWFDEC_ERROR ("no function for %u %s in v%u", action, 
	actions[action].name ? actions[action].name : "Unknown",
	script->version);
    return FALSE;
  }
  /* we might want to do stuff here for certain actions */
#if 0
  {
    char *foo = swfdec_script_print_action (action, data, len);
    if (foo == NULL)
      return FALSE;
    g_print ("%s\n", foo);
  }
#endif
  return TRUE;
}

SwfdecScript *
swfdec_script_new_for_player (SwfdecPlayer *player, SwfdecBits *bits, 
    const char *name, unsigned int version)
{
  SwfdecScript *script;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (bits != NULL, NULL);

  script = swfdec_script_new (bits, name, version);
  if (SWFDEC_IS_DEBUGGER (player) && script) {
    swfdec_debugger_add_script (SWFDEC_DEBUGGER (player), script);
    script->debugger = player;
  }
  return script;
}

SwfdecScript *
swfdec_script_new (SwfdecBits *bits, const char *name, unsigned int version)
{
  SwfdecScript *script;
  const guchar *start;
  
  g_return_val_if_fail (bits != NULL, NULL);

  if (version < MINSCRIPTVERSION) {
    SWFDEC_ERROR ("swfdec version %u doesn't support scripts", version);
    return NULL;
  }

  swfdec_bits_syncbits (bits);
  start = bits->ptr;
  script = g_new0 (SwfdecScript, 1);
  script->refcount = 1;
  script->name = g_strdup (name ? name : "Unnamed script");
  script->version = version;

  if (!swfdec_script_foreach_internal (bits, validate_action, script)) {
    /* assign a random buffer here so we have something to unref */
    script->buffer = bits->buffer;
    swfdec_buffer_ref (script->buffer);
    swfdec_script_unref (script);
    return NULL;
  }
  script->buffer = swfdec_buffer_new_subbuffer (bits->buffer, start - bits->buffer->data,
      bits->ptr - start);
  return script;
}

void
swfdec_script_ref (SwfdecScript *script)
{
  g_return_if_fail (script != NULL);

  script->refcount++;
}

void
swfdec_script_unref (SwfdecScript *script)
{
  g_return_if_fail (script != NULL);
  g_return_if_fail (script->refcount > 0);

  script->refcount--;
  if (script->refcount == 1 && script->debugger) {
    script->debugger = NULL;
    swfdec_debugger_remove_script (script->debugger, script);
    return;
  }
  if (script->refcount > 0)
    return;

  swfdec_buffer_unref (script->buffer);
  if (script->constant_pool)
    swfdec_buffer_unref (script->constant_pool);
  g_free (script->name);
  g_free (script->preloads);
  g_free (script);
}

#ifndef MAX_INTERP_LEVEL
#if defined(XP_OS2)
#define MAX_INTERP_LEVEL 250
#elif defined _MSC_VER && _MSC_VER <= 800
#define MAX_INTERP_LEVEL 30
#else
#define MAX_INTERP_LEVEL 1000
#endif
#endif

/* random guess */
#define STACKSIZE 100

/* FIXME: the implementation of this function needs the usual debugging hooks 
 * found in mozilla */
JSBool
swfdec_script_interpret (SwfdecScript *script, JSContext *cx, jsval *rval)
{
  JSStackFrame *fp;
  guint8 *startpc, *pc, *endpc, *nextpc;
  JSBool ok = JS_TRUE;
  void *mark;
  jsval *startsp, *endsp, *checksp;
  int stack_check;
  guint action, len;
  guint8 *data;
  guint version;
  const SwfdecActionSpec *spec;
  
  /* set up general stuff */
  swfdec_script_ref (script);
  version = EXTRACT_VERSION (script->version);
  *rval = JSVAL_VOID;
  fp = cx->fp;
  /* do the preloading */
  if (script->preloads) {
    guint i;
    for (i = 0; i < fp->fun->nargs; i++) {
      if (script->preloads[i])
	fp->vars[script->preloads[i]] = fp->argv[i];
    }
  }
  if (script->flags) {
    guint preload_reg = 0;
    SwfdecPlayer *player = JS_GetContextPrivate (cx);
    if (script->flags & SWFDEC_SCRIPT_PRELOAD_THIS)
      fp->vars[preload_reg++] = OBJECT_TO_JSVAL (fp->thisp);
    if (script->flags & SWFDEC_SCRIPT_PRELOAD_ARGS)
  
    if (script->flags & SWFDEC_SCRIPT_PRELOAD_SUPER ||
	script->flags & SWFDEC_SCRIPT_PRELOAD_ROOT ||
	script->flags & SWFDEC_SCRIPT_PRELOAD_PARENT) {
      g_assert_not_reached ();
    }
    if (script->flags & SWFDEC_SCRIPT_PRELOAD_GLOBAL)
      fp->vars[preload_reg++] = OBJECT_TO_JSVAL (player->jsobj);
  }
  /* set up the script */
  startpc = pc = script->buffer->data;
  endpc = startpc + script->buffer->length;
  fp->pc = pc;
  /* set up stack */
  startsp = js_AllocStack (cx, STACKSIZE, &mark);
  if (!startsp) {
    ok = JS_FALSE;
    goto out;
  }
  fp->spbase = startsp;
  fp->sp = startsp;
  endsp = startsp + STACKSIZE;
  /* Check for too much nesting, or too deep a C stack. */
  if (++cx->interpLevel == MAX_INTERP_LEVEL ||
      !JS_CHECK_STACK_SIZE(cx, stack_check)) {
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
    ok = JS_FALSE;
    goto out;
  }
  /* initialize the constant pool */
  if (script->constant_pool) {
    spec = actions + 0x88;
    ok = spec->exec[version] (cx, 0x88, script->constant_pool->data + 3,
	script->constant_pool->length - 3);
    if (!ok) {
      SWFDEC_WARNING ("Constant pool initialization failed");
      goto out;
    }
  }

  while (TRUE) {
    /* check pc */
    if (pc == endpc) {
      /* scripts created via DefineFunction or the Return action use this way out */
      *rval = fp->rval;
      break;
    }
    if (pc < startpc || pc >= endpc) {
      SWFDEC_ERROR ("pc %p not in valid range [%p, %p) anymore", pc, startpc, endpc);
      goto internal_error;
    }

    /* run interrupt handler */
    if (cx->runtime->interruptHandler) {
      jsval tmp;
      switch (cx->runtime->interruptHandler (cx, NULL, pc, &tmp,
	      cx->runtime->interruptHandlerData)) {
	case JSTRAP_ERROR:
	  ok = JS_FALSE;
          goto out;
        case JSTRAP_CONTINUE:
          break;
        case JSTRAP_RETURN:
          fp->rval = tmp;
          goto out;
        case JSTRAP_THROW:
          cx->throwing = JS_TRUE;
          cx->exception = tmp;
          ok = JS_FALSE;
          goto out;
        default:
	  g_assert_not_reached ();
	  break;
      }
    }

    /* decode next action */
    action = *pc;
    spec = actions + action;
    if (action == 0)
      break;
    if (action & 0x80) {
      if (pc + 2 >= endpc) {
	SWFDEC_ERROR ("action %u length value out of range", action);
	goto internal_error;
      }
      data = pc + 3;
      len = pc[1] | pc[2] << 8;
      if (data + len > endpc) {
	SWFDEC_ERROR ("action %u length %u out of range", action, len);
	goto internal_error;
      }
      nextpc = pc + 3 + len;
    } else {
      data = NULL;
      len = 0;
      nextpc = pc + 1;
    }
    /* check action is valid */
    if (spec->exec[version] == NULL) {
      SWFDEC_ERROR ("cannot interpret action %u %s for version %u", action,
	  spec->name ? spec->name : "Unknown", script->version);
      goto internal_error;
    }
    if (fp->sp - spec->remove < startsp) {
      SWFDEC_ERROR ("stack underflow while trying to execute %s - requires %u args, only got %u", 
	  spec->name, spec->remove, fp->sp - fp->spbase);
      goto internal_error;
    }
    if (spec->add < 0) {
      /* HACK FIXME: if added args are -1, we pass the number of free space on the stack 
       * instead of the action */
      action = endsp - fp->sp;
    } else {
      if (fp->sp + spec->add - MAX (spec->remove, 0) > endsp) {
	SWFDEC_ERROR ("FIXME: implement stack expansion, we got an overflow");
	goto internal_error;
      }
    }
    checksp = (spec->add >= 0 && spec->remove >= 0) ? fp->sp + spec->add - spec->remove : NULL;
    ok = spec->exec[version] (cx, action, data, len);
    if (!ok) {
      SWFDEC_WARNING ("action %s failed", spec->name);
      goto out;
    }
    if (checksp != NULL && checksp != fp->sp) {
      /* check stack was handled like expected */
      g_error ("action %s was supposed to change the stack by %d (+%d -%d), but it changed by %d",
	  spec->name, spec->add - spec->remove, spec->add, spec->remove,
	  fp->sp - checksp + spec->add - spec->remove);
    }
    if (fp->pc == pc) {
      fp->pc = pc = nextpc;
    } else {
      pc = fp->pc;
    }
  }

out:
#if 0
  /* FIXME: exception handling */
  if (!ok && cx->throwing) {
      /*
       * Look for a try block within this frame that can catch the exception.
       */
      SCRIPT_FIND_CATCH_START(script, pc, pc);
      if (pc) {
	  len = 0;
	  cx->throwing = JS_FALSE;    /* caught */
	  ok = JS_TRUE;
	  goto advance_pc;
      }
  }
#endif
no_catch:

  /* Reset sp before freeing stack slots, because our caller may GC soon. */
  fp->sp = fp->spbase;
  fp->spbase = NULL;
  js_FreeStack(cx, mark);
  cx->interpLevel--;
  swfdec_script_unref (script);
  return ok;

internal_error:
  *rval = JSVAL_VOID;
  ok = JS_TRUE;
  goto no_catch;
}

jsval
swfdec_script_execute (SwfdecScript *script, SwfdecScriptable *scriptable)
{
  JSContext *cx;
  JSStackFrame *oldfp, frame;
  JSObject *obj;
  JSBool ok;
  void *mark;

  g_return_val_if_fail (script != NULL, JSVAL_VOID);
  g_return_val_if_fail (SWFDEC_IS_SCRIPTABLE (scriptable), JSVAL_VOID);

  cx = scriptable->jscx;
  obj = swfdec_scriptable_get_object (scriptable);
  if (obj == NULL)
    return JSVAL_VOID;
  oldfp = cx->fp;

  frame.callobj = frame.argsobj = NULL;
  frame.script = NULL;
  frame.varobj = obj;
  frame.fun = NULL;
  frame.swf = script;
  frame.constant_pool = NULL;
  frame.thisp = obj;
  frame.argc = 0;
  frame.argv = NULL;
  frame.annotation = NULL;
  frame.sharpArray = NULL;
  frame.rval = JSVAL_VOID;
  frame.down = NULL;
  frame.scopeChain = obj;
  frame.pc = NULL;
  frame.sp = oldfp ? oldfp->sp : NULL;
  frame.spbase = NULL;
  frame.sharpDepth = 0;
  frame.flags = 0;
  frame.dormantNext = NULL;
  frame.objAtomMap = NULL;

  /* allocate stack for variables */
  frame.nvars = 4;
  frame.vars = js_AllocStack (cx, frame.nvars, &mark);
  if (frame.vars == NULL) {
    return JS_FALSE;
  }
  frame.vars[0] = frame.vars[1] = frame.vars[2] = frame.vars[3] = JSVAL_VOID;

  if (oldfp) {
    g_assert (!oldfp->dormantNext);
    oldfp->dormantNext = cx->dormantFrameChain;
    cx->dormantFrameChain = oldfp;
  }

  cx->fp = &frame;

  /*
   * Use frame.rval, not result, so the last result stays rooted across any
   * GC activations nested within this js_Interpret.
   */
  ok = swfdec_script_interpret (script, cx, &frame.rval);

  js_FreeStack (cx, mark);
  if (frame.constant_pool)
    swfdec_constant_pool_free (frame.constant_pool);

  cx->fp = oldfp;
  if (oldfp) {
    g_assert (cx->dormantFrameChain == oldfp);
    cx->dormantFrameChain = oldfp->dormantNext;
    oldfp->dormantNext = NULL;
  }

  return ok ? frame.rval : JSVAL_VOID;
}

/*** UTILITY FUNCTIONS ***/

const char *
swfdec_action_get_name (guint action)
{
  g_return_val_if_fail (action < 256, NULL);

  return actions[action].name;
}

guint
swfdec_action_get_from_name (const char *name)
{
  guint i;

  g_return_val_if_fail (name != NULL, 0);

  for (i = 0; i < 256; i++) {
    if (actions[i].name && g_str_equal (name, actions[i].name))
      return i;
  }
  return 0;
}


