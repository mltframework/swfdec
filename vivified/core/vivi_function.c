/* Vivified
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

#include "vivi_function.h"
#include "vivi_breakpoint.h"
#include "vivi_function_list.h"

/* include vivi_function_list with special macro definition, so we get a nice
 * way to initialize it */
#undef VIVI_FUNCTION
#define VIVI_FUNCTION(name, fun) \
  { name, fun },
static const struct {
  const char *		name;
  SwfdecAsNative	fun;
} functions[] = {
#include "vivi_function_list.h"
  { NULL, NULL }
};
#undef VIVI_FUNCTION

/* defined in vivi_initialize.s */
extern const char vivi_initialize[];

static void
vivi_function_not_reached (ViviApplication *app, guint type, char *message, gpointer unused)
{
  if (type == VIVI_MESSAGE_ERROR)
    g_error ("%s", message);
}

void
vivi_function_init_context (ViviApplication *app)
{ 
  SwfdecAsContext *cx = SWFDEC_AS_CONTEXT (app);
  SwfdecAsFunction *fun;
  SwfdecAsObject *obj;
  SwfdecAsValue val;
  guint i;

  obj = swfdec_as_object_new (cx);
  if (obj == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, obj);
  swfdec_as_object_set_variable (cx->global, 
      swfdec_as_context_get_string (cx, "Native"), &val);

  for (i = 0; functions[i].name; i++) {
    swfdec_as_object_add_function (obj,
      swfdec_as_context_get_string (cx, functions[i].name),
      functions[i].fun);
  }
  /* FIXME: find a better solution than this */
  fun = swfdec_as_object_add_function (obj,
    swfdec_as_context_get_string (cx, "Breakpoint"),
    functions[i].fun);
  swfdec_as_native_function_set_construct_type (SWFDEC_AS_NATIVE_FUNCTION (fun),
      VIVI_TYPE_BREAKPOINT);
  obj = swfdec_as_object_new (cx);
  if (obj == NULL)
    return;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, obj);
  swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (fun), 
      swfdec_as_context_get_string (cx, "prototype"), &val);

  g_signal_connect (app, "message", G_CALLBACK (vivi_function_not_reached), NULL);
  vivi_application_execute (app, vivi_initialize);
  g_signal_handlers_disconnect_by_func (app, G_CALLBACK (vivi_function_not_reached), NULL);
}

