/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2008 Benjamin Otte <otte@gnome.org>
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

/**
 * SWFDEC_ERROR:
 * @...: a printf-style message
 *
 * Emits an error debugging message. Error messages are used to indicate 
 * mal-formed content. Examples are invalid data in the Flash file or results
 * of broken code. Note that error messages do not indicate that Swfdec stopped
 * interpretation of the Flash file.
 */

/**
 * SWFDEC_FIXME:
 * @...: a printf-style message
 *
 * Emits a fixme debugging message. FIXMEs are used when it is known that a
 * feature is not yet correctly implemented and could cause wrong behavior while
 * playing back this Flash file.
 */

/**
 * SWFDEC_WARNING:
 * @...: a printf-style message
 *
 * Emits a warning debugging message. Warnings are used to indicate unusual
 * situations. An example would be a wrong argument type passed to a native 
 * script function. Warning messages are output by default on development 
 * builds. So make sure they are important enough to warrant being printed.
 */

/**
 * SWFDEC_INFO:
 * @...: a printf-style message
 *
 * Outputs an informational debugging message. This debugging category should 
 * be used for rare messages that contain useful information. An example is
 * printing the version and default size of a newly loaded Flash file.
 */

/**
 * SWFDEC_DEBUG:
 * @...: a printf-style message
 *
 * Outputs a debugging message. This debugging category should be used whenever
 * something happens that is worth noting, but not the default behavior. For
 * default messages, use SWFDEC_LOG(). An example would be if a special argument
 * was passed by a script to a native function.
 */

/**
 * SWFDEC_LOG:
 * @...: a printf-style message
 *
 * Outputs a logging message. This debugging category should be used whenever
 * there is something that might be interesting to a developer inspecting a run
 * of Swfdec. It is used for example to dump every parsed token of the Flash 
 * file.
 */

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

