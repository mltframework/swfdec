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

#include "swfdec_loadertarget.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h" /* HACK */

static void
swfdec_loader_target_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (G_UNLIKELY (!initialized)) {
    initialized = TRUE;
  }
}

GType
swfdec_loader_target_get_type ()
{
  static GType loader_target_type = 0;
  
  if (!loader_target_type) {
    static const GTypeInfo loader_target_info = {
      sizeof (SwfdecLoaderTargetInterface),
      swfdec_loader_target_base_init,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
    };
    
    loader_target_type = g_type_register_static (G_TYPE_INTERFACE,
        "SwfdecLoaderTarget", &loader_target_info, 0);
    g_type_interface_add_prerequisite (loader_target_type, G_TYPE_OBJECT);
  }
  
  return loader_target_type;
}

SwfdecPlayer *
swfdec_loader_target_get_player (SwfdecLoaderTarget *target)
{
  SwfdecLoaderTargetInterface *iface;
  
  g_return_val_if_fail (SWFDEC_IS_LOADER_TARGET (target), NULL);

  iface = SWFDEC_LOADER_TARGET_GET_INTERFACE (target);
  g_assert (iface->get_player != NULL);
  return iface->get_player (target);
}

void
swfdec_loader_target_parse (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecLoaderTargetInterface *iface;
  
  g_return_if_fail (SWFDEC_IS_LOADER_TARGET (target));
  g_return_if_fail (SWFDEC_IS_LOADER (loader));

  SWFDEC_LOG ("parsing %p%s%s", loader,
      loader->error ? " ERROR" : "", loader->eof ? " EOF" : "");

  iface = SWFDEC_LOADER_TARGET_GET_INTERFACE (target);
  g_return_if_fail (iface->parse != NULL);
  iface->parse (target, loader);
}

