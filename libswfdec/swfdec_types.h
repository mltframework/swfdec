
#ifndef _SWFDEC_TYPES_H_
#define _SWFDEC_TYPES_H_

#include <glib-object.h>
#include <cairo.h>

/* audio is 44100Hz, framerate is multiple of 256Hz, FLV timestamps are 1000Hz
 * This is a multiple of all these numbers, so we can be always accurate
 */
#define SWFDEC_TICKS_PER_SECOND G_GUINT64_CONSTANT (44100 * 256 * 10)
typedef guint64 SwfdecTick;

#define SWFDEC_TWIPS_SCALE_FACTOR	      	20
typedef int SwfdecTwips;
#define SWFDEC_TWIPS_TO_DOUBLE(t) ((t) * (1.0 / SWFDEC_TWIPS_SCALE_FACTOR))
#define SWFDEC_DOUBLE_TO_TWIPS(d) ((SwfdecTwips)((d) * SWFDEC_TWIPS_SCALE_FACTOR))

#define SWFDEC_FIXED_SCALE_FACTOR		65536
typedef int SwfdecFixed;
#define SWFDEC_FIXED_TO_DOUBLE(f) ((f) * (1.0 / SWFDEC_FIXED_SCALE_FACTOR))
#define SWFDEC_DOUBLE_TO_FIXED(d) ((SwfdecFixed)((d) * SWFDEC_FIXED_SCALE_FACTOR))
#define SWFDEC_FIXED_TO_INT(f) ((f) / SWFDEC_FIXED_SCALE_FACTOR)
#define SWFDEC_INT_TO_FIXED(i) ((i) * SWFDEC_FIXED_SCALE_FACTOR)

typedef struct _SwfdecButton SwfdecButton;
typedef struct _SwfdecCache SwfdecCache;
typedef struct _SwfdecCharacter SwfdecCharacter;
typedef struct _SwfdecColorTransform SwfdecColorTransform;
typedef struct _SwfdecContent SwfdecContent;
typedef struct _SwfdecDebugger SwfdecDebugger;
typedef struct _SwfdecDecoder SwfdecDecoder;
typedef struct _SwfdecEventList SwfdecEventList;
typedef struct _SwfdecFont SwfdecFont;
typedef struct _SwfdecGraphic SwfdecGraphic;
typedef struct _SwfdecHandle SwfdecHandle;
typedef struct _SwfdecImage SwfdecImage;
typedef struct _SwfdecMovie SwfdecMovie;
typedef struct _SwfdecShape SwfdecShape;
typedef struct _SwfdecShapeVec SwfdecShapeVec;
typedef struct _SwfdecRect SwfdecRect;
typedef struct _SwfdecRootMovie SwfdecRootMovie;
typedef struct _SwfdecSound SwfdecSound;
typedef struct _SwfdecSoundChunk SwfdecSoundChunk;
typedef struct _SwfdecSprite SwfdecSprite;
typedef struct _SwfdecSpriteFrame SwfdecSpriteFrame;
typedef struct _SwfdecSwfDecoder SwfdecSwfDecoder;
typedef struct _SwfdecText SwfdecText;

#endif
