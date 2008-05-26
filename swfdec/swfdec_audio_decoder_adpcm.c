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

#include "swfdec_audio_decoder_adpcm.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"

G_DEFINE_TYPE (SwfdecAudioDecoderAdpcm, swfdec_audio_decoder_adpcm, SWFDEC_TYPE_AUDIO_DECODER)

static const int indexTable[4][16] = {
  { -1, 2 },
  { -1, -1, 2, 4 },
  { -1, -1, -1, -1, 2, 4, 6, 8 },
  { -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16 }
};

static const int stepSizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static SwfdecBuffer *
swfdec_audio_decoder_adpcm_decode_chunk (SwfdecBits *bits, guint n_bits, 
    guint channels, guint granularity)
{
  SwfdecBuffer *ret;
  guint len, repeat;
  guint i, j, ch;
  guint index[2];
  int pred[2];
  gint16 *out;
  guint delta, sign, sign_mask;
  int diff;
  const int *realIndexTable;
  guint step[2];

  /* for scaling up the audio to 44100kHz */
  repeat = 2 * granularity - channels;

  realIndexTable = indexTable[n_bits - 2];
  for (ch = 0; ch < channels; ch++) {
    /* can't use get_s16 here since that would be aligned */
    pred[ch] = swfdec_bits_getsbits (bits, 16);
    index[ch] = swfdec_bits_getbits (bits, 6);
    if (index[ch] >= G_N_ELEMENTS (stepSizeTable)) {
      SWFDEC_ERROR ("initial index too big: %u, max allowed is %td",
	  index[ch], G_N_ELEMENTS (stepSizeTable) - 1);
      index[ch] = G_N_ELEMENTS (stepSizeTable) - 1;
    }
    step[ch] = stepSizeTable[index[ch]];
  }
  len = swfdec_bits_left (bits) / channels / n_bits;
  len = MIN (len, 4095);
  ret = swfdec_buffer_new ((len + 1) * sizeof (gint16) * granularity * 2);
  out = (gint16 *) (void *) ret->data;
  /* output initial value */
  SWFDEC_LOG ("decoding %u samples", len + 1);
  for (ch = 0; ch < channels; ch++)
    *out++ = pred[ch];
  /* upscale to 44.1kHz */
  for (ch = 0; ch < repeat; ch++) {
    *out = out[-(gssize) channels];
    out++;
  }

  sign_mask = 1 << (n_bits - 1);
  for (i = 0; i < len; i++) {
    for (ch = 0; ch < channels; ch++) {
      /* Step 1 - get the delta value */
      delta = swfdec_bits_getbits (bits, n_bits);
      
      /* Step 2 - Separate sign and magnitude */
      sign = delta & sign_mask;
      delta -= sign;

      /* Step 3 - Find new index value (for later) */
      index[ch] += realIndexTable[delta];
      if ( index[ch] >= G_MAXINT ) index[ch] = 0; /* underflow */
      if ( index[ch] >= G_N_ELEMENTS (stepSizeTable) ) index[ch] = G_N_ELEMENTS (stepSizeTable) - 1;

      /* Step 4 - Compute difference and new predicted value */
      j = n_bits - 1;
      diff = step[ch] >> j;
      do {
	j--;
	if (delta & 1)
	  diff += step[ch] >> j;
	delta >>= 1;
      } while (j > 0 && delta);

      if ( sign )
	pred[ch] -= diff;
      else
	pred[ch] += diff;

      /* Step 5 - clamp output value */
      pred[ch] = CLAMP (pred[ch], -32768, 32767);

      /* Step 6 - Update step value */
      step[ch] = stepSizeTable[index[ch]];

      /* Step 7 - Output value */
      *out++ = pred[ch];
    }

    /* upscale to 44.1kHz */
    for (ch = 0; ch < repeat; ch++) {
      *out = out[-(gssize) channels];
      out++;
    }
  }
  return ret;
}

static void
swfdec_audio_decoder_adpcm_push (SwfdecAudioDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecAudioDecoderAdpcm *adpcm = (SwfdecAudioDecoderAdpcm *) dec;
  guint channels, n_bits, granularity;
  SwfdecBits bits;

  if (buffer == NULL)
    return;

  channels = swfdec_audio_format_get_channels (dec->format);
  granularity = swfdec_audio_format_get_granularity (dec->format);
  swfdec_bits_init (&bits, buffer);
  n_bits = swfdec_bits_getbits (&bits, 2) + 2;
  SWFDEC_DEBUG ("starting decoding: %u channels, %u bits", channels, n_bits);
  /* 22 is minimum required header size */
  while (swfdec_bits_left (&bits) >= 22) {
    buffer = swfdec_audio_decoder_adpcm_decode_chunk (&bits, n_bits, channels, granularity);
    if (buffer)
      swfdec_buffer_queue_push (adpcm->queue, buffer);
  }
}

static SwfdecBuffer *
swfdec_audio_decoder_adpcm_pull (SwfdecAudioDecoder *dec)
{
  SwfdecAudioDecoderAdpcm *adpcm = (SwfdecAudioDecoderAdpcm *) dec;

  return swfdec_buffer_queue_pull_buffer (adpcm->queue);
}

static void
swfdec_audio_decoder_adpcm_dispose (GObject *object)
{
  SwfdecAudioDecoderAdpcm *dec = (SwfdecAudioDecoderAdpcm *) object;

  swfdec_buffer_queue_unref (dec->queue);

  G_OBJECT_CLASS (swfdec_audio_decoder_adpcm_parent_class)->dispose (object);
}

static void
swfdec_audio_decoder_adpcm_class_init (SwfdecAudioDecoderAdpcmClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAudioDecoderClass *decoder_class = SWFDEC_AUDIO_DECODER_CLASS (klass);

  object_class->dispose = swfdec_audio_decoder_adpcm_dispose;

  decoder_class->pull = swfdec_audio_decoder_adpcm_pull;
  decoder_class->push = swfdec_audio_decoder_adpcm_push;
}

static void
swfdec_audio_decoder_adpcm_init (SwfdecAudioDecoderAdpcm *audio_decoder_adpcm)
{
}

