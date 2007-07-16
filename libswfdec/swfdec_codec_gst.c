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

#if 0
#define swfdec_cond_wait(cond, mutex) G_STMT_START { \
  g_print ("waiting at %s\n", G_STRLOC); \
  g_cond_wait (cond, mutex); \
  g_print ("   done at %s\n", G_STRLOC); \
}G_STMT_END
#else
#define swfdec_cond_wait g_cond_wait
#endif

/*** AUDIO ***/

typedef struct _SwfdecGstAudio SwfdecGstAudio;
struct _SwfdecGstAudio {
  SwfdecAudioDecoder	decoder;

  GMutex *	  	mutex;		/* mutex that blocks everything below */
  GCond *		cond;		/* cond used to signal when stuff below changes */
  volatile int		refcount;	/* refcount (d'oh) */

  GstElement *		pipeline;	/* pipeline that is playing or NULL when done */
  SwfdecBuffer *	in;		/* next input buffer or NULL */
  SwfdecBufferQueue *	out;		/* all the stored output buffers */
  GstCaps *		srccaps;	/* caps to set on buffers */
  gboolean		eof;      	/* we've pushed EOF */
  gboolean		done;		/* TRUE after decoding stopped (error or EOF) */
};

static void
swfdec_gst_audio_unref (gpointer data, GObject *unused)
{
  SwfdecGstAudio *player = data;

  if (!g_atomic_int_dec_and_test (&player->refcount))
    return;
  g_cond_free (player->cond);
  g_mutex_free (player->mutex);
  gst_caps_unref (player->srccaps);
  if (player->in)
    swfdec_buffer_unref (player->in);
  swfdec_buffer_queue_unref (player->out);
  g_slice_free (SwfdecGstAudio, player);
}

static void
swfdec_audio_decoder_gst_free (SwfdecAudioDecoder *dec)
{
  SwfdecGstAudio *player = (SwfdecGstAudio *) dec;
  GstElement *pipeline;

  g_mutex_lock (player->mutex);
  pipeline = player->pipeline;
  player->pipeline = NULL;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);

  swfdec_gst_audio_unref (player, NULL);
}

static void
swfdec_audio_decoder_gst_push (SwfdecAudioDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecGstAudio *player = (SwfdecGstAudio *) dec;

  g_mutex_lock (player->mutex);
  g_return_if_fail (!player->eof);
  while (player->in != NULL && !player->done) {
    swfdec_cond_wait (player->cond, player->mutex);
  }
  if (buffer) {
    player->in = buffer;
  } else {
    player->eof = TRUE;
  }
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
}

static SwfdecBuffer *
swfdec_audio_decoder_gst_pull (SwfdecAudioDecoder *dec)
{
  SwfdecGstAudio *player = (SwfdecGstAudio *) dec;
  SwfdecBuffer *buffer;

  g_mutex_lock (player->mutex);
  if (player->eof) {
    while (!player->done)
      swfdec_cond_wait (player->cond, player->mutex);
  }
  buffer = swfdec_buffer_queue_pull_buffer (player->out);
  g_mutex_unlock (player->mutex);
  return buffer;
}

static void
swfdec_audio_decoder_gst_fakesrc_handoff (GstElement *fakesrc, GstBuffer *buf, 
    GstPad *pad, SwfdecGstAudio *player)
{
  g_mutex_lock (player->mutex);
  while (player->pipeline != NULL && player->in == NULL && player->eof == FALSE)
    swfdec_cond_wait (player->cond, player->mutex);
  if (player->pipeline == NULL) {
    g_mutex_unlock (player->mutex);
    return;
  }
  if (player->eof) {
    //doesn't work: g_object_set (fakesrc, "num-buffers", 1, NULL);
    /* HACK: just tell everyone we're done, that'll probably lose data in the
     * gst stream, since we can't properly push EOF, but that's life... */
    player->done = TRUE;
  }
  if (player->in) {
    buf->data = g_memdup (player->in->data, player->in->length);
    buf->malloc_data = buf->data;
    buf->size = player->in->length;
  }
  gst_buffer_set_caps (buf, player->srccaps);
  player->in = NULL;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
}

static void
swfdec_audio_decoder_gst_fakesink_handoff (GstElement *fakesrc, GstBuffer *buf, 
    GstPad *pad, SwfdecGstAudio *player)
{
  SwfdecBuffer *buffer;

  g_mutex_lock (player->mutex);

  while (player->pipeline == NULL && player->out != NULL)
    swfdec_cond_wait (player->cond, player->mutex);
  buffer = swfdec_buffer_new_for_data (
      g_memdup (buf->data, buf->size), buf->size);
  swfdec_buffer_queue_push (player->out, buffer);
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
}

static void
swfdec_audio_decoder_gst_link (GstElement *src, GstPad *pad, GstElement *sink)
{
  if (!gst_element_link (src, sink)) {
    SWFDEC_ERROR ("no delayed link");
  }
}

