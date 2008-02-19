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

#include "swfdec_test_utils.h"

void
swfdec_test_throw (SwfdecAsContext *cx, const char *message, ...)
{
  SwfdecAsValue val;

  if (!swfdec_as_context_catch (cx, &val)) {
    va_list varargs;
    char *s;

    va_start (varargs, message);
    s = g_strdup_vprintf (message, varargs);
    va_end (varargs);

    /* FIXME: Throw a real object here? */
    SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_give_string (cx, s));
  }
  swfdec_as_context_throw (cx, &val);
}

