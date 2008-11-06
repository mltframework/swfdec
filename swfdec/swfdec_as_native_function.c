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

#include "swfdec_as_native_function.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

/*** GTK-DOC ***/

/**
 * SwfdecAsNative:
 * @context: #SwfdecAsContext
 * @thisp: the this object. <warning>Can be %NULL.</warning>
 * @argc: number of arguments passed to this function
 * @argv: the @argc arguments passed to this function
 * @retval: set to the return value. Initialized to undefined by default
 *
 * This is the prototype for all native functions.
 */

/**
 * SwfdecAsNativeFunction:
 *
 * This is the object type for native functions.
 */

/*** IMPLEMENTATION ***/

G_DEFINE_TYPE (SwfdecAsNativeFunction, swfdec_as_native_function, SWFDEC_TYPE_AS_FUNCTION)

static void
swfdec_as_native_function_call (SwfdecAsFunction *function, SwfdecAsObject *thisp, 
    gboolean construct, SwfdecAsObject *super_reference, guint n_args, 
    const SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  SwfdecAsNativeFunction *native = SWFDEC_AS_NATIVE_FUNCTION (function);
  SwfdecAsContext *cx = swfdec_gc_object_get_context (function);
  SwfdecAsFrame frame = { NULL, };
  SwfdecAsValue rval = { 0, };
  SwfdecAsValue *argv;

  g_assert (native->name);

  swfdec_as_frame_init_native (&frame, cx);
  frame.construct = construct;
  frame.function = function;
  /* We copy the target here so we have a proper SwfdecMovie reference inside native 
   * functions. This is for example necessary for swfdec_player_get_movie_by_value()
   * and probably other stuff that does variable lookups inside native functions.
   */
  /* FIXME: copy target or original target? */
  if (frame.next) {
    frame.target = frame.next->original_target;
    frame.original_target = frame.target;
  }
  if (thisp) {
    thisp = swfdec_as_object_resolve (thisp);
    swfdec_as_frame_set_this (&frame, thisp);
  }
  frame.argc = n_args;
  frame.argv = args;
  frame.return_value = return_value;
  frame.construct = construct;

  if (frame.argc == 0 || frame.argv != NULL) {
    /* FIXME FIXME FIXME: no casting here please! */
    argv = (SwfdecAsValue *) frame.argv;
  } else {
    SwfdecAsStack *stack;
    SwfdecAsValue *cur;
    guint i;
    if (frame.argc > 128) {
      SWFDEC_FIXME ("allow calling native functions with more than 128 args (this one has %u)",
	  frame.argc);
      frame.argc = 128;
    }
    argv = g_new (SwfdecAsValue, frame.argc);
    stack = cx->stack;
    cur = cx->cur;
    for (i = 0; i < frame.argc; i++) {
      if (cur <= &stack->elements[0]) {
	stack = stack->next;
	cur = &stack->elements[stack->used_elements];
      }
      cur--;
      argv[i] = *cur;
    }
  }
  native->native (cx, thisp, frame.argc, argv, &rval);
  if (argv != frame.argv)
    g_free (argv);
  swfdec_as_context_return (cx, &rval);
}

static void
swfdec_as_native_function_dispose (GObject *object)
{
  SwfdecAsNativeFunction *function = SWFDEC_AS_NATIVE_FUNCTION (object);

  g_free (function->name);
  function->name = NULL;

  G_OBJECT_CLASS (swfdec_as_native_function_parent_class)->dispose (object);
}

static void
swfdec_as_native_function_class_init (SwfdecAsNativeFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsFunctionClass *function_class = SWFDEC_AS_FUNCTION_CLASS (klass);

  object_class->dispose = swfdec_as_native_function_dispose;

  function_class->call = swfdec_as_native_function_call;
}

static void
swfdec_as_native_function_init (SwfdecAsNativeFunction *function)
{
}

/**
 * swfdec_as_native_function_new:
 * @context: a #SwfdecAsContext
 * @name: name of the function
 * @native: function to call when executed
 *
 * Creates a new native function, that will execute @native when called. You 
 * might want to use swfdec_as_object_add_function() instead of this function.
 *
 * Returns: a new #SwfdecAsFunction
 **/
