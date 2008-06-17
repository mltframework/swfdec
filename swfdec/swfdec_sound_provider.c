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

#include "swfdec_sound_provider.h"
#include "swfdec_actor.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

static void
swfdec_sound_provider_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (G_UNLIKELY (!initialized)) {
    initialized = TRUE;
  }
}

GType
swfdec_sound_provider_get_type (void)
{
  static GType sound_provider_type = 0;
  
  if (!sound_provider_type) {
    static const GTypeInfo sound_provider_info = {
      sizeof (SwfdecSoundProviderInterface),
      swfdec_sound_provider_base_init,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
    };
    
    sound_provider_type = g_type_register_static (G_TYPE_INTERFACE,
        "SwfdecSoundProvider", &sound_provider_info, 0);
    g_type_interface_add_prerequisite (sound_provider_type, G_TYPE_OBJECT);
  }
  
  return sound_provider_type;
}

void
swfdec_sound_provider_start (SwfdecSoundProvider *provider, 
    SwfdecActor *actor, gsize samples_offset, guint loops)
{
  SwfdecSoundProviderInterface *iface;
  
  g_return_if_fail (SWFDEC_IS_SOUND_PROVIDER (provider));
  g_return_if_fail (SWFDEC_IS_ACTOR (actor));
  g_return_if_fail (loops > 0);

  iface = SWFDEC_SOUND_PROVIDER_GET_INTERFACE (provider);
  iface->start (provider, actor, samples_offset, loops);
}

void
swfdec_sound_provider_stop (SwfdecSoundProvider *provider, SwfdecActor *actor)
{
  SwfdecSoundProviderInterface *iface;
  
  g_return_if_fail (SWFDEC_IS_SOUND_PROVIDER (provider));
  g_return_if_fail (SWFDEC_IS_ACTOR (actor));

  iface = SWFDEC_SOUND_PROVIDER_GET_INTERFACE (provider);
  iface->stop (provider, actor);
}

