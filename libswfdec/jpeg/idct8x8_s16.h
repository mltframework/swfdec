/* forward discrete cosine transform on 8x8 block
 * Copyright (C) 2001,2002  David A. Schleef <ds@schleef.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
Kernel: idct8x8_s16
Description: inverse discrete cosine transform on 8x8 block

XXX
*/

#ifndef _idct8x8_s16_h_
#define _idct8x8_s16_h_

#include <math.h>

#include <sl_types.h>
#include <sl_block8x8.h>

/* storage class */
#ifndef SL_idct8x8_s16_storage
 #ifdef SL_storage
  #define SL_idct8x8_s16_storage SL_storage
 #else
  #define SL_idct8x8_s16_storage static inline
 #endif
#endif



#include <idct8x8_f64.h>
#include <conv8x8_f64_s16.h>
/* IMPL idct8x8_s16_ref */
SL_idct8x8_s16_storage
void idct8x8_s16_ref(s16 *dest, s16 *src, int dstr, int sstr)
{
	f64 s[64], d[64];
	int i,j;

	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			block8x8_f64(s,8*sizeof(f64),i,j) =
				block8x8_s16(src,sstr,i,j);
		}
	}

	idct8x8_f64_ref(d,s,8*sizeof(f64),8*sizeof(f64));
	conv8x8_f64_s16_ref(dest,d,dstr,8*sizeof(f64));
}

/* IMPL idct8x8_s16_fast */
SL_idct8x8_s16_storage
void idct8x8_s16_fast(s16 *dest, s16 *src, int dstr, int sstr)
{
	f64 s[64], d[64];
	int i,j;

	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			block8x8_f64(s,8*sizeof(f64),i,j) =
				block8x8_s16(src,sstr,i,j);
		}
	}

	idct8x8_f64(d,s,8*sizeof(f64),8*sizeof(f64));
	conv8x8_f64_s16(dest,d,dstr,8*sizeof(f64));
}
#endif

#ifdef TEST_idct8x8_s16
int TEST_idct8x8_s16(void)
{
	int i;
	int pass;
	int failures = 0;
	s16 *src, *dest_ref, *dest_test;
	u32 sad;
	u32 sad_sum;
	u32 sad_max;
	struct sl_profile_struct t;

	src = sl_malloc_s16(64);
	dest_ref = sl_malloc_s16(64);
	dest_test = sl_malloc_s16(64);
	
	sl_profile_init(t);
	srand(20020306);

	sad_sum = 0;
	sad_max = 0;

	printf("I: " sl_stringify(idct8x8_s16_FUNC) "\n");

	for(pass=0;pass<N_PASS;pass++){
		for(i=0;i<64;i++)src[i] = sl_rand_s16_l9();

		idct8x8_s16_ref(dest_ref, src, 8*sizeof(s16), 8*sizeof(s16));
		sl_profile_start(t);
		idct8x8_s16_FUNC(dest_test, src, 8*sizeof(s16), 8*sizeof(s16));
		sl_profile_stop(t);

		sad = 0;
		for(i=0;i<64;i++)sad += abs(dest_test[i] - dest_ref[i]);
		if(sad_max<sad)sad_max = sad;
		sad_sum += sad;
		if(sad >= 64){
			block8x8_dump_s16(src, 8*sizeof(s16));
			block8x8_dump_s16(dest_test, 8*sizeof(s16));
			block8x8_dump_s16(dest_ref, 8*sizeof(s16));
			failures++;
		}
	}
	printf("sad average: %g\n",((double)sad_sum)/N_PASS);
	printf("sad max: %d\n",sad_max);

	sl_free(src);
	sl_free(dest_ref);
	sl_free(dest_test);

	if(failures){
		printf("E: %d failures\n",failures);
	}

	sl_profile_print(t);

	return failures;
}
#endif

