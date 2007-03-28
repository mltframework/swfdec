/* SwfdecAs
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

#include <string.h>
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_object.h"
#include "swfdec_as_types.h"
#include "swfdec_debug.h"
#include "swfdec_script.h"

/*** GTK_DOC ***/

/**
 * SwfdecAsContextState
 * @SWFDEC_AS_CONTEXT_RUNNING: the context is running normally
 * @SWFDEC_AS_CONTEXT_INTERRUPTED: the context has been interrupted by a 
 *                             debugger
 * @SWFDEC_AS_CONTEXT_ABORTED: the context has aborted execution due to a 
 *                         fatal error
 *
 * The state of the context describes what operations are possible on the context.
 * It will be in the state @SWFDEC_AS_CONTEXT_STATE_RUNNING almost all the time. If
 * it is in the state @SWFDEC_AS_CONTEXT_STATE_ABORTED, it will not work anymore and
 * every operation on it will instantly fail.
 */

/*** RUNNING STATE ***/

static void
swfdec_context_trace (SwfdecAsContext *context, const char *message)
{
  SWFDEC_ERROR ("FIXME: implement trace");
}

void
swfdec_as_context_abort (SwfdecAsContext *context, const char *reason)
{
  g_return_if_fail (context);
  g_return_if_fail (context->state != SWFDEC_AS_CONTEXT_ABORTED);

  SWFDEC_ERROR (reason);
  context->state = SWFDEC_AS_CONTEXT_ABORTED;
  swfdec_context_trace (context, reason);
}

/*** MEMORY MANAGEMENT ***/

gboolean
swfdec_as_context_use_mem (SwfdecAsContext *context, gsize len)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), FALSE);
  g_return_val_if_fail (len > 0, FALSE);

  context->memory += len;
  return TRUE;
}

void
swfdec_as_context_unuse_mem (SwfdecAsContext *context, gsize len)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (len > 0);
  g_return_if_fail (context->memory >= len);

  context->memory -= len;
}

/*** GC ***/

static gboolean
swfdec_as_context_remove_strings (gpointer key, gpointer value, gpointer data)
{
  SwfdecAsContext *context = data;
  char *string;

  string = (char *) value;
  /* it doesn't matter that rooted strings aren't destroyed, they're constant */
  if (string[0] & SWFDEC_AS_GC_ROOT) {
    g_print ("rooted: %s\n", (char *) key);
    return FALSE;
  } else if (string[0] & SWFDEC_AS_GC_MARK) {
    g_print ("marked: %s\n", (char *) key);
    string[0] &= ~SWFDEC_AS_GC_MARK;
    return FALSE;
  } else {
    gsize len;
    g_print ("deleted: %s\n", (char *) key);
    len = (strlen ((char *) key) + 2);
    swfdec_as_context_unuse_mem (context, len);
    g_slice_free1 (len, value);
    return TRUE;
  }
}

static gboolean
swfdec_as_context_remove_objects (gpointer key, gpointer value, gpointer data)
{
  SwfdecAsObject *object;

  object = key;
  /* we only check for mark here, not root, since this works on destroy, too */
  if (object->flags & SWFDEC_AS_GC_MARK) {
    object->flags &= ~SWFDEC_AS_GC_MARK;
    g_print ("%s: %s %p\n", (object->flags & SWFDEC_AS_GC_ROOT) ? "rooted" : "marked",
	G_OBJECT_TYPE_NAME (object), object);
    return FALSE;
  } else {
    g_print ("deleted: %s %p\n", G_OBJECT_TYPE_NAME (object), object);
    swfdec_as_object_collect (object);
    return TRUE;
  }
}

static void
swfdec_as_context_collect (SwfdecAsContext *context)
{
  g_print (">> collecting garbage\n");
  /* NB: This functions is called without GC from swfdec_as_context_dispose */
  g_hash_table_foreach_remove (context->strings, 
    swfdec_as_context_remove_strings, context);
  g_hash_table_foreach_remove (context->objects, 
    swfdec_as_context_remove_objects, context);
  g_print (">> done collecting garbage\n");
}

void
swfdec_as_object_mark (SwfdecAsObject *object)
{
  SwfdecAsObjectClass *klass;

  if (object->flags & SWFDEC_AS_GC_MARK)
    return;
  object->flags |= SWFDEC_AS_GC_MARK;
  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  g_assert (klass->mark);
  klass->mark (object);
}

void
swfdec_as_string_mark (const char *string)
{
  char *str = (char *) string - 1;
  if (*str == 0)
    *str = SWFDEC_AS_GC_MARK;
}

void
swfdec_as_value_mark (SwfdecAsValue *value)
{
  g_return_if_fail (SWFDEC_IS_AS_VALUE (value));

  if (SWFDEC_AS_VALUE_IS_OBJECT (value)) {
    swfdec_as_object_mark (SWFDEC_AS_VALUE_GET_OBJECT (value));
  } else if (SWFDEC_AS_VALUE_IS_STRING (value)) {
    swfdec_as_string_mark (SWFDEC_AS_VALUE_GET_STRING (value));
  }
}

