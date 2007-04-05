#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <config.h>
#include <liboil/liboil.h>
#include <mad.h>

#include "swfdec_codec.h"
#include "swfdec_debug.h"

typedef struct {
  struct mad_stream	stream;
  struct mad_frame	frame;
  struct mad_synth	synth;
  guint8		data[MAD_BUFFER_MDLEN * 3];
  guint			data_len;
} MadData;

static gpointer
swfdec_codec_mad_init (SwfdecAudioFormat type, gboolean width, SwfdecAudioOut format)
{
  MadData *data = g_new (MadData, 1);

  mad_stream_init (&data->stream);
  mad_frame_init (&data->frame);
  mad_synth_init (&data->synth);
  data->data_len = 0;

  return data;
}

static SwfdecAudioOut
swfdec_codec_mad_get_format (gpointer data)
{
  /* FIXME: improve this */
  return SWFDEC_AUDIO_OUT_STEREO_44100;
}

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

  buffer = swfdec_buffer_new_and_alloc (n_samples * 2 * 2);
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

static SwfdecBuffer *
swfdec_codec_mad_decode (gpointer datap, SwfdecBuffer *buffer)
{
  MadData *data = datap;
  SwfdecBuffer *out;
  SwfdecBufferQueue *queue;
  guint amount = 0, size;

  queue = swfdec_buffer_queue_new ();

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
      if (out) {
	swfdec_buffer_queue_push (queue, out);
      }
    }
    if (data->stream.next_frame == NULL) {
      data->data_len = 0;
    } else {
      data->data_len = data->stream.bufend - data->stream.next_frame;
      memmove (data->data, data->stream.next_frame, data->data_len);
    }
  }
  //g_print ("%u bytes left\n", data->data_len);

  size = swfdec_buffer_queue_get_depth (queue);
  if (size > 0)
    out = swfdec_buffer_queue_pull (queue, size);
  else
    out = NULL;
  swfdec_buffer_queue_unref (queue);

  return out;
}

static SwfdecBuffer *
swfdec_codec_mad_finish (gpointer datap)
{
  MadData *data = datap;
  SwfdecBuffer *empty, *result;

  empty = swfdec_buffer_new ();
  empty->data = g_malloc0 (MAD_BUFFER_GUARD * 3);
  empty->length = MAD_BUFFER_GUARD * 3;
  result = swfdec_codec_mad_decode (data, empty);
  swfdec_buffer_unref (empty);

  mad_synth_finish (&data->synth);
  mad_frame_finish (&data->frame);
  mad_stream_finish (&data->stream);
  g_free (data);

  return result;
}

const SwfdecAudioCodec swfdec_codec_mad = {
  swfdec_codec_mad_init,
  swfdec_codec_mad_get_format,
  swfdec_codec_mad_decode,
  swfdec_codec_mad_finish
};