static GstBusSyncReply
swfdec_audio_decoder_gst_handle_bus (GstBus *bus, GstMessage *message, gpointer data)
{
  SwfdecGstAudio *player = data;

  switch (message->type) {
    case GST_MESSAGE_EOS:
    case GST_MESSAGE_ERROR:
      g_mutex_lock (player->mutex);
      g_cond_signal (player->cond);
      player->done = TRUE;
      g_mutex_unlock (player->mutex);
      break;
    default:
      break;
  }
  return GST_BUS_PASS;
}

SwfdecAudioDecoder *
swfdec_audio_decoder_gst_new (SwfdecAudioFormat type, gboolean width, SwfdecAudioOut format)
{
  SwfdecGstAudio *player;
  GstElement *fakesrc, *fakesink, *decoder, *convert;
  GstBus *bus;
  GstCaps *caps;

  if (!gst_init_check (NULL, NULL, NULL))
    return NULL;

  switch (type) {
    case SWFDEC_AUDIO_FORMAT_MP3:
      caps = gst_caps_from_string ("audio/mpeg, mpegversion=(int)1, layer=(int)3");
      break;
    default:
      return NULL;
  }
  g_assert (caps);

  player = g_slice_new0 (SwfdecGstAudio);
  player->decoder.out_format = SWFDEC_AUDIO_OUT_STEREO_44100;
  player->decoder.pull = swfdec_audio_decoder_gst_pull;
  player->decoder.push = swfdec_audio_decoder_gst_push;
  player->decoder.free = swfdec_audio_decoder_gst_free;
  player->pipeline = gst_pipeline_new ("pipeline");
  player->refcount = 1;
  g_assert (player->pipeline);
  bus = gst_element_get_bus (player->pipeline);
  g_atomic_int_inc (&player->refcount);
  g_object_weak_ref (G_OBJECT (bus), swfdec_gst_audio_unref, player);
  gst_bus_set_sync_handler (bus, swfdec_audio_decoder_gst_handle_bus, player);
  player->mutex = g_mutex_new ();
  player->cond = g_cond_new ();
  player->out = swfdec_buffer_queue_new ();
  player->srccaps = caps;
  fakesrc = gst_element_factory_make ("fakesrc", NULL);
  if (fakesrc == NULL) {
    SWFDEC_ERROR ("failed to create fakesrc");
    swfdec_audio_decoder_gst_free (&player->decoder);
    return NULL;
  }
  g_object_set (fakesrc, "signal-handoffs", TRUE, 
      "can-activate-pull", FALSE, NULL);
  g_signal_connect (fakesrc, "handoff", 
      G_CALLBACK (swfdec_audio_decoder_gst_fakesrc_handoff), player);
  g_atomic_int_inc (&player->refcount);
  g_object_weak_ref (G_OBJECT (fakesrc), swfdec_gst_audio_unref, player);
  gst_bin_add (GST_BIN (player->pipeline), fakesrc);
  fakesink = gst_element_factory_make ("fakesink", NULL);
  if (fakesink == NULL) {
    SWFDEC_ERROR ("failed to create fakesink");
    swfdec_audio_decoder_gst_free (&player->decoder);
    return NULL;
  }
  g_object_set (fakesink, "signal-handoffs", TRUE, NULL);
  g_signal_connect (fakesink, "handoff", 
      G_CALLBACK (swfdec_audio_decoder_gst_fakesink_handoff), player);
  g_atomic_int_inc (&player->refcount);
  g_object_weak_ref (G_OBJECT (fakesink), swfdec_gst_audio_unref, player);
  gst_bin_add (GST_BIN (player->pipeline), fakesink);
  decoder = gst_element_factory_make ("decodebin", NULL);
  if (decoder == NULL) {
    SWFDEC_ERROR ("failed to create decoder");
    swfdec_audio_decoder_gst_free (&player->decoder);
    return NULL;
  }
  gst_bin_add (GST_BIN (player->pipeline), decoder);
  convert = gst_element_factory_make ("audioconvert", NULL);
  if (convert == NULL) {
    SWFDEC_ERROR ("failed to create audioconvert");
    swfdec_audio_decoder_gst_free (&player->decoder);
    return NULL;
  }
  gst_bin_add (GST_BIN (player->pipeline), convert);
  g_signal_connect (decoder, "pad-added", 
      G_CALLBACK (swfdec_audio_decoder_gst_link), convert);

  caps = gst_caps_from_string ("audio/x-raw-int, endianness=byte_order, signed=(boolean)true, width=16, depth=16, rate=44100, channels=2");
  g_assert (caps);
  if (!gst_element_link_filtered (fakesrc, decoder, player->srccaps) ||
      !gst_element_link_filtered (convert, fakesink, caps)) {
    SWFDEC_ERROR ("linking failed");
    swfdec_audio_decoder_gst_free (&player->decoder);
    return NULL;
  }
  gst_caps_unref (caps);
  if (gst_element_set_state (player->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    SWFDEC_ERROR ("failed to change sate");
    swfdec_audio_decoder_gst_free (&player->decoder);
    return NULL;
  }

  return &player->decoder;
}

/*** VIDEO ***/

typedef struct _SwfdecGstVideo SwfdecGstVideo;
struct _SwfdecGstVideo {
  SwfdecVideoDecoder	decoder;

  GMutex *	  	mutex;		/* mutex that blocks everything below */
  GCond *		cond;		/* cond used to signal when stuff below changes */
  volatile int		refcount;	/* refcount (d'oh) */

  GstElement *		pipeline;	/* pipeline that is playing or NULL when done */
  SwfdecBuffer *	in;		/* next input buffer or NULL */
  SwfdecBuffer *	out;		/* available output or NULL */
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
    swfdec_buffer_unref (player->out);
  g_slice_free (SwfdecGstVideo, player);
}

static void
swfdec_video_decoder_gst_free (SwfdecVideoDecoder *dec)
{
  SwfdecGstVideo *player = (SwfdecGstVideo *) dec;
  GstElement *pipeline;

  g_mutex_lock (player->mutex);
  pipeline = player->pipeline;
  player->pipeline = NULL;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);

  swfdec_gst_video_unref (player, NULL);
}

