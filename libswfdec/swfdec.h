
#ifndef __SWFDEC_H__
#define __SWFDEC_H__

#include <swfdec_types.h>
#include <swfdec_rect.h>

G_BEGIN_DECLS 

typedef enum {
  /* An error occured during parsing */
  SWFDEC_ERROR = 0,
  /* processing continued as expected */
  SWFDEC_OK,
  /* more data needs to be made available for processing */
  SWFDEC_NEEDBITS,
  /* at least one new image is available for display */
  SWFDEC_IMAGE,
  /* header parsing is complete, framerate, image size etc are known */
  SWFDEC_CHANGE,
  /* parsing is finished */
  SWFDEC_EOF
} SwfdecStatus;

void swfdec_init (void);

SwfdecDecoder *swfdec_decoder_new (void);
int swfdec_decoder_add_data (SwfdecDecoder * s, const unsigned char *data,
    int length);
int swfdec_decoder_add_buffer (SwfdecDecoder * s, SwfdecBuffer * buffer);
int swfdec_decoder_parse (SwfdecDecoder * s);

unsigned int swfdec_decoder_get_n_frames (SwfdecDecoder * s);
double swfdec_decoder_get_rate (SwfdecDecoder * s);
int swfdec_decoder_get_version (SwfdecDecoder *s);
gboolean swfdec_decoder_get_image_size (SwfdecDecoder * s, int *width, int *height);

int swfdec_decoder_set_debug_level (SwfdecDecoder * s, int level);

void swfdec_decoder_get_invalid (SwfdecDecoder *s, SwfdecRect *rect);
void swfdec_decoder_clear_invalid (SwfdecDecoder *s);
void *swfdec_decoder_get_sound_chunk (SwfdecDecoder * s, int *length);
char *swfdec_decoder_get_url (SwfdecDecoder * s);

void swfdec_decoder_eof (SwfdecDecoder * s);

void swfdec_decoder_iterate (SwfdecDecoder *dec);
void swfdec_decoder_render (SwfdecDecoder *dec, cairo_t *cr, SwfdecRect *area);
void swfdec_decoder_handle_mouse (SwfdecDecoder *dec, 
    double x, double y, int button);

guint swfdec_decoder_get_audio_samples (SwfdecDecoder *dec);
void swfdec_decoder_render_audio (SwfdecDecoder *dec, gint16* dest, guint dest_size);
SwfdecBuffer *swfdec_decoder_render_audio_to_buffer (SwfdecDecoder *dec);

G_END_DECLS
#endif
