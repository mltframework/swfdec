/* convert block8x8 f64 to s16 functions
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
Kernel: conv8x8_f64_s16
Description: round source value to nearest integer and convert to s16.

Result is undefined if src<-32768 or src>32767.
Half integers may be rounded to either nearby integer.
*/

#ifndef _conv8x8_f64_s16_h_
#define _conv8x8_f64_s16_h_

#include <math.h>
#include <sl_types.h>

#include <sl_block8x8.h>


/* storage class */
#ifndef SL_conv8x8_f64_s16_storage
 #ifdef SL_storage
  #define SL_conv8x8_f64_s16_storage SL_storage
 #else
  #define SL_conv8x8_f64_s16_storage static inline
 #endif
#endif

/* IMPL conv8x8_f64_s16_ref */
SL_conv8x8_f64_s16_storage
void conv8x8_f64_s16_ref(s16 *dest, f64 *src, int dstr, int sstr)
{
	int i,j;

	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			block8x8_s16(dest,dstr,i,j) =
				floor(block8x8_f64(src,sstr,i,j)+0.5);
		}
	}
}

#endif

