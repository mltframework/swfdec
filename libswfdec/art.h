
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

void art_irect_union_to_masked (ArtIRect * rect, ArtIRect * a, ArtIRect * mask);
int art_affine_inverted (double x[6]);
void art_vpath_dump (FILE * out, ArtVpath * vpath);
void art_bpath_dump (FILE * out, ArtBpath * vpath);
int art_bpath_len (ArtBpath * a);
int art_vpath_len (ArtVpath * a);
ArtBpath *art_bpath_cat (ArtBpath * a, ArtBpath * b);
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
void art_rgb565_fillrect (char *buffer, int stride, unsigned int color,
    ArtIRect * rect);
void art_rgb_fillrect (char *buffer, int stride, unsigned int color,
    ArtIRect * rect);
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

ArtBpath *swfdec_art_bpath_from_points (GArray *array, const double src[6]);

static inline void
art_affine_copy (double dst[6], const double src[6])
{
  memcpy (dst, src, sizeof (double) * 6);
}

#endif
