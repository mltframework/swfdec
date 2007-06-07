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

#include "swfdec_utils.h"

gboolean
swfdec_str_case_equal (gconstpointer v1, gconstpointer v2)
{
  const char *s1 = v1;
  const char *s2 = v2;
  
  return g_ascii_strcasecmp (s1, s2) == 0;
}

guint
swfdec_str_case_hash (gconstpointer v)
{
  const signed char *p;
  guint32 h = 0;

  for (p = v; *p != '\0'; p++)
    h = (h << 5) - h + (*p & 0xDF);

  return h;
}

