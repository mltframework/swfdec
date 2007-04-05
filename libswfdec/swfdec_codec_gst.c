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

#include "swfdec_codec.h"
#include "swfdec_debug.h"

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
  GMutex *	  	mutex;		/* mutex that blocks everything below */
  GCond *		cond;		/* cond used to signal when stuff below changes */
  volatile int		refcount;	/* refcount (d'oh) */

  GstElement *		pipeline;	/* pipeline that is playing or NULL when done */
  SwfdecBuffer *	in;		/* next input buffer or NULL */
  SwfdecBuffer *	out;		/* available output or NULL */
  int			width;		/* width of last output buffer */
  int			height;		/* height of last output buffer */
  GstCaps *		srccaps;	/* caps to set on buffers */
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
swfdec_codec_gst_video_finish (gpointer codec_data)
{
  SwfdecGstVideo *player = codec_data;
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

static void
swfdec_codec_gst_fakesrc_handoff (GstElement *fakesrc, GstBuffer *buf, 
    GstPad *pad, SwfdecGstVideo *player)
{
  g_mutex_lock (player->mutex);
  while (player->pipeline != NULL && player->in == NULL)
    swfdec_cond_wait (player->cond, player->mutex);
  if (player->pipeline == NULL) {
    g_mutex_unlock (player->mutex);
    return;
  }
  buf->data = g_memdup (player->in->data, player->in->length);
  buf->size = player->in->length;
  gst_buffer_set_caps (buf, player->srccaps);
  player->in = NULL;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
}

static void
swfdec_codec_gst_fakesink_handoff (GstElement *fakesrc, GstBuffer *buf, 
    GstPad *pad, SwfdecGstVideo *player)
{
  GstCaps *caps;

  g_mutex_lock (player->mutex);
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
  if (player->pipeline == NULL)
    return;
  player->out = swfdec_buffer_new ();
  player->out->data = g_memdup (buf->data, buf->size);
  player->out->length = buf->size;
  g_cond_signal (player->cond);
  g_mutex_unlock (player->mutex);
}

static void
do_the_link (GstElement *src, GstPad *pad, GstElement *sink)
{
  if (!gst_element_link (src, sink)) {
    SWFDEC_ERROR ("no delayed link");
  }
}

static gpointer
swfdec_codec_gst_h263_init (void)
{
  SwfdecGstVideo *player;
  GstElement *fakesrc, *fakesink, *decoder, *csp;
  GstCaps *sinkcaps;

  if (!gst_init_check (NULL, NULL, NULL))
    return FALSE;

  player = g_slice_new0 (SwfdecGstVideo);
  player->pipeline = gst_pipeline_new ("pipeline");
  player->refcount = 1;
  g_assert (player->pipeline);
  player->mutex = g_mutex_new ();
  player->cond = g_cond_new ();
  player->srccaps = gst_caps_from_string ("video/x-flash-video");
  g_assert (player->srccaps);
  fakesrc = gst_element_factory_make ("fakesrc", NULL);
  if (fakesrc == NULL) {
    SWFDEC_ERROR ("failed to create fakesrc");
    swfdec_codec_gst_video_finish (player);
    return NULL;
  }
  g_object_set (fakesrc, "signal-handoffs", TRUE, 
      "can-activate-pull", FALSE, NULL);
  g_signal_connect (fakesrc, "handoff", 
      G_CALLBACK (swfdec_codec_gst_fakesrc_handoff), player);
  g_atomic_int_inc (&player->refcount);
  g_object_weak_ref (G_OBJECT (fakesrc), swfdec_gst_video_unref, player);
  gst_bin_add (GST_BIN (player->pipeline), fakesrc);
  fakesink = gst_element_factory_make ("fakesink", NULL);
  if (fakesink == NULL) {
    SWFDEC_ERROR ("failed to create fakesink");
    swfdec_codec_gst_video_finish (player);
    return NULL;
  }
  g_object_set (fakesink, "signal-handoffs", TRUE, NULL);
  g_signal_connect (fakesink, "handoff", 
      G_CALLBACK (swfdec_codec_gst_fakesink_handoff), player);
  g_atomic_int_inc (&player->refcount);
  g_object_weak_ref (G_OBJECT (fakesink), swfdec_gst_video_unref, player);
  gst_bin_add (GST_BIN (player->pipeline), fakesink);
  decoder = gst_element_factory_make ("decodebin", NULL);
  if (decoder == NULL) {
    SWFDEC_ERROR ("failed to create decoder");
    swfdec_codec_gst_video_finish (player);
    return NULL;
  }
  gst_bin_add (GST_BIN (player->pipeline), decoder);
  csp = gst_element_factory_make ("ffmpegcolorspace", NULL);
  if (csp == NULL) {
    SWFDEC_ERROR ("failed to create colorspace");
    swfdec_codec_gst_video_finish (player);
    return NULL;
  }
  gst_bin_add (GST_BIN (player->pipeline), csp);
  g_signal_connect (decoder, "pad-added", G_CALLBACK (do_the_link), csp);

#if G_BYTE_ORDER == G_BIG_ENDIAN
  sinkcaps = gst_caps_from_string ("video/x-raw-rgb, bpp=32, endianness=4321, depth=24, "
      "red_mask=16711680, green_mask=65280, blue_mask=255");
#else
  sinkcaps = gst_caps_from_string ("video/x-raw-rgb, bpp=32, endianness=4321, depth=24, "
      "red_mask=65280, green_mask=16711680, blue_mask=-16777216");
#endif
  g_assert (sinkcaps);
  if (!gst_element_link_filtered (fakesrc, decoder, player->srccaps) ||
#if 0
      !gst_element_link (decoder, csp) ||
#endif
      !gst_element_link_filtered (csp, fakesink, sinkcaps)) {
    SWFDEC_ERROR ("linking failed");
    swfdec_codec_gst_video_finish (player);
    return NULL;
  }
  gst_caps_unref (sinkcaps);
  if (gst_element_set_state (player->pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    SWFDEC_ERROR ("failed to change sate");
    swfdec_codec_gst_video_finish (player);
    return NULL;
  }

  return player;
}

static gboolean
swfdec_codec_gst_video_get_size (gpointer codec_data,
    guint *width, guint *height)
{
  SwfdecGstVideo *player = codec_data;

  g_mutex_lock (player->mutex);
  if (player->width == 0 || player->height == 0) {
    g_mutex_unlock (player->mutex);
    return FALSE;
  }
  *width = player->width;
  *height = player->height;
  g_mutex_unlock (player->mutex);
  return TRUE;
}

SwfdecBuffer *
swfdec_codec_gst_video_decode (gpointer codec_data, SwfdecBuffer *buffer)
{
  SwfdecGstVideo *player = codec_data;

  g_mutex_lock (player->mutex);
  g_assert (player->in == NULL);
  player->in = buffer;
  g_cond_signal (player->cond);
  while (player->out == NULL) {
    swfdec_cond_wait (player->cond, player->mutex);
  }
  buffer = player->out;
  player->out = NULL;
  g_mutex_unlock (player->mutex);
  return buffer;
}

const SwfdecVideoCodec swfdec_codec_gst_h263 = {
  swfdec_codec_gst_h263_init,
  swfdec_codec_gst_video_get_size,
  swfdec_codec_gst_video_decode,
  swfdec_codec_gst_video_finish
};

