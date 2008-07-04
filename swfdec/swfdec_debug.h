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

#ifndef __SWFDEC_DEBUG_H__
#define __SWFDEC_DEBUG_H__

#include <glib.h>

G_BEGIN_DECLS

enum {
  SWFDEC_LEVEL_NONE = 0,
  SWFDEC_LEVEL_ERROR,
  SWFDEC_LEVEL_FIXME,
  SWFDEC_LEVEL_WARNING,
  SWFDEC_LEVEL_INFO,
  SWFDEC_LEVEL_DEBUG,
  SWFDEC_LEVEL_LOG
};

#define SWFDEC_ERROR(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_ERROR, __VA_ARGS__)
#define SWFDEC_FIXME(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_FIXME, __VA_ARGS__)
#define SWFDEC_WARNING(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_WARNING, __VA_ARGS__)
#define SWFDEC_INFO(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_INFO, __VA_ARGS__)
#define SWFDEC_DEBUG(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_DEBUG, __VA_ARGS__)
#define SWFDEC_LOG(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_LOG, __VA_ARGS__)

#define SWFDEC_STUB(fun) \
  SWFDEC_DEBUG_LEVEL (SWFDEC_LEVEL_FIXME, "%s %s", fun, "is not implemented yet")

#ifdef SWFDEC_DISABLE_DEBUG
#define SWFDEC_DEBUG_LEVEL(level,...) (void) 0
#else
#define SWFDEC_DEBUG_LEVEL(level,...) \
  swfdec_debug_log ((level), __FILE__, G_STRFUNC, __LINE__, __VA_ARGS__)
#endif

void swfdec_debug_log (guint level, const char *file, const char *function,
    int line, const char *format, ...) G_GNUC_PRINTF (5, 6);
void swfdec_debug_set_level (guint level);
int swfdec_debug_get_level (void);

G_END_DECLS
#endif
