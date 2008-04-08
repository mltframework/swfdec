/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_cached.h"
#include "swfdec_debug.h"


G_DEFINE_ABSTRACT_TYPE (SwfdecCached, swfdec_cached, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_SIZE
};

enum {
  USE,
  UNUSE,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL] = { 0, };

static void
swfdec_cached_get_property (GObject *object, guint param_id, GValue *value,
    GParamSpec *pspec)
{
  SwfdecCached *cached = SWFDEC_CACHED (object);

  switch (param_id) {
    case PROP_SIZE:
      g_value_set_ulong (value, cached->size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_cached_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecCached *cached = SWFDEC_CACHED (object);

  switch (param_id) {
    case PROP_SIZE:
      cached->size = g_value_get_ulong (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_cached_class_init (SwfdecCachedClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->get_property = swfdec_cached_get_property;
  object_class->set_property = swfdec_cached_set_property;

  /* FIXME: should be g_param_spec_size(), but no such thing exists */
  g_object_class_install_property (object_class, PROP_SIZE,
      g_param_spec_ulong ("size", "size", "size of this object in bytes",
	  0, G_MAXULONG, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  signals[USE] = g_signal_new ("use", G_TYPE_FROM_CLASS (g_class),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
  signals[UNUSE] = g_signal_new ("unuse", G_TYPE_FROM_CLASS (g_class),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
}

static void
swfdec_cached_init (SwfdecCached * cached)
{
  cached->size = sizeof (SwfdecCached);
}

void
swfdec_cached_use (SwfdecCached *cached)
{
  g_return_if_fail (SWFDEC_IS_CACHED (cached));

  g_signal_emit (cached, signals[USE], 0);
}

void
swfdec_cached_unuse (SwfdecCached *cached)
{
  g_return_if_fail (SWFDEC_IS_CACHED (cached));

  g_signal_emit (cached, signals[UNUSE], 0);
}

gsize
swfdec_cached_get_size (SwfdecCached *cached)
{
  g_return_val_if_fail (SWFDEC_IS_CACHED (cached), 0);

  return cached->size;
}

