/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec/swfdec_as_context.h"
#include "swfdec/swfdec_as_object.h"
#include "swfdec/swfdec_as_strings.h"

#define ERROR(...) G_STMT_START { \
  g_printerr ("ERROR (line %u): ", __LINE__); \
  g_printerr (__VA_ARGS__); \
  g_printerr ("\n"); \
  errors++; \
}G_STMT_END

static guint
check_strings (void)
{
  const char *s;
  guint errors = 0;
  SwfdecAsContext *context;
  
  context = g_object_new (SWFDEC_TYPE_AS_CONTEXT, NULL);
  swfdec_as_context_startup (context);

  s = swfdec_as_context_get_string (context, "hi mom");
  if (!g_str_equal (s, "hi mom")) {
    ERROR ("swfdec_as_context_get_string returns different string from input");
  }

  g_object_unref (context);
  return errors;
}

static guint
check_objects (void)
{
  SwfdecAsObject *object;
  guint errors = 0;
  SwfdecAsContext *context;
  SwfdecAsValue val;
  gpointer check = GUINT_TO_POINTER (-1); /* NOT NULL */
  
  context = g_object_new (SWFDEC_TYPE_AS_CONTEXT, NULL);
  swfdec_as_context_startup (context);
  g_assert (check != NULL);

  object = swfdec_as_object_new (context, NULL);
  g_object_add_weak_pointer (G_OBJECT (object), &check);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, object);
  swfdec_as_object_set_variable (context->global, SWFDEC_AS_STR__root, &val);
  swfdec_as_context_gc (context);
  if (check == NULL) {
    ERROR ("GC collected a rooted object, bailing");
    g_object_unref (context);
    return errors;
  }
  swfdec_as_object_delete_variable (context->global, SWFDEC_AS_STR__root);
  swfdec_as_context_gc (context);
  if (check != NULL) {
    ERROR ("GC did not collect an unreferenced object");
  }

  g_object_unref (context);
  return errors;
}

static guint
check_object_variables (void)
{
  SwfdecAsObject *o, *o2;
  guint errors = 0;
  SwfdecAsContext *context;
  const char *s;
  gpointer check = GUINT_TO_POINTER (-1); /* NOT NULL */
  gpointer check2 = GUINT_TO_POINTER (-1); /* NOT NULL */
  SwfdecAsValue v1, v2;
  
  context = g_object_new (SWFDEC_TYPE_AS_CONTEXT, NULL);
  swfdec_as_context_startup (context);
  g_assert (check != NULL);

  o = swfdec_as_object_new (context, NULL);
  o2 = swfdec_as_object_new (context, NULL);
  g_object_add_weak_pointer (G_OBJECT (o), &check);
  g_object_add_weak_pointer (G_OBJECT (o2), &check2);
  s = swfdec_as_context_get_string (context, "foo");
  /* step one: set a variable */
  SWFDEC_AS_VALUE_SET_OBJECT (&v1, o);
  swfdec_as_object_set_variable (context->global, s, &v1);
  SWFDEC_AS_VALUE_SET_OBJECT (&v2, o2);
  swfdec_as_object_set_variable (o, s, &v2);
  SWFDEC_AS_VALUE_SET_UNDEFINED (&v2);
  swfdec_as_object_get_variable (o, s, &v2);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&v2)) {
    ERROR ("variable changed type");
  } else if (o2 != SWFDEC_AS_VALUE_GET_OBJECT (&v2)) {
    ERROR ("variable changed value");
  }
  /* step 2: gc */
  swfdec_as_context_gc (context);
  if (check == NULL || check2 == NULL) {
    ERROR ("GC collected a used object, bailing");
    g_object_unref (context);
    return errors;
  }
  /* step 3: unset root reference and set cyclic variable */
  swfdec_as_object_delete_variable (context->global, s);
  swfdec_as_object_set_variable (o2, s, &v1);
  SWFDEC_AS_VALUE_SET_UNDEFINED (&v1);
  swfdec_as_object_get_variable (o2, s, &v1);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&v1)) {
    ERROR ("variable changed type");
  } else if (o != SWFDEC_AS_VALUE_GET_OBJECT (&v1)) {
    ERROR ("variable changed value");
  }
  /* step 4: gc, ensure that both objects disappears */
  swfdec_as_context_gc (context);
  if (check != NULL || check2 != NULL) {
    ERROR ("GC didn't collect unused object");
  }

  g_object_unref (context);
  return errors;

}

int
main (int argc, char **argv)
{
  guint errors = 0;

  g_type_init ();

  errors += check_strings ();
  errors += check_objects ();
  errors += check_object_variables ();

  g_print ("TOTAL ERRORS: %u\n", errors);
  return errors;
}

