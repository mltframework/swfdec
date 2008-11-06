/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "swfdec_as_context.h"
#include "swfdec_as_array.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_function.h"
#include "swfdec_as_gcable.h"
#include "swfdec_as_initialize.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_interpret.h"
#include "swfdec_as_movie_value.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_types.h"
#include "swfdec_constant_pool.h"
#include "swfdec_debug.h"
#include "swfdec_gc_object.h"
#include "swfdec_internal.h" /* for swfdec_player_preinit_global() */
#include "swfdec_script.h"

/*** GARBAGE COLLECTION DOCS ***/

/**
 * SECTION:Internals
 * @title: Internals of the script engine
 * @short_description: understanding internals such as garbage collection
 * @see_also: #SwfdecAsContext, #SwfdecGcObject
 *
 * This section deals with the internals of the Swfdec Actionscript engine. You
 * should probably read this first when trying to write code with it. If you're
 * just trying to use Swfdec for creating Flash content, you can probably skip
 * this section.
 *
 * First, I'd like to note that the Swfdec script engine has to be modeled very 
 * closely after the existing Flash players. So if there are some behaviours
 * that seem stupid at first sight, it might very well be that it was chosen for
 * a very particular reason. Now on to the features.
 *
 * The Swfdec script engine tries to imitate Actionscript as good as possible.
 * Actionscript is similar to Javascript, but not equal. Depending on the 
 * version of the script executed it might be more or less similar. An important
 * example is that Flash in versions up to 6 did case-insensitive variable 
 * lookups.
 *
 * The script engine starts its life when it is initialized via 
 * swfdec_as_context_startup(). At that point, the basic objects are created.
 * After this function has been called, you can start executing code. Code
 * execution happens by calling swfdec_as_function_call_full() or convenience
 * wrappers like swfdec_as_object_run() or swfdec_as_object_call().
 *
 * It is also easily possible to extend the environment by adding new objects.
 * In fact, without doing so, the environment is pretty bare as it just contains
 * the basic Math, String, Number, Array, Date and Boolean objects. This is done
 * by adding #SwfdecAsNative functions to the environment. The easy way
 * to do this is via swfdec_as_object_add_function().
 *
 * The Swfdec script engine is dynamically typed and knows about different types
 * of values. See #SwfdecAsValue for the different values. Memory management is
 * done using a mark and sweep garbage collector. You can initiate a garbage 
 * collection cycle by calling swfdec_as_context_gc() or 
 * swfdec_as_context_maybe_gc(). You should do this regularly to avoid excessive
 * memory use. The #SwfdecAsContext will then collect the objects and strings it
 * is keeping track of. If you want to use an object or string in the script 
 * engine, you'll have to add it first via swfdec_as_object_add() or
 * swfdec_as_context_get_string() and swfdec_as_context_give_string(), 
 * respectively.
 *
 * Garbage-collected strings are special in Swfdec in that they are unique. This
 * means the same string exists exactly once. Because of this you can do 
 * equality comparisons using == instead of strcmp. It also means that if you 
 * forget to add a string to the context before using it, your app will most 
 * likely not work, since the string will not compare equal to any other string.
 *
 * When a garbage collection cycle happens, all reachable objects and strings 
 * are marked and all unreachable ones are freed. This is done by calling the
 * context class's mark function which will mark all reachable objects. This is
 * usually called the "root set". For any reachable object, the object's mark
 * function is called so that the object in turn can mark all objects it can 
 * reach itself. Marking is done via functions described below.
 */

/*** GTK-DOC ***/

/**
 * SECTION:SwfdecAsContext
 * @title: SwfdecAsContext
 * @short_description: the main script engine context
 * @see_also: SwfdecPlayer
 *
 * A #SwfdecAsContext provides the main execution environment for Actionscript
 * execution. It provides the objects typically available in ECMAScript and
 * manages script execution, garbage collection etc. #SwfdecPlayer is a
 * subclass of the context that implements Flash specific objects on top of it.
 * However, it is possible to use the context for completely different functions
 * where a sandboxed scripting environment is needed. An example is the Swfdec 
 * debugger.
 * <note>The Actionscript engine is similar, but not equal to Javascript. It
 * is not very different, but it is different.</note>
 */

/**
 * SwfdecAsContext
 *
 * This is the main object ued to hold the state of a script engine. All members 
 * are private and should not be accessed.
 *
 * Subclassing this structure to get scripting support in your own appliation is
 * encouraged.
 */

/**
 * SwfdecAsContextState
 * @SWFDEC_AS_CONTEXT_NEW: the context is not yet initialized, 
 *                         swfdec_as_context_startup() needs to be called.
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

/**
 * swfdec_as_context_abort:
 * @context: a #SwfdecAsContext
 * @reason: a string describing why execution was aborted
 *
 * Aborts script execution in @context. Call this functon if the script engine 
 * encountered a fatal error and cannot continue. A possible reason for this is
 * an out-of-memory condition.
 **/
void
swfdec_as_context_abort (SwfdecAsContext *context, const char *reason)
{
  g_return_if_fail (context);

  if (context->state != SWFDEC_AS_CONTEXT_ABORTED) {
    SWFDEC_ERROR ("abort: %s", reason);
    context->state = SWFDEC_AS_CONTEXT_ABORTED;
    g_object_notify (G_OBJECT (context), "aborted");
  }
}

