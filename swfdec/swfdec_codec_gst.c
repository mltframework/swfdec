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
#include <string.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include "swfdec_codec_audio.h"
#include "swfdec_codec_video.h"
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
    default:
      return NULL;
  }
  g_assert (caps);
  return caps;
}

/*** BUFFER ***/

/* NB: references argument more than once */
#define swfdec_buffer_new_from_gst(buffer) \
  swfdec_buffer_new_full (GST_BUFFER_DATA (buffer), GST_BUFFER_SIZE (buffer), \
      (SwfdecBufferFreeFunc) gst_mini_object_unref, (buffer))

static GstBuffer *
swfdec_gst_buffer_new (SwfdecBuffer *buffer)
{
  /* FIXME: make this a zero-copy operation */
  GstBuffer *ret;

  g_return_val_if_fail (buffer != NULL , NULL);
  
  ret = gst_buffer_new_and_alloc (buffer->length);
  memcpy (GST_BUFFER_DATA (ret), buffer->data, buffer->length);
  swfdec_buffer_unref (buffer);

  return ret;
}

/*** TYPEFINDING ***/

/* NB: try to mirror decodebin behavior */
static gboolean
swfdec_gst_feature_filter (GstPluginFeature *feature, gpointer caps)
{
  const GList *walk;
  const gchar *klass;

  /* we only care about element factories */
  if (!GST_IS_ELEMENT_FACTORY (feature))
    return FALSE;

  /* only decoders are interesting */
  klass = gst_element_factory_get_klass (GST_ELEMENT_FACTORY (feature));
  if (strstr (klass, "Decoder") == NULL)
    return FALSE;

  /* only select elements with autoplugging rank */
  if (gst_plugin_feature_get_rank (feature) < GST_RANK_MARGINAL)
    return FALSE;

  /* only care about the right sink caps */
  for (walk = gst_element_factory_get_static_pad_templates (GST_ELEMENT_FACTORY (feature));
       walk; walk = walk->next) {
    GstStaticPadTemplate *template = walk->data;
    GstCaps *intersect;
    GstCaps *template_caps;

    if (template->direction != GST_PAD_SINK)
      continue;

    template_caps = gst_static_caps_get (&template->static_caps);
    intersect = gst_caps_intersect (caps, template_caps);
    
    gst_caps_unref (template_caps);
    if (gst_caps_is_empty (intersect)) {
      gst_caps_unref (intersect);
    } else {
      gst_caps_unref (intersect);
      return TRUE;
    }
  }
  return FALSE;
}

static int
swfdec_gst_compare_features (gconstpointer a_, gconstpointer b_)
{
  int diff;
  GstPluginFeature *a = GST_PLUGIN_FEATURE (a_);
  GstPluginFeature *b = GST_PLUGIN_FEATURE (b_);

  diff = gst_plugin_feature_get_rank (b) - gst_plugin_feature_get_rank (a);
  if (diff != 0)
    return diff;

  return strcmp (gst_plugin_feature_get_name (a), gst_plugin_feature_get_name (b));
}

static GstElementFactory *
swfdec_gst_get_element_factory (GstCaps *caps)
{
  GstElementFactory *ret;
  GList *list;

  list = gst_registry_feature_filter (gst_registry_get_default (), 
      swfdec_gst_feature_filter, FALSE, caps);
  if (list == NULL)
    return NULL;

  list = g_list_sort (list, swfdec_gst_compare_features);
  ret = list->data;
  gst_object_ref (ret);
  gst_plugin_feature_list_free (list);
  return ret;
}

/*** PADS ***/

static GstPad *
swfdec_gst_connect_srcpad (GstElement *element, GstCaps *caps)
{
  GstPadTemplate *tmpl;
  GstPad *srcpad, *sinkpad;

  sinkpad = gst_element_get_pad (element, "sink");
  if (sinkpad == NULL)
    return NULL;
  gst_caps_ref (caps);
  tmpl = gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS, caps);
  srcpad = gst_pad_new_from_template (tmpl, "src");
  g_object_unref (tmpl);
  if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK)
    goto error;
  
  gst_object_unref (sinkpad);
  gst_pad_set_active (srcpad, TRUE);
  return srcpad;

error:
  SWFDEC_ERROR ("failed to create or link srcpad");
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);
  return NULL;
}

