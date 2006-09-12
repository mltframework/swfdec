
#ifndef __SWFDEC_RENDER_H__
#define __SWFDEC_RENDER_H__

#include "config.h"

#include "swfdec_types.h"
#include <swfdec_rect.h>

G_BEGIN_DECLS

struct swfdec_render_struct
{
  int frame_index;
  SwfdecRect drawrect;
};

SwfdecRender *swfdec_render_new (void);
void swfdec_render_free (SwfdecRender * render);
void swfdec_render_seek (SwfdecDecoder * s, int frame);
SwfdecBuffer *swfdec_render_get_audio (SwfdecDecoder * s);
int swfdec_render_get_frame_index (SwfdecDecoder * s);
void swfdec_render_resize (SwfdecDecoder *s);

/* functions implemented by rendering backends */

void swf_render_frame (SwfdecDecoder * s, int frame_index);
void swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec);
void swfdec_shape_render (SwfdecDecoder * decoder, cairo_t *cr, 
    const SwfdecColorTransform *trans, SwfdecObject *object, SwfdecRect *inval);
void swfdec_shape_render (SwfdecDecoder * decoder, 
      cairo_t *cr, const SwfdecColorTransform *trans,
      SwfdecObject * object, SwfdecRect *inval);
void swfdec_text_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object);
void swf_config_colorspace (SwfdecDecoder * s);
void swfdec_render_layervec_free (SwfdecLayerVec * layervec);

G_END_DECLS

#endif
