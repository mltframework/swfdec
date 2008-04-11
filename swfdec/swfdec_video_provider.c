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

#include "swfdec_video_provider.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"

enum {
  NEW_IMAGE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
swfdec_video_provider_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (G_UNLIKELY (!initialized)) {
    initialized = TRUE;

    signals[NEW_IMAGE] = g_signal_new ("new-image", G_TYPE_FROM_CLASS (klass),
	G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
	G_TYPE_NONE, 0);
  }
}

GType
swfdec_video_provider_get_type (void)
{
  static GType video_provider_type = 0;
  
  if (!video_provider_type) {
    static const GTypeInfo video_provider_info = {
      sizeof (SwfdecVideoProviderInterface),
      swfdec_video_provider_base_init,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      0,
      NULL,
    };
    
    video_provider_type = g_type_register_static (G_TYPE_INTERFACE,
        "SwfdecVideoProvider", &video_provider_info, 0);
    g_type_interface_add_prerequisite (video_provider_type, G_TYPE_OBJECT);
  }
  
  return video_provider_type;
}

cairo_surface_t *
swfdec_video_provider_get_image (SwfdecVideoProvider *provider, 
    SwfdecRenderer *renderer, guint *width, guint *height)
{
  SwfdecVideoProviderInterface *iface;
  
  g_return_val_if_fail (SWFDEC_IS_VIDEO_PROVIDER (provider), NULL);
  g_return_val_if_fail (SWFDEC_IS_RENDERER (renderer), NULL);
  g_return_val_if_fail (width != NULL, NULL);
  g_return_val_if_fail (height != NULL, NULL);

  iface = SWFDEC_VIDEO_PROVIDER_GET_INTERFACE (provider);
  g_assert (iface->get_image != NULL);
  return iface->get_image (provider, renderer, width, height);
}

void
swfdec_video_provider_set_ratio (SwfdecVideoProvider *provider, guint ratio)
{
  SwfdecVideoProviderInterface *iface;
  
  g_return_if_fail (SWFDEC_IS_VIDEO_PROVIDER (provider));

  iface = SWFDEC_VIDEO_PROVIDER_GET_INTERFACE (provider);
  if (iface->set_ratio != NULL)
    iface->set_ratio (provider, ratio);
}

void
swfdec_video_provider_new_image	(SwfdecVideoProvider *provider)
{
  g_return_if_fail (SWFDEC_IS_VIDEO_PROVIDER (provider));
  g_signal_emit (provider, signals[NEW_IMAGE], 0);
}

