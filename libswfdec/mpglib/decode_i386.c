/* 
 * Mpeg Layer-1,2,3 audio decoder 
 * ------------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README'
 *
 * slighlty optimized for machines without autoincrement/decrement.
 * The performance is highly compiler dependend. Maybe
 * the decode.c version for 'normal' processor may be faster
 * even for Intel processors.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <mpglib_internal.h>
#include <clipconv_f32_s16.h>

extern MpglibDecoder *gmp;

#define clipconv_f32_s16 clipconv_f32_s16_ref

static inline void altmultsum16_f32_ref(float *dest, float *src1, float *src2);
static inline void multsum_str_f32_ref(float *dest, float *src1, float *src2,
	int sstr1, int sstr2, int n);
static inline void memcpy2_str(void *dest, void *src, int dstr, int sstr, int n);

int synth_1to1_mono(real *bandPtr,unsigned char *samples,int *pnt)
{
  short samples_tmp[64];
  short *tmp1 = samples_tmp;
  int ret;
  int pnt1 = 0;

  ret = synth_1to1(bandPtr,0,(unsigned char *) samples_tmp,&pnt1);
  samples += *pnt;

  memcpy2_str(samples, tmp1, 4, 4, 32);
#if 0
  for(i=0;i<32;i++) {
    *( (short *) samples) = *tmp1;
    samples += 2;
    tmp1 += 2;
  }
#endif
  *pnt += 64;

  return ret;
}


int synth_1to1(real *bandPtr,int channel,unsigned char *out,int *pnt)
{
  //static const int step = 2;
  int bo;
  short *samples = (short *) (out + *pnt);
  float fsamples[32];
  real *b0,(*buf)[0x110];
  int clip = 0; 
  int bo1;

  bo = gmp->synth_bo;

  if(!channel) {
    bo--;
    bo &= 0xf;
    buf = gmp->synth_buffs[0];
  }
  else {
    samples++;
    buf = gmp->synth_buffs[1];
  }

  if(bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
    dct64(buf[1]+((bo+1)&0xf),buf[0]+bo,bandPtr);
  }
  else {
    b0 = buf[1];
    bo1 = bo+1;
    dct64(buf[0]+bo,buf[1]+bo+1,bandPtr);
  }

  gmp->synth_bo = bo;
  
  {
    int i;
    real *window = decwin + 16 - bo1;

    for (i=0; i<16; i++)
    {
#if 1
      altmultsum16_f32_ref(fsamples+i, window, b0);
#else

      sum  = window[0x0] * b0[0x0];
      sum -= window[0x1] * b0[0x1];
      sum += window[0x2] * b0[0x2];
      sum -= window[0x3] * b0[0x3];
      sum += window[0x4] * b0[0x4];
      sum -= window[0x5] * b0[0x5];
      sum += window[0x6] * b0[0x6];
      sum -= window[0x7] * b0[0x7];
      sum += window[0x8] * b0[0x8];
      sum -= window[0x9] * b0[0x9];
      sum += window[0xA] * b0[0xA];
      sum -= window[0xB] * b0[0xB];
      sum += window[0xC] * b0[0xC];
      sum -= window[0xD] * b0[0xD];
      sum += window[0xE] * b0[0xE];
      sum -= window[0xF] * b0[0xF];
      fsamples[i] = sum;
#endif

      b0+=0x10;
      window+=0x20;
    }

    for (i=16; i<17; i++)
    {
#if 0
      multsum_str_f32_ref(fsamples + i, window, b0, 2*sizeof(float), 2*sizeof(float), 8);
#else
      real sum;
      sum  = window[0x0] * b0[0x0];
      sum += window[0x2] * b0[0x2];
      sum += window[0x4] * b0[0x4];
      sum += window[0x6] * b0[0x6];
      sum += window[0x8] * b0[0x8];
      sum += window[0xA] * b0[0xA];
      sum += window[0xC] * b0[0xC];
      sum += window[0xE] * b0[0xE];
      fsamples[i] = sum;
#endif

      b0-=0x10;
      window-=0x20;
    }
    window += bo1<<1;

    for (i=17; i<32; i++)
    {
#if 1
      multsum_str_f32_ref(fsamples + i, window - 1, b0, -sizeof(float), sizeof(float), 15);
      fsamples[i] += window[-0x0] * b0[0xF];
      fsamples[i] = - fsamples[i];
#else
      real sum;
      sum = -window[-0x1] * b0[0x0];
      sum -= window[-0x2] * b0[0x1];
      sum -= window[-0x3] * b0[0x2];
      sum -= window[-0x4] * b0[0x3];
      sum -= window[-0x5] * b0[0x4];
      sum -= window[-0x6] * b0[0x5];
      sum -= window[-0x7] * b0[0x6];
      sum -= window[-0x8] * b0[0x7];
      sum -= window[-0x9] * b0[0x8];
      sum -= window[-0xA] * b0[0x9];
      sum -= window[-0xB] * b0[0xA];
      sum -= window[-0xC] * b0[0xB];
      sum -= window[-0xD] * b0[0xC];
      sum -= window[-0xE] * b0[0xD];
      sum -= window[-0xF] * b0[0xE];
      sum -= window[-0x0] * b0[0xF];

      fsamples[i] = sum;
#endif

      b0-=0x10;
      window-=0x20;
    }
  }

#if 0
  clipconv_str_f32_s16(samples, fsamples, 2*sizeof(short), sizeof(float), 32);
#else
  {
    short ssamples[32];
    int i;

    clipconv_f32_s16(ssamples, fsamples, 32);

    for(i=0;i<32;i++){
      samples[i*2] = ssamples[i];
    }
  }
#endif

  *pnt += 128;

  return clip;
}



static inline void altmultsum16_f32_ref(float *dest, float *src1, float *src2)
{
	float sum = 0;
	int i;

	for(i=0;i<16;i+=2){
		sum += src1[i] * src2[i];
		sum -= src1[i+1] * src2[i+1];
	}
	*dest = sum;
}

static inline void multsum_str_f32_ref(float *dest, float *src1, float *src2,
	int sstr1, int sstr2, int n)
{
	int i;
	float sum = 0;
	void *s1 = src1, *s2 = src2;

	for(i=0;i<n;i++){
		sum += *(float *)(s1+sstr1*i) * *(float *)(s2 + sstr2*i);
	}

	*dest = sum;
}

static inline void memcpy2_str(void *dest, void *src, int dstr, int sstr, int n)
{
	int i;

	for(i=0;i<n;i++){
		*((short *)dest) = *((short *)src);
		dest += dstr;
		src += sstr;
	}
}

