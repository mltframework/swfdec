/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include <glib.h>
#include "swfdec_debug.h"

static const char *swfdec_debug_level_names[] = {
  "NONE ",
  "ERROR",
  "FIXME",
  "WARN ",
  "INFO ",
  "DEBUG",
  "LOG  "
};

#ifndef SWFDEC_LEVEL_DEFAULT
#  define SWFDEC_LEVEL_DEFAULT SWFDEC_LEVEL_FIXME
#endif

static guint swfdec_debug_level = SWFDEC_LEVEL_DEFAULT;

void
swfdec_debug_log (guint level, const char *file, const char *function,
    int line, const char *format, ...)
{
  va_list varargs;
  char *s;

  if (level > swfdec_debug_level)
    return;

  va_start (varargs, format);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  g_printerr ("SWFDEC: %s: %s(%d): %s: %s\n",
      swfdec_debug_level_names[level], file, line, function, s);
  g_free (s);
}

void
swfdec_debug_set_level (guint level)
{
  if (swfdec_debug_level >= G_N_ELEMENTS (swfdec_debug_level_names))
    swfdec_debug_level = G_N_ELEMENTS (swfdec_debug_level_names) - 1;
  else
    swfdec_debug_level = level;
}

int
swfdec_debug_get_level (void)
{
  return swfdec_debug_level;
}

