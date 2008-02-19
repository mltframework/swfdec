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

#include "swfdec_stream_target.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"

static void
swfdec_stream_target_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (G_UNLIKELY (!initialized)) {
    initialized = TRUE;
  }
}

GType
swfdec_stream_target_get_type (void)
{
  static GType stream_target_type = 0;
  
  if (!stream_target_type) {
    static const GTypeInfo stream_target_info = {
      sizeof (SwfdecStreamTargetInterface),
      swfdec_stream_target_base_init,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
    };
    
    stream_target_type = g_type_register_static (G_TYPE_INTERFACE,
        "SwfdecStreamTarget", &stream_target_info, 0);
    g_type_interface_add_prerequisite (stream_target_type, G_TYPE_OBJECT);
  }
  
  return stream_target_type;
}

SwfdecPlayer *
swfdec_stream_target_get_player (SwfdecStreamTarget *target)
{
  SwfdecStreamTargetInterface *iface;
  
  g_return_val_if_fail (SWFDEC_IS_STREAM_TARGET (target), NULL);

  iface = SWFDEC_STREAM_TARGET_GET_INTERFACE (target);
  g_assert (iface->get_player != NULL);
  return iface->get_player (target);
}

void
swfdec_stream_target_open (SwfdecStreamTarget *target, SwfdecStream *stream)
{
  SwfdecStreamTargetInterface *iface;
  
  g_return_if_fail (SWFDEC_IS_STREAM_TARGET (target));
  g_return_if_fail (SWFDEC_IS_STREAM (stream));

  SWFDEC_LOG ("opening %s", swfdec_stream_describe (stream));

  iface = SWFDEC_STREAM_TARGET_GET_INTERFACE (target);
  if (iface->open)
    iface->open (target, stream);
}

gboolean
swfdec_stream_target_parse (SwfdecStreamTarget *target, SwfdecStream *stream)
{
  SwfdecStreamTargetInterface *iface;
  gboolean call_again;
  
  g_return_val_if_fail (SWFDEC_IS_STREAM_TARGET (target), FALSE);
  g_return_val_if_fail (SWFDEC_IS_STREAM (stream), FALSE);

  SWFDEC_LOG ("parsing %s", swfdec_stream_describe (stream));

  iface = SWFDEC_STREAM_TARGET_GET_INTERFACE (target);
  if (iface->parse)
    call_again = iface->parse (target, stream);
  else
    call_again = FALSE;

  return call_again;
}

void
swfdec_stream_target_close (SwfdecStreamTarget *target, SwfdecStream *stream)
{
  SwfdecStreamTargetInterface *iface;
  
  g_return_if_fail (SWFDEC_IS_STREAM_TARGET (target));
  g_return_if_fail (SWFDEC_IS_STREAM (stream));

  SWFDEC_LOG ("close on %s", swfdec_stream_describe (stream));

  iface = SWFDEC_STREAM_TARGET_GET_INTERFACE (target);
  if (iface->close)
    iface->close (target, stream);
}

void
swfdec_stream_target_error (SwfdecStreamTarget *target, SwfdecStream *stream)
{
  SwfdecStreamTargetInterface *iface;
  
  g_return_if_fail (SWFDEC_IS_STREAM_TARGET (target));
  g_return_if_fail (SWFDEC_IS_STREAM (stream));

  SWFDEC_LOG ("error on %s", swfdec_stream_describe (stream));

  iface = SWFDEC_STREAM_TARGET_GET_INTERFACE (target);
  if (iface->error)
    iface->error (target, stream);
}

