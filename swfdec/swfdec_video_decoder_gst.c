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

#include <string.h>
#include <gst/pbutils/pbutils.h>

#include "swfdec_video_decoder_gst.h"
#include "swfdec_codec_gst.h"
#include "swfdec_debug.h"

static GstCaps *
swfdec_video_decoder_get_caps (guint codec)
{
  GstCaps *caps;

  switch (codec) {
    case SWFDEC_VIDEO_CODEC_H263:
      caps = gst_caps_from_string ("video/x-flash-video");
      break;
    case SWFDEC_VIDEO_CODEC_VP6:
      caps = gst_caps_from_string ("video/x-vp6-flash");
      break;
    case SWFDEC_VIDEO_CODEC_H264:
      caps = gst_caps_from_string ("video/x-h264");
      break;
    default:
      return NULL;
  }
  g_assert (caps);
  return caps;
}

static GstCaps *
swfdec_video_decoder_get_sink_caps (guint codec)
{
  switch (swfdec_video_codec_get_format (codec)) {
    case SWFDEC_VIDEO_FORMAT_RGBA:
#if G_BYTE_ORDER == G_BIG_ENDIAN
      return gst_caps_from_string ("video/x-raw-rgb, bpp=32, endianness=4321, depth=24, "
	  "red_mask=16711680, green_mask=65280, blue_mask=255");
#else
      return gst_caps_from_string ("video/x-raw-rgb, bpp=32, endianness=4321, depth=24, "
	  "red_mask=65280, green_mask=16711680, blue_mask=-16777216");
#endif
    case SWFDEC_VIDEO_FORMAT_I420:
      return gst_caps_from_string ("video/x-raw-yuv, format=(fourcc)I420");
    default:
      g_return_val_if_reached (NULL);
  }
}

G_DEFINE_TYPE (SwfdecVideoDecoderGst, swfdec_video_decoder_gst, SWFDEC_TYPE_VIDEO_DECODER)

static gboolean
swfdec_video_decoder_gst_prepare (guint codec, char **missing)
{
  GstElementFactory *factory;
  GstCaps *caps;

  /* Check if we can handle the format at all. If not, no plugin will help us. */
  caps = swfdec_video_decoder_get_caps (codec);
  if (caps == NULL)
    return FALSE;

  /* If we can already handle it, woohoo! */
  factory = swfdec_gst_get_element_factory (caps);
  if (factory != NULL) {
    gst_object_unref (factory);
    gst_caps_unref (caps);
    return TRUE;
  }

  /* need to install plugins... */
  *missing = gst_missing_decoder_installer_detail_new (caps);
  gst_caps_unref (caps);
  return FALSE;
}

static SwfdecVideoDecoder *
swfdec_video_decoder_gst_create (guint codec)
{
  SwfdecVideoDecoderGst *player;
  GstCaps *srccaps, *sinkcaps;

  srccaps = swfdec_video_decoder_get_caps (codec);
  if (srccaps == NULL)
    return NULL;
  sinkcaps = swfdec_video_decoder_get_sink_caps (codec);

  player = g_object_new (SWFDEC_TYPE_VIDEO_DECODER_GST, NULL);

  if (!swfdec_gst_decoder_init (&player->dec, srccaps, sinkcaps, NULL)) {
    g_object_unref (player);
    gst_caps_unref (srccaps);
    gst_caps_unref (sinkcaps);
    return NULL;
  }

  gst_caps_unref (srccaps);
  gst_caps_unref (sinkcaps);
  return &player->decoder;
}

static void
swfdec_video_decoder_gst_set_codec_data (SwfdecVideoDecoder *dec,
    SwfdecBuffer *buffer)
{
  SwfdecVideoDecoderGst *player = SWFDEC_VIDEO_DECODER_GST (dec);

  if (buffer) {
    GstBuffer *buf = swfdec_gst_buffer_new (swfdec_buffer_ref (buffer));
    swfdec_gst_decoder_set_codec_data (&player->dec, buf);
    gst_buffer_unref (buf);
  } else {
    swfdec_gst_decoder_set_codec_data (&player->dec, NULL);
  }
}

