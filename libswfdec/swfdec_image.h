
#ifndef _SWFDEC_IMAGE_H_
#define _SWFDEC_IMAGE_H_

#include <swfdec_decoder.h>
#include <swfdec_object.h>

G_BEGIN_DECLS

//typedef struct _SwfdecImage SwfdecImage;
typedef struct _SwfdecImageClass SwfdecImageClass;

#define SWFDEC_TYPE_IMAGE                    (swfdec_image_get_type()) 
#define SWFDEC_IS_IMAGE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_IMAGE))
#define SWFDEC_IS_IMAGE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_IMAGE))
#define SWFDEC_IMAGE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_IMAGE, SwfdecImage))
#define SWFDEC_IMAGE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_IMAGE, SwfdecImageClass))

struct _SwfdecImage {
  SwfdecObject object;

  int width, height;
  int rowstride;
  unsigned char *image_data;
};

struct _SwfdecImageClass {
  SwfdecObjectClass object_class;

};

GType swfdec_image_get_type (void);

int swfdec_image_jpegtables (SwfdecDecoder * s);
int tag_func_define_bits_jpeg (SwfdecDecoder * s);
int tag_func_define_bits_jpeg_2 (SwfdecDecoder * s);
int tag_func_define_bits_jpeg_3 (SwfdecDecoder * s);
int tag_func_define_bits_lossless (SwfdecDecoder * s);
int tag_func_define_bits_lossless_2 (SwfdecDecoder * s);
int swfdec_image_render (SwfdecDecoder * s, SwfdecLayer * layer,
    SwfdecObject * obj);

G_END_DECLS

#endif