static GstPad *
swfdec_gst_connect_sinkpad (GstElement *element, GstCaps *caps)
{
  GstPadTemplate *tmpl;
  GstPad *srcpad, *sinkpad;

  srcpad = gst_element_get_pad (element, "src");
  if (srcpad == NULL)
    return NULL;
  gst_caps_ref (caps);
  tmpl = gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS, caps);
  sinkpad = gst_pad_new_from_template (tmpl, "sink");
  g_object_unref (tmpl);
  if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK)
    goto error;
  
  gst_object_unref (srcpad);
  gst_pad_set_active (sinkpad, TRUE);
  return sinkpad;

error:
  SWFDEC_ERROR ("failed to create or link sinkpad");
  gst_object_unref (srcpad);
  gst_object_unref (sinkpad);
  return NULL;
}

/*** DECODER ***/

typedef struct {
  GstElement *		decoder;
  GstPad *		src;
  GstPad *		sink;
  GQueue *		queue;		/* all the stored output GstBuffers */
} SwfdecGstDecoder;

static GstFlowReturn
swfdec_gst_chain_func (GstPad *pad, GstBuffer *buffer)
{
  GQueue *queue = g_object_get_data (G_OBJECT (pad), "swfdec-queue");

  g_queue_push_tail (queue, buffer);

  return GST_FLOW_OK;
}

