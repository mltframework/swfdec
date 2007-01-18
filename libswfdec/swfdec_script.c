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
#include "swfdec_scriptable.h"
#include "js/jscntxt.h"
#include "js/jsinterp.h"

#include <string.h>
#include "swfdec_decoder.h"
#include "swfdec_js.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_root_movie.h"

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

/*** SUPPORT FUNCTIONS ***/

static SwfdecMovie *
swfdec_action_get_target (JSContext *cx)
{
  return swfdec_scriptable_from_jsval (cx, OBJECT_TO_JSVAL (cx->fp->scopeChain), SWFDEC_TYPE_MOVIE);
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
    SwfdecDecoder *dec = SWFDEC_ROOT_MOVIE (movie->root)->decoder;
    loaded = dec->frames_loaded;
    g_assert (loaded <= movie->n_frames);
  } else {
    loaded = movie->n_frames;
  }
  if (loaded < frame) {
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
      case 5: /* boolean */
	*cx->fp->sp++ = swfdec_bits_get_u8 (&bits) ? JSVAL_TRUE : JSVAL_FALSE;
	break;
      case 6: /* double */
	{
	  double d = swfdec_bits_get_float (&bits);
	  if (!JS_NewDoubleValue (cx, d, cx->fp->sp))
	    return JS_FALSE;
	  cx->fp->sp++;
	  break;
	}
      case 7: /* 32bit int */
	{
	  /* FIXME: spec says U32, do they mean this? */
	  guint i = swfdec_bits_get_u32 (&bits);
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
      case 4: /* register */
      default:
	SWFDEC_ERROR ("Push: type %u not implemented", type);
	return JS_FALSE;
    }
  }
  return swfdec_bits_left (&bits) ? JS_TRUE : JS_FALSE;
}

static JSBool
swfdec_action_get_variable (JSContext *cx, guint action, const guint8 *data, guint len)
{
  const char *s;

  s = swfdec_js_to_string (cx, cx->fp->sp[-1]);
  if (s == NULL)
    return JS_FALSE;
  cx->fp->sp[-1] = swfdec_js_eval (cx, cx->fp->scopeChain, s);
  return JS_TRUE;
}

static JSBool
swfdec_action_trace (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecPlayer *player = JS_GetContextPrivate (cx);
  const char *bytes;

  bytes = swfdec_js_to_string (cx, cx->fp->sp[-1]);
  if (bytes == NULL)
    return JS_TRUE;

  swfdec_player_trace (player, bytes);
  return JS_TRUE;
}

