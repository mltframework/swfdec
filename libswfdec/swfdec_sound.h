
#ifndef _SWFDEC_SOUND_H_
#define _SWFDEC_SOUND_H_

#include <swfdec_object.h>
#include <swfdec_types.h>
#include <swfdec_codec.h>

G_BEGIN_DECLS

//typedef struct _SwfdecSoundChunk SwfdecSoundChunk;
//typedef struct _SwfdecSound SwfdecSound;
typedef struct _SwfdecSoundClass SwfdecSoundClass;
typedef struct _SwfdecSoundEnvelope SwfdecSoundEnvelope;

#define SWFDEC_TYPE_SOUND                    (swfdec_sound_get_type())
#define SWFDEC_IS_SOUND(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SOUND))
#define SWFDEC_IS_SOUND_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SOUND))
#define SWFDEC_SOUND(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SOUND, SwfdecSound))
#define SWFDEC_SOUND_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SOUND, SwfdecSoundClass))

struct _SwfdecSoundEnvelope {
  unsigned int		offset;			/* offset in frames */
  guint16		volume[2];		/* volume to use */
};

struct _SwfdecSoundChunk
{
  SwfdecSound *		sound;			/* sound to play */

  int			stop;	      		/* stop the sample being played */
  int			no_restart;	      	/* don't restart if already playing */

  unsigned int		start_sample; 		/* sample at which to start playing */
  unsigned int		stop_sample;	      	/* first sample to not play anymore */
  unsigned int		loop_count;		/* amount of times this sample should be played back */
  unsigned int		n_envelopes;		/* amount of points in the envelope */
  SwfdecSoundEnvelope *	envelope;		/* volume envelope or NULL if none */
};

struct _SwfdecSound
{
  SwfdecObject		object;

  SwfdecAudioFormat	format;			/* format in use */
  const SwfdecCodec *	codec;			/* codec for this sound */
  gboolean		width;			/* TRUE for 16bit, FALSE for 8bit */
  unsigned int		channels;		/* 1 or 2 */
  unsigned int		rate_multiplier;	/* rate = 44100 / rate_multiplier */
  unsigned int		n_samples;		/* total number of samples */

  SwfdecBuffer *	decoded;		/* decoded data in 44.1kHz stereo host-endian, or NULL for stream objects */
};

struct _SwfdecSoundClass
{
  SwfdecObjectClass object_class;

};

GType swfdec_sound_get_type (void);

int tag_func_define_sound (SwfdecDecoder * s);
int tag_func_sound_stream_block (SwfdecDecoder * s);
int tag_func_sound_stream_head (SwfdecDecoder * s);
int tag_func_start_sound (SwfdecDecoder * s);
int tag_func_define_button_sound (SwfdecDecoder * s);

gpointer		swfdec_sound_init_decoder	(SwfdecSound *	sound);
void			swfdec_sound_finish_decoder	(SwfdecSound *	sound,
							 gpointer	data);
SwfdecBuffer *		swfdec_sound_decode_buffer	(SwfdecSound *	sound,
							 gpointer	data,
							 SwfdecBuffer *	buffer);

void			swfdec_sound_render		(SwfdecSound *	sound, 
							 gint16 *	dest, 
							 unsigned int	offset,
		  					 unsigned int	len,
							 const guint16	volume[2]);
void			swfdec_sound_add		(gint16 *	dest,
							 const gint16 *	src,
							 unsigned int	n_samples);

SwfdecSoundChunk *	swfdec_sound_parse_chunk	(SwfdecDecoder *s,
							 int		id);
void			swfdec_sound_chunk_free		(SwfdecSoundChunk *chunk);


G_END_DECLS
#endif
