/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_codec.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"

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
    
static gpointer
swfdec_codec_adpcm_init (gboolean width, guint channels, guint rate_multiplier)
{
  guint ret = rate_multiplier | (channels << 16);
  return GUINT_TO_POINTER (ret);
}

static SwfdecBuffer *
swfdec_codec_adpcm_decode_chunk (SwfdecBits *bits, guint n_bits, 
    guint multiplier, guint channels)
{
  SwfdecBuffer *ret;
  guint len;
  guint i, j, ch;
  guint index[2];
  int pred[2];
  gint16 *out;
  guint delta, sign, sign_mask;
  int diff;
  const int *realIndexTable;
  guint step[2];

  realIndexTable = indexTable[n_bits - 2];
  for (ch = 0; ch < channels; ch++) {
    /* can't use get_s16 here since that would be aligned */
    pred[ch] = swfdec_bits_getsbits (bits, 16);
    index[ch] = swfdec_bits_getbits (bits, 6);
    if (index[ch] >= G_N_ELEMENTS (stepSizeTable)) {
      SWFDEC_ERROR ("initial index too big: %u, max allowed is %u",
	  index[ch], G_N_ELEMENTS (stepSizeTable) - 1);
      index[ch] = G_N_ELEMENTS (stepSizeTable) - 1;
    }
    step[ch] = stepSizeTable[index[ch]];
  }
  len = swfdec_bits_left (bits) / channels / n_bits;
  len = MIN (len, 4095);
  ret = swfdec_buffer_new_and_alloc ((len + 1) * sizeof (gint16) * 2 * multiplier);
  if (channels == 1)
    multiplier *= 2;
  out = (gint16 *) ret->data;
  /* output initial value */
  SWFDEC_LOG ("decoding %u samples\n", len + 1);
  for (ch = 0; ch < channels; ch++)
    for (j = 0; j < multiplier; j++)
      *out++ = pred[ch];
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
      for (j = 0; j < multiplier; j++)
	*out++ = pred[ch];
    }
  }
  return ret;
}

static SwfdecBuffer *
swfdec_codec_adpcm_decode (gpointer data, SwfdecBuffer *buffer)
{
  guint multiplier = GPOINTER_TO_UINT (data);
  guint channels, n_bits;
  SwfdecBits bits;
  SwfdecBufferQueue *queue = swfdec_buffer_queue_new ();

  channels = multiplier >> 16;
  multiplier &= 0xFFFF;
  swfdec_bits_init (&bits, buffer);
  n_bits = swfdec_bits_getbits (&bits, 2) + 2;
  SWFDEC_DEBUG ("starting decoding: %u channels, %uHz, %u bits\n", channels,
      44100 / multiplier, n_bits);
  /* 22 is minimum required header size */
  while (swfdec_bits_left (&bits) >= 22) {
    buffer = swfdec_codec_adpcm_decode_chunk (&bits, n_bits, multiplier, channels);
    if (buffer)
      swfdec_buffer_queue_push (queue, buffer);
  }
  if (swfdec_buffer_queue_get_depth (queue))
    buffer = swfdec_buffer_queue_pull (queue,
	swfdec_buffer_queue_get_depth (queue));
  else
    buffer = NULL;
  swfdec_buffer_queue_free (queue);
  return buffer;
}

static void
swfdec_codec_adpcm_finish (gpointer data)
{
}

const SwfdecCodec swfdec_codec_adpcm = {
  swfdec_codec_adpcm_init,
  swfdec_codec_adpcm_decode,
  swfdec_codec_adpcm_finish
};

#if 0
void
adpcm_decoder(indata, outdata, len, state)
    char indata[];
    short outdata[];
    int len;
    struct adpcm_state *state;
{
    signed char *inp;		/* Input buffer pointer */
    short *outp;		/* output buffer pointer */
    int sign;			/* Current adpcm sign bit */
    int delta;			/* Current adpcm output value */
    int step;			/* Stepsize */
    int valpred;		/* Predicted value */
    int vpdiff;			/* Current change to valpred */
    int index;			/* Current step change index */
    int inputbuffer;		/* place to keep next 4-bit value */
    int bufferstep;		/* toggle between inputbuffer/input */

    outp = outdata;
    inp = (signed char *)indata;

    valpred = state->valprev;
    index = state->index;
    step = stepSizeTable[index];

    bufferstep = 0;
    
    for ( ; len > 0 ; len-- ) {
	
	/* Step 1 - get the delta value */
	if ( bufferstep ) {
	    delta = inputbuffer & 0xf;
	} else {
	    inputbuffer = *inp++;
	    delta = (inputbuffer >> 4) & 0xf;
	}
	bufferstep = !bufferstep;

	/* Step 2 - Find new index value (for later) */
	index += indexTable[delta];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;

	/* Step 3 - Separate sign and magnitude */
	sign = delta & 8;
	delta = delta & 7;

	/* Step 4 - Compute difference and new predicted value */
	/*
	** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
	** in adpcm_coder.
	*/
	vpdiff = step >> 3;
	if ( delta & 4 ) vpdiff += step;
	if ( delta & 2 ) vpdiff += step>>1;
	if ( delta & 1 ) vpdiff += step>>2;

	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;

	/* Step 5 - clamp output value */
	if ( valpred > 32767 )
	  valpred = 32767;
	else if ( valpred < -32768 )
	  valpred = -32768;

	/* Step 6 - Update step value */
	step = stepSizeTable[index];

	/* Step 7 - Output value */
	*outp++ = valpred;
    }

    state->valprev = valpred;
    state->index = index;
}
#endif