/*** MEMORY MANAGEMENT ***/

/**
 * swfdec_as_context_try_use_mem:
 * @context: a #SwfdecAsContext
 * @bytes: number of bytes to use
 *
 * Tries to register @bytes additional bytes as in use by the @context. This
 * function keeps track of the memory that script code consumes. The scripting
 * engine won't be stopped, even if there wasn't enough memory left.
 *
 * Returns: %TRUE if the memory could be allocated. %FALSE on OOM.
 **/
gboolean
swfdec_as_context_try_use_mem (SwfdecAsContext *context, gsize bytes)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), FALSE);
  g_return_val_if_fail (bytes > 0, FALSE);

  if (context->state == SWFDEC_AS_CONTEXT_ABORTED)
    return FALSE;
  
  context->memory += bytes;
  context->memory_since_gc += bytes;
  SWFDEC_LOG ("+%4"G_GSIZE_FORMAT" bytes, total %7"G_GSIZE_FORMAT" (%7"G_GSIZE_FORMAT" since GC)",
      bytes, context->memory, context->memory_since_gc);

  return TRUE;
}

/**
 * swfdec_as_context_use_mem:
 * @context: a #SwfdecAsContext
 * @bytes: number of bytes to use
 *
 * Registers @bytes additional bytes as in use by the @context. This function
 * keeps track of the memory that script code consumes. If too much memory is
 * in use, this function may decide to stop the script engine with an out of
 * memory error.
 **/
void
swfdec_as_context_use_mem (SwfdecAsContext *context, gsize bytes)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (bytes > 0);

  /* FIXME: Don't forget to abort on OOM */
  if (!swfdec_as_context_try_use_mem (context, bytes)) {
    swfdec_as_context_abort (context, "Out of memory");
    /* add the memory anyway, as we're gonna make use of it. */
    context->memory += bytes;
    context->memory_since_gc += bytes;
  }
}

/**
 * swfdec_as_context_unuse_mem:
 * @context: a #SwfdecAsContext
 * @bytes: number of bytes to release
 *
 * Releases a number of bytes previously allocated using 
 * swfdec_as_context_use_mem(). See that function for details.
 **/
void
swfdec_as_context_unuse_mem (SwfdecAsContext *context, gsize bytes)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (bytes > 0);
  g_return_if_fail (context->memory >= bytes);

  context->memory -= bytes;
  SWFDEC_LOG ("-%4"G_GSIZE_FORMAT" bytes, total %7"G_GSIZE_FORMAT" (%7"G_GSIZE_FORMAT" since GC)",
      bytes, context->memory, context->memory_since_gc);
}

/*** GC ***/

static void
swfdec_as_context_remove_gc_objects (SwfdecAsContext *context)
{
  SwfdecGcObject *gc, *prev, *next;

  prev = NULL;
  gc = context->gc_objects;
  while (gc) {
    next = gc->next;
    /* we only check for mark here, not root, since this works on destroy, too */
    if (gc->flags & SWFDEC_AS_GC_MARK) {
      gc->flags &= ~SWFDEC_AS_GC_MARK;
      SWFDEC_LOG ("%s: %s %p", (gc->flags & SWFDEC_AS_GC_ROOT) ? "rooted" : "marked",
	  G_OBJECT_TYPE_NAME (gc), gc);
      prev = gc;
    } else {
      SWFDEC_LOG ("deleted: %s %p", G_OBJECT_TYPE_NAME (gc), gc);
      g_object_unref (gc);
      if (prev) {
	prev->next = next;
      } else {
	context->gc_objects = next;
      }
    }
    gc = next;
  }
}

static void
swfdec_as_context_collect_string (SwfdecAsContext *context, gpointer gc)
{
  SwfdecAsStringValue *string;

  string = gc;
  if (!g_hash_table_remove (context->interned_strings, string->string)) {
    g_assert_not_reached ();
  }
  swfdec_as_gcable_free (context, gc, sizeof (SwfdecAsStringValue) + string->length + 1);
}

static void
swfdec_as_context_collect_double (SwfdecAsContext *context, gpointer gc)
{
  swfdec_as_gcable_free (context, gc, sizeof (SwfdecAsDoubleValue));
}

static void
swfdec_as_context_collect_movie (SwfdecAsContext *context, gpointer gc)
{
  swfdec_as_movie_value_free ((SwfdecAsMovieValue *) gc);
}

static void
swfdec_as_context_collect (SwfdecAsContext *context)
{
  /* NB: This functions is called without GC from swfdec_as_context_dispose */
  SWFDEC_INFO (">> collecting garbage");
  
  swfdec_as_context_remove_gc_objects (context);

  context->objects = swfdec_as_gcable_collect (context, context->objects,
      (SwfdecAsGcableDestroyNotify) swfdec_as_object_free);
  context->strings = swfdec_as_gcable_collect (context, context->strings,
      swfdec_as_context_collect_string);
  context->numbers = swfdec_as_gcable_collect (context, context->numbers,
      swfdec_as_context_collect_double);
  context->movies = swfdec_as_gcable_collect (context, context->movies,
      swfdec_as_context_collect_movie);

  SWFDEC_INFO (">> done collecting garbage");
}

