/* inverse discrete cosine transform on length 8 array
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
Kernel: idct8_f64
Description: inverse discrete cosine transform on length 8 array

XXX
*/

#ifndef _idct8_f64_h_
#define _idct8_f64_h_

#include <math.h>

#include <sl_types.h>

/* storage class */
#ifndef SL_idct8_f64_storage
 #ifdef SL_storage
  #define SL_idct8_f64_storage SL_storage
 #else
  #define SL_idct8_f64_storage static inline
 #endif
#endif


/* IMPL idct8_f64_ref */
SL_idct8_f64_storage
void idct8_f64_ref(f64 *dest, f64 *src, int dstr, int sstr)
{
	static f64 idct_coeff[8][8];
	static int idct_coeff_init = 0;
	int i,j;
	f64 x;

	if(!idct_coeff_init){
		f64 scale;

		for(i=0;i<8;i++){
			scale = (i==0) ? sqrt(0.125) : 0.5;
			for(j=0;j<8;j++){
				idct_coeff[j][i] = scale *
					cos((M_PI/8)*i*(j+0.5));
			}
		}
		idct_coeff_init = 1;
	}

	for(i=0;i<8;i++){
		x = 0;
		for(j=0;j<8;j++){
			x += idct_coeff[i][j] *
				*(f64 *)(((void *)src)+sstr*j);
		}
		*(f64 *)(((void *)dest)+dstr*i) = x;
	}
}


#define get_f64(ptr,str,i) (*(f64 *)((void *)(ptr) + (str)*(i)))
#define put_f64(ptr,str,i) (*(f64 *)((void *)(ptr) + (str)*(i)))


/* IMPL idct8_f64_fast */
SL_idct8_f64_storage
void idct8_f64_fast(f64 *dest, f64 *src, int dstr, int sstr)
{
	f64 s07, s16, s25, s34;
	f64 d07, d16, d25, d34;
	f64 ss07s34, ss16s25;
	f64 ds07s34, ds16s25;

	ss07s34 = C0_7071*(get_f64(src,sstr,0) + get_f64(src,sstr,4));
	ss16s25 = C0_7071*(get_f64(src,sstr,0) - get_f64(src,sstr,4));

	ds07s34 = C0_9239*get_f64(src,sstr,2) + C0_3827*get_f64(src,sstr,6);
	ds16s25 = C0_3827*get_f64(src,sstr,2) - C0_9239*get_f64(src,sstr,6);

	s07 = ss07s34 + ds07s34;
	s34 = ss07s34 - ds07s34;

	s16 = ss16s25 + ds16s25;
	s25 = ss16s25 - ds16s25;

	d07 = C0_9808*get_f64(src,sstr,1) + C0_8315*get_f64(src,sstr,3)
		+ C0_5556*get_f64(src,sstr,5) + C0_1951*get_f64(src,sstr,7);
	d16 = C0_8315*get_f64(src,sstr,1) - C0_1951*get_f64(src,sstr,3)
		- C0_9808*get_f64(src,sstr,5) - C0_5556*get_f64(src,sstr,7);
	d25 = C0_5556*get_f64(src,sstr,1) - C0_9808*get_f64(src,sstr,3)
		+ C0_1951*get_f64(src,sstr,5) + C0_8315*get_f64(src,sstr,7);
	d34 = C0_1951*get_f64(src,sstr,1) - C0_5556*get_f64(src,sstr,3)
		+ C0_8315*get_f64(src,sstr,5) - C0_9808*get_f64(src,sstr,7);

	put_f64(dest,dstr,0) = 0.5 * (s07 + d07);
	put_f64(dest,dstr,1) = 0.5 * (s16 + d16);
	put_f64(dest,dstr,2) = 0.5 * (s25 + d25);
	put_f64(dest,dstr,3) = 0.5 * (s34 + d34);
	put_f64(dest,dstr,4) = 0.5 * (s34 - d34);
	put_f64(dest,dstr,5) = 0.5 * (s25 - d25);
	put_f64(dest,dstr,6) = 0.5 * (s16 - d16);
	put_f64(dest,dstr,7) = 0.5 * (s07 - d07);

}

#endif


#ifdef TEST_idct8_f64
int TEST_idct8_f64(void)
{
	int i;
	int pass;
	int failures = 0;
	f64 *src, *dest_ref, *dest_test;
	f64 sad;
	f64 sad_sum;
	f64 sad_max;
	struct sl_profile_struct t;

	src = sl_malloc_f64(8);
	dest_ref = sl_malloc_f64(8);
	dest_test = sl_malloc_f64(8);
	
	sl_profile_init(t);
	srand(20020306);

	sad_sum = 0;
	sad_max = 0;

	printf("I: " sl_stringify(idct8_f64_FUNC) "\n");

	for(pass=0;pass<N_PASS;pass++){
		for(i=0;i<8;i++)src[i] = sl_rand_f64_0_1();

		//block8_dump(src);

		idct8_f64_ref(dest_test, src, 8, 8);
		//block8_dump(dest_test);

		sl_profile_start(t);
		idct8_f64_FUNC(dest_ref, src, 8, 8);
		sl_profile_stop(t);
		//block8_dump(dest_ref);

		sad = 0;
		for(i=0;i<8;i++)sad += fabs(dest_test[i] - dest_ref[i]);
		if(sad_max<sad)sad_max = sad;
		sad_sum += sad;
		if(sad >= 1.0){
			failures++;
		}
	}
	printf("sad average: %g\n",sad_sum/N_PASS);
	printf("sad max: %g\n",sad_max);

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

