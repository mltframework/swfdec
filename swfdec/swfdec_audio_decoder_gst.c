/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#include <gst/pbutils/pbutils.h>

#include "swfdec_audio_decoder_gst.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

/*** CAPS MATCHING ***/

static GstCaps *
swfdec_audio_decoder_get_caps (guint codec, SwfdecAudioFormat format)
{
  GstCaps *caps;
  char *s;

  switch (codec) {
    case SWFDEC_AUDIO_CODEC_MP3:
      s = g_strdup ("audio/mpeg, mpegversion=(int)1, layer=(int)3");
      break;
    case SWFDEC_AUDIO_CODEC_NELLYMOSER_8KHZ:
      s = g_strdup ("audio/x-nellymoser, rate=8000, channels=1");
      break;
    case SWFDEC_AUDIO_CODEC_NELLYMOSER:
      s = g_strdup_printf ("audio/x-nellymoser, rate=%d, channels=%d",
	  swfdec_audio_format_get_rate (format), 
	  swfdec_audio_format_get_channels (format));
      break;
    default:
      return NULL;
  }

  caps = gst_caps_from_string (s);
  g_assert (caps);
  g_free (s);
  return caps;
}

G_DEFINE_TYPE (SwfdecAudioDecoderGst, swfdec_audio_decoder_gst, SWFDEC_TYPE_AUDIO_DECODER)

static gboolean
swfdec_audio_decoder_gst_prepare (guint codec, SwfdecAudioFormat format, char **detail)
{
  GstElementFactory *factory;
  GstCaps *caps;

  /* Check if we can handle the format at all. If not, no plugin will help us. */
  caps = swfdec_audio_decoder_get_caps (codec, format);
  if (caps == NULL)
    return FALSE;

  /* If we can already handle it, woohoo! */
  factory = swfdec_gst_get_element_factory (caps);
  if (factory != NULL) {
    gst_object_unref (factory);
    return TRUE;
  }

  /* need to install plugins... */
  *detail = gst_missing_decoder_installer_detail_new (caps);
  gst_caps_unref (caps);
  return FALSE;
}

static const char *
swfdec_audio_decoder_get_resampler (void)
{
  /* FIXME: This is hardcoded as there's no autopluggable way to get the 
   * best resampler by rank.
   * Even if there were, audioresample (which has the highest rank) is so slow
   * it takes roughly a second to resample stuff that ffaudioresample does in 
   * 0.05 seconds.
   */
  static const char *options[] = { "ffaudioresample", "speexresample", "audioresample" };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (options); i++) {
    GstElementFactory *factory = gst_element_factory_find (options[i]);
    if (factory) {
      gst_object_unref (factory);
      return options[i];
    }
  }
  SWFDEC_ERROR ("no resampling element found. Check that GStreamer's base plugins are installed.");
  return NULL;
}

static SwfdecAudioDecoder *
swfdec_audio_decoder_gst_create (guint type, SwfdecAudioFormat format)
{
  SwfdecAudioDecoderGst *player;
  GstCaps *srccaps, *sinkcaps;
  const char *resample;

  srccaps = swfdec_audio_decoder_get_caps (type, format);
  if (srccaps == NULL)
    return NULL;

  player = g_object_new (SWFDEC_TYPE_AUDIO_DECODER_GST, NULL);

  /* create decoder */
  sinkcaps = gst_caps_from_string ("audio/x-raw-int, endianness=byte_order, signed=(boolean)true, width=16, depth=16, rate=44100, channels=2");
  g_assert (sinkcaps);
  resample = swfdec_audio_decoder_get_resampler ();
  if (resample == NULL)
    goto error;
  if (!swfdec_gst_decoder_init (&player->dec, srccaps, sinkcaps, 
	"audioconvert", resample, NULL))
    goto error;

  gst_caps_unref (srccaps);
  gst_caps_unref (sinkcaps);
  return SWFDEC_AUDIO_DECODER (player);

error:
  g_object_unref (player);
  gst_caps_unref (srccaps);
  gst_caps_unref (sinkcaps);
  return NULL;
}

static void
swfdec_audio_decoder_gst_push (SwfdecAudioDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecAudioDecoderGst *player = SWFDEC_AUDIO_DECODER_GST (dec);
  GstBuffer *buf;

  if (buffer == NULL) {
    swfdec_gst_decoder_push_eos (&player->dec);
  } else {
    swfdec_buffer_ref (buffer);
    buf = swfdec_gst_buffer_new (buffer);
    if (!swfdec_gst_decoder_push (&player->dec, buf))
      swfdec_audio_decoder_error (dec, "error pushing");
  }
}

static SwfdecBuffer *
swfdec_audio_decoder_gst_pull (SwfdecAudioDecoder *dec)
{
  SwfdecAudioDecoderGst *player = SWFDEC_AUDIO_DECODER_GST (dec);
  GstBuffer *buf;

  buf = swfdec_gst_decoder_pull (&player->dec);
  if (buf == NULL)
    return NULL;
  return swfdec_buffer_new_from_gst (buf);
}

static void
swfdec_audio_decoder_gst_dispose (GObject *object)
{
  SwfdecAudioDecoderGst *player = (SwfdecAudioDecoderGst *) object;

  swfdec_gst_decoder_finish (&player->dec);

  G_OBJECT_CLASS (swfdec_audio_decoder_gst_parent_class)->dispose (object);
}

static void
swfdec_audio_decoder_gst_class_init (SwfdecAudioDecoderGstClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioDecoderClass *decoder_class = SWFDEC_AUDIO_DECODER_CLASS (klass);

  object_class->dispose = swfdec_audio_decoder_gst_dispose;

  decoder_class->prepare = swfdec_audio_decoder_gst_prepare;
  decoder_class->create = swfdec_audio_decoder_gst_create;
  decoder_class->pull = swfdec_audio_decoder_gst_pull;
  decoder_class->push = swfdec_audio_decoder_gst_push;
}

static void
swfdec_audio_decoder_gst_init (SwfdecAudioDecoderGst *audio_decoder_gst)
{
}