static void
swfdec_video_decoder_gst_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecVideoDecoderGst *player = SWFDEC_VIDEO_DECODER_GST (dec);
#define SWFDEC_ALIGN(x, n) (((x) + (n) - 1) & (~((n) - 1)))
  GstBuffer *buf;
  GstCaps *caps;
  GstStructure *structure;

  buf = swfdec_gst_buffer_new (swfdec_buffer_ref (buffer));
  if (!swfdec_gst_decoder_push (&player->dec, buf)) {
    swfdec_video_decoder_error (dec, "failed to push buffer");
    return;
  }

  buf = swfdec_gst_decoder_pull (&player->dec);
  if (buf == NULL) {
    SWFDEC_ERROR ("failed to pull decoded buffer. Broken stream?");
    return;
  } else {
    if (player->last)
      gst_buffer_unref (player->last);
    player->last = buf;
  }

  while ((buf = swfdec_gst_decoder_pull (&player->dec))) {
    SWFDEC_ERROR ("too many output buffers!");
    gst_buffer_unref (buf);
  }
  caps = gst_buffer_get_caps (player->last);
  if (caps == NULL) {
    swfdec_video_decoder_error (dec, "no caps on decoded buffer");
    return;
  }
  structure = gst_caps_get_structure (caps, 0);
  if (!gst_structure_get_int (structure, "width", (int *) &dec->width) ||
      !gst_structure_get_int (structure, "height", (int *) &dec->height)) {
    swfdec_video_decoder_error (dec, "invalid caps on decoded buffer");
    return;
  }
  buf = player->last;
  switch (swfdec_video_codec_get_format (dec->codec)) {
    case SWFDEC_VIDEO_FORMAT_RGBA:
      dec->plane[0] = buf->data;
      dec->rowstride[0] = dec->width * 4;
      break;
    case SWFDEC_VIDEO_FORMAT_I420:
      dec->plane[0] = buf->data;
      dec->rowstride[0] = SWFDEC_ALIGN (dec->width, 4);
      dec->plane[1] = dec->plane[0] + dec->rowstride[0] * SWFDEC_ALIGN (dec->height, 2);
      dec->rowstride[1] = SWFDEC_ALIGN (dec->width, 8) / 2;
      dec->plane[2] = dec->plane[1] + dec->rowstride[1] * SWFDEC_ALIGN (dec->height, 2) / 2;
      dec->rowstride[2] = dec->rowstride[1];
      g_assert (dec->plane[2] + dec->rowstride[2] * SWFDEC_ALIGN (dec->height, 2) / 2 == dec->plane[0] + buf->size);
      break;
    default:
      g_return_if_reached ();
  }
#undef SWFDEC_ALIGN
}

static void
swfdec_video_decoder_gst_dispose (GObject *object)
{
  SwfdecVideoDecoderGst *player = SWFDEC_VIDEO_DECODER_GST (object);

  swfdec_gst_decoder_finish (&player->dec);
  if (player->last)
    gst_buffer_unref (player->last);

  G_OBJECT_CLASS (swfdec_video_decoder_gst_parent_class)->dispose (object);
}

static void
swfdec_video_decoder_gst_class_init (SwfdecVideoDecoderGstClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecVideoDecoderClass *decoder_class = SWFDEC_VIDEO_DECODER_CLASS (klass);

  object_class->dispose = swfdec_video_decoder_gst_dispose;

  decoder_class->prepare = swfdec_video_decoder_gst_prepare;
  decoder_class->create = swfdec_video_decoder_gst_create;
  decoder_class->set_codec_data = swfdec_video_decoder_gst_set_codec_data;
  decoder_class->decode = swfdec_video_decoder_gst_decode;
}

static void
swfdec_video_decoder_gst_init (SwfdecVideoDecoderGst *dec)
{
}

