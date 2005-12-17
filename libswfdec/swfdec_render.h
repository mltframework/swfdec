
#ifndef __SWFDEC_RENDER_H__
#define __SWFDEC_RENDER_H__

#include "config.h"

#include "swfdec_types.h"
#include "swfdec_transform.h"
#include <swfdec_rect.h>

G_BEGIN_DECLS

struct swfdec_render_struct
{
  int frame_index;
  SwfdecRect drawrect;

  GList *object_states;

  int mouse_check;
  int mouse_in_button;
  SwfdecObject *active_button;
};

SwfdecRender *swfdec_render_new (void);
void swfdec_render_free (SwfdecRender * render);
gboolean swfdec_render_iterate (SwfdecDecoder * s);
void swfdec_render_seek (SwfdecDecoder * s, int frame);
SwfdecBuffer *swfdec_render_get_image (SwfdecDecoder * s);
SwfdecBuffer *swfdec_render_get_audio (SwfdecDecoder * s);
int swfdec_render_get_frame_index (SwfdecDecoder * s);
void swfdec_render_resize (SwfdecDecoder *s);

/* functions implemented by rendering backends */

void swfdec_render_be_start (SwfdecDecoder *s);
void swfdec_render_be_clear (SwfdecDecoder *s);
void swfdec_render_be_stop (SwfdecDecoder *s);
void swf_render_frame (SwfdecDecoder * s, int frame_index);
void swfdec_layervec_render (SwfdecDecoder * s, SwfdecLayerVec * layervec);
void swfdec_shape_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * obj);
void swfdec_text_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object);
void swf_config_colorspace (SwfdecDecoder * s);
void swfdec_render_layervec_free (SwfdecLayerVec * layervec);

G_END_DECLS

#endif
