#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_codec.h"
#include "swfdec_debug.h"

/*** DECODER LIST ***/

#ifdef HAVE_MAD
extern const SwfdecCodec swfdec_codec_mad;
#endif
#ifdef HAVE_FFMPEG
extern const SwfdecCodec swfdec_codec_ffmpeg_adpcm;
extern const SwfdecCodec swfdec_codec_ffmpeg_mp3;
#endif

/*** PUBLIC API ***/

const SwfdecCodec *
swfdec_codec_get_audio (SwfdecAudioFormat format)
{
  switch (format) {
    case SWFDEC_AUDIO_FORMAT_UNDEFINED:
    case SWFDEC_AUDIO_FORMAT_UNCOMPRESSED:
      SWFDEC_ERROR ("uncompressed sound is not implemented yet");
      return NULL;
    case SWFDEC_AUDIO_FORMAT_ADPCM:
#ifdef HAVE_FFMPEG
      return &swfdec_codec_ffmpeg_adpcm;
#else
      SWFDEC_ERROR ("adpcm sound requires ffmpeg");
      return NULL;
#endif
    case SWFDEC_AUDIO_FORMAT_MP3:
#ifdef HAVE_FFMPEG
      return &swfdec_codec_ffmpeg_mp3;
#else
#ifdef HAVE_MAD
      //return &swfdec_codec_mad;
#else
      SWFDEC_ERROR ("mp3 sound requires ffmpeg or mad");
      return NULL;
#endif
#endif
    case SWFDEC_AUDIO_FORMAT_NELLYMOSER:
      SWFDEC_ERROR ("Nellymoser sound is not implemented yet");
      return NULL;
    default:
      SWFDEC_ERROR ("undefined sound format %u", format);
      return NULL;
  }
}

const SwfdecCodec *
swfdec_codec_get_video (unsigned int format)
{
  SWFDEC_ERROR ("video not implemented yet");
  return NULL;
}