/**
 * swfdec_as_string_mark:
 * @string: a garbage-collected string
 *
 * Mark @string as being in use. Calling this function is only valid during
 * the marking phase of garbage collection.
 **/
void
swfdec_as_string_mark (const char *string)
{
  SwfdecAsStringValue *value;

  g_return_if_fail (string != NULL);

  value = (SwfdecAsStringValue *) (gpointer) ((guint8 *) string - G_STRUCT_OFFSET (SwfdecAsStringValue, string));
  if (!SWFDEC_AS_GCABLE_FLAG_IS_SET (value, SWFDEC_AS_GC_ROOT))
    SWFDEC_AS_GCABLE_SET_FLAG (value, SWFDEC_AS_GC_MARK);
}

/**
 * swfdec_as_value_mark:
 * @value: a #SwfdecAsValue
 *
 * Mark @value as being in use. This is just a convenience function that calls
 * the right marking function depending on the value's type. Calling this 
 * function is only valid during the marking phase of garbage collection.
 **/
void
swfdec_as_value_mark (SwfdecAsValue *value)
{
  SwfdecAsGcable *gcable;
  SwfdecAsValueType type = SWFDEC_AS_VALUE_GET_TYPE (*value);
  
  if (!SWFDEC_AS_TYPE_IS_GCABLE (type))
    return;

  gcable = SWFDEC_AS_VALUE_GET_VALUE (*value);
  if (SWFDEC_AS_GCABLE_FLAG_IS_SET (gcable, SWFDEC_AS_GC_ROOT | SWFDEC_AS_GC_MARK))
    return;

  switch (type) {
    case SWFDEC_AS_TYPE_STRING:
    case SWFDEC_AS_TYPE_NUMBER:
      SWFDEC_AS_GCABLE_SET_FLAG (gcable, SWFDEC_AS_GC_MARK);
      break;
    case SWFDEC_AS_TYPE_OBJECT:
      swfdec_as_object_mark ((SwfdecAsObject *) gcable);
      break;
    case SWFDEC_AS_TYPE_MOVIE:
      swfdec_as_movie_value_mark ((SwfdecAsMovieValue *) gcable);
      break;
    case SWFDEC_AS_TYPE_UNDEFINED:
    case SWFDEC_AS_TYPE_NULL:
    case SWFDEC_AS_TYPE_BOOLEAN:
    case SWFDEC_AS_TYPE_INT:
    default:
      g_assert_not_reached ();
      break;
  }
}

/* FIXME: replace this with refcounted strings? */
static void 
swfdec_as_context_mark_constant_pools (gpointer key, gpointer value, gpointer unused)
{
  SwfdecConstantPool *pool = value;
  guint i;

  for (i = 0; i < swfdec_constant_pool_size (pool); i++) {
    const char *s = swfdec_constant_pool_get (pool, i);
    if (s)
      swfdec_as_string_mark (s);
  }
}

static void
swfdec_as_context_do_mark (SwfdecAsContext *context)
{
  /* This if is needed for SwfdecPlayer */
  if (context->global)
    swfdec_as_object_mark (context->global);
  if (context->exception)
    swfdec_as_value_mark (&context->exception_value);
  g_hash_table_foreach (context->constant_pools, swfdec_as_context_mark_constant_pools, NULL);
}

/**
 * swfdec_as_context_gc:
 * @context: a #SwfdecAsContext
 *
 * Calls the Swfdec Gargbage collector and reclaims any unused memory. You 
 * should call this function or swfdec_as_context_maybe_gc() regularly.
 * <warning>Calling the GC during execution of code or initialization is not
 *          allowed.</warning>
 **/
void
swfdec_as_context_gc (SwfdecAsContext *context)
{
  SwfdecAsContextClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (context->frame == NULL);
  g_return_if_fail (context->state == SWFDEC_AS_CONTEXT_RUNNING);

  if (context->state == SWFDEC_AS_CONTEXT_ABORTED)
    return;
  SWFDEC_INFO ("invoking the garbage collector");
  klass = SWFDEC_AS_CONTEXT_GET_CLASS (context);
  g_assert (klass->mark);
  klass->mark (context);
  swfdec_as_context_collect (context);
  context->memory_since_gc = 0;
}

static gboolean
swfdec_as_context_needs_gc (SwfdecAsContext *context)
{
  return context->memory_since_gc >= context->memory_until_gc;
}

/**
 * swfdec_as_context_maybe_gc:
 * @context: a #SwfdecAsContext
 *
 * Calls the garbage collector if necessary. It's a good idea to call this
 * function regularly instead of swfdec_as_context_gc() as it only does collect
 * garage as needed. For example, #SwfdecPlayer calls this function after every
 * frame advancement.
 **/
void
swfdec_as_context_maybe_gc (SwfdecAsContext *context)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (context->state == SWFDEC_AS_CONTEXT_RUNNING);
  g_return_if_fail (context->frame == NULL);

  if (swfdec_as_context_needs_gc (context))
    swfdec_as_context_gc (context);
}

/*** SWFDEC_AS_CONTEXT ***/

enum {
  TRACE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_DEBUGGER,
  PROP_RANDOM_SEED,
  PROP_ABORTED,
  PROP_UNTIL_GC
};