static SwfdecBuffer *
swfdec_video_decoder_gst_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer,
    guint *width, guint *height, guint *rowstride)
{
  SwfdecGstVideo *player = (SwfdecGstVideo *) dec;

  g_mutex_lock (player->mutex);
  while (player->in != NULL && !player->error) {
    swfdec_cond_wait (player->cond, player->mutex);
  }
  player->in = buffer;
  g_cond_signal (player->cond);
  while (player->out == NULL && !player->error) {
    swfdec_cond_wait (player->cond, player->mutex);
  }
  if (player->error) {
    g_mutex_unlock (player->mutex);
    return NULL;
  }
  buffer = player->out;
  player->out = NULL;
  *width = player->width;
  *height = player->height;
  *rowstride = player->width * 4;
  g_mutex_unlock (player->mutex);
  return buffer;
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
  while (player->pipeline != NULL && player->out != NULL)
    swfdec_cond_wait (player->cond, player->mutex);
  if (player->pipeline == NULL) {
    g_mutex_unlock (player->mutex);
    return;
  }
  player->out = swfdec_buffer_new_for_data (
      g_memdup (buf->data, buf->size), buf->size);
  player->out_next = FALSE;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
}

static void
swfdec_video_decoder_gst_link (GstElement *src, GstPad *pad, GstElement *sink)
{
  if (!gst_element_link (src, sink)) {
    SWFDEC_ERROR ("no delayed link");
  }
}

SwfdecVideoDecoder *
swfdec_video_decoder_gst_new (SwfdecVideoFormat type)
{
  SwfdecGstVideo *player;
  GstElement *fakesrc, *fakesink, *decoder, *csp;
  GstCaps *caps;

  if (!gst_init_check (NULL, NULL, NULL))
    return NULL;

  switch (type) {
    case SWFDEC_VIDEO_FORMAT_H263:
      caps = gst_caps_from_string ("video/x-flash-video");
      break;
    case SWFDEC_VIDEO_FORMAT_VP6:
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
  csp = gst_element_factory_make ("ffmpegcolorspace", NULL);
  if (csp == NULL) {
    SWFDEC_ERROR ("failed to create colorspace");
    swfdec_video_decoder_gst_free (&player->decoder);
    return NULL;
  }
  gst_bin_add (GST_BIN (player->pipeline), csp);
  g_signal_connect (decoder, "pad-added", 
      G_CALLBACK (swfdec_video_decoder_gst_link), csp);

#if G_BYTE_ORDER == G_BIG_ENDIAN
  caps = gst_caps_from_string ("video/x-raw-rgb, bpp=32, endianness=4321, depth=24, "
      "red_mask=16711680, green_mask=65280, blue_mask=255");
#else
  caps = gst_caps_from_string ("video/x-raw-rgb, bpp=32, endianness=4321, depth=24, "
      "red_mask=65280, green_mask=16711680, blue_mask=-16777216");
#endif
  g_assert (caps);
  if (!gst_element_link_filtered (fakesrc, decoder, player->srccaps) ||
      !gst_element_link_filtered (csp, fakesink, caps)) {
    SWFDEC_ERROR ("linking failed");
    swfdec_video_decoder_gst_free (&player->decoder);
    return NULL;
  }
  gst_caps_unref (caps);
  if (gst_element_set_state (player->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    SWFDEC_ERROR ("failed to change sate");
    swfdec_video_decoder_gst_free (&player->decoder);
    return NULL;
  }

  return &player->decoder;
}

