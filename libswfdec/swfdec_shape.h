
#ifndef _SWFDEC_SHAPE_H_
#define _SWFDEC_SHAPE_H_

#include <glib-object.h>
#include <swfdec_types.h>
#include <swfdec_object.h>
#include <color.h>
#include <swfdec_bits.h>

G_BEGIN_DECLS

//typedef struct _SwfdecShape SwfdecShape;
typedef struct _SwfdecShapeClass SwfdecShapeClass;

#define SWFDEC_TYPE_SHAPE                    (swfdec_shape_get_type()) 
#define SWFDEC_IS_SHAPE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SHAPE))
#define SWFDEC_IS_SHAPE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SHAPE))
#define SWFDEC_SHAPE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SHAPE, SwfdecShape))
#define SWFDEC_SHAPE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SHAPE, SwfdecShapeClass))

struct _SwfdecShapeVec
{
  int type;
  int index;
  unsigned int color;
  double width;

  GArray *path;
  int array_len;

  int fill_type;
  int fill_id;
  double fill_matrix[6];

  SwfdecGradient *grad;
};

struct _SwfdecShape {
  SwfdecObject object;

  GPtrArray *lines;
  GPtrArray *fills;
  GPtrArray *fills2;

  /* used while defining */
  int fills_offset;
  int lines_offset;
  int n_fill_bits;
  int n_line_bits;
  int rgba;
};

struct _SwfdecShapeClass {
  SwfdecObjectClass object_class;

};

GType swfdec_shape_get_type (void);

void swfdec_shape_free (SwfdecObject * object);
void _swfdec_shape_free (SwfdecShape * shape);
int tag_func_define_shape (SwfdecDecoder * s);
SwfdecShapeVec *swf_shape_vec_new (void);
int art_define_shape (SwfdecDecoder * s);
int art_define_shape_3 (SwfdecDecoder * s);
void swf_shape_add_styles (SwfdecDecoder * s, SwfdecShape * shape,
    SwfdecBits * bits);
void swf_shape_get_recs (SwfdecDecoder * s, SwfdecBits * bits, SwfdecShape * shape);
int art_define_shape_2 (SwfdecDecoder * s);
int tag_func_define_button_2 (SwfdecDecoder * s);
int tag_func_define_sprite (SwfdecDecoder * s);
void dump_layers (SwfdecDecoder * s);

void swfdec_shape_render (SwfdecDecoder * s, SwfdecLayer * layer,
    SwfdecObject * shape);


G_END_DECLS

#endif

