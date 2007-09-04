/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *		 2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_debug.h"
#include "swfdec_as_context.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_object.h"
#include "swfdec_as_string.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_types.h"

/*** AS CODE ***/

SWFDEC_AS_NATIVE (301, 3, swfdec_loadvars_decode)
void
swfdec_loadvars_decode (SwfdecAsContext *cx, SwfdecAsObject *obj,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  if (argc > 0) {
    SwfdecAsValue val;
    const char *str;
    char **varlist, *p;
    guint i;

    str =
      swfdec_as_string_unescape (cx, swfdec_as_value_to_string (cx, &argv[0]));
    if (str == NULL)
      return;

    varlist = g_strsplit (str, "&", -1);

    for (i = 0; varlist[i] != NULL; i++) {
      p = strchr (varlist[i], '=');
      if (p != NULL) {
	*p++ = '\0';
	if (*p == '\0')
	  p = NULL;
      }

      if (p != NULL) {
	SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_get_string (cx, p));
      } else {
	SWFDEC_AS_VALUE_SET_STRING (&val, SWFDEC_AS_STR_EMPTY);
      }
      swfdec_as_object_set_variable (obj, swfdec_as_context_get_string (cx, varlist[i]), &val);
    }
  }
}