G_DEFINE_TYPE (SwfdecAsContext, swfdec_as_context, G_TYPE_OBJECT)
static guint signals[LAST_SIGNAL] = { 0, };

static void
swfdec_as_context_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecAsContext *context = SWFDEC_AS_CONTEXT (object);

  switch (param_id) {
    case PROP_DEBUGGER:
      g_value_set_object (value, context->debugger);
      break;
    case PROP_ABORTED:
      g_value_set_boolean (value, context->state == SWFDEC_AS_CONTEXT_ABORTED);
      break;
    case PROP_UNTIL_GC:
      g_value_set_ulong (value, (gulong) context->memory_until_gc);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_as_context_set_property (GObject *object, guint param_id, const GValue *value, 
    GParamSpec * pspec)
{
  SwfdecAsContext *context = SWFDEC_AS_CONTEXT (object);

  switch (param_id) {
    case PROP_DEBUGGER:
      context->debugger = SWFDEC_AS_DEBUGGER (g_value_dup_object (value));
      break;
    case PROP_RANDOM_SEED:
      g_rand_set_seed (context->rand, g_value_get_uint (value));
      break;
    case PROP_UNTIL_GC:
      context->memory_until_gc = g_value_get_ulong (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_as_context_dispose (GObject *object)
{
  SwfdecAsContext *context = SWFDEC_AS_CONTEXT (object);

  while (context->stack)
    swfdec_as_stack_pop_segment (context);
  /* We need to make sure there's no exception here. Otherwise collecting 
   * frames that are inside a try block will assert */
  swfdec_as_context_catch (context, NULL);
  swfdec_as_context_collect (context);
  if (context->memory != 0) {
    g_critical ("%zu bytes of memory left over\n", context->memory);
  }
  g_assert (context->strings == NULL);
  g_assert (context->numbers == NULL);
  g_assert (g_hash_table_size (context->constant_pools) == 0);
  g_assert (context->gc_objects == 0);
  g_hash_table_destroy (context->constant_pools);
  g_hash_table_destroy (context->interned_strings);
  g_rand_free (context->rand);
  if (context->debugger) {
    g_object_unref (context->debugger);
    context->debugger = NULL;
  }

  G_OBJECT_CLASS (swfdec_as_context_parent_class)->dispose (object);
}

static void
swfdec_as_context_class_init (SwfdecAsContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_context_dispose;
  object_class->get_property = swfdec_as_context_get_property;
  object_class->set_property = swfdec_as_context_set_property;

  g_object_class_install_property (object_class, PROP_DEBUGGER,
      g_param_spec_object ("debugger", "debugger", "debugger used in this player",
	  SWFDEC_TYPE_AS_DEBUGGER, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_RANDOM_SEED,
      g_param_spec_uint ("random-seed", "random seed", 
	  "seed used for calculating random numbers",
	  0, G_MAXUINT32, 0, G_PARAM_WRITABLE)); /* FIXME: make this readwrite for replaying? */
  g_object_class_install_property (object_class, PROP_ABORTED,
      g_param_spec_boolean ("aborted", "aborted", "set when the script engine aborts due to an error",
	FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_UNTIL_GC,
      g_param_spec_ulong ("memory-until-gc", "memory until gc", 
	  "amount of bytes that need to be allocated before garbage collection triggers",
	  0, G_MAXULONG, 8 * 1024 * 1024, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /**
   * SwfdecAsContext::trace:
   * @context: the #SwfdecAsContext affected
   * @text: the debugging string
   *
   * Emits a debugging string while running. The effect of calling any swfdec 
   * functions on the emitting @context is undefined.
   */
  signals[TRACE] = g_signal_new ("trace", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING,
      G_TYPE_NONE, 1, G_TYPE_STRING);

  klass->mark = swfdec_as_context_do_mark;
}

static void
swfdec_as_context_init (SwfdecAsContext *context)
{
  const SwfdecAsConstantStringValue *s;

  context->version = G_MAXUINT;

  context->interned_strings = g_hash_table_new (g_str_hash, g_str_equal);
  context->constant_pools = g_hash_table_new (g_direct_hash, g_direct_equal);

  for (s = swfdec_as_strings; s->next; s++) {
    g_hash_table_insert (context->interned_strings, (gpointer) s->string, (gpointer) s);
  }
  context->rand = g_rand_new ();
  g_get_current_time (&context->start_time);
}

/*** STRINGS ***/

static const char *
swfdec_as_context_create_string (SwfdecAsContext *context, const char *string, gsize len)
{
  SwfdecAsStringValue *new;

  new = swfdec_as_gcable_alloc (context, sizeof (SwfdecAsStringValue) + len + 1);
  new->length = len;
  memcpy (new->string, string, new->length + 1);
  g_hash_table_insert (context->interned_strings, new->string, new);
  SWFDEC_AS_GCABLE_SET_NEXT (new, context->strings);
  context->strings = new;

  return new->string;
}

/**
 * swfdec_as_context_get_string:
 * @context: a #SwfdecAsContext
 * @string: a sting that is not garbage-collected
 *
 * Gets the garbage-collected version of @string. You need to call this function
 * for every not garbage-collected string that you want to use in Swfdecs script
 * interpreter.
 *
 * Returns: the garbage-collected version of @string
 **/
const char *
swfdec_as_context_get_string (SwfdecAsContext *context, const char *string)
{
  const SwfdecAsStringValue *ret;
  gsize len;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (string != NULL, NULL);

  ret = g_hash_table_lookup (context->interned_strings, string);
  if (ret)
    return ret->string;

  len = strlen (string);
  return swfdec_as_context_create_string (context, string, len);
}

/**
 * swfdec_as_context_give_string:
 * @context: a #SwfdecAsContext
 * @string: string to make refcounted
 *
 * Takes ownership of @string and returns a refcounted version of the same 
 * string. This function is the same as swfdec_as_context_get_string(), but 
 * takes ownership of @string.
 *
 * Returns: A refcounted string
 **/
const char *
swfdec_as_context_give_string (SwfdecAsContext *context, char *string)
{
  const char *ret;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (string != NULL, NULL);

  ret = swfdec_as_context_get_string (context, string);
  g_free (string);
  return ret;
}

/**
 * swfdec_as_context_is_constructing:
 * @context: a #SwfdecAsConstruct
 *
 * Determines if the contexxt is currently constructing. This information is
 * used by various constructors to do different things when they are 
 * constructing and when they are not. The Boolean, Number and String functions
 * for example setup the newly constructed objects when constructing but only
 * cast the provided argument when being called.
 *
 * Returns: %TRUE if the currently executing frame is a constructor
 **/
gboolean
swfdec_as_context_is_constructing (SwfdecAsContext *context)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), FALSE);

  return context->frame && context->frame->construct;
}

/**
 * swfdec_as_context_get_frame:
 * @context: a #SwfdecAsContext
 *
 * This is a debugging function. It gets the topmost stack frame that is 
 * currently executing. If no function is executing, %NULL is returned.
 *
 * Returns: the currently executing frame or %NULL if none
 **/
SwfdecAsFrame *
swfdec_as_context_get_frame (SwfdecAsContext *context)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  return context->frame;
}

/**
 * swfdec_as_context_throw:
 * @context: a #SwfdecAsContext
 * @value: a #SwfdecAsValue to be thrown
 *
 * Throws a new exception in the @context using the given @value. This function
 * can only be called if the @context is not already throwing an exception.
 **/
void
swfdec_as_context_throw (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (!context->exception);

  context->exception = TRUE;
  context->exception_value = *value;
}

/**
 * swfdec_as_context_catch:
 * @context: a #SwfdecAsContext
 * @value: a #SwfdecAsValue to be thrown
 *
 * Removes the currently thrown exception from @context and sets @value to the
 * thrown value
 *
 * Returns: %TRUE if an exception was catched, %FALSE otherwise
 **/
gboolean
swfdec_as_context_catch (SwfdecAsContext *context, SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), FALSE);

  if (!context->exception)
    return FALSE;

  if (value != NULL)
    *value = context->exception_value;

  context->exception = FALSE;
  SWFDEC_AS_VALUE_SET_UNDEFINED (&context->exception_value);

  return TRUE;
}

/**
 * swfdec_as_context_get_time:
 * @context: a #SwfdecAsContext
 * @tv: a #GTimeVal to be set to the context's time
 *
 * This function queries the time to be used inside this context. By default,
 * this is the same as g_get_current_time(), but it may be overwriten to allow
 * things such as slower or faster playback.
 **/
void
swfdec_as_context_get_time (SwfdecAsContext *context, GTimeVal *tv)
{
  SwfdecAsContextClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (tv != NULL);

  klass = SWFDEC_AS_CONTEXT_GET_CLASS (context);
  if (klass->get_time)
    klass->get_time (context, tv);
  else
    g_get_current_time (tv);
}

/**
 * swfdec_as_context_return:
 * @context: the context to return the topmost frame in
 * @return_value: return value of the function or %NULL for none. An undefined
 *                value will be used in that case.
 *
 * Ends execution of the currently executing frame and continues execution with 
 * its parent frame.
 **/
void
swfdec_as_context_return (SwfdecAsContext *context, SwfdecAsValue *return_value)
{
  SwfdecAsValue retval;
  SwfdecAsFrame *frame, *next;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (context->frame != NULL);

  frame = context->frame;

  /* save return value in case it was on the stack somewhere */
  if (frame->construct) {
    retval = frame->thisp;
  } else if (return_value) {
    retval = *return_value;
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&retval);
  }
  /* pop frame and leftover stack */
  next = frame->next;
  context->frame = next;
  g_assert (context->call_depth > 0);
  context->call_depth--;
  while (context->base > frame->stack_begin || 
      context->end < frame->stack_begin)
    swfdec_as_stack_pop_segment (context);
  context->cur = frame->stack_begin;
  /* setup stack for previous frame */
  if (next) {
    if (next->stack_begin >= &context->stack->elements[0] &&
	next->stack_begin <= context->cur) {
      context->base = next->stack_begin;
    } else {
      context->base = &context->stack->elements[0];
    }
  } else {
    g_assert (context->stack->next == NULL);
    context->base = &context->stack->elements[0];
  }
  /* pop argv if on stack */
  if (frame->argv == NULL && frame->argc > 0) {
    guint i = frame->argc;
    while (TRUE) {
      guint n = context->cur - context->base;
      n = MIN (n, i);
      swfdec_as_stack_pop_n (context, n);
      i -= n;
      if (i == 0)
	break;
      swfdec_as_stack_pop_segment (context);
    }
  }
  if (context->debugger) {
    SwfdecAsDebuggerClass *klass = SWFDEC_AS_DEBUGGER_GET_CLASS (context->debugger);

    if (klass->leave_frame)
      klass->leave_frame (context->debugger, context, frame, &retval);
  }
  /* set return value */
  if (frame->return_value) {
    *frame->return_value = retval;
  } else {
    swfdec_as_stack_ensure_free (context, 1);
    *swfdec_as_stack_push (context) = retval;
  }
  swfdec_as_frame_free (context, frame);
}

/**
 * swfdec_as_context_run:
 * @context: a #SwfdecAsContext
 *
 * Continues running the script engine. Executing code in this engine works
 * in 2 steps: First, you push the frame to be executed onto the stack, then
 * you call this function to execute it. So this function is the single entry
 * point to script execution. This might be helpful when debugging your 
 * application. 
 * <note>A lot of convenience functions like swfdec_as_object_run() call this 
 * function automatically.</note>
 **/
void
swfdec_as_context_run (SwfdecAsContext *context)
{
  SwfdecAsFrame *frame;
  SwfdecScript *script;
  const SwfdecActionSpec *spec;
  const guint8 *startpc, *pc, *endpc, *nextpc, *exitpc;
#ifndef G_DISABLE_ASSERT
  SwfdecAsValue *check;
#endif
  guint action, len;
  const guint8 *data;
  guint original_version;
  void (* step) (SwfdecAsDebugger *debugger, SwfdecAsContext *context);
  gboolean check_block; /* some opcodes avoid a scope check */

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (context->frame != NULL);
  g_return_if_fail (context->frame->script != NULL);
  g_return_if_fail (context->global); /* check here because of swfdec_sandbox_(un)use() */

  /* setup data */
  frame = context->frame;
  original_version = context->version;

  /* sanity checks */
  if (context->state == SWFDEC_AS_CONTEXT_ABORTED)
    goto error;
  if (!swfdec_as_context_check_continue (context))
    goto error;
  if (context->call_depth > 256) {
    /* we've exceeded our maximum call depth, throw an error and abort */
    swfdec_as_context_abort (context, "Stack overflow");
    goto error;
  }

  if (context->debugger) {
    SwfdecAsDebuggerClass *klass = SWFDEC_AS_DEBUGGER_GET_CLASS (context->debugger);
    step = klass->step;
  } else {
    step = NULL;
  }

  script = frame->script;
  context->version = script->version;
  startpc = script->buffer->data;
  endpc = startpc + script->buffer->length;
  exitpc = script->exit;
  pc = frame->pc;
  check_block = TRUE;

  while (context->state < SWFDEC_AS_CONTEXT_ABORTED) {
    if (context->exception) {
      swfdec_as_frame_handle_exception (context, frame);
      if (frame != context->frame)
	goto out;
      pc = frame->pc;
      continue;
    }
    if (pc == exitpc) {
      swfdec_as_context_return (context, NULL);
      goto out;
    }
    if (pc < startpc || pc >= endpc) {
      SWFDEC_ERROR ("pc %p not in valid range [%p, %p) anymore", pc, startpc, endpc);
      goto error;
    }
    while (check_block && (pc < frame->block_start || pc >= frame->block_end)) {
      SWFDEC_LOG ("code exited block");
      swfdec_as_frame_pop_block (frame, context);
      pc = frame->pc;
      if (frame != context->frame)
	goto out;
      if (context->exception)
	break;
    }
    if (context->exception)
      continue;

    /* decode next action */
    action = *pc;
    /* invoke debugger if there is one */
    if (step) {
      frame->pc = pc;
      (* step) (context->debugger, context);
      if (frame != context->frame)
	goto out;
      if (frame->pc != pc)
	continue;
    }
    /* prepare action */
    spec = swfdec_as_actions + action;
    if (action & 0x80) {
      if (pc + 2 >= endpc) {
	SWFDEC_ERROR ("action %u length value out of range", action);
	goto error;
      }
      data = pc + 3;
      len = pc[1] | pc[2] << 8;
      if (data + len > endpc) {
	SWFDEC_ERROR ("action %u length %u out of range", action, len);
	goto error;
      }
      nextpc = pc + 3 + len;
    } else {
      data = NULL;
      len = 0;
      nextpc = pc + 1;
    }
    /* check action is valid */
    if (!spec->exec) {
      SWFDEC_WARNING ("cannot interpret action %3u 0x%02X %s for version %u, skipping it", action,
	  action, spec->name ? spec->name : "Unknown", script->version);
      frame->pc = pc = nextpc;
      check_block = TRUE;
      continue;
    }
    if (script->version < spec->version) {
      SWFDEC_WARNING ("cannot interpret action %3u 0x%02X %s for version %u, using version %u instead",
	  action, action, spec->name ? spec->name : "Unknown", script->version, spec->version);
    }
    if (spec->remove > 0) {
      if (spec->add > spec->remove)
	swfdec_as_stack_ensure_free (context, spec->add - spec->remove);
      swfdec_as_stack_ensure_size (context, spec->remove);
    } else {
      if (spec->add > 0)
	swfdec_as_stack_ensure_free (context, spec->add);
    }
    if (context->state > SWFDEC_AS_CONTEXT_RUNNING) {
      SWFDEC_WARNING ("context not running anymore, aborting");
      goto error;
    }
#ifndef G_DISABLE_ASSERT
    check = (spec->add >= 0 && spec->remove >= 0) ? context->cur + spec->add - spec->remove : NULL;
#endif
    /* execute action */
    spec->exec (context, action, data, len);
    /* adapt the pc if the action did not, otherwise, leave it alone */
    /* FIXME: do this via flag? */
    if (frame->pc == pc) {
      frame->pc = pc = nextpc;
      check_block = TRUE;
    } else {
      if (frame->pc < pc &&
	  !swfdec_as_context_check_continue (context)) {
	goto error;
      }
      pc = frame->pc;
      check_block = FALSE;
    }
    if (frame == context->frame) {
#ifndef G_DISABLE_ASSERT
      if (check != NULL && check != context->cur) {
	g_error ("action %s was supposed to change the stack by %d (+%d -%d), but it changed by %td",
	    spec->name, spec->add - spec->remove, spec->add, spec->remove,
	    context->cur - check + spec->add - spec->remove);
      }
#endif
    } else {
      /* someone called/returned from a function, reread variables */
      goto out;
    }
  }

error:
  if (context->frame == frame)
    swfdec_as_context_return (context, NULL);
out:
  context->version = original_version;
  return;
}

/*** AS CODE ***/

static void
swfdec_as_context_ASSetPropFlags_set_one_flag (SwfdecAsObject *object,
    const char *s, guint *flags)
{
  swfdec_as_object_unset_variable_flags (object, s, flags[1]);
  swfdec_as_object_set_variable_flags (object, s, flags[0]);
}

static gboolean
swfdec_as_context_ASSetPropFlags_foreach (SwfdecAsObject *object,
    const char *s, SwfdecAsValue *val, guint cur_flags, gpointer data)
{
  guint *flags = data;

  /* shortcut if the flags already match */
  if (cur_flags == ((cur_flags &~ flags[1]) | flags[0]))
    return TRUE;

  swfdec_as_context_ASSetPropFlags_set_one_flag (object, s, flags);
  return TRUE;
}

SWFDEC_AS_NATIVE (1, 0, swfdec_as_context_ASSetPropFlags)
void
swfdec_as_context_ASSetPropFlags (SwfdecAsContext *cx, SwfdecAsObject *object, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  guint flags[2]; /* flags and mask - array so we can pass it as data pointer */
  SwfdecAsObject *obj;

  if (argc < 3)
    return;

  if (!SWFDEC_AS_VALUE_IS_COMPOSITE (argv[0]))
    return;
  obj = SWFDEC_AS_VALUE_GET_COMPOSITE (argv[0]);
  flags[0] = swfdec_as_value_to_integer (cx, argv[2]);
  flags[1] = (argc > 3) ? swfdec_as_value_to_integer (cx, argv[3]) : 0;

  if (flags[0] == 0 && flags[1] == 0) {
    // we should add autosizing length attribute here
    SWFDEC_FIXME ("ASSetPropFlags to set special length attribute not implemented");
    return;
  }

  if (SWFDEC_AS_VALUE_IS_NULL (argv[1])) {
    swfdec_as_object_foreach (obj, swfdec_as_context_ASSetPropFlags_foreach, flags);
  } else {
    char **split =
      g_strsplit (swfdec_as_value_to_string (cx, argv[1]), ",", -1);
    guint i;
    for (i = 0; split[i]; i++) {
      swfdec_as_context_ASSetPropFlags_set_one_flag (obj, 
	  swfdec_as_context_get_string (cx, split[i]), flags);
    }
    g_strfreev (split); 
  }
}

SWFDEC_AS_NATIVE (200, 19, swfdec_as_context_isFinite)
void
swfdec_as_context_isFinite (SwfdecAsContext *cx, SwfdecAsObject *object, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  double d;

  if (argc < 1)
    return;

  d = swfdec_as_value_to_number (cx, argv[0]);
  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, isfinite (d) ? TRUE : FALSE);
}

