
#ifndef _ART_H_
#define _ART_H_

#include <libart_lgpl/libart.h>
#include <stdio.h>
#include <string.h>
#include <swfdec_shape.h>

struct swf_svp_render_struct
{
  unsigned char *scanline;
  unsigned int color;
  unsigned char *buf;
  int rowstride;
  int x0, x1;
  unsigned char *compose;
  int compose_rowstride;
  int compose_y;
  int compose_height;
  int compose_width;
};

ArtVpath *art_vpath_cat (ArtVpath * a, ArtVpath * b);
ArtVpath *art_vpath_reverse (ArtVpath * a);
ArtVpath *art_vpath_reverse_free (ArtVpath * a);
void art_svp_make_convex (ArtSVP * svp);
void art_rgb_svp_alpha2 (const ArtSVP * svp, int x0, int y0,
    int x1, int y1, art_u32 rgba, art_u8 * buf, int rowstride,
    ArtAlphaGamma * alphagamma);
void art_rgb_fill_run (unsigned char *buf, unsigned char r,
    unsigned char g, unsigned char b, int n);
void art_rgb_run_alpha (unsigned char *buf, unsigned char r,
    unsigned char g, unsigned char b, int alpha, int n);
void art_rgb565_run_alpha (unsigned char *buf, unsigned char r,
    unsigned char g, unsigned char b, int alpha, int n);
void art_rgb565_fill_run (unsigned char *buf, unsigned char r,
    unsigned char g, unsigned char b, int n);
void art_rgb565_fillrect (unsigned char *buffer, int stride, unsigned int color,
    SwfdecRect * rect);
void art_rgb_fillrect (unsigned char *buffer, int stride, unsigned int color,
    SwfdecRect * rect);
void art_rgb_render_callback (void *data, int y, int start,
    ArtSVPRenderAAStep * steps, int n_steps);
void art_rgb565_svp_alpha_callback (void *callback_data, int y,
    int start, ArtSVPRenderAAStep * steps, int n_steps);
void art_rgb_svp_alpha_callback (void *callback_data, int y,
    int start, ArtSVPRenderAAStep * steps, int n_steps);
void art_rgb_svp_alpha_compose_callback (void *callback_data, int y,
    int start, ArtSVPRenderAAStep * steps, int n_steps);
void art_grey_svp_alpha_callback (void *callback_data, int y,
    int start, ArtSVPRenderAAStep * steps, int n_steps);

ArtBpath *swfdec_art_bpath_from_points (GArray * array,
    SwfdecTransform * trans);
void art_bpath_affine_transform_inplace (ArtBpath * bpath,
    SwfdecTransform * trans);

static inline void
art_affine_copy (double dst[6], const double src[6])
{
  memcpy (dst, src, sizeof (double) * 6);
}

#endif
