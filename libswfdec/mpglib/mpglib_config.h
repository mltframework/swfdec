
#define REAL_IS_FLOAT


#define NEW_DCT9

#define random rand

#define srandom srand


#ifdef REAL_IS_FLOAT
#  define real float
#elif defined(REAL_IS_LONG_DOUBLE)
#  define real long double
#else
#  define real double
#endif

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

/* AUDIOBUFSIZE = n*64 with n=1,2,3 ...  */
#define		AUDIOBUFSIZE		16384

#ifndef FALSE
#define         FALSE                   0
#endif
#ifndef FALSE
#define         TRUE                    1
#endif

#define         SBLIMIT                 32
#define         SSLIMIT                 18

#define         SCALE_BLOCK             12


#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

#define MAXFRAMESIZE 1792


/* Pre Shift fo 16 to 8 bit converter table */
#define AUSHIFT (3)


