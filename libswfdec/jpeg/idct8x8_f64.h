/* inverse discrete cosine transform on 8x8 block
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
Kernel: idct8x8_f64
Description: inverse discrete cosine transform on 8x8 block

XXX
*/

#ifndef _idct8x8_f64_h_
#define _idct8x8_f64_h_

#include <math.h>

#include <sl_types.h>
#include <sl_block8x8.h>

/* storage class */
#ifndef SL_idct8x8_f64_storage
 #ifdef SL_storage
  #define SL_idct8x8_f64_storage SL_storage
 #else
  #define SL_idct8x8_f64_storage static inline
 #endif
#endif


/* IMPL idct8x8_f64_ref */
SL_idct8x8_f64_storage
void idct8x8_f64(f64 *dest, f64 *src, int dstr, int sstr)
{
	static f64 idct_coeff[8][8];
	static int idct_coeff_init = 0;
	int i,j,k,l;
	f64 tmp1,tmp2;

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
		for(j=0;j<8;j++){
			tmp1 = 0;
			for(k=0;k<8;k++){
				tmp2 = 0;
				for(l=0;l<8;l++){
					tmp2 += idct_coeff[j][l] *
						block8x8_f64(src,sstr,k,l);
				}
				tmp1 += idct_coeff[i][k] * tmp2;
			}
			block8x8_f64(dest,dstr,i,j) = tmp1;
		}
	}
}

#endif

