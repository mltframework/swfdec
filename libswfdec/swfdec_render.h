
#ifndef __SWFDEC_RENDER_H__
#define __SWFDEC_RENDER_H__

#include "config.h"

#include "swfdec_types.h"
#include "swfdec_transform.h"
#include <swfdec_rect.h>

struct swfdec_render_struct
{
  int frame_index;
  int seek_frame;
  SwfdecRect drawrect;

  GList *object_states;
};

struct _SwfdecRenderState
{
  int layer;
  int id;
  int frame_index;
};

SwfdecRender *swfdec_render_new (void);
void swfdec_render_free (SwfdecRender * render);
gboolean swfdec_render_iterate (SwfdecDecoder * s);
void swfdec_render_seek (SwfdecDecoder * s, int frame);
SwfdecBuffer *swfdec_render_get_image (SwfdecDecoder * s);
SwfdecBuffer *swfdec_render_get_audio (SwfdecDecoder * s);
int swfdec_render_get_frame_index (SwfdecDecoder * s);
SwfdecRenderState *swfdec_render_get_object_state (SwfdecRender * render,
    int layer, int id);

/* functions implemented by rendering backends */

void swfdec_render_be_start (SwfdecDecoder *s);
void swfdec_render_be_stop (SwfdecDecoder *s);
void swf_render_frame (SwfdecDecoder * s, int frame_index);
void swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec);
void swfdec_shape_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * obj);
void swfdec_text_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object);
void swf_config_colorspace (SwfdecDecoder * s);
void swfdec_render_layervec_free (SwfdecLayerVec * layervec);


#endif
