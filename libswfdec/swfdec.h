
#ifndef __SWFDEC_H__
#define __SWFDEC_H__

#include <swfdec_types.h>
#include <swfdec_rect.h>

G_BEGIN_DECLS enum
{
  SWF_OK = 0,
  SWF_NEEDBITS,
  SWF_WAIT,
  SWF_ERROR,
  SWF_EOF,
  SWF_IMAGE,
  SWF_CHANGE,
};

void swfdec_init (void);

SwfdecDecoder *swfdec_decoder_new (void);
int swfdec_decoder_add_data (SwfdecDecoder * s, const unsigned char *data,
    int length);
int swfdec_decoder_add_buffer (SwfdecDecoder * s, SwfdecBuffer * buffer);
int swfdec_decoder_parse (SwfdecDecoder * s);
void swfdec_decoder_free (SwfdecDecoder * s);

int swfdec_decoder_get_n_frames (SwfdecDecoder * s, int *n_frames);
int swfdec_decoder_get_rate (SwfdecDecoder * s, double *rate);
int swfdec_decoder_get_image (SwfdecDecoder * s, unsigned char **image);
int swfdec_decoder_peek_image (SwfdecDecoder * s, unsigned char **image);
int swfdec_decoder_get_image_size (SwfdecDecoder * s, int *width, int *height);
int swfdec_decoder_get_version (SwfdecDecoder *s);

int swfdec_decoder_set_debug_level (SwfdecDecoder * s, int level);

void *swfdec_decoder_get_sound_chunk (SwfdecDecoder * s, int *length);
SwfdecBuffer *swfdec_decoder_get_audio (SwfdecDecoder * s);
char *swfdec_decoder_get_url (SwfdecDecoder * s);

void swfdec_decoder_eof (SwfdecDecoder * s);

void swfdec_decoder_iterate (SwfdecDecoder *dec);
void swfdec_decoder_render (SwfdecDecoder *dec, cairo_t *cr, SwfdecRect *area);
void swfdec_decoder_handle_mouse (SwfdecDecoder *dec, 
    double x, double y, int button);

G_END_DECLS
#endif
