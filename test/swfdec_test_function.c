/* SwfdecTestfied
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

#include "swfdec_test_function.h"
#include "swfdec_test_function_list.h"

/* needed by the function list */
#ifdef HAVE_GTK
#include "swfdec_test_http_request.h"
#include "swfdec_test_http_server.h"
#endif
#include "swfdec_test_buffer.h"
#include "swfdec_test_image.h"
#include "swfdec_test_socket.h"
#include "swfdec_test_test.h"


/* include swfdec_test_function_list with special macro definition, so we get a nice
 * way to initialize it */
#undef SWFDEC_TEST_FUNCTION
#define SWFDEC_TEST_FUNCTION(name, fun, type) \
  { name, fun, type },
static const struct {
  const char *		name;
  SwfdecAsNative	fun;
  GType			(* type) (void);
} functions[] = {
#include "swfdec_test_function_list.h"
  { NULL, NULL, NULL }
};
#undef SWFDEC_TEST_FUNCTION

void
swfdec_test_function_init_context (SwfdecAsContext *cx)
{ 
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
    GType type = functions[i].type ? functions[i].type () : 0;
    swfdec_as_object_add_constructor (obj,
      swfdec_as_context_get_string (cx, functions[i].name),
      type, functions[i].fun, NULL);
  }
}

