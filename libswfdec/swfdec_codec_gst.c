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

#include "swfdec_codec_audio.h"
#include "swfdec_codec_video.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

/*** BUFFER ***/

static void
swfdec_gst_buffer_free (unsigned char *data, gpointer priv)
{
  gst_buffer_unref (priv);
}

static SwfdecBuffer *
swfdec_buffer_new_from_gst (GstBuffer *buffer)
{
  SwfdecBuffer *ret;

  g_return_val_if_fail (GST_IS_BUFFER (buffer), NULL);

  ret = swfdec_buffer_new ();
  ret->data = GST_BUFFER_DATA (buffer);
  ret->length = GST_BUFFER_SIZE (buffer);
  ret->free = swfdec_gst_buffer_free;
  ret->priv = buffer;

  return ret;
}

static GstBuffer *
swfdec_gst_buffer_new (SwfdecBuffer *buffer)
{
  /* FIXME: make this a zero-copy operation */
  GstBuffer *ret;

  g_return_val_if_fail (buffer != NULL , NULL);
  
  ret = gst_buffer_new_and_alloc (buffer->length);
  memcpy (GST_BUFFER_DATA (ret), buffer->data, buffer->length);

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

static GstElement *
swfdec_gst_get_element (GstCaps *caps)
{
  GstElement *element;
  GList *list;

  list = gst_registry_feature_filter (gst_registry_get_default (), 
      swfdec_gst_feature_filter, FALSE, caps);
  if (list == NULL)
    return NULL;

  list = g_list_sort (list, swfdec_gst_compare_features);
  element = gst_element_factory_create (list->data, "decoder");
  gst_plugin_feature_list_free (list);
  return element;
}

/*** PADS ***/

static GstPad *
swfdec_gst_connect_srcpad (GstElement *element, GstCaps *caps)
{
  GstPad *srcpad, *sinkpad;

  sinkpad = gst_element_get_pad (element, "sink");
  if (sinkpad == NULL)
    return NULL;
  srcpad = gst_pad_new ("src", GST_PAD_SRC);
  if (!gst_pad_set_caps (srcpad, caps))
    goto error;
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
  GstPad *srcpad, *sinkpad;

  srcpad = gst_element_get_pad (element, "src");
  if (srcpad == NULL)
    return NULL;
  sinkpad = gst_pad_new ("sink", GST_PAD_SINK);
  if (!gst_pad_set_caps (sinkpad, caps))
    goto error;
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
    dec->decoder = swfdec_gst_get_element (srccaps);
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
  return GST_FLOW_IS_SUCCESS (gst_pad_push (dec->src, buffer));
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
swfdec_audio_decoder_gst_new (SwfdecAudioCodec type, SwfdecAudioFormat format)
{
  SwfdecGstAudio *player;
  GstCaps *srccaps, *sinkcaps;

  if (!gst_init_check (NULL, NULL, NULL))
    return NULL;

  switch (type) {
    case SWFDEC_AUDIO_CODEC_MP3:
      srccaps = gst_caps_from_string ("audio/mpeg, mpegversion=(int)1, layer=(int)3");
      break;
    default:
      return NULL;
  }
  g_assert (srccaps);

  player = g_slice_new0 (SwfdecGstAudio);
  player->decoder.format = swfdec_audio_format_new (44100, 2, TRUE);
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

#if 0
#define swfdec_cond_wait(cond, mutex) G_STMT_START { \
  g_print ("waiting at %s\n", G_STRLOC); \
  g_cond_wait (cond, mutex); \
  g_print ("   done at %s\n", G_STRLOC); \
}G_STMT_END
#else
#define swfdec_cond_wait g_cond_wait
#endif

typedef struct _SwfdecGstVideo SwfdecGstVideo;
struct _SwfdecGstVideo {
  SwfdecVideoDecoder	decoder;

  GMutex *	  	mutex;		/* mutex that blocks everything below (NB: locked by default) */
  GCond *		cond;		/* cond used to signal when stuff below changes */
  volatile int		refcount;	/* refcount (d'oh) */

  GstElement *		pipeline;	/* pipeline that is playing or NULL when done */
  SwfdecBuffer *	in;		/* next input buffer or NULL */
  GstBuffer *		out;		/* available output or NULL */
  int			width;		/* width of last output buffer */
  int			height;		/* height of last output buffer */
  GstCaps *		srccaps;	/* caps to set on buffers */
  gboolean		out_next;	/* wether the pipeline expects input or output */
  gboolean		error;		/* we're in an error state */
};

static void
swfdec_gst_video_unref (gpointer data, GObject *unused)
{
  SwfdecGstVideo *player = data;

  if (!g_atomic_int_dec_and_test (&player->refcount))
    return;
  g_cond_free (player->cond);
  g_mutex_free (player->mutex);
  gst_caps_unref (player->srccaps);
  if (player->in)
    swfdec_buffer_unref (player->in);
  if (player->out)
    gst_buffer_unref (player->out);
  g_slice_free (SwfdecGstVideo, player);
}

static void
swfdec_video_decoder_gst_free (SwfdecVideoDecoder *dec)
{
  SwfdecGstVideo *player = (SwfdecGstVideo *) dec;
  GstElement *pipeline;

  pipeline = player->pipeline;
  player->pipeline = NULL;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);

  swfdec_gst_video_unref (player, NULL);
}

static gboolean
swfdec_video_decoder_gst_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer,
    SwfdecVideoImage *image)
{
  SwfdecGstVideo *player = (SwfdecGstVideo *) dec;
#define ALIGN(x, n) (((x) + (n) - 1) & (~((n) - 1)))

  while (player->in != NULL && !player->error) {
    swfdec_cond_wait (player->cond, player->mutex);
  }
  player->in = swfdec_buffer_ref (buffer);
  g_cond_signal (player->cond);
  if (player->out) {
    gst_buffer_unref (player->out);
    player->out = NULL;
  }
  while (player->out == NULL && !player->error) {
    swfdec_cond_wait (player->cond, player->mutex);
  }
  if (player->error)
    return FALSE;
  image->width = player->width;
  image->height = player->height;
  image->mask = NULL;
  switch (swfdec_video_codec_get_format (dec->codec)) {
    case SWFDEC_VIDEO_FORMAT_RGBA:
      image->plane[0] = player->out->data;
      image->rowstride[0] = player->width * 4;
      break;
    case SWFDEC_VIDEO_FORMAT_I420:
      image->plane[0] = player->out->data;
      image->rowstride[0] = ALIGN (player->width, 4);
      image->plane[1] = image->plane[0] + image->rowstride[0] * ALIGN (player->height, 2);
      image->rowstride[1] = ALIGN (player->width, 8) / 2;
      image->plane[2] = image->plane[1] + image->rowstride[1] * ALIGN (player->height, 2) / 2;
      image->rowstride[2] = image->rowstride[1];
      g_assert (image->plane[2] + image->rowstride[2] * ALIGN (player->height, 2) / 2 == image->plane[0] + player->out->size);
      break;
  }
  return TRUE;
}

