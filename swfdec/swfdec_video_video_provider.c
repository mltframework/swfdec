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

#include <stdlib.h>

#include "swfdec_video_video_provider.h"
#include "swfdec_cached_video.h"
#include "swfdec_debug.h"
#include "swfdec_font.h"
#include "swfdec_player_internal.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_swf_decoder.h"

static SwfdecVideoFrame *
swfdec_video_video_provider_find_frame (SwfdecVideo *video, guint frame)
{
  SwfdecVideoFrame *cur;
  guint i;

  for (i = 0; i < video->images->len; i++) {
    cur = &g_array_index (video->images, SwfdecVideoFrame, i);
    if (cur->frame > frame)
      break;
  }

  return &g_array_index (video->images, SwfdecVideoFrame, MAX (1, i) - 1);
}

/*** VIDEO PROVIDER INTERFACE ***/

static void
swfdec_video_video_provider_set_ratio (SwfdecVideoProvider *prov, guint ratio)
{
  SwfdecVideoVideoProvider *provider = SWFDEC_VIDEO_VIDEO_PROVIDER (prov);
  SwfdecVideoFrame *frame;

  if (ratio >= provider->video->n_frames) {
    SWFDEC_ERROR ("ratio %u too big: only %u frames", ratio, provider->video->n_frames);
    ratio = provider->video->n_frames - 1;
  }
  
  frame = swfdec_video_video_provider_find_frame (provider->video, ratio);
  ratio = frame->frame;

  if (ratio != provider->current_frame) {
    provider->current_frame = ratio;
    swfdec_video_provider_new_image (prov);
  }
}

static gboolean
swfdec_cached_video_compare (SwfdecCached *cached, gpointer data)
{
  return swfdec_cached_video_get_frame (SWFDEC_CACHED_VIDEO (cached)) == GPOINTER_TO_UINT (data);
}

static cairo_surface_t *
swfdec_video_video_provider_get_image (SwfdecVideoProvider *prov,
    SwfdecRenderer *renderer, guint *width, guint *height)
{
  SwfdecVideoVideoProvider *provider = SWFDEC_VIDEO_VIDEO_PROVIDER (prov);
  SwfdecCachedVideo *cached;
  SwfdecVideoFrame *frame;
  cairo_surface_t *surface;
  guint w, h;

  if (provider->current_frame == 0)
    return NULL;

  cached = SWFDEC_CACHED_VIDEO (swfdec_renderer_get_cache (renderer, provider->video, 
	swfdec_cached_video_compare, GUINT_TO_POINTER (provider->current_frame)));
  if (cached != NULL) {
    swfdec_cached_use (SWFDEC_CACHED (cached));
    swfdec_cached_video_get_size (cached, width, height);
    return swfdec_cached_video_get_surface (cached);
  }

  if (provider->decoder == NULL || provider->current_frame < provider->decoder_frame) {
    if (provider->decoder != NULL) {
      g_object_unref (provider->decoder);
    }
    provider->decoder = swfdec_video_decoder_new (provider->video->format);
    if (provider->decoder == NULL)
      return NULL;
    frame = &g_array_index (provider->video->images, SwfdecVideoFrame, 0);
    g_assert (frame->frame <= provider->current_frame);
  } else {
    frame = swfdec_video_video_provider_find_frame (provider->video, provider->decoder_frame);
    g_assert (provider->decoder_frame == frame->frame);
    frame++;
  }

  for (;;) {
    g_assert (frame->buffer);
    swfdec_video_decoder_decode (provider->decoder, frame->buffer);
    provider->decoder_frame = provider->current_frame;
    if (frame->frame == provider->current_frame)
      break;
    frame++;
  };

  surface = swfdec_video_decoder_get_image (provider->decoder, renderer);
  if (surface == NULL)
    return NULL;
  w = swfdec_video_decoder_get_width (provider->decoder);
  h = swfdec_video_decoder_get_height (provider->decoder);
  cached = swfdec_cached_video_new (surface, w * h * 4);
  swfdec_cached_video_set_frame (cached, provider->current_frame);
  swfdec_cached_video_set_size (cached, w, h);
  swfdec_renderer_add_cache (renderer, FALSE, provider->video, SWFDEC_CACHED (cached));
  g_object_unref (cached);

  *width = w;
  *height = h;

  return surface;
}

static void
swfdec_video_video_provider_get_size (SwfdecVideoProvider *prov, guint *width, guint *height)
{
  SwfdecVideoVideoProvider *provider = SWFDEC_VIDEO_VIDEO_PROVIDER (prov);

  if (provider->decoder) {
    *width = swfdec_video_decoder_get_width (provider->decoder);
    *height = swfdec_video_decoder_get_height (provider->decoder);
  } else {
    *width = 0;
    *height = 0;
  }
}

static void
swfdec_video_video_provider_video_provider_init (SwfdecVideoProviderInterface *iface)
{
  iface->get_image = swfdec_video_video_provider_get_image;
  iface->get_size = swfdec_video_video_provider_get_size;
  iface->set_ratio = swfdec_video_video_provider_set_ratio;
}

/*** SWFDEC_VIDEO_VIDEO_PROVIDER ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecVideoVideoProvider, swfdec_video_video_provider, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_VIDEO_PROVIDER, swfdec_video_video_provider_video_provider_init))

static void
swfdec_video_video_provider_dispose (GObject *object)
{
  SwfdecVideoVideoProvider * provider = SWFDEC_VIDEO_VIDEO_PROVIDER (object);

  if (provider->decoder != NULL) {
    g_object_unref (provider->decoder);
    provider->decoder = NULL;
  }
  g_object_unref (provider->video);

  G_OBJECT_CLASS (swfdec_video_video_provider_parent_class)->dispose (object);
}

static void
swfdec_video_video_provider_class_init (SwfdecVideoVideoProviderClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_video_video_provider_dispose;
}

static void
swfdec_video_video_provider_init (SwfdecVideoVideoProvider * video)
{
}

SwfdecVideoProvider *
swfdec_video_video_provider_new (SwfdecVideo *video)
{
  SwfdecVideoVideoProvider *ret;

  g_return_val_if_fail (SWFDEC_IS_VIDEO (video), NULL);

  ret = g_object_new (SWFDEC_TYPE_VIDEO_VIDEO_PROVIDER, NULL);
  ret->video = g_object_ref (video);

  return SWFDEC_VIDEO_PROVIDER (ret);
}

