
#ifndef __SWFDEC_DECODER_H__
#define __SWFDEC_DECODER_H__

#include <glib.h>
#include <zlib.h>
#include <js/jspubtd.h>
#include "swfdec_bits.h"

#include "swfdec_types.h"
#include "swfdec_rect.h"


#define SWF_COLOR_SCALE_FACTOR		(1/256.0)
#define SWF_TRANS_SCALE_FACTOR		(1/63356.0)
#define SWF_SCALE_FACTOR		(1/20.0)
#define SWF_TEXT_SCALE_FACTOR		(1/1024.0)

enum
{
  SWF_STATE_INIT1 = 0,
  SWF_STATE_INIT2,
  SWF_STATE_PARSETAG,
  SWF_STATE_EOF,
};

typedef int SwfdecTagFunc (SwfdecDecoder *);

#define SWFDEC_IS_DECODER(s) ((s) != NULL)

struct _SwfdecDecoder
{
  int version;
  int length;
  int loaded;
  int width, height;
  int parse_width, parse_height;
  double rate;
  int n_frames;
  guint8 *buffer;

  void *sound_buffer;
  int sound_len;
  int sound_offset;

  int compressed;

  /* End of legacy elements */

  z_stream *z;
  SwfdecBuffer *uncompressed_buffer;

  SwfdecBufferQueue *input_queue;

  /* where we are in the top-level state engine */
  int state;

  /* where we are in global parsing */
  SwfdecBits parse;

  /* temporary state while parsing */
  SwfdecBits b;

  /* defined objects */
  GList *objects;
  GList *exports;

  SwfdecSound *stream_sound_obj;

  /* rendering state */
  SwfdecRect irect;
  SwfdecRect invalid;	    /* in pixels */

  SwfdecSprite *main_sprite;
  SwfdecMovieClip *root;
  SwfdecSprite *parse_sprite;

  double flatness;
  int disable_render;

  unsigned char *tmp_scanline;

  SwfdecBuffer *jpegtables;

  int pixels_rendered;

  int stats_n_points;

  void *backend_private;

  guint8 *kept_buffer;
  GList *kept_list;
  int kept_layers;

  JSContext *jscx;

  char *url;

  GList *audio_streams;
  int audio_stream_index;

  GList *execute_list;

  SwfdecCache *cache;
};

SwfdecDecoder *swf_init (void);
SwfdecDecoder *swfdec_decoder_new (void);

int swf_addbits (SwfdecDecoder * s, unsigned char *bits, int len);
int swf_parse (SwfdecDecoder * s);
int swf_parse_header (SwfdecDecoder * s);
int swf_parse_tag (SwfdecDecoder * s);
int tag_func_ignore (SwfdecDecoder * s);

void swfdec_decoder_eof (SwfdecDecoder * s);

SwfdecTagFunc *swfdec_decoder_get_tag_func (int tag);
const char *swfdec_decoder_get_tag_name (int tag);
int swfdec_decoder_get_tag_flag (int tag);
void swfdec_decoder_queue_script (SwfdecDecoder *s, JSScript *script);
void swfdec_decoder_execute_scripts (SwfdecDecoder *s);


#endif