static gboolean
swfdec_gst_decoder_init (SwfdecGstDecoder *dec, const char *name, GstCaps *srccaps, GstCaps *sinkcaps)
{
  if (name) {
    dec->decoder = gst_element_factory_make (name, "decoder");
  } else {
    GstElementFactory *factory = swfdec_gst_get_element_factory (srccaps);
    if (factory) {
      dec->decoder = gst_element_factory_create (factory, "decoder");
      gst_object_unref (factory);
    }
  }
  if (dec->decoder == NULL) {
    SWFDEC_ERROR ("failed to create decoder");
    return FALSE;
  }
  dec->src = swfdec_gst_connect_srcpad (dec->decoder, srccaps);
  if (dec->src == NULL)
    return FALSE;
  dec->sink = swfdec_gst_connect_sinkpad (dec->decoder, sinkcaps);
  if (dec->sink == NULL)
    return FALSE;
  gst_pad_set_chain_function (dec->sink, swfdec_gst_chain_func);
  dec->queue = g_queue_new ();
  g_object_set_data (G_OBJECT (dec->sink), "swfdec-queue", dec->queue);
  if (!gst_element_set_state (dec->decoder, GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS) {
    SWFDEC_ERROR ("could not change element state");
    return FALSE;
  }
  return TRUE;
}

static void
swfdec_gst_decoder_finish (SwfdecGstDecoder *dec)
{
  if (dec->decoder) {
    gst_element_set_state (dec->decoder, GST_STATE_NULL);
    g_object_unref (dec->decoder);
    dec->decoder = NULL;
  }
  if (dec->src) {
    g_object_unref (dec->src);
    dec->src = NULL;
  }
  if (dec->sink) {
    g_object_unref (dec->sink);
    dec->sink = NULL;
  }
  if (dec->queue) {
    GstBuffer *buffer;
    while ((buffer = g_queue_pop_head (dec->queue)) != NULL) {
      gst_buffer_unref (buffer);
    }
    g_queue_free (dec->queue);
    dec->queue = NULL;
  }
}

static gboolean
swfdec_gst_decoder_push (SwfdecGstDecoder *dec, GstBuffer *buffer)
{
  GstFlowReturn ret;
  GstCaps *caps;

  /* set caps if none set yet */
  caps = gst_buffer_get_caps (buffer);
  if (caps) {
    gst_caps_unref (caps);
  } else {
    caps = GST_PAD_CAPS (dec->src);
    if (caps == NULL) {
      caps = (GstCaps *) gst_pad_get_pad_template_caps (dec->src);
      g_assert (gst_caps_is_fixed (caps));
      gst_pad_set_caps (dec->src, caps);
    }
    gst_buffer_set_caps (buffer, GST_PAD_CAPS (dec->src));
  }

  ret = gst_pad_push (dec->src, buffer);
  if (GST_FLOW_IS_SUCCESS (ret))
    return TRUE;
  SWFDEC_ERROR ("error %d pushing data", (int) ret);
  return FALSE;
}

static void
swfdec_gst_decoder_push_eos (SwfdecGstDecoder *dec)
{
  gst_pad_push_event (dec->src, gst_event_new_eos ());
}

static GstBuffer *
swfdec_gst_decoder_pull (SwfdecGstDecoder *dec)
{
  return g_queue_pop_head (dec->queue);
}

/*** AUDIO ***/

typedef struct _SwfdecGstAudio SwfdecGstAudio;
struct _SwfdecGstAudio {
  SwfdecAudioDecoder	decoder;

  gboolean		error;
  SwfdecGstDecoder	dec;
  SwfdecGstDecoder	convert;
  SwfdecGstDecoder	resample;
};

static void
swfdec_audio_decoder_gst_free (SwfdecAudioDecoder *dec)
{
  SwfdecGstAudio *player = (SwfdecGstAudio *) dec;

  swfdec_gst_decoder_finish (&player->dec);
  swfdec_gst_decoder_finish (&player->convert);
  swfdec_gst_decoder_finish (&player->resample);

  g_slice_free (SwfdecGstAudio, player);
}

static void
swfdec_audio_decoder_gst_push (SwfdecAudioDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecGstAudio *player = (SwfdecGstAudio *) dec;
  GstBuffer *buf;

  if (player->error)
    return;
  if (buffer == NULL) {
    swfdec_gst_decoder_push_eos (&player->dec);
  } else {
    swfdec_buffer_ref (buffer);
    buf = swfdec_gst_buffer_new (buffer);
    if (!swfdec_gst_decoder_push (&player->dec, buf))
      goto error;
  }
  while ((buf = swfdec_gst_decoder_pull (&player->dec))) {
    if (!swfdec_gst_decoder_push (&player->convert, buf))
      goto error;
  }
  while ((buf = swfdec_gst_decoder_pull (&player->convert))) {
    if (!swfdec_gst_decoder_push (&player->resample, buf))
      goto error;
  }
  return;

error:
  SWFDEC_ERROR ("error pushing");
  player->error = TRUE;
}

static SwfdecBuffer *
swfdec_audio_decoder_gst_pull (SwfdecAudioDecoder *dec)
{
  SwfdecGstAudio *player = (SwfdecGstAudio *) dec;
  GstBuffer *buf;

  if (player->error)
    return NULL;
  buf = swfdec_gst_decoder_pull (&player->resample);
  if (buf == NULL)
    return NULL;
  return swfdec_buffer_new_from_gst (buf);
}

SwfdecAudioDecoder *
swfdec_audio_decoder_gst_new (guint type, SwfdecAudioFormat format)
{
  SwfdecGstAudio *player;
  GstCaps *srccaps, *sinkcaps;

  srccaps = swfdec_audio_decoder_get_caps (type, format);
  if (srccaps == NULL)
    return NULL;

  player = g_slice_new0 (SwfdecGstAudio);
  player->decoder.pull = swfdec_audio_decoder_gst_pull;
  player->decoder.push = swfdec_audio_decoder_gst_push;
  player->decoder.free = swfdec_audio_decoder_gst_free;

  /* create decoder */
  sinkcaps = gst_caps_from_string ("audio/x-raw-int");
  g_assert (sinkcaps);
  if (!swfdec_gst_decoder_init (&player->dec, NULL, srccaps, sinkcaps))
    goto error;
  /* create audioconvert */
  gst_caps_unref (srccaps);
  srccaps = sinkcaps;
  sinkcaps = gst_caps_from_string ("audio/x-raw-int, endianness=byte_order, signed=(boolean)true, width=16, depth=16, channels=2");
  g_assert (sinkcaps);
  if (!swfdec_gst_decoder_init (&player->convert, "audioconvert", srccaps, sinkcaps))
    goto error;
  /* create audiorate */
  gst_caps_unref (srccaps);
  srccaps = sinkcaps;
  sinkcaps = gst_caps_from_string ("audio/x-raw-int, endianness=byte_order, signed=(boolean)true, width=16, depth=16, rate=44100, channels=2");
  g_assert (sinkcaps);
  if (!swfdec_gst_decoder_init (&player->resample, "audioresample", srccaps, sinkcaps))
    goto error;
  g_object_set_data (G_OBJECT (player->resample.sink), "swfdec-player", player);

  gst_caps_unref (srccaps);
  gst_caps_unref (sinkcaps);
  return &player->decoder;

error:
  swfdec_audio_decoder_gst_free (&player->decoder);
  gst_caps_unref (srccaps);
  gst_caps_unref (sinkcaps);
  return NULL;
}

/*** VIDEO ***/

/* NB: We don't put a colorspace tansform here, we just assume that the codecs
 * in GStreamer decode to the native format that we enforce. */
typedef struct _SwfdecGstVideo SwfdecGstVideo;
struct _SwfdecGstVideo {
  SwfdecVideoDecoder	decoder;

  gboolean		error;
  SwfdecGstDecoder	dec;		/* the decoder element */
  GstBuffer *		last;		/* last decoded buffer */
};

static void
swfdec_video_decoder_gst_free (SwfdecVideoDecoder *dec)
{
  SwfdecGstVideo *player = (SwfdecGstVideo *) dec;

  swfdec_gst_decoder_finish (&player->dec);
  if (player->last)
    gst_buffer_unref (player->last);

  g_slice_free (SwfdecGstVideo, player);
}

static gboolean
swfdec_video_decoder_gst_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer,
    SwfdecVideoImage *image)
{
  SwfdecGstVideo *player = (SwfdecGstVideo *) dec;
#define SWFDEC_ALIGN(x, n) (((x) + (n) - 1) & (~((n) - 1)))
  GstBuffer *buf;
  GstCaps *caps;
  GstStructure *structure;

  if (player->error)
    return FALSE;
  if (player->last) {
    gst_buffer_unref (player->last);
    player->last = NULL;
  }

  buf = swfdec_gst_buffer_new (swfdec_buffer_ref (buffer));
  if (!swfdec_gst_decoder_push (&player->dec, buf)) {
    SWFDEC_ERROR ("failed to push buffer");
    player->error = TRUE;
    return FALSE;
  }

  player->last = swfdec_gst_decoder_pull (&player->dec);
  if (player->last == NULL) {
    SWFDEC_ERROR ("failed to pull decoded buffer");
    player->error = TRUE;
    return FALSE;
  }
  while ((buf = swfdec_gst_decoder_pull (&player->dec))) {
    SWFDEC_WARNING ("too many output buffers!");
  }
  caps = gst_buffer_get_caps (player->last);
  if (caps == NULL) {
    SWFDEC_ERROR ("no caps on decoded buffer");
    player->error = TRUE;
    return FALSE;
  }
  structure = gst_caps_get_structure (caps, 0);
  if (!gst_structure_get_int (structure, "width", (int *) &image->width) ||
      !gst_structure_get_int (structure, "height", (int *) &image->height)) {
    SWFDEC_ERROR ("invalid caps on decoded buffer");
    player->error = TRUE;
    return FALSE;
  }
  image->mask = NULL;
  buf = player->last;
  switch (swfdec_video_codec_get_format (dec->codec)) {
    case SWFDEC_VIDEO_FORMAT_RGBA:
      image->plane[0] = buf->data;
      image->rowstride[0] = image->width * 4;
      break;
    case SWFDEC_VIDEO_FORMAT_I420:
      image->plane[0] = buf->data;
      image->rowstride[0] = SWFDEC_ALIGN (image->width, 4);
      image->plane[1] = image->plane[0] + image->rowstride[0] * SWFDEC_ALIGN (image->height, 2);
      image->rowstride[1] = SWFDEC_ALIGN (image->width, 8) / 2;
      image->plane[2] = image->plane[1] + image->rowstride[1] * SWFDEC_ALIGN (image->height, 2) / 2;
      image->rowstride[2] = image->rowstride[1];
      g_assert (image->plane[2] + image->rowstride[2] * SWFDEC_ALIGN (image->height, 2) / 2 == image->plane[0] + buf->size);
      break;
    default:
      g_return_val_if_reached (FALSE);
  }
  return TRUE;
#undef SWFDEC_ALIGN
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

SwfdecVideoDecoder *
swfdec_video_decoder_gst_new (guint codec)
{
  SwfdecGstVideo *player;
  GstCaps *srccaps, *sinkcaps;

  srccaps = swfdec_video_decoder_get_caps (codec);
  if (srccaps == NULL)
    return NULL;
  sinkcaps = swfdec_video_decoder_get_sink_caps (codec);

  player = g_slice_new0 (SwfdecGstVideo);
  player->decoder.decode = swfdec_video_decoder_gst_decode;
  player->decoder.free = swfdec_video_decoder_gst_free;

  if (!swfdec_gst_decoder_init (&player->dec, NULL, srccaps, sinkcaps)) {
    swfdec_video_decoder_gst_free (&player->decoder);
    gst_caps_unref (srccaps);
    gst_caps_unref (sinkcaps);
    return NULL;
  }

  gst_caps_unref (srccaps);
  gst_caps_unref (sinkcaps);
  return &player->decoder;
}

/*** MISSING PLUGIN SUPPORT ***/
  
gboolean
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

gboolean
swfdec_video_decoder_gst_prepare (guint codec, char **detail)
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
    return TRUE;
  }

  /* need to install plugins... */
  *detail = gst_missing_decoder_installer_detail_new (caps);
  gst_caps_unref (caps);
  return FALSE;
}

