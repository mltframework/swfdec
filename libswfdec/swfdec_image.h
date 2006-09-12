
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

typedef enum {
  SWFDEC_IMAGE_TYPE_UNKNOWN = 0,
  SWFDEC_IMAGE_TYPE_JPEG,
  SWFDEC_IMAGE_TYPE_JPEG2,
  SWFDEC_IMAGE_TYPE_JPEG3,
  SWFDEC_IMAGE_TYPE_LOSSLESS,
  SWFDEC_IMAGE_TYPE_LOSSLESS2,
} SwfdecImageType;

struct _SwfdecImage
{
  SwfdecObject object;
  SwfdecHandle *handle;

  int width, height;
  int rowstride;

  SwfdecBuffer *jpegtables;
  SwfdecBuffer *raw_data;

  SwfdecImageType type;
};

struct _SwfdecImageClass
{
  SwfdecObjectClass object_class;

};

GType swfdec_image_get_type (void);

int swfdec_image_jpegtables (SwfdecDecoder * s);
int tag_func_define_bits_jpeg (SwfdecDecoder * s);
int tag_func_define_bits_jpeg_2 (SwfdecDecoder * s);
int tag_func_define_bits_jpeg_3 (SwfdecDecoder * s);
int tag_func_define_bits_lossless (SwfdecDecoder * s);
int tag_func_define_bits_lossless_2 (SwfdecDecoder * s);

G_END_DECLS
#endif