SWFDEC_AS_NATIVE (200, 18, swfdec_as_context_isNaN)
void
swfdec_as_context_isNaN (SwfdecAsContext *cx, SwfdecAsObject *object, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  double d;

  if (argc < 1)
    return;

  d = swfdec_as_value_to_number (cx, argv[0]);
  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, isnan (d) ? TRUE : FALSE);
}

SWFDEC_AS_NATIVE (100, 2, swfdec_as_context_parseInt)
void
swfdec_as_context_parseInt (SwfdecAsContext *cx, SwfdecAsObject *object, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  const char *s;
  char *tail;
  int radix = 10;
  gint64 i;

  SWFDEC_AS_CHECK (0, NULL, "s|i", &s, &radix);

  if (argc >= 2 && (radix < 2 || radix > 36)) {
    swfdec_as_value_set_number (cx, retval, NAN);
    return;
  }

  // special case, don't allow sign in front of the 0x
  if ((s[0] == '-' || s[0] == '+') && s[1] == '0' &&
      (s[2] == 'x' || s[2] == 'X')) {
    swfdec_as_value_set_number (cx, retval, NAN);
    return;
  }

  // automatic radix
  if (argc < 2) {
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
      radix = 16;
    } else if ((s[0] == '0' || ((s[0] == '+' || s[0] == '-') && s[1] == '0')) &&
	s[strspn (s+1, "01234567") + 1] == '\0') {
      radix = 8;
    } else {
      radix = 10;
    }
  }

  // skip 0x at the start
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    s += 2;

  // strtoll parses strings with 0x when given radix 16, but we don't want that
  if (radix == 16) {
    const char *skip = s + strspn (s, " \t\r\n");
    if (skip != s && (skip[0] == '-' || skip[0] == '+'))
      skip++;
    if (skip != s && skip[0] == '0' && (skip[1] == 'x' || skip[1] == 'X')) {
      swfdec_as_value_set_number (cx, retval, 0);
      return;
    }
  }

  i = g_ascii_strtoll (s, &tail, radix);

  if (tail == s) {
    swfdec_as_value_set_number (cx, retval, NAN);
    return;
  }

  if (i > G_MAXINT32 || i < G_MININT32) {
    swfdec_as_value_set_number (cx, retval, i);
  } else {
    swfdec_as_value_set_integer (cx, retval, i);
  }
}