SwfdecAsFunction *
swfdec_as_native_function_new (SwfdecAsContext *context, const char *name,
    SwfdecAsNative native)
{
  SwfdecAsFunction *fun;
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (native != NULL, NULL);

  fun = swfdec_as_native_function_new_bare (context, name, native);
  object = swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (fun));

  swfdec_as_object_set_constructor_by_name (object, SWFDEC_AS_STR_Function, NULL);
  swfdec_as_object_set_variable_flags (object, SWFDEC_AS_STR___proto__, SWFDEC_AS_VARIABLE_VERSION_6_UP);

  return fun;
}

SwfdecAsFunction *
swfdec_as_native_function_new_bare (SwfdecAsContext *context, const char *name,
    SwfdecAsNative native)
{
  SwfdecAsNativeFunction *fun;
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (native != NULL, NULL);

  fun = g_object_new (SWFDEC_TYPE_AS_NATIVE_FUNCTION, "context", context, NULL);
  fun->native = native;
  fun->name = g_strdup (name);

  object = swfdec_as_object_new_empty (context);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (fun));

  return SWFDEC_AS_FUNCTION (fun);
}

/**
 * SWFDEC_AS_CHECK:
 * @type: required type of this object or 0 for ignoring
 * @result: converted this object
 * @...: conversion string and pointers taking converted values
 *
 * This is a shortcut macro for calling swfdec_as_native_function_check() at
 * the beginning of a native function. See that function for details.
 * It requires the native function parameters to have the default name. So your
 * function must be declared like this:
 * |[static void
 * my_function (SwfdecAsContext *cx, SwfdecAsObject *object,
 *     guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval);]|
 */
/**
 * swfdec_as_native_function_check:
 * @cx: a #SwfdecAsContext
 * @object: this object passed to the native function
 * @type: expected type of @object or 0 for any
 * @result: pointer to variable taking cast result of @object
 * @argc: count of arguments passed to the function
 * @argv: arguments passed to the function
 * @args: argument conversion string
 * @...: pointers to variables taking converted arguments
 *
 * This function is a convenience function to validate and convert arguments to 
 * a native function while avoiding common pitfalls. You typically want to call
 * it at the beginning of every native function you write. Or you can use the 
 * SWFDEC_AS_CHECK() macro instead which calls this function.
 * The @cx, @object, @argc and @argv paramters should be passed verbatim from 
 * the function call to your native function. If @type is not 0, @object is then
 * checked to be of that type and cast to @result. After that the @args string 
 * is used to convert the arguments. Every character in @args describes the 
 * conversion of one argument. For that argument, you have to pass a pointer 
 * that takes the value. For the conversion, the default conversion functions 
 * like swfdec_as_value_to_string() are used. If not enough arguments are 
 * available, the function stops converting and returns %NULL. The following 
 * conversion characters are allowed:<itemizedlist>
 * <listitem><para>"b": convert to boolean. Requires a %gboolean pointer
 *                 </para></listitem>
 * <listitem><para>"i": convert to integer. Requires an %integer pointer
 *                 </para></listitem>
 * <listitem><para>"m": convert to an existing movieclip. This is only valid 
 *                 inside Swfdec. Requires a %SwfdecMovie pointer.
 *                 </para></listitem>
 * <listitem><para>"M": convert to a movieclip or %NULL. This is only valid 
 *                 inside Swfdec. If the movie has already been destroyed, 
 *                 the pointer will be set to %NULL</para></listitem>
 * <listitem><para>"n": convert to number. Requires a %double pointer
 *                 </para></listitem>
 * <listitem><para>"o": convert to object. Requires a #SwfdecAsObject pointer.
 *                 If the conversion fails, this function immediately returns
 *                 %FALSE.</para></listitem>
 * <listitem><para>"O": convert to object or %NULL. Requires a #SwfdecAsObject
 *                 pointer.</para></listitem>
 * <listitem><para>"s": convert to garbage-collected string. Requires a const 
 *                 %char pointer</para></listitem>
 * <listitem><para>"v": copy the value. The given argument must be a pointer 
 *                 to a #SwfdecAsValue</para></listitem>
 * <listitem><para>"|": optional arguments follow. Optional arguments will be
 *		   initialized to the empty value for their type. This 
 *		   conversion character is only allowed once in the conversion 
 *		   string.</para></listitem>
 * </itemizedlist>
 *
 * Returns: %TRUE if the conversion succeeded, %FALSE otherwise
 **/
