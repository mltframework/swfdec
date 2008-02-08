/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include <config.h>
#include <liboil/liboil.h>
#include <mad.h>

#include "swfdec_codec_audio.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

typedef struct {
  SwfdecAudioDecoder	decoder;

  struct mad_stream	stream;
  struct mad_frame	frame;
  struct mad_synth	synth;
  guint8		data[MAD_BUFFER_MDLEN * 3];
  guint			data_len;
  SwfdecBufferQueue *	queue;
} MadData;

static SwfdecBuffer *
convert_synth_to_buffer (MadData *mdata)
{
  SwfdecBuffer *buffer;
  int n_samples;
  short *data;
  int i;
  short c0,c1;
#define MAD_F_TO_S16(x) (CLAMP (x, -MAD_F_ONE, MAD_F_ONE) >> (MAD_F_FRACBITS - 14))

  n_samples = mdata->synth.pcm.length;
  if (n_samples == 0) {
    return NULL;
  }

  switch (mdata->synth.pcm.samplerate) {
    case 11025:
      n_samples *= 4;
      break;
    case 22050:
      n_samples *= 2;
      break;
    case 44100:
      break;
    default:
      SWFDEC_ERROR ("sample rate not handled (%d)",
          mdata->synth.pcm.samplerate);
      return NULL;
  }

  buffer = swfdec_buffer_new (n_samples * 2 * 2);
  data = (gint16 *) buffer->data;

  if (mdata->synth.pcm.samplerate == 11025) {
    if (mdata->synth.pcm.channels == 2) {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        c1 = MAD_F_TO_S16 (mdata->synth.pcm.samples[1][i]);
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
      }
    } else {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16( mdata->synth.pcm.samples[0][i]);
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
      }
    }
  } else if (mdata->synth.pcm.samplerate == 22050) {
    if (mdata->synth.pcm.channels == 2) {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        c1 = MAD_F_TO_S16 (mdata->synth.pcm.samples[1][i]);
        *data++ = c0;
        *data++ = c1;
        *data++ = c0;
        *data++ = c1;
      }
    } else {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
        *data++ = c0;
      }
    }
  } else if (mdata->synth.pcm.samplerate == 44100) {
    if (mdata->synth.pcm.channels == 2) {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        c1 = MAD_F_TO_S16 (mdata->synth.pcm.samples[1][i]);
        *data++ = c0;
        *data++ = c1;
      }
    } else {
      for (i = 0; i < mdata->synth.pcm.length; i++) {
        c0 = MAD_F_TO_S16 (mdata->synth.pcm.samples[0][i]);
        *data++ = c0;
        *data++ = c0;
      }
    }
  } else {
    SWFDEC_ERROR ("sample rate not handled (%d)",
        mdata->synth.pcm.samplerate);
  }
  return buffer;
}

static void
swfdec_audio_decoder_mad_push (SwfdecAudioDecoder *dec, SwfdecBuffer *buffer)
{
  MadData *data = (MadData *) dec;
  SwfdecBuffer *out, *empty = NULL;
  guint amount = 0, size;

  if (buffer == NULL)
    buffer = empty = swfdec_buffer_new0 (MAD_BUFFER_GUARD * 3);

  //write (1, buffer->data, buffer->length);
  //g_print ("buffer %p gave us %u bytes\n", buffer, buffer->length);
  while (amount < buffer->length) {
    size = MIN (buffer->length - amount, MAD_BUFFER_MDLEN * 3 - data->data_len);
    memcpy (&data->data[data->data_len], buffer->data + amount, size);
    //write (1, buffer->data + amount, size);
    amount += size;
    data->data_len += size;
    mad_stream_buffer (&data->stream, data->data, data->data_len);
    while (1) {
      if (mad_frame_decode (&data->frame, &data->stream)) {
	if (data->stream.error == MAD_ERROR_BUFLEN)
	  break;
	if (MAD_RECOVERABLE (data->stream.error)) {
	  SWFDEC_LOG ("recoverable error 0x%04x", data->stream.error);
	  continue;
	}
	SWFDEC_ERROR ("stream error 0x%04x", data->stream.error);
	break;
      }

      mad_synth_frame (&data->synth, &data->frame);
      out = convert_synth_to_buffer (data);
      if (out)
	swfdec_buffer_queue_push (data->queue, out);
    }
    if (data->stream.next_frame == NULL) {
      data->data_len = 0;
    } else {
      data->data_len = data->stream.bufend - data->stream.next_frame;
      memmove (data->data, data->stream.next_frame, data->data_len);
    }
  }
  //g_print ("%u bytes left\n", data->data_len);

  if (empty)
    swfdec_buffer_unref (empty);
}

static void
swfdec_audio_decoder_mad_free (SwfdecAudioDecoder *dec)
{
  MadData *data = (MadData *) dec;

  mad_synth_finish (&data->synth);
  mad_frame_finish (&data->frame);
  mad_stream_finish (&data->stream);
  swfdec_buffer_queue_unref (data->queue);
  g_slice_free (MadData, data);
}

static SwfdecBuffer *
swfdec_audio_decoder_mad_pull (SwfdecAudioDecoder *dec)
{
  return swfdec_buffer_queue_pull_buffer (((MadData *) dec)->queue);
}

gboolean
swfdec_audio_decoder_mad_prepare (guint type, SwfdecAudioFormat format, char **details)
{
  return type == SWFDEC_AUDIO_CODEC_MP3;
}

SwfdecAudioDecoder *
swfdec_audio_decoder_mad_new (guint type, SwfdecAudioFormat format)
{
  MadData *data;
  
  if (type != SWFDEC_AUDIO_CODEC_MP3)
    return NULL;

  data = g_slice_new (MadData);
  data->decoder.format = swfdec_audio_format_new (44100, 2, TRUE);
  data->decoder.push = swfdec_audio_decoder_mad_push;
  data->decoder.pull = swfdec_audio_decoder_mad_pull;
  data->decoder.free = swfdec_audio_decoder_mad_free;
  mad_stream_init (&data->stream);
  mad_frame_init (&data->frame);
  mad_synth_init (&data->synth);
  data->data_len = 0;
  data->queue = swfdec_buffer_queue_new ();

  return &data->decoder;
}

