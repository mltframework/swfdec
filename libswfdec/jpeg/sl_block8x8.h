/* block8x8 functions
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

#ifndef _SL_BLOCK8X8_H
#define _SL_BLOCK8X8_H

#include <math.h>

#include <sl_types.h>


#define block8x8_f64(ptr,str,major,minor) \
	(*(f64 *)(((void *)(ptr)) + (major) * (str) + (minor) * sizeof(f64)))

#define block8x8_f32(ptr,str,major,minor) \
	(*(f32 *)(((void *)(ptr)) + (major) * (str) + (minor) * sizeof(f32)))

#define block8x8_s32(ptr,str,major,minor) \
	(*(s32 *)(((void *)(ptr)) + (major) * (str) + (minor) * sizeof(s32)))

#define block8x8_u32(ptr,str,major,minor) \
	(*(u32 *)(((void *)(ptr)) + (major) * (str) + (minor) * sizeof(u32)))

#define block8x8_s16(ptr,str,major,minor) \
	(*(s16 *)(((void *)(ptr)) + (major) * (str) + (minor) * sizeof(s16)))

#define block8x8_u16(ptr,str,major,minor) \
	(*(u16 *)(((void *)(ptr)) + (major) * (str) + (minor) * sizeof(u16)))

#define block8x8_s8(ptr,str,major,minor) \
	(*(s8 *)(((void *)(ptr)) + (major) * (str) + (minor) * sizeof(s8)))

#define block8x8_u8(ptr,str,major,minor) \
	(*(u8 *)(((void *)(ptr)) + (major) * (str) + (minor) * sizeof(u8)))


static inline void block8x8_dump_s16(s16 *p, int str)
{
	int i,j;

	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			printf("%6d ",block8x8_s16(p,str,i,j));
		}
		printf("\n");
	}
	printf("\n");
}

static inline void block8x8_dump_f64(f64 *p, int str)
{
	int i,j;

	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			printf("%9.4f ",block8x8_f64(p,str,i,j));
		}
		printf("\n");
	}
	printf("\n");
}

static inline void block8x8_dumpdiff_s16(s16 *p1, s16 *p2, int str)
{
	int i,j;

	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			printf("%6d ",
				block8x8_s16(p1,str,i,j) -
				block8x8_s16(p2,str,i,j));
		}
		printf("\n");
	}
	printf("\n");
}

static inline void block8x8_dumpratio_s16(s16 *p1, s16 *p2, int str)
{
	int i,j;

	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			printf("%9.4g ",
				(double)block8x8_s16(p1,str,i,j)/
				(double)block8x8_s16(p2,str,i,j));
		}
		printf("\n");
	}
	printf("\n");
}

#endif

