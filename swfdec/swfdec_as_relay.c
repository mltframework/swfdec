/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_as_relay.h"
#include "swfdec_as_context.h"
#include "swfdec_as_object.h"
#include "swfdec_as_function.h"

G_DEFINE_ABSTRACT_TYPE (SwfdecAsRelay, swfdec_as_relay, SWFDEC_TYPE_GC_OBJECT)

static void
swfdec_as_relay_mark (SwfdecGcObject *object)
{
  SwfdecAsRelay *relay = SWFDEC_AS_RELAY (object);

  if (relay->relay)
    swfdec_gc_object_mark (relay->relay);

  SWFDEC_GC_OBJECT_CLASS (swfdec_as_relay_parent_class)->mark (object);
}

static void
swfdec_as_relay_class_init (SwfdecAsRelayClass *klass)
{
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);

  gc_class->mark = swfdec_as_relay_mark;
}

static void
swfdec_as_relay_init (SwfdecAsRelay *object)
{
}

/**
 * swfdec_as_relay_get_as_object:
 * @object: a #SwfdecAsRelay.
 *
 * Gets the Actionscript object associated with this object.
 *
 * Returns: The #SwfdecAsObject associated with this relay.
 **/
SwfdecAsObject *
swfdec_as_relay_get_as_object (SwfdecAsRelay *relay)
{
  g_return_val_if_fail (SWFDEC_IS_AS_RELAY (relay), NULL);
  g_return_val_if_fail (relay->relay != NULL, NULL);

  return relay->relay;
}

/**
 * swfdec_as_relay_call:
 * @object: a #SwfdecAsRelay
 * @name: garbage-collected string naming the function to call. 
 * @argc: number of arguments to provide to function
 * @argv: arguments or %NULL when @argc is 0
 * @return_value: location to take the return value of the call or %NULL to 
 *                ignore the return value.
 *
 * Calls the function named @name on the given object. This function is 
 * essentially equal to the folloeing Actionscript code: 
 * <informalexample><programlisting>
 * @return_value = @object.@name (@argv[0], ..., @argv[argc-1]);
 * </programlisting></informalexample>
 *
 * Returns: %TRUE if @object had a function with the given name, %FALSE otherwise
 **/
gboolean
swfdec_as_relay_call (SwfdecAsRelay *relay, const char *name, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *return_value)
{
  SwfdecAsValue tmp;
  SwfdecAsFunction *fun;

  g_return_val_if_fail (SWFDEC_IS_AS_RELAY (relay), TRUE);
  g_return_val_if_fail (name != NULL, TRUE);
  g_return_val_if_fail (argc == 0 || argv != NULL, TRUE);
  g_return_val_if_fail (swfdec_gc_object_get_context (relay)->global != NULL, TRUE); /* for SwfdecPlayer */

  /* If this doesn't hold, we need to use swfdec_as_relay_get_as_object()
   * and have that function create the relay on demand. */
  g_assert (relay->relay);

  if (return_value)
    SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);
  swfdec_as_object_get_variable (relay->relay, name, &tmp);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&tmp))
    return FALSE;
  fun = (SwfdecAsFunction *) (SWFDEC_AS_VALUE_GET_OBJECT (&tmp)->relay);
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    return FALSE;
  swfdec_as_function_call (fun, relay->relay, argc, argv, return_value ? return_value : &tmp);

  return TRUE;
}