static void
swfdec_as_context_mark_roots (gpointer key, gpointer value, gpointer data)
{
  SwfdecAsObject *object = key;

  if ((object->flags & (SWFDEC_AS_GC_MARK | SWFDEC_AS_GC_ROOT)) == SWFDEC_AS_GC_ROOT)
    swfdec_as_object_mark (object);
}

void
swfdec_as_context_gc (SwfdecAsContext *context)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  SWFDEC_INFO ("invoking the garbage collector");
  g_hash_table_foreach (context->objects, swfdec_as_context_mark_roots, NULL);
  swfdec_as_context_collect (context);
}

/*** SWFDEC_AS_CONTEXT ***/

G_DEFINE_TYPE (SwfdecAsContext, swfdec_as_context, G_TYPE_OBJECT)

static void
swfdec_as_context_dispose (GObject *object)
{
  SwfdecAsContext *context = SWFDEC_AS_CONTEXT (object);

  swfdec_as_context_collect (context);
  if (context->memory != 0) {
    g_critical ("%zu bytes of memory left over\n", context->memory);
  }
  g_assert (g_hash_table_size (context->objects) == 0);
  g_hash_table_destroy (context->objects);
  g_hash_table_destroy (context->strings);

  G_OBJECT_CLASS (swfdec_as_context_parent_class)->dispose (object);
}

static void
swfdec_as_context_class_init (SwfdecAsContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_context_dispose;
}

static void
swfdec_as_context_init (SwfdecAsContext *context)
{
  guint i;

  context->strings = g_hash_table_new (g_str_hash, g_str_equal);
  context->objects = g_hash_table_new (g_direct_hash, g_direct_equal);

  for (i = 0; swfdec_as_strings[i]; i++) {
    g_hash_table_insert (context->strings, (char *) swfdec_as_strings[i] + 1, 
	(char *) swfdec_as_strings[i]);
  }
}

/*** STRINGS ***/

static const char *
swfdec_as_context_create_string (SwfdecAsContext *context, const char *string, gsize len)
{
  char *new;
  
  if (!swfdec_as_context_use_mem (context, sizeof (char) * (2 + len)))
    return SWFDEC_AS_STR_EMPTY;

  new = g_slice_alloc (2 + len);
  memcpy (&new[1], string, len);
  new[len + 1] = 0;
  new[0] = 0; /* GC flags */
  g_hash_table_insert (context->strings, new + 1, new);

  return new + 1;
}

const char *
swfdec_as_context_get_string (SwfdecAsContext *context, const char *string)
{
  const char *ret;
  gsize len;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (string != NULL, NULL);

  ret = g_hash_table_lookup (context->strings, string);
  if (ret != NULL)
    return ret;

  len = strlen (string);
  return swfdec_as_context_create_string (context, string, len);
}

SwfdecAsContext *
swfdec_as_context_new (void)
{
  return g_object_new (SWFDEC_TYPE_AS_CONTEXT, NULL);
}

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

extern const SwfdecActionSpec actions[256];
void
swfdec_as_context_run (SwfdecAsContext *context)
{
  SwfdecAsFrame *frame;
  SwfdecScript *script;
  const SwfdecActionSpec *spec;
  guint8 *startpc, *pc, *endpc, *nextpc;
  guint action, len;
  guint8 *data;
  int version;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  /* setup data */
  frame = context->frame;
  if (frame == NULL)
    return;
  script = frame->script;
  version = EXTRACT_VERSION (script->version);
  startpc = script->buffer->data;
  endpc = startpc + script->buffer->length;

  while (pc != endpc) {
    if (pc < startpc || pc >= endpc) {
      SWFDEC_ERROR ("pc %p not in valid range [%p, %p) anymore", pc, startpc, endpc);
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
#if 0
    if (spec->remove > 0) {
      //!swfdec_script_ensure_stack (cx, spec->remove)) {
    }
    if (spec->add > 0 &&
	TRUE) { //fp->sp + spec->add - MAX (spec->remove, 0) > fp->spend) {
      SWFDEC_ERROR ("FIXME: implement stack expansion, we got an overflow");
      goto internal_error;
    }
#ifndef G_DISABLE_ASSERT
    checksp = (spec->add >= 0 && spec->remove >= 0) ? fp->sp + spec->add - spec->remove : NULL;
#endif
#endif
    spec->exec[version] (NULL, action, data, len);
#if 0
#ifndef G_DISABLE_ASSERT
    if (checksp != NULL && checksp != fp->sp) {
      /* check stack was handled like expected */
      g_error ("action %s was supposed to change the stack by %d (+%d -%d), but it changed by %td",
	  spec->name, spec->add - spec->remove, spec->add, spec->remove,
	  fp->sp - checksp + spec->add - spec->remove);
    }
#endif
    if (fp->pc == pc) {
      fp->pc = pc = nextpc;
    } else {
      pc = fp->pc;
    }
#endif
  }

internal_error:
  return;
}

