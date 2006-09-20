
#ifndef _SWFDEC_TYPES_H_
#define _SWFDEC_TYPES_H_

#include <glib-object.h>
#include <cairo.h>

typedef struct _SwfdecActionContext SwfdecActionContext;
typedef struct _SwfdecBuffer SwfdecBuffer;
typedef struct _SwfdecBufferQueue SwfdecBufferQueue;
typedef struct _SwfdecButton SwfdecButton;
typedef struct _SwfdecCache SwfdecCache;
typedef struct _SwfdecColorTransform SwfdecColorTransform;
typedef struct _SwfdecDecoder SwfdecDecoder;
typedef struct _SwfdecDecoderClass SwfdecDecoderClass;
typedef struct _SwfdecFont SwfdecFont;
typedef struct _SwfdecHandle SwfdecHandle;
typedef struct _SwfdecImage SwfdecImage;
typedef struct _SwfdecMovieClip SwfdecMovieClip;
typedef struct _SwfdecObject SwfdecObject;
typedef struct _SwfdecShape SwfdecShape;
typedef struct _SwfdecShapeVec SwfdecShapeVec;
typedef struct _SwfdecRect SwfdecRect;
typedef struct _SwfdecSound SwfdecSound;
typedef struct _SwfdecSoundChunk SwfdecSoundChunk;
typedef struct _SwfdecSprite SwfdecSprite;
typedef struct _SwfdecSpriteFrame SwfdecSpriteFrame;
typedef struct _SwfdecText SwfdecText;
typedef struct _SwfdecTextGlyph SwfdecTextGlyph;

#define SWFDEC_TYPE_DECODER                    (swfdec_decoder_get_type())
#define SWFDEC_IS_DECODER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DECODER))
#define SWFDEC_IS_DECODER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DECODER))
#define SWFDEC_DECODER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DECODER, SwfdecDecoder))
#define SWFDEC_DECODER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DECODER, SwfdecDecoderClass))
#define SWFDEC_DECODER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_DECODER, SwfdecDecoderClass))

GType swfdec_decoder_get_type ();

#endif
