
#ifndef __SWFDEC_DECODER_H__
#define __SWFDEC_DECODER_H__

#include <glib.h>
#include <zlib.h>
#include "swfdec_bits.h"

#include "swfdec_types.h"
#include "swfdec_rect.h"
#include "swfdec_transform.h"


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

struct _SwfdecColorTransform
{
  double mult[4];
  double add[4];
};

typedef int SwfdecTagFunc (SwfdecDecoder *);

struct _SwfdecDecoder
{
  int version;
  int length;
  int width, height;
  int parse_width, parse_height;
  double rate;
  int n_frames;
  char *buffer;
  int frame_number;

  void *sound_buffer;
  int sound_len;
  int sound_offset;

  int colorspace;
  int no_render;
  int compressed;

  /* End of legacy elements */

  z_stream *z;
  SwfdecBuffer *uncompressed_buffer;

  SwfdecBufferQueue *input_queue;

  int stride;
  int bytespp;
  void (*callback) (void *, int, int, void *, int);
  void (*compose_callback) (void *, int, int, void *, int);
  void (*fillrect) (unsigned char *, int, unsigned int, SwfdecRect *);

  double scale_factor;
  SwfdecTransform transform;

  /* where we are in the top-level state engine */
  int state;

  /* where we are in global parsing */
  SwfdecBits parse;

  /* temporary state while parsing */
  SwfdecBits b;

  /* defined objects */
  GList *objects;

  SwfdecSound *stream_sound_obj;

  /* rendering state */
  SwfdecRender *render;
  unsigned int bg_color;
  SwfdecRect irect;

  SwfdecSprite *main_sprite;
  SwfdecSprite *parse_sprite;

  double flatness;
  int disable_render;

  unsigned char *tmp_scanline;

  unsigned char *jpegtables;
  unsigned int jpegtables_len;

  GList *sound_buffers;
  GList *stream_sound_buffers;

  int pixels_rendered;

  int stats_n_points;

  /* should be in SwfdecRender */
  gboolean stopped;

  void *backend_private;

  char *kept_buffer;
  GList *kept_list;
  int kept_layers;

  SwfdecActionContext *context;

  int mouse_x;
  int mouse_y;
  int mouse_button;
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


#endif