static void
swfdec_video_decoder_gst_fakesrc_handoff (GstElement *fakesrc, GstBuffer *buf, 
    GstPad *pad, SwfdecGstVideo *player)
{
  g_mutex_lock (player->mutex);
  if (player->out_next) {
    player->error = TRUE;
    g_cond_signal (player->cond);
    g_mutex_unlock (player->mutex);
    return;
  }
  while (player->pipeline != NULL && player->in == NULL)
    swfdec_cond_wait (player->cond, player->mutex);
  if (player->pipeline == NULL) {
    g_mutex_unlock (player->mutex);
    return;
  }
  buf->data = g_memdup (player->in->data, player->in->length);
  buf->malloc_data = buf->data;
  buf->size = player->in->length;
  gst_buffer_set_caps (buf, player->srccaps);
  swfdec_buffer_unref (player->in);
  player->in = NULL;
  player->out_next = TRUE;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
}

static void
swfdec_video_decoder_gst_fakesink_handoff (GstElement *fakesrc, GstBuffer *buf, 
    GstPad *pad, SwfdecGstVideo *player)
{
  GstCaps *caps;

  g_mutex_lock (player->mutex);
  if (!player->out_next) {
    player->error = TRUE;
    g_cond_signal (player->cond);
    g_mutex_unlock (player->mutex);
    return;
  }
  caps = gst_buffer_get_caps (buf);
  if (caps) {
    GstStructure *structure = gst_caps_get_structure (caps, 0);
    if (!gst_structure_get_int (structure, "width", &player->width) ||
        !gst_structure_get_int (structure, "height", &player->height)) {
      player->width = 0;
      player->height = 0;
    }
    gst_caps_unref (caps);
  }
  if (player->pipeline == NULL) {
    g_mutex_unlock (player->mutex);
    return;
  }
  if (player->out != NULL)
    gst_buffer_unref (player->out);
  player->out = gst_buffer_ref (buf);
  player->out_next = FALSE;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
}

