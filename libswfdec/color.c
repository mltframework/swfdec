
#include <math.h>

#include "swfdec_internal.h"

unsigned int
transform_color (unsigned int in, double mult[4], double add[4])
{
  int r, g, b, a;

  r = SWF_COLOR_R (in);
  g = SWF_COLOR_G (in);
  b = SWF_COLOR_B (in);
  a = SWF_COLOR_A (in);

  SWFDEC_LOG ("in rgba %d,%d,%d,%d",r,g,b,a);

  r = rint ((r * mult[0] + add[0]));
  g = rint ((g * mult[1] + add[1]));
  b = rint ((b * mult[2] + add[2]));
  a = rint ((a * mult[3] + add[3]));

  r = CLAMP (r, 0, 255);
  g = CLAMP (g, 0, 255);
  b = CLAMP (b, 0, 255);
  a = CLAMP (a, 0, 255);

  SWFDEC_LOG("out rgba %d,%d,%d,%d",r,g,b,a);

  return SWF_COLOR_COMBINE (r, g, b, a);
}

void
swf_config_colorspace (SwfdecDecoder * s)
{
  switch (s->colorspace) {
    case SWF_COLORSPACE_RGB565:
      s->stride = s->width * 2;
      s->bytespp = 2;
      s->callback = art_rgb565_svp_alpha_callback;
      s->compose_callback = art_rgb565_svp_alpha_callback;
      break;
    case SWF_COLORSPACE_RGB888:
    default:
      s->stride = s->width * 4;
      s->bytespp = 4;
      s->callback = art_rgb_svp_alpha_callback;
      s->compose_callback = art_rgb_svp_alpha_compose_callback;
      break;
  }
}

int
tag_func_set_background_color (SwfdecDecoder * s)
{
  ArtIRect rect;

  s->bg_color = swfdec_bits_get_color (&s->b);

  rect.x0 = 0;
  rect.y0 = 0;
  rect.x1 = s->width;
  rect.y1 = s->height;

  swf_invalidate_irect (s, &rect);

  return SWF_OK;
}
