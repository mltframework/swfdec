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

#include "swfdec_as_types.h"
#include "swfdec_as_object.h"
#include "swfdec_as_context.h"
#include "swfdec_debug.h"

#define SWFDEC_AS_CONSTANT_STRING(str) "\2" str
const char *swfdec_as_strings[] = {
  SWFDEC_AS_CONSTANT_STRING (""),
  SWFDEC_AS_CONSTANT_STRING ("__proto__"),
  SWFDEC_AS_CONSTANT_STRING ("this"),
  /* add more here */
  NULL
};

const char *
swfdec_as_value_to_string (SwfdecAsContext *context, const SwfdecAsValue *value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), SWFDEC_AS_STR_EMPTY);
  g_return_val_if_fail (SWFDEC_IS_AS_VALUE (value), SWFDEC_AS_STR_EMPTY);

  if (SWFDEC_AS_VALUE_IS_STRING (value)) {
    return SWFDEC_AS_VALUE_GET_STRING (value);
  }
  g_assert_not_reached ();
}
