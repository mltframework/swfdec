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

static SwfdecAsFrame *
swfdec_as_native_function_call (SwfdecAsFunction *function)
{
  SwfdecAsNativeFunction *native = SWFDEC_AS_NATIVE_FUNCTION (function);
  SwfdecAsFrame *frame;
  SwfdecAsContext *cx;

  cx = swfdec_gc_object_get_context (function);
  frame = swfdec_as_frame_new_native (cx);
  if (frame == NULL)
    return NULL;
  g_assert (native->name);
  frame->function_name = native->name;
  frame->function = function;
  /* We copy the target here so we have a proper SwfdecMovie reference inside native 
   * functions. This is for example necessary for swfdec_player_get_movie_by_value()
   * and probably other stuff that does variable lookups inside native functions.
   */
  /* FIXME: copy target or original target? */
  if (frame->next) {
    frame->target = frame->next->original_target;
    frame->original_target = frame->target;
  }
  return frame;
}

static char *
swfdec_as_native_function_debug (SwfdecAsObject *object)
{
  SwfdecAsNativeFunction *native = SWFDEC_AS_NATIVE_FUNCTION (object);

  return g_strdup_printf ("%s ()", native->name);
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
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);
  SwfdecAsFunctionClass *function_class = SWFDEC_AS_FUNCTION_CLASS (klass);

  object_class->dispose = swfdec_as_native_function_dispose;

  asobject_class->debug = swfdec_as_native_function_debug;

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
 * @min_args: minimum number of arguments required
 * @prototype: The object to be used as "prototype" property for the created 
 *             function or %NULL for none.
 *
 * Creates a new native function, that will execute @native when called. The
 * @min_args parameter sets a requirement for the minimum number of arguments
 * to pass to @native. If the function gets called with less arguments, it
 * will just redurn undefined. You might want to use 
 * swfdec_as_object_add_function() instead of this function.
 *
 * Returns: a new #SwfdecAsFunction
 **/
SwfdecAsFunction *
swfdec_as_native_function_new (SwfdecAsContext *context, const char *name,
    SwfdecAsNative native, guint min_args, SwfdecAsObject *prototype)
{
  SwfdecAsNativeFunction *fun;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (native != NULL, NULL);
  g_return_val_if_fail (prototype == NULL || SWFDEC_IS_AS_OBJECT (prototype), NULL);

  fun = g_object_new (SWFDEC_TYPE_AS_NATIVE_FUNCTION, "context", context, NULL);
  fun->native = native;
  fun->min_args = min_args;
  fun->name = g_strdup (name);
  /* need to set prototype before setting the constructor or Function.constructor 
   * being CONSTANT disallows setting it. */
  if (prototype) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_OBJECT (&val, prototype);
    swfdec_as_object_set_variable_and_flags (SWFDEC_AS_OBJECT (fun), SWFDEC_AS_STR_prototype, 
	&val, SWFDEC_AS_VARIABLE_HIDDEN | SWFDEC_AS_VARIABLE_PERMANENT);
  }
  swfdec_as_function_set_constructor (SWFDEC_AS_FUNCTION (fun));

  return SWFDEC_AS_FUNCTION (fun);
}

/**
 * swfdec_as_native_function_set_object_type:
 * @function: a #SwfdecAsNativeFunction
 * @type: required #GType for the this object
 *
 * Sets the required type for the this object to @type. If the this object 
 * isn't of the required type, the function will not be called and its
 * return value will be undefined.
 **/
void
swfdec_as_native_function_set_object_type (SwfdecAsNativeFunction *function, GType type)
{
  GTypeQuery query;

  g_return_if_fail (SWFDEC_IS_AS_NATIVE_FUNCTION (function));
  g_return_if_fail (g_type_is_a (type, SWFDEC_TYPE_AS_OBJECT));

  g_type_query (type, &query);
  function->type = type;
}

/**
 * swfdec_as_native_function_set_construct_type:
 * @function: a #SwfdecAsNativeFunction
 * @type: #GType used when constructing an object with @function
 *
 * Sets the @type to be used when using @function as a constructor. If this is
 * not set, using @function as a constructor will create a #SwfdecAsObject.
 **/
void
swfdec_as_native_function_set_construct_type (SwfdecAsNativeFunction *function, GType type)
{
  GTypeQuery query;

  g_return_if_fail (SWFDEC_IS_AS_NATIVE_FUNCTION (function));
  g_return_if_fail (g_type_is_a (type, SWFDEC_TYPE_AS_OBJECT));

  g_type_query (type, &query);
  function->construct_type = type;
  function->construct_size = query.instance_size;
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
    if (!G_TYPE_CHECK_INSTANCE_TYPE (object, type))
      return FALSE;
    *result = object;
  }
  for (i = 0; *args; i++, args++) {
    if (!optional && i >= argc && *args != '|')
      break;
    switch (*args) {
      case 'v':
	{
	  SwfdecAsValue *val = va_arg (varargs, SwfdecAsValue *);
	  if (i < argc)
	    *val = argv[i];
	  else
	    SWFDEC_AS_VALUE_SET_UNDEFINED (val);
	}
	break;
      case 'b':
	{
	  gboolean *b = va_arg (varargs, gboolean *);
	  if (i < argc)
	    *b = swfdec_as_value_to_boolean (cx, &argv[i]);
	  else
	    *b = FALSE;
	}
	break;
      case 'i':
	{
	  int *j = va_arg (varargs, int *);
	  if (i < argc)
	    *j = swfdec_as_value_to_integer (cx, &argv[i]);
	  else
	    *j = 0;
	}
	break;
      case 'n':
	{
	  double *d = va_arg (varargs, double *);
	  if (i < argc)
	    *d = swfdec_as_value_to_number (cx, &argv[i]);
	  else
	    *d = 0;
	}
	break;
      case 's':
	{
	  const char **s = va_arg (varargs, const char **);
	  if (i < argc)
	    *s = swfdec_as_value_to_string (cx, &argv[i]);
	  else
	    *s = SWFDEC_AS_STR_EMPTY;
	}
	break;
      case 'o':
      case 'O':
	{
	  SwfdecAsObject **o = va_arg (varargs, SwfdecAsObject **);
	  if (i < argc)
	    *o = swfdec_as_value_to_object (cx, &argv[i]);
	  else
	    *o = NULL;
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
  if (*args)
    return FALSE;
  return TRUE;
}
