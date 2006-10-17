
#ifndef _SWFDEC_CODEC_H_
#define _SWFDEC_CODEC_H_

#include <glib.h>
#include <libswfdec/swfdec_buffer.h>

typedef struct _SwfdecCodec SwfdecCodec;

typedef enum {
  SWFDEC_AUDIO_FORMAT_UNDEFINED = 0,
  SWFDEC_AUDIO_FORMAT_ADPCM = 1,
  SWFDEC_AUDIO_FORMAT_MP3 = 2,
  SWFDEC_AUDIO_FORMAT_UNCOMPRESSED = 3,
  SWFDEC_AUDIO_FORMAT_NELLYMOSER = 6
} SwfdecAudioFormat;

typedef enum {
  SWFDEC_VIDEO_FORMAT_UNDEFINED
} SwfdecVideoFormat;

struct _SwfdecCodec {
  gpointer		(* init)	(gboolean	width,
					 guint		channels,
					 guint		rate_multiplier);
  SwfdecBuffer *	(* decode)	(gpointer	codec_data,
					 SwfdecBuffer *	buffer);
  void			(* finish)	(gpointer	codec_data);
};

const SwfdecCodec *   	swfdec_codec_get_audio		(SwfdecAudioFormat	format);
const SwfdecCodec *  	swfdec_codec_get_video		(SwfdecVideoFormat	format);

#define swfdec_codec_init(codec,width,channels,rate) (codec)->init (width, channels, rate)
#define swfdec_codec_decode(codec, codec_data, buffer) (codec)->decode (codec_data, buffer)
#define swfdec_codec_finish(codec, codec_data) (codec)->finish (codec_data)


G_END_DECLS
#endif
