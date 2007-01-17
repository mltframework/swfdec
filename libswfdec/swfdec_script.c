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

/*** SUPPORT FUNCTIONS ***/

#include "swfdec_movie.h"

static SwfdecMovie *
swfdec_action_get_target (JSContext *cx)
{
  return swfdec_scriptable_from_jsval (cx, OBJECT_TO_JSVAL (cx->fp->scopeChain), SWFDEC_TYPE_MOVIE);
}

/*** ALL THE ACTION IS HERE ***/

static JSBool
swfdec_action_stop (JSContext *cx, guint action, const guint8 *data, guint len)
{
  SwfdecMovie *movie = swfdec_action_get_target (cx);
  if (movie)
    movie->stopped = TRUE;
  return JS_TRUE;
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

typedef JSBool (* SwfdecActionExec) (JSContext *cx, guint action, const guint8 *data, guint len);
typedef struct {
  const char *		name;		/* name identifying the action */
  char *		(* print)	(guint action, const guint8 *data, guint len);
  int			remove;		/* values removed from stack or -1 for dynamic */
  guint			add;		/* values added to the stack */
  SwfdecActionExec	exec[MAXSCRIPTVERSION - MINSCRIPTVERSION + 1];
					/* array is for version 3, 4, 5, 6, 7+ */
} SwfdecActionSpec;

static const SwfdecActionSpec actions[256] = {
  /* version 3 */
  [0x04] = { "NextFrame", NULL },
  [0x05] = { "PreviousFrame", NULL },
  [0x06] = { "Play", NULL },
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
  [0x1c] = { "GetVariable", NULL },
  [0x1d] = { "SetVariable", NULL },
  [0x20] = { "SetTarget2", NULL },
  [0x21] = { "StringAdd", NULL },
  [0x22] = { "GetProperty", NULL },
  [0x23] = { "SetProperty", NULL },
  [0x24] = { "CloneSprite", NULL },
  [0x25] = { "RemoveSprite", NULL },
  [0x26] = { "Trace", NULL },
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
  [0x81] = { "GotoFrame", swfdec_action_print_goto_frame },
  [0x83] = { "GetURL", NULL },
  /* version 5 */
  [0x87] = { "StoreRegister", NULL },
  [0x88] = { "ConstantPool", NULL },
  /* version 3 */
  [0x8a] = { "WaitForFrame", swfdec_action_print_wait_for_frame },
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
  [0x96] = { "Push", NULL },
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
  guint8 *data;

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
validate_action (guint action, const guint8 *data, guint len, gpointer script)
{
  /* we might want to do stuff here for certain actions */
  {
    char *foo = swfdec_script_print_action (action, data, len);
    if (foo == NULL)
      return FALSE;
    g_print ("%s\n", foo);
  }
  return TRUE;
}

SwfdecScript *
swfdec_script_new (SwfdecBits *bits, const char *name, unsigned int version)
{
  SwfdecScript *script;
  guchar *start;
  
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
  version = MAX (script->version - MINSCRIPTVERSION, MAXSCRIPTVERSION - MINSCRIPTVERSION);
  *rval = JSVAL_VOID;
  fp = cx->fp;
  /* set up the script */
  startpc = pc = script->buffer->data;
  endpc = startpc + script->buffer->length;
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
    if (fp->sp + spec->add - MAX (spec->remove, 0) > endsp) {
      SWFDEC_ERROR ("FIXME: implement stack expansion, we got an overflow");
      goto internal_error;
    }
    ok = spec->exec[version] (cx, action, data, len);
    if (!ok)
      goto out;
    pc = nextpc;
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

  cx->fp = oldfp;
  if (oldfp) {
    g_assert (cx->dormantFrameChain == oldfp);
    cx->dormantFrameChain = oldfp->dormantNext;
    oldfp->dormantNext = NULL;
  }

  return ok ? frame.rval : JSVAL_VOID;
}