SWFDEC_AS_NATIVE (100, 3, swfdec_as_context_parseFloat)
void
swfdec_as_context_parseFloat (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  char *s, *p, *tail;
  double d;

  if (argc < 1)
    return;

  // we need to remove everything after x or I, since strtod parses hexadecimal
  // numbers and Infinity
  s = g_strdup (swfdec_as_value_to_string (cx, argv[0]));
  if ((p = strpbrk (s, "xXiI")) != NULL) {
    *p = '\0';
  }

  d = g_ascii_strtod (s, &tail);

  if (tail == s) {
    swfdec_as_value_set_number (cx, retval, NAN);
  } else {
    swfdec_as_value_set_number (cx, retval, d);
  }

  g_free (s);
}

static void
swfdec_as_context_init_global (SwfdecAsContext *context)
{
  SwfdecAsValue val;

  swfdec_as_value_set_number (context, &val, NAN);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_NaN, &val);
  swfdec_as_value_set_number (context, &val, HUGE_VAL);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR_Infinity, &val);
}

void
swfdec_as_context_run_init_script (SwfdecAsContext *context, const guint8 *data, 
    gsize length, guint version)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (data != NULL);
  g_return_if_fail (length > 0);

  if (version > 4) {
    SwfdecBits bits;
    SwfdecScript *script;
    swfdec_bits_init_data (&bits, data, length);
    script = swfdec_script_new_from_bits (&bits, "init", version);
    if (script == NULL) {
      g_warning ("script passed to swfdec_as_context_run_init_script is invalid");
      return;
    }
    swfdec_as_object_run (context->global, script);
    swfdec_script_unref (script);
  } else {
    SWFDEC_LOG ("not running init script, since version is <= 4");
  }
}

