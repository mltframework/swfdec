
#ifndef _SWFDEC_TYPES_H_
#define _SWFDEC_TYPES_H_

#include <glib-object.h>
#include <cairo.h>

G_BEGIN_DECLS

/* Pixel value in the same colorspace as cairo - endian-dependant ARGB.
 * The alpha pixel must be present */
typedef guint32 SwfdecColor;

/* audio is 44100Hz, framerate is multiple of 256Hz, FLV timestamps are 1000Hz
 * This is a multiple of all these numbers, so we can be always accurate
 */
#define SWFDEC_TICKS_PER_SECOND G_GUINT64_CONSTANT (44100 * 256 * 10)
typedef guint64 SwfdecTick;
#define SWFDEC_MSECS_TO_TICKS(msecs) ((SwfdecTick) (msecs) * (SWFDEC_TICKS_PER_SECOND / 1000))
#define SWFDEC_TICKS_TO_MSECS(ticks) ((ticks) / (SWFDEC_TICKS_PER_SECOND / 1000))
#define SWFDEC_SAMPLES_TO_TICKS(msecs) ((SwfdecTick) (msecs) * (SWFDEC_TICKS_PER_SECOND / 44100))
#define SWFDEC_TICKS_TO_SAMPLES(ticks) ((ticks) / (SWFDEC_TICKS_PER_SECOND / 44100))

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

typedef struct _SwfdecActor SwfdecActor;
typedef struct _SwfdecButton SwfdecButton;
typedef struct _SwfdecCharacter SwfdecCharacter;
typedef struct _SwfdecColorTransform SwfdecColorTransform;
typedef struct _SwfdecDecoder SwfdecDecoder;
typedef struct _SwfdecDisplayObject SwfdecDisplayObject;
typedef struct _SwfdecDisplayObjectContainer SwfdecDisplayObjectContainer;
typedef struct _SwfdecDraw SwfdecDraw;
typedef struct _SwfdecEventDispatcher SwfdecEventDispatcher;
typedef struct _SwfdecEventList SwfdecEventList;
typedef struct _SwfdecFilter SwfdecFilter;
typedef struct _SwfdecFont SwfdecFont;
typedef struct _SwfdecGraphic SwfdecGraphic;
typedef struct _SwfdecImage SwfdecImage;
typedef struct _SwfdecInteractiveObject SwfdecInteractiveObject;
typedef struct _SwfdecListener SwfdecListener;
typedef struct _SwfdecMovie SwfdecMovie;
typedef struct _SwfdecMovieClipLoader SwfdecMovieClipLoader;
typedef struct _SwfdecShape SwfdecShape;
typedef struct _SwfdecShapeVec SwfdecShapeVec;
typedef struct _SwfdecRect SwfdecRect;
typedef struct _SwfdecResource SwfdecResource;
typedef struct _SwfdecRootSprite SwfdecRootSprite;
typedef struct _SwfdecSandbox SwfdecSandbox;
typedef struct _SwfdecScriptable SwfdecScriptable;
typedef struct _SwfdecSound SwfdecSound;
typedef struct _SwfdecSoundChunk SwfdecSoundChunk;
typedef struct _SwfdecSprite SwfdecSprite;
typedef struct _SwfdecSpriteFrame SwfdecSpriteFrame;
typedef struct _SwfdecSpriteMovie SwfdecSpriteMovie;
typedef struct _SwfdecSwfDecoder SwfdecSwfDecoder;
typedef struct _SwfdecText SwfdecText;

G_END_DECLS
#endif
