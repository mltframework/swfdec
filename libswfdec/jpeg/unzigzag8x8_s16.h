/* un-zig-zag transform on 8x8 block
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
Kernel: unzigzag8x8_s16
Description: list elements in 8x8 block in zig-zag order

XXX
*/

#ifndef _unzigzag8x8_s16_h_
#define _unzigzag8x8_s16_h_

#include <math.h>


/* storage class */
#ifndef SL_unzigzag8x8_s16_storage
 #ifdef SL_storage
  #define SL_unzigzag8x8_s16_storage SL_storage
 #else
  #define SL_unzigzag8x8_s16_storage static inline
 #endif
#endif

#if 0
static const unsigned char zigzag_order[64] = {
	0,
	8, 1,
	2, 9, 16,
	24, 17, 10, 3, 
	4, 11, 18, 25, 32,
	40, 33, 26, 19, 12, 5, 
	6, 13, 20, 27, 34, 41, 48,
	56, 49, 42, 35, 28, 21, 14, 7,
	15, 22, 29, 36, 43, 50, 57,
	58, 51, 44, 37, 30, 23,
	31, 38, 45, 52, 59,
	60, 53, 46, 39,
	47, 54, 61,
	62, 55,
	63
};
#endif

static const unsigned char unzigzag_order[64] = {
	0,  1,  5,  6,  14, 15, 27, 28,
	2,  4,  7,  13, 16, 26, 29, 42,
	3,  8,  12, 17, 25, 30, 41, 43,
	9,  11, 18, 24, 31, 40, 44, 53,
	10, 19, 23, 32, 39, 45, 52, 54,
	20, 22, 33, 38, 46, 51, 55, 60,
	21, 34, 37, 47, 50, 56, 59, 61,
	35, 36, 48, 49, 57, 58, 62, 63,
};

/* IMPL zigzag8x8_s16_ref */
SL_unzigzag8x8_s16_storage
void unzigzag8x8_s16_ref(short *dest, short *src)
{
	int i;
	unsigned int z;

	for(i=0;i<64;i++){
		z = unzigzag_order[i];
		dest[i] = src[z];
	}
}

#endif

#ifdef TEST_unzigzag8x8_s16
int TEST_unzigzag8x8_s16(void)
{
	int i;
	int pass;
	int failures = 0;
	s16 *src, *dest_ref, *dest_test;
	struct sl_profile_struct t;

#ifdef ALIGNED16
	src = sl_malloc_s16_a16(64);
	dest_ref = sl_malloc_s16_a16(64);
	dest_test = sl_malloc_s16_a16(64);
#else
	src = sl_malloc_s16(64);
	dest_ref = sl_malloc_s16(64);
	dest_test = sl_malloc_s16(64);
#endif
	
	sl_profile_init(t);
	srand(20020326);

	printf("I: " sl_stringify(unzigzag8x8_s16_FUNC) "\n");

	for(pass=0;pass<N_PASS;pass++){
		for(i=0;i<64;i++)src[i] = sl_rand_s16();

		unzigzag8x8_s16_ref(dest_test, src, 16);
		sl_profile_start(t);
		unzigzag8x8_s16_FUNC(dest_ref, src, 16);
		sl_profile_stop(t);

		for(i=0;i<64;i++){
			if(dest_test[i]!=dest_ref[i]){
				failures++;
			}
		}
	}

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