/**
 * swfdec_as_context_startup:
 * @context: a #SwfdecAsContext
 *
 * Starts up the context. This function must be called before any Actionscript
 * is called on @context.
 **/
void
swfdec_as_context_startup (SwfdecAsContext *context)
{
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (context->state == SWFDEC_AS_CONTEXT_NEW);

  if (context->cur == NULL &&
      !swfdec_as_stack_push_segment (context))
    return;
  if (context->global == NULL)
    context->global = swfdec_as_object_new_empty (context);
  /* init the two internal functions */
  /* FIXME: remove them for normal contexts? */
  swfdec_player_preinit_global (context);
  /* get the necessary objects up to define objects and functions sanely */
  swfdec_as_object_init_context (context);
  /* define the global object and other important ones */
  swfdec_as_context_init_global (context);

  /* run init script */
  swfdec_as_context_run_init_script (context, swfdec_as_initialize, sizeof (swfdec_as_initialize), 8);

  if (context->state == SWFDEC_AS_CONTEXT_NEW)
    context->state = SWFDEC_AS_CONTEXT_RUNNING;
}

/**
 * swfdec_as_context_check_continue:
 * @context: the context that might be running too long
 *
 * Checks if the context has been running too long. If it has, it gets aborted.
 *
 * Returns: %TRUE if this player aborted.
 **/
gboolean
swfdec_as_context_check_continue (SwfdecAsContext *context)
{
  SwfdecAsContextClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), TRUE);

  klass = SWFDEC_AS_CONTEXT_GET_CLASS (context);
  if (klass->check_continue == NULL)
    return TRUE;
  if (!klass->check_continue (context)) {
    swfdec_as_context_abort (context, "Runtime exceeded");
    return FALSE;
  }
  return TRUE;
}

/**
 * swfdec_as_context_is_aborted:
 * @context: a #SwfdecAsContext
 *
 * Determines if the given context is aborted. An aborted context is not able
 * to execute any scripts. Aborting can happen if the script engine detects bad 
 * scripts that cause excessive memory usage, infinite loops or other problems.
 * In that case the script engine aborts for safety reasons.
 *
 * Returns: %TRUE if the player is aborted, %FALSE if it runs normally.
 **/
gboolean
swfdec_as_context_is_aborted (SwfdecAsContext *context)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), TRUE);

  return context->state == SWFDEC_AS_CONTEXT_ABORTED;
}

