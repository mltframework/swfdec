
#ifndef __SWFDEC_RENDER_LIBART_H__
#define __SWFDEC_RENDER_LIBART_H__

#include "swfdec_internal.h"

void swf_render_frame (SwfdecDecoder * s, int frame_index);
void swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec);
ArtSVP * art_svp_translate (ArtSVP * svp, double x, double y);
void art_svp_bbox (ArtSVP * svp, ArtIRect * box);
SwfdecLayer * swfdec_shape_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg, SwfdecObject * obj, SwfdecLayer * oldlayer);
SwfdecLayer * swfdec_text_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg, SwfdecObject * object, SwfdecLayer * oldlayer);
void swf_config_colorspace (SwfdecDecoder * s);
void swfdec_render_layervec_free (SwfdecLayerVec *layervec);

#endif

