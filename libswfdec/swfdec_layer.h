
#ifndef __SWFDEC_LAYER_H__
#define __SWFDEC_LAYER_H__

#include "config.h"

#include "swfdec_types.h"
#include "swfdec_transform.h"
#ifdef HAVE_LIBART
#include <libart_lgpl/libart.h>
#endif

struct swfdec_render_struct
{
  GList *layers;
};

struct swfdec_layer_vec_struct
{
#ifdef HAVE_LIBART
  ArtSVP *svp;
#endif
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

SwfdecRender *swfdec_render_new (void);
void swfdec_render_free (SwfdecRender * render);

SwfdecLayer *swfdec_layer_new (void);
void swfdec_layer_free (SwfdecLayer * layer);
SwfdecLayer *swfdec_layer_get (SwfdecDecoder * s, int depth);
void swfdec_render_add_layer (SwfdecRender * s, SwfdecLayer * lnew);
void swfdec_render_del_layer (SwfdecRender * s, SwfdecLayer * layer);
void swfdec_layer_prerender (SwfdecDecoder * s, SwfdecLayer * layer);
void swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec);
void swfdec_layer_render (SwfdecDecoder * s, SwfdecLayer * layer);
SwfdecLayer *swfdec_render_get_seg (SwfdecRender * render,
    SwfdecSpriteSegment * seg);
SwfdecLayer *swfdec_render_get_layer (SwfdecRender * render, int layer,
    int frame);
SwfdecLayer *swfdec_render_get_sublayer (SwfdecLayer * layer, int depth,
    int frame);


#endif