/*** PRINT FUNCTIONS ***/

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
      case 5: /* boolean */
	g_string_append (string, swfdec_bits_get_u8 (&bits) ? "True" : "False");
	break;
      case 6: /* double */
	g_string_append_printf (string, "%g", swfdec_bits_get_double (&bits));
	break;
      case 7: /* 32bit int */
	g_string_append_printf (string, "%u", swfdec_bits_get_u32 (&bits));
	break;
      case 8: /* 8bit ConstantPool address */
	g_string_append_printf (string, "Pool %u", swfdec_bits_get_u8 (&bits));
	break;
      case 9: /* 16bit ConstantPool address */
	g_string_append_printf (string, "Pool %u", swfdec_bits_get_u16 (&bits));
	break;
      case 4: /* register */
      default:
	SWFDEC_ERROR ("Push: type %u not implemented", type);
	return JS_FALSE;
    }
  }
  return g_string_free (string, FALSE);
}

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
swfdec_action_print_goto_frame (guint action, const guint8 *data, guint len)
{
  guint frame;

  if (len != 2)
    return NULL;

  frame = GUINT16_FROM_LE (*((guint16 *) data));
  return g_strdup_printf ("GotoFrame %u", frame);
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
#define EXTRACT_VERSION(v) MAX ((v) - MINSCRIPTVERSION, MAXSCRIPTVERSION - MINSCRIPTVERSION)

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
  [0x04] = { "NextFrame", NULL },
  [0x05] = { "PreviousFrame", NULL },
  [0x06] = { "Play", NULL, 0, 0, { swfdec_action_play, swfdec_action_play, swfdec_action_play, swfdec_action_play, swfdec_action_play } },
  [0x07] = { "Stop", NULL, 0, 0, { swfdec_action_stop, swfdec_action_stop, swfdec_action_stop, swfdec_action_stop, swfdec_action_stop } },
  [0x08] = { "ToggleQuality", NULL },
  [0x09] = { "StopSounds", NULL },
  /* version 4 */
  [0x0a] = { "Add", NULL },
  [0x0b] = { "Subtract", NULL },
  [0x0c] = { "Multiply", NULL },
  [0x0d] = { "Divide", NULL },
  [0x0e] = { "Equals", NULL },
  [0x0f] = { "Less", NULL },
  [0x10] = { "And", NULL },
  [0x11] = { "Or", NULL },
  [0x12] = { "Not", NULL },
  [0x13] = { "StringEquals", NULL },
  [0x14] = { "StringLength", NULL },
  [0x15] = { "StringExtract", NULL },
  [0x17] = { "Pop", NULL },
  [0x18] = { "ToInteger", NULL },
  [0x1c] = { "GetVariable", NULL, 1, 1, { NULL, swfdec_action_get_variable, swfdec_action_get_variable, swfdec_action_get_variable, swfdec_action_get_variable } },
  [0x1d] = { "SetVariable", NULL },
  [0x20] = { "SetTarget2", NULL },
  [0x21] = { "StringAdd", NULL },
  [0x22] = { "GetProperty", NULL },
  [0x23] = { "SetProperty", NULL },
  [0x24] = { "CloneSprite", NULL },
  [0x25] = { "RemoveSprite", NULL },
  [0x26] = { "Trace", NULL, 1, 0, { NULL, swfdec_action_trace, swfdec_action_trace, swfdec_action_trace, swfdec_action_trace } },
  [0x27] = { "StartDrag", NULL },
  [0x28] = { "EndDrag", NULL },
  [0x29] = { "StringLess", NULL },
  /* version 7 */
  [0x2a] = { "Throw", NULL },
  [0x2b] = { "Cast", NULL },
  [0x2c] = { "Implements", NULL },
  /* version 4 */
  [0x30] = { "RandomNumber", NULL },
  [0x31] = { "MBStringLength", NULL },
  [0x32] = { "CharToAscii", NULL },
  [0x33] = { "AsciiToChar", NULL },
  [0x34] = { "GetTime", NULL },
  [0x35] = { "MBStringExtract", NULL },
  [0x36] = { "MBCharToAscii", NULL },
  [0x37] = { "MVAsciiToChar", NULL },
  /* version 5 */
  [0x3a] = { "Delete", NULL },
  [0x3b] = { "Delete2", NULL },
  [0x3c] = { "DefineLocal", NULL },
  [0x3d] = { "CallFunction", NULL },
  [0x3e] = { "Return", NULL },
  [0x3f] = { "Modulo", NULL },
  [0x40] = { "NewObject", NULL },
  [0x41] = { "DefineLocal2", NULL },
  [0x42] = { "InitArray", NULL },
  [0x43] = { "InitObject", NULL },
  [0x44] = { "Typeof", NULL },
  [0x45] = { "TargetPath", NULL },
  [0x46] = { "Enumerate", NULL },
  [0x47] = { "Add2", NULL },
  [0x48] = { "Less2", NULL },
  [0x49] = { "Equals2", NULL },
  [0x4a] = { "ToNumber", NULL },
  [0x4b] = { "ToString", NULL },
  [0x4c] = { "PushDuplicate", NULL },
  [0x4d] = { "Swap", NULL },
  [0x4e] = { "GetMember", NULL },
  [0x4f] = { "SetMember", NULL }, /* apparently the result is ignored */
  [0x50] = { "Increment", NULL },
  [0x51] = { "Decrement", NULL },
  [0x52] = { "CallMethod", NULL },
  [0x53] = { "NewMethod", NULL },
  /* version 6 */
  [0x54] = { "InstanceOf", NULL },
  [0x55] = { "Enumerate2", NULL },
  /* version 5 */
  [0x60] = { "BitAnd", NULL },
  [0x61] = { "BitOr", NULL },
  [0x62] = { "BitXor", NULL },
  [0x63] = { "BitLShift", NULL },
  [0x64] = { "BitRShift", NULL },
  [0x65] = { "BitURShift", NULL },
  /* version 6 */
  [0x66] = { "StrictEquals", NULL },
  [0x67] = { "Greater", NULL },
  [0x68] = { "StringGreater", NULL },
  /* version 7 */
  [0x69] = { "Extends", NULL },

  /* version 3 */
  [0x81] = { "GotoFrame", swfdec_action_print_goto_frame, 0, 0, { swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame, swfdec_action_goto_frame } },
  [0x83] = { "GetURL", NULL },
  /* version 5 */
  [0x87] = { "StoreRegister", NULL },
  [0x88] = { "ConstantPool", swfdec_action_print_constant_pool, 0, 0, { NULL, NULL, swfdec_action_constant_pool, swfdec_action_constant_pool, swfdec_action_constant_pool } },
  /* version 3 */
  [0x8a] = { "WaitForFrame", swfdec_action_print_wait_for_frame, 0, 0, { swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame, swfdec_action_wait_for_frame } },
  [0x8b] = { "SetTarget", NULL },
  [0x8c] = { "GotoLabel", NULL },
  /* version 4 */
  [0x8d] = { "WaitForFrame2", NULL },
  /* version 7 */
  [0x8e] = { "DefineFunction2", NULL },
  [0x8f] = { "Try", NULL },
  /* version 5 */
  [0x94] = { "With", NULL },
  /* version 4 */
  [0x96] = { "Push", swfdec_action_print_push, 0, -1, { NULL, swfdec_action_push, swfdec_action_push, swfdec_action_push, swfdec_action_push } },
  [0x99] = { "Jump", NULL },
  [0x9a] = { "GetURL2", NULL },
  /* version 5 */
  [0x9b] = { "DefineFunction", NULL },
  /* version 4 */
  [0x9d] = { "If", NULL },
  [0x9e] = { "Call", NULL },
  [0x9f] = { "GotoFrame2", NULL }
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

typedef gboolean (* SwfdecScriptForeachFunc) (guint action, const guint8 *data, guint len, gpointer user_data);
static gboolean
swfdec_script_foreach (SwfdecBits *bits, SwfdecScriptForeachFunc func, gpointer user_data)
{
  guint action, len;
  const guint8 *data;

  while ((action = swfdec_bits_get_u8 (bits))) {
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
    if (!func (action, data, len, user_data))
      return FALSE;
  }
  return TRUE;
}

/*** PUBLIC API ***/

static gboolean
validate_action (guint action, const guint8 *data, guint len, gpointer scriptp)
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
#if 1
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

  if (!swfdec_script_foreach (bits, validate_action, script)) {
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
  if (script->refcount > 0)
    return;

  swfdec_buffer_unref (script->buffer);
  g_free (script->name);
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
  jsval *startsp, *endsp;
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

  /* only valid return value */
  while (TRUE) {
    /* check pc */
    if (pc < startpc || pc >= endpc) {
      SWFDEC_ERROR ("pc %p not in valid range [%p, %p] anymore", pc, startpc, endpc);
      goto internal_error;
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
      action = endsp - fp->sp;
    } else {
      if (fp->sp + spec->add - MAX (spec->remove, 0) > endsp) {
	SWFDEC_ERROR ("FIXME: implement stack expansion, we got an overflow");
	goto internal_error;
      }
    }
    ok = spec->exec[version] (cx, action, data, len);
    if (!ok)
      goto out;
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
  frame.argc = frame.nvars = 0;
  frame.argv = frame.vars = NULL;
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

  /* FIXME: where to clean this up? */
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