gboolean
swfdec_as_native_function_check (SwfdecAsContext *cx, SwfdecAsObject *object, 
    GType type, gpointer *result, guint argc, SwfdecAsValue *argv, 
    const char *args, ...)
{
  gboolean ret;
  va_list varargs;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (cx), FALSE);
  g_return_val_if_fail (type == 0 || result != NULL, FALSE);

  va_start (varargs, args);
  ret = swfdec_as_native_function_checkv (cx, object, type, result, argc, argv, args, varargs);
  va_end (varargs);
  return ret;
}

/**
 * swfdec_as_native_function_checkv:
 * @cx: a #SwfdecAsContext
 * @object: this object passed to the native function
 * @type: expected type of @object
 * @result: pointer to variable taking cast result of @object
 * @argc: count of arguments passed to the function
 * @argv: arguments passed to the function
 * @args: argument conversion string
 * @varargs: pointers to variables taking converted arguments
 *
 * This is the valist version of swfdec_as_native_function_check(). See that
 * function for details.
 *
 * Returns: %TRUE if the conversion succeeded, %FALSE otherwise
 **/
gboolean
swfdec_as_native_function_checkv (SwfdecAsContext *cx, SwfdecAsObject *object, 
    GType type, gpointer *result, guint argc, SwfdecAsValue *argv, 
    const char *args, va_list varargs)
{
  guint i;
  gboolean optional = FALSE;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (cx), FALSE);
  g_return_val_if_fail (type == 0 || result != NULL, FALSE);

  /* check that we got a valid type */
  if (type) {
    if (G_TYPE_CHECK_INSTANCE_TYPE (object, type)) {
      *result = object;
    } else if (object && G_TYPE_CHECK_INSTANCE_TYPE (object->relay, type)) {
      *result = object->relay;
    } else {
	return FALSE;
    }
  }
  for (i = 0; *args && i < argc; i++, args++) {
    switch (*args) {
      case 'v':
	{
	  SwfdecAsValue *val = va_arg (varargs, SwfdecAsValue *);
	  *val = argv[i];
	}
	break;
      case 'b':
	{
	  gboolean *b = va_arg (varargs, gboolean *);
	  *b = swfdec_as_value_to_boolean (cx, &argv[i]);
	}
	break;
      case 'i':
	{
	  int *j = va_arg (varargs, int *);
	  *j = swfdec_as_value_to_integer (cx, &argv[i]);
	}
	break;
      case 'm':
      case 'M':
	{
	  SwfdecMovie *m;
	  SwfdecMovie **arg = va_arg (varargs, SwfdecMovie **);
	  if (SWFDEC_AS_VALUE_IS_MOVIE (argv[i])) {
	    m = SWFDEC_AS_VALUE_GET_MOVIE (argv[i]);
	  } else {
	    m = NULL;
	  }
	  if (m == NULL && *args != 'M')
	    return FALSE;
	  *arg = m;
	}
	break;
      case 'n':
	{
	  double *d = va_arg (varargs, double *);
	  *d = swfdec_as_value_to_number (cx, &argv[i]);
	}
	break;
      case 's':
	{
	  const char **s = va_arg (varargs, const char **);
	  *s = swfdec_as_value_to_string (cx, argv[i]);
	}
	break;
      case 'o':
      case 'O':
	{
	  SwfdecAsObject **o = va_arg (varargs, SwfdecAsObject **);
	  *o = swfdec_as_value_to_object (cx, &argv[i]);
	  if (*o == NULL && *args != 'O')
	    return FALSE;
	}
	break;
      case '|':
	g_return_val_if_fail (optional == FALSE, FALSE);
	optional = TRUE;
	i--;
	break;
      default:
	g_warning ("'%c' is not a valid type conversion", *args);
	return FALSE;
    }
  }
  if (*args && !optional && *args != '|')
    return FALSE;
  return TRUE;
}
