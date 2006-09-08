
#ifndef __SWFDEC_LAYER_H__
#define __SWFDEC_LAYER_H__

#include "config.h"

#include "swfdec_types.h"
#include "swfdec_transform.h"

struct swfdec_layer_vec_struct
{
  unsigned int color;
  SwfdecRect rect;

  unsigned char *compose;
  unsigned int compose_rowstride;
  unsigned int compose_height;
  unsigned int compose_width;
};

struct swfdec_layer_struct
{
  SwfdecSpriteSegment *seg;
  int first_frame;
  int last_frame;
  SwfdecRect rect;

  int frame_number;

  SwfdecTransform transform;
  SwfdecColorTransform color_transform;
  int ratio;

  GArray *lines;
  GArray *fills;

  GList *sublayers;
};

SwfdecLayer *swfdec_layer_new (void);
void swfdec_layer_free (SwfdecLayer * layer);
void swfdec_layer_prerender (SwfdecDecoder * s, SwfdecLayer * layer);
void swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec);
void swfdec_layer_render (SwfdecDecoder * s, SwfdecLayer * layer);
SwfdecLayer *swfdec_render_get_sublayer (SwfdecLayer * layer, int depth,
    int frame);


#endif
