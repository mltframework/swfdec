
#ifndef _SL_TYPES_H
#define _SL_TYPES_H

//#include <sl_profile.h>
//#include <sl_palette.h>

#include <malloc.h>

#define HAVE_IEEE754_H

typedef unsigned int sl_len;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;


/* Generic unrolling */

#define SL_UNROLL2( statement, n ) \
	if(n&1){ \
		statement; \
		n--; \
	} \
	while(n>0){ \
		statement; \
		n--; \
		statement; \
		n--; \
	}

#define SL_UNROLL4( statement, n ) \
	if(n&1){ \
		statement; \
		n--; \
	} \
	if(n&2){ \
		statement; \
		n--; \
		statement; \
		n--; \
	} \
	while(n>0){ \
		statement; \
		n--; \
		statement; \
		n--; \
		statement; \
		n--; \
		statement; \
		n--; \
	}

/* preprocessor magic */

#define sl_stringify(x) sl_stringify2(x)
#define sl_stringify2(x) #x

/* memory allocation */

#define sl_malloc(x) malloc(x)
#define sl_free(x) free(x)
#define sl_malloc_a16(x) memalign(16,x)
#if 0
static inline void *sl_malloc_a16(size_t len)
{
	void *p;
	posix_memalign(&p,16,len);
	return p;
}
#endif

#define sl_malloc_f64(x) ((f64 *)sl_malloc(x*sizeof(f64)))
#define sl_malloc_f32(x) ((f32 *)sl_malloc(x*sizeof(f32)))
#define sl_malloc_s32(x) ((s32 *)sl_malloc(x*sizeof(s32)))
#define sl_malloc_u32(x) ((u32 *)sl_malloc(x*sizeof(u32)))
#define sl_malloc_s16(x) ((s16 *)sl_malloc(x*sizeof(s16)))
#define sl_malloc_u16(x) ((u16 *)sl_malloc(x*sizeof(u16)))
#define sl_malloc_s8(x) ((s8 *)sl_malloc(x*sizeof(s8)))
#define sl_malloc_u8(x) ((u8 *)sl_malloc(x*sizeof(u8)))

#define sl_malloc_f64_a16(x) ((f64 *)sl_malloc_a16(x*sizeof(f64)))
#define sl_malloc_f32_a16(x) ((f32 *)sl_malloc_a16(x*sizeof(f32)))
#define sl_malloc_s32_a16(x) ((s32 *)sl_malloc_a16(x*sizeof(s32)))
#define sl_malloc_u32_a16(x) ((u32 *)sl_malloc_a16(x*sizeof(u32)))
#define sl_malloc_s16_a16(x) ((s16 *)sl_malloc_a16(x*sizeof(s16)))
#define sl_malloc_u16_a16(x) ((u16 *)sl_malloc_a16(x*sizeof(u16)))
#define sl_malloc_s8_a16(x) ((s8 *)sl_malloc_a16(x*sizeof(s8)))
#define sl_malloc_u8_a16(x) ((u8 *)sl_malloc_a16(x*sizeof(u8)))

/* random number generation */

#define sl_rand_s32() ((rand()&0xffff)<<16 | (rand()&0xffff))
#define sl_rand_s32_l31() (((s32)rand())-0x40000000)

#define sl_rand_s16() ((s16)(rand()&0xffff))
#define sl_rand_s16_l15() (sl_rand_s16()>>1)
#define sl_rand_s16_l9() (sl_rand_s16()>>7)
#define sl_rand_s16_l8() (sl_rand_s16()>>8)
#define sl_rand_s16_l4() (sl_rand_s16()>>12)

#define sl_rand_s8() ((s8)(rand()&0xffff))

#define sl_rand_u32() ((rand()&0xffff)<<16 | (rand()&0xffff))
#define sl_rand_u32_l31() ((u32)rand())

#define sl_rand_u16() ((u16)(rand()&0xffff))

#define sl_rand_u8() ((u8)(rand()&0xffff))


#define sl_rand_f64_0_1() (((rand()/(RAND_MAX+1.0))+rand())/(RAND_MAX+1.0))

#define sl_rand_f64_s32() (sl_rand_f64_0_1()*4294967296.0-2147483648.0)
#define sl_rand_f64_s16() (sl_rand_f64_0_1()*65536.0-32768.0)
#define sl_rand_f64_s8() (sl_rand_f64_0_1()*256.0-128.0)

#define sl_rand_f64_u32() (sl_rand_f64_0_1()*4294967296.0)
#define sl_rand_f64_u16() (sl_rand_f64_0_1()*65536.0)
#define sl_rand_f64_u8() (sl_rand_f64_0_1()*256.0)

#define sl_rand_f32_0_1() (rand()/(RAND_MAX+1.0))

#define sl_rand_f32_s32() (sl_rand_f64_0_1()*4294967296.0-2147483648.0)
#define sl_rand_f32_s16() (sl_rand_f64_0_1()*65536.0-32768.0)
#define sl_rand_f32_s8() (sl_rand_f64_0_1()*256.0-128.0)

#define sl_rand_f32_u32() (sl_rand_f64_0_1()*4294967296.0)
#define sl_rand_f32_u16() (sl_rand_f64_0_1()*65536.0)
#define sl_rand_f32_u8() (sl_rand_f64_0_1()*256.0)

/* floating point equality, used for testing */

#define TWO_TO_NEG52 2.220446049e-16
#define TWO_TO_NEG23 1.192092896e-7

#define sl_equal_f64(a,b) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG52)
#define sl_equal_f32(a,b) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG23)

/* inaccurate equality, used for testing */
/* the _iXX tags use a decilogarithmic scale, i.e., 00->1.0, 10->10.0,
 * 11->12.6, etc.  _i00 represents "perfect" according to the reference
 * function.
 *
 *  11 -> 12.6
 *  12 -> 15.8
 *  13 -> 20.0
 *  14 -> 25.1
 *  15 -> 31.6
 *  16 -> 39.8
 *  17 -> 50.1
 *  18 -> 63.1
 *  19 -> 79.4
 */

#define sl_equal_f64_i10(a,b) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG52*10)
#define sl_equal_f64_i20(a,b) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG52*100)
#define sl_equal_f64_i30(a,b) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG52*1000)

#define sl_equal_f32_i10(a,b) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG23*10)
#define sl_equal_f32_i20(a,b) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG23*100)
#define sl_equal_f32_i30(a,b) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG23*1000)

#define sl_equal_f32_factor(a,b,x) (fabs((a)-(b)) < fabs((a)+(b))*TWO_TO_NEG23*(x))


/* useful constants */

/* cos(i*pi/16) */

#define C0_9808 0.980785280
#define C0_9239 0.923879532
#define C0_8315 0.831469612
#define C0_7071 0.707106781
#define C0_5556 0.555570233
#define C0_3827 0.382683432
#define C0_1951 0.195090322

#endif

