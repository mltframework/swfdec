/* clip and convert f32 to s16 functions
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
Kernel: clipconv_f32_s16
Description: round source value to nearest integer, clip to output
 range, and convert to s16.

Half integers may be rounded to either nearby integer.
*/

#ifndef _clipconv_f32_s16_h_
#define _clipconv_f32_s16_h_

#include <math.h>

//#include <sl_types.h>
//#include <sl_altivec.h>
#define f32 float
#define s16 short
#define HAVE_IEEE754_H


/* storage class */
#ifndef SL_clipconv_f32_s16_storage
 #ifdef SL_storage
  #define SL_clipconv_f32_s16_storage SL_storage
 #else
  #define SL_clipconv_f32_s16_storage static inline
 #endif
#endif



/*
 * clipconv_f32_s16:
 *
 * Clip source to dest range, round to nearest integer, and
 * convert to s16.
 *
 * Half integers may be rounded to either nearby integer.
 */

/* IMPL clipconv_f32_s16_ref */
SL_clipconv_f32_s16_storage
void clipconv_f32_s16_ref(s16 *dest, f32 *src, int n)
{
	int i;
	f32 x;

	for(i=0;i<n;i++){
		x = src[i];
		if(x<-32768.0)x=-32768.0;
		if(x>32767.0)x=32767.0;
		dest[i] = floor(x + 0.5);
	}
}

#define signbit_s32(x) (((unsigned int)(x))>>31)

#ifdef HAVE_IEEE754_H
/* This function is inaccurate to _i22, but we're only claiming _i30 */
#include <ieee754.h>
/* IMPL clipconv_f32_s16_i30_bits defined(HAVE_IEEE754_H) */
SL_clipconv_f32_s16_storage
void clipconv_f32_s16_i30_bits(s16 *dest, f32 *src, int n)
{
	int i;
	union ieee754_float id;
	s16 d;
	const float offset = 98304.5;

	for(i=0;i<n;i++){
		id.f = offset + src[i];
		d = 0x8000 ^ (id.ieee.mantissa >> 7);
		d += (-32768-d)*signbit_s32(id.ieee.exponent-143);
		d += (32767-d)*signbit_s32(143-id.ieee.exponent);
//printf("%g %g exp=%d mant=%d\n",src[i], id.f, id.ieee.exponent, id.ieee.mantissa>>7);
#if 0
		if(id.ieee.exponent<1039){
			d=-32768;
		}
		if(id.ieee.exponent>1039){
			d=32767;
		}
#endif
		dest[i] = d;
	}
}
#endif

#if defined(__powerpc__)
/* IMPL clipconv_f32_s16_ppcasm defined(__powerpc__) */
SL_clipconv_f32_s16_storage
void clipconv_f32_s16_ppcasm(s16 *dest, f32 *src, int n)
{
	f32 min = -32768.0;
	f32 max = 32767.0;
	f32 ftmp;

	dest--;
	src--;
#if 0
	"\taddic %[src],%[src],-8\n"
	"\taddic %[dest],%[dest],-2\n"
	".loop:\n"
	"\tlfdu %[ftmp0],8(%[src])\n"
	"\tfsub %[ftmp1],%[ftmp0],%[min]\n"
	"\tfsel %[ftmp0],%[ftmp1],%[ftmp0],%[min]\n"
	"\tfsub %[ftmp1],%[ftmp0],%[max]\n"
	"\tfsel %[ftmp0],%[ftmp1],%[max],%[ftmp0]\n"
	"\tfctiw %[ftmp1],%[ftmp0]\n"
	"\taddic. %[n],%[n],-1\n"
	"\tstfd %[ftmp1],0(%[tmp])\n"
	"\tlhz %[tmp2],6(%[tmp])\n"
	"\tsthu %[tmp2],2(%[dest])\n"
	"\tbge .loop\n"
#endif
	__asm__ __volatile__("\n"
		"1:	lfsu	0,4(%1)\n"
		"	fsub	1,0,%3\n"
		"	fsel	0,1,0,%3\n"
		"	fsub	1,0,%4\n"
		"	fsel	0,1,%4,0\n"
		"	fctiw	1,0\n"
		"	addic.	%2,%2,-1\n"
		"	stfdu	1,0(%5)\n"
		"	lhz	11,6(%5)\n"
		"	sthu	11,2(%0)\n"
		"	bge	1b\n"
	: "+b" (dest), "+b" (src), "+r" (n)
	: "f" (min), "f" (max), "b" (&ftmp)
	: "32", "33", "11" );
}

/* IMPL clipconv_f32_s16_a16_altivec defined(__powerpc__) */
SL_clipconv_f32_s16_storage
void clipconv_f32_s16_a16_altivec(s16 *dest, f32 *src, int n)
{
	__asm__ __volatile__("\n"
		"	addic	%2,%2,-8\n"
		"	li	%%r12, 0\n"
		"	li	%%r11, 0\n"
		"1:	lvx	%%v0, %1, %%r12\n"
		"	addi	%%r12, %%r12, 16\n"
		"	vrfin	%%v1, %%v0\n"
		"	lvx	%%v0, %1, %%r12\n"
		"	addi	%%r12, %%r12, 16\n"
		"	vrfin	%%v2, %%v0\n"
		"	vctsxs	%%v3, %%v1, 0\n"
		"	vctsxs	%%v4, %%v2, 0\n"
		"	vpkswss	%%v5, %%v3, %%v4\n"
		"	stvx	%%v5, %0, %%r11\n"
		"	addi	%%r11, %%r11, 16\n"
		"	addic.	%2,%2,-8\n"
		"	bge 1b\n"
	: "+b" (dest), "+b" (src), "+b" (n)
	:
	: "12", "11"
	);

}

#endif

#endif

#ifdef TEST_clipconv_f32_s16
int TEST_clipconv_f32_s16(void)
{
	int i;
	int pass;
	int failures = 0;
	f32 *src;
	s16 *dest_test, *dest_ref;
	struct sl_profile_struct t;

#ifdef ALIGNED16
	src = sl_malloc_f32_a16(N);
	dest_ref = sl_malloc_s16_a16(N);
	dest_test = sl_malloc_s16_a16(N);
#else
	src = sl_malloc_f32(N);
	dest_ref = sl_malloc_s16(N);
	dest_test = sl_malloc_s16(N);
#endif

	sl_profile_init(t);
	srand(20020326);

	printf("I: " sl_stringify(clipconv_f32_s16_FUNC) "\n");

	for(pass=0;pass<N_PASS;pass++){
		for(i=0;i<N;i++){
			src[i] = 2.0*sl_rand_f32_s16();
		}
		clipconv_f32_s16_ref(dest_ref,src,N);
		sl_profile_start(t);
		clipconv_f32_s16_FUNC(dest_test,src,N);	
		sl_profile_stop(t);

		for(i=0;i<N;i++){
			int ok;
			if(dest_test[i] == dest_ref[i])continue;
#ifdef INACCURATE30
			ok = sl_equal_f32_i30(src[i],floor(src[i])+0.5);
#else
			ok = sl_equal_f32(src[i],floor(src[i])+0.5);
#endif
			if(!ok){
				printf("%d %.10g %d %d\n",i,src[i],dest_ref[i],
					dest_test[i]);
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