static void
swfdec_video_decoder_gst_link (GstElement *src, GstPad *pad, GstElement *sink)
{
  GstCaps *caps;
  
  caps = g_object_get_data (G_OBJECT (sink), "swfdec-caps");
  g_assert (caps);
  if (!gst_element_link_filtered (src, sink, caps)) {
    SWFDEC_ERROR ("no delayed link");
  }
}

static GstCaps *
swfdec_video_decoder_get_sink_caps (SwfdecVideoCodec codec)
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
  }
  g_assert_not_reached ();
  return NULL;
}

SwfdecVideoDecoder *
swfdec_video_decoder_gst_new (SwfdecVideoCodec codec)
{
  SwfdecGstVideo *player;
  GstElement *fakesrc, *fakesink, *decoder;
  GstCaps *caps;

  if (!gst_init_check (NULL, NULL, NULL))
    return NULL;

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

  player = g_slice_new0 (SwfdecGstVideo);
  player->decoder.decode = swfdec_video_decoder_gst_decode;
  player->decoder.free = swfdec_video_decoder_gst_free;
  player->pipeline = gst_pipeline_new ("pipeline");
  player->refcount = 1;
  g_assert (player->pipeline);
  player->mutex = g_mutex_new ();
  g_mutex_lock (player->mutex);
  player->cond = g_cond_new ();
  player->srccaps = caps;
  fakesrc = gst_element_factory_make ("fakesrc", NULL);
  if (fakesrc == NULL) {
    SWFDEC_ERROR ("failed to create fakesrc");
    swfdec_video_decoder_gst_free (&player->decoder);
    return NULL;
  }
  g_object_set (fakesrc, "signal-handoffs", TRUE, 
      "can-activate-pull", FALSE, NULL);
  g_signal_connect (fakesrc, "handoff", 
      G_CALLBACK (swfdec_video_decoder_gst_fakesrc_handoff), player);
  g_atomic_int_inc (&player->refcount);
  g_object_weak_ref (G_OBJECT (fakesrc), swfdec_gst_video_unref, player);
  gst_bin_add (GST_BIN (player->pipeline), fakesrc);
  fakesink = gst_element_factory_make ("fakesink", NULL);
  if (fakesink == NULL) {
    SWFDEC_ERROR ("failed to create fakesink");
    swfdec_video_decoder_gst_free (&player->decoder);
    return NULL;
  }
  g_object_set (fakesink, "signal-handoffs", TRUE, NULL);
  g_signal_connect (fakesink, "handoff", 
      G_CALLBACK (swfdec_video_decoder_gst_fakesink_handoff), player);
  caps = swfdec_video_decoder_get_sink_caps (codec);
  g_assert (caps);
  g_object_set_data_full (G_OBJECT (fakesink), "swfdec-caps", caps, 
      (GDestroyNotify) gst_caps_unref);
  g_atomic_int_inc (&player->refcount);
  g_object_weak_ref (G_OBJECT (fakesink), swfdec_gst_video_unref, player);
  gst_bin_add (GST_BIN (player->pipeline), fakesink);
  decoder = gst_element_factory_make ("decodebin", NULL);
  if (decoder == NULL) {
    SWFDEC_ERROR ("failed to create decoder");
    swfdec_video_decoder_gst_free (&player->decoder);
    return NULL;
  }
  gst_bin_add (GST_BIN (player->pipeline), decoder);
  g_signal_connect (decoder, "pad-added", 
      G_CALLBACK (swfdec_video_decoder_gst_link), fakesink);

  if (!gst_element_link_filtered (fakesrc, decoder, player->srccaps)) {
    SWFDEC_ERROR ("linking failed");
    swfdec_video_decoder_gst_free (&player->decoder);
    return NULL;
  }
  if (gst_element_set_state (player->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    SWFDEC_ERROR ("failed to change sate");
    swfdec_video_decoder_gst_free (&player->decoder);
    return NULL;
  }

  return &player->decoder;
}

