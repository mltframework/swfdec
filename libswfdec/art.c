
#include "swfdec_internal.h"
#include "art.h"

#include <liboil/liboil.h>
#include <libart_lgpl/libart.h>
#include <string.h>

/*
 * This file defines some libart-related functions that aren't
 * in libart.
 *
 */
static inline void art_grey_run_alpha (unsigned char *buf, int alpha, int n);

#ifdef unused
static int
art_bpath_len (ArtBpath * a)
{
  int i;

  for (i = 0; a[i].code != ART_END; i++);
  return i;
}
#endif

static int
art_vpath_len (ArtVpath * a)
{
  int i;

  for (i = 0; a[i].code != ART_END; i++);
  return i;
}

#ifdef unused
ArtBpath *
art_bpath_cat (ArtBpath * a, ArtBpath * b)
{
  ArtBpath *dest;
  int len_a, len_b;

  len_a = art_bpath_len (a);
  len_b = art_bpath_len (b);
  dest = g_malloc ((len_a + len_b + 1) * sizeof (ArtBpath));

  oil_copy_u8 (dest, a, sizeof (ArtBpath) * len_a);
  oil_copy_u8 (dest + len_a, b, sizeof (ArtBpath) * (len_b + 1));

  return dest;
}
#endif

ArtVpath *
art_vpath_cat (ArtVpath * a, ArtVpath * b)
{
  ArtVpath *dest;
  int len_a, len_b;

  len_a = art_vpath_len (a);
  len_b = art_vpath_len (b);
  dest = g_malloc ((len_a + len_b + 1) * sizeof (ArtVpath));

  oil_copy_u8 ((void *)dest, (void *)a, sizeof (ArtVpath) * len_a);
  oil_copy_u8 ((void *)(dest + len_a), (void *)b,
      sizeof (ArtVpath) * (len_b + 1));

  return dest;
}

ArtVpath *
art_vpath_reverse (ArtVpath * a)
{
  ArtVpath *dest;
  ArtVpath it;
  int len;
  int state = 0;
  int i;

  len = art_vpath_len (a);
  dest = g_malloc ((len + 1) * sizeof (ArtVpath));

  for (i = 0; i < len; i++) {
    it = a[len - i - 1];
    if (state) {
      it.code = ART_LINETO;
    } else {
      it.code = ART_MOVETO_OPEN;
      state = 1;
    }
    if (a[len - i - 1].code == ART_MOVETO ||
        a[len - i - 1].code == ART_MOVETO_OPEN) {
      state = 0;
    }
    dest[i] = it;
  }
  dest[len] = a[len];

  return dest;
}

ArtVpath *
art_vpath_reverse_free (ArtVpath * a)
{
  ArtVpath *dest;

  dest = art_vpath_reverse (a);
  art_free (a);

  return dest;
}

void
art_svp_make_convex (ArtSVP * svp)
{
  int i;

  if (svp->n_segs > 0 && svp->segs[0].dir == 0) {
    for (i = 0; i < svp->n_segs; i++) {
      svp->segs[i].dir = !svp->segs[i].dir;
    }
  }
}

void
art_rgb_svp_alpha2 (const ArtSVP * svp, int x0, int y0,
    int x1, int y1, art_u32 rgba, art_u8 * buf, int rowstride,
    ArtAlphaGamma * alphagamma)
{
  ArtUta *uta;
  int i, x, y;

  uta = art_uta_from_svp (svp);

  for (y = 0; y < uta->width; y++) {
    if ((uta->y0 + y) * 32 < y0 || (uta->y0 + y + 1) * 32 > y1)
      continue;

    for (x = 0; x < uta->width; x++) {
      i = y * uta->width + x;
      if ((uta->x0 + x) * 32 < x0 || (uta->x0 + x + 1) * 32 > x1)
        continue;
      if (uta->utiles[i]) {
        art_rgb_svp_alpha (svp,
            (uta->x0 + x) * 32,
            (uta->y0 + y) * 32,
            (uta->x0 + x) * 32 + 32,
            (uta->y0 + y) * 32 + 32,
            rgba,
            buf + rowstride * (uta->y0 + y) * 32 +
            (uta->x0 + x) * 32 * 4, rowstride, alphagamma);
      }
    }
  }

  art_uta_free (uta);
}

void
art_rgb_fill_run (unsigned char *buf, unsigned char r,
    unsigned char g, unsigned char b, int n)
{
  int i;

  for (i = 0; i < n; i++) {
    *buf++ = b;
    *buf++ = g;
    *buf++ = r;
    *buf++ = 0;
  }
}

void
art_rgb_run_alpha_2 (unsigned char *buf, unsigned char r,
    unsigned char g, unsigned char b, int alpha, int n)
{
  int i;
  int add_r, add_g, add_b, unalpha;

  if (alpha == 0)
    return;
  if (alpha >= 0xff) {
    char color[4];

    color[0] = b;
    color[1] = g;
    color[2] = r;
    color[3] = 0;

    oil_splat_u32 ((void *) buf, 4, (void *) color, n);
    return;
  }
#define APPLY_ALPHA(x,y,a) (x) = (((y)*(alpha)+(x)*(255-alpha))>>8)
  unalpha = 255 - alpha;
  add_r = r * alpha + 0x80;
  add_g = g * alpha + 0x80;
  add_b = b * alpha + 0x80;
  for (i = 0; i < n; i++) {
    *buf = (add_b + unalpha * (*buf)) >> 8;
    buf++;
    *buf = (add_g + unalpha * (*buf)) >> 8;
    buf++;
    *buf = (add_r + unalpha * (*buf)) >> 8;
    buf++;
    *buf = 0;
    buf++;
  }
}

void
art_rgb565_run_alpha (unsigned char *buf, unsigned char r,
    unsigned char g, unsigned char b, int alpha, int n)
{
  int i;
  unsigned short *x = (void *) buf;
  unsigned short c;
  int rr, gg, bb;
  int ar, ag, ab;
  int unalpha;

  if (alpha == 0)
    return;
  if (alpha >= 0xff) {
    c = RGB565_COMBINE (r, g, b);
    for (i = 0; i < n; i++) {
      *x++ = c;
    }
    return;
  }
  unalpha = 255 - alpha;
  ar = r * alpha + 0x80;
  ag = g * alpha + 0x80;
  ab = b * alpha + 0x80;
  for (i = 0; i < n; i++) {
    rr = (RGB565_R (*x) * unalpha + ar) >> 8;
    gg = (RGB565_G (*x) * unalpha + ag) >> 8;
    bb = (RGB565_B (*x) * unalpha + ab) >> 8;
    *x = RGB565_COMBINE (rr, gg, bb);
    x++;
  }
}

void
art_rgb565_fill_run (unsigned char *buf, unsigned char r,
    unsigned char g, unsigned char b, int n)
{
  int i;
  unsigned short *x = (void *) buf;
  unsigned short c = RGB565_COMBINE (r, g, b);

  for (i = 0; i < n; i++) {
    *x++ = c;
  }
}

void
art_rgb565_fillrect (unsigned char *buffer, int stride, unsigned int color,
    SwfdecRect * rect)
{
  int i;

  buffer += rect->x0 * 2;
  for (i = rect->y0; i < rect->y1; i++) {
    art_rgb565_run_alpha (buffer + i * stride,
        SWF_COLOR_R (color),
        SWF_COLOR_G (color),
        SWF_COLOR_B (color), SWF_COLOR_A (color), rect->x1 - rect->x0);
  }
}

void
art_rgb_fillrect (unsigned char *buffer, int stride, unsigned int color,
    SwfdecRect * rect)
{
  int i;

  buffer += rect->x0 * 4;
  for (i = rect->y0; i < rect->y1; i++) {
    art_rgb_run_alpha_2 (buffer + i * stride,
        SWF_COLOR_R (color),
        SWF_COLOR_G (color),
        SWF_COLOR_B (color), SWF_COLOR_A (color), rect->x1 - rect->x0);
  }
}

void
art_rgb_render_callback (void *data, int y, int start,
    ArtSVPRenderAAStep * steps, int n_steps)
{


}


/* taken from:
 * Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
 */

void
art_rgb565_svp_alpha_callback (void *callback_data, int y,
    int start, ArtSVPRenderAAStep * steps, int n_steps)
{
  struct swf_svp_render_struct *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  art_u32 running_sum = start;
  int x0, x1;
  int k;
  art_u8 r, g, b, a;
  int alpha;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  r = SWF_COLOR_R (data->color);
  g = SWF_COLOR_G (data->color);
  b = SWF_COLOR_B (data->color);
  a = SWF_COLOR_A (data->color);

  if (n_steps > 0) {
    run_x1 = steps[0].x;
    if (run_x1 > x0) {
      alpha = (a * running_sum >> 8) >> 16;
      if (alpha)
        art_rgb565_run_alpha (linebuf, r, g, b, alpha, run_x1 - x0);
    }

    for (k = 0; k < n_steps - 1; k++) {
      running_sum += steps[k].delta;
      run_x0 = run_x1;
      run_x1 = steps[k + 1].x;
      if (run_x1 > run_x0) {
        alpha = (a * (running_sum >> 8)) >> 16;
        if (alpha)
          art_rgb565_run_alpha (linebuf + (run_x0 - x0) * 2,
              r, g, b, alpha, run_x1 - run_x0);
      }
    }
    running_sum += steps[k].delta;
    if (x1 > run_x1) {
      alpha = (a * (running_sum >> 8)) >> 16;
      if (alpha)
        art_rgb565_run_alpha (linebuf + (run_x1 - x0) * 2,
            r, g, b, alpha, x1 - run_x1);
    }
  } else {
    alpha = (a * (running_sum >> 8)) >> 16;
    if (alpha)
      art_rgb565_run_alpha (linebuf, r, g, b, alpha, x1 - x0);
  }

  data->buf += data->rowstride;
}

#define COMPOSE(a, b, x) (((a)*(255 - (x)) + ((b)*(x)))>>8)
#define compose_const_rgb888_u8 compose_const_rgb888_u8_fast

void
compose_const_rgb888_u8_ref (unsigned char *dest, unsigned char *src,
    unsigned int color, int n)
{
  int r, g, b;
  int i;

  r = SWF_COLOR_R (color);
  g = SWF_COLOR_G (color);
  b = SWF_COLOR_B (color);

  for (i = 0; i < n; i++) {
    dest[0] = COMPOSE (dest[2], b, src[0]);
    dest[1] = COMPOSE (dest[1], g, src[0]);
    dest[2] = COMPOSE (dest[0], r, src[0]);
    dest[2] = 0;
    dest += 4;
    src++;
  }
}

void
compose_const_rgb888_u8_fast (unsigned char *dest, unsigned char *src,
    unsigned int color, int n)
{
  unsigned int r, g, b;
  unsigned int un_a, a;
  int i;

  r = SWF_COLOR_R (color);
  g = SWF_COLOR_G (color);
  b = SWF_COLOR_B (color);

  for (i = 0; i < n; i++) {
    a = src[0];
    if (a == 0) {
    } else if (a == 255) {
      dest[0] = b;
      dest[1] = g;
      dest[2] = r;
      dest[3] = 0;
    } else {
      un_a = 255 - a;
      dest[0] = (un_a * dest[0] + a * b) >> 8;
      dest[1] = (un_a * dest[1] + a * g) >> 8;
      dest[2] = (un_a * dest[2] + a * r) >> 8;
      dest[3] = 0;
    }
    dest += 4;
    src++;
  }
}

#define compose_rgb888_u8 compose_rgb888_u8_ref
void
compose_rgb888_u8_ref (unsigned char *dest, unsigned char *a_src,
    unsigned char *src, int n)
{
  int i;
  int a;

  for (i = 0; i < n; i++) {
    a = (a_src[0] * src[3] + 255) >> 8;
    dest[0] = COMPOSE (dest[0], src[0], a);
    dest[1] = COMPOSE (dest[1], src[1], a);
    dest[2] = COMPOSE (dest[2], src[2], a);
    dest[3] = 0;
    dest += 4;
    src += 4;
    a_src++;
  }
}

#define compose_const_rgb888_rgb888 compose_const_rgb888_rgb888_ref
void
compose_const_rgb888_rgb888_ref (unsigned char *dest, unsigned char *src,
    unsigned int color, int n)
{
  int r, g, b;
  int i;

  r = SWF_COLOR_R (color);
  g = SWF_COLOR_G (color);
  b = SWF_COLOR_B (color);

  for (i = 0; i < n; i++) {
    dest[0] = COMPOSE (dest[0], b, src[0]);
    dest[1] = COMPOSE (dest[1], g, src[1]);
    dest[2] = COMPOSE (dest[2], r, src[2]);
    dest[3] = 0;
    dest += 4;
    src += 4;
  }
}

#define compose_rgb888_rgb888 compose_rgb888_rgb888_ref
void
compose_rgb888_rgb888_ref (unsigned char *dest, unsigned char *a_src,
    unsigned char *src, int n)
{
  int i;
  int a;

  for (i = 0; i < n; i++) {
    a = a_src[0];
    dest[0] = COMPOSE (dest[0], src[0], a_src[0]);
    dest[1] = COMPOSE (dest[1], src[1], a_src[1]);
    dest[2] = COMPOSE (dest[2], src[2], a_src[2]);
    dest[3] = 0;
    dest += 4;
    src += 4;
    a_src += 4;
  }
}

void
art_rgb_svp_alpha_compose_callback (void *callback_data, int y,
    int start, ArtSVPRenderAAStep * steps, int n_steps)
{
  struct swf_svp_render_struct *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  art_u32 running_sum = start;
  int x0, x1;
  int k;
  int alpha;
  int a;

  a = SWF_COLOR_A (data->color);
  linebuf = data->scanline;
  x0 = data->x0;
  x1 = data->x1;

  if (n_steps > 0) {
    run_x1 = steps[0].x;
    if (run_x1 > x0) {
      alpha = (a * (running_sum >> 8)) >> 16;
      art_grey_run_alpha (linebuf, alpha, run_x1 - x0);
    }

    for (k = 0; k < n_steps - 1; k++) {
      running_sum += steps[k].delta;
      run_x0 = run_x1;
      run_x1 = steps[k + 1].x;
      if (run_x1 > run_x0) {
        alpha = (a * (running_sum >> 8)) >> 16;
        art_grey_run_alpha (linebuf + (run_x0 - x0), alpha, run_x1 - run_x0);
      }
    }
    running_sum += steps[k].delta;
    if (x1 > run_x1) {
      alpha = (a * (running_sum >> 8)) >> 16;
      art_grey_run_alpha (linebuf + (run_x1 - x0), alpha, x1 - run_x1);
    }
  } else {
    alpha = (a * (running_sum >> 8)) >> 16;
    art_grey_run_alpha (linebuf, alpha, x1 - x0);
  }

  compose_rgb888_u8 (data->buf, linebuf,
      data->compose + data->compose_y * data->compose_rowstride,
      data->x1 - data->x0);
  data->compose_y++;

  data->buf += data->rowstride;
}

#if 0
#define imult(a,b) (((a)*(b) + (((a)*(b)) >> 8))>>8)
#define apply(a,b,c) (imult(a,255-c) + imult(b,c))

static void
paint (uint8_t *dest, uint8_t *color, uint8_t *alpha, int n)
{
  int i;

  for(i=0;i<n;i++){
    if (alpha[0] == 0) {
    } else if (alpha[0] == 0xff) {
      dest[0] = color[1];
      dest[1] = color[2];
      dest[2] = color[3];
      dest[3] = color[0];
    } else {
      dest[0] = apply(dest[0],color[1],alpha[0]);
      dest[1] = apply(dest[1],color[2],alpha[0]);
      dest[2] = apply(dest[2],color[3],alpha[0]);
      dest[3] = apply(dest[3],color[0],alpha[0]);
    }
    dest+=4;
    alpha++;
  }
}
#endif

void
art_rgb_svp_alpha_callback (void *callback_data, int y,
    int start, ArtSVPRenderAAStep * steps, int n_steps)
{
  struct swf_svp_render_struct *data = callback_data;
  art_u8 *linebuf;
  art_u32 running_sum = start;
  int x0, x1;
  int k;
  uint8_t color[4];
  int alpha;
  unsigned char alphabuf[1000];
  int x;
  int a;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  color[0] = SWF_COLOR_B (data->color);
  color[1] = SWF_COLOR_G (data->color);
  color[2] = SWF_COLOR_R (data->color);
  color[3] = SWF_COLOR_A (data->color);
  a = SWF_COLOR_A (data->color);

  x = 0;
  if (n_steps > 0) {
    alpha = (a * (running_sum >> 8)) >> 16;
    while(x < steps[0].x) {
      alphabuf[x] = alpha;
      x++;
    }

    for (k = 0; k < n_steps - 1; k++) {
      running_sum += steps[k].delta;
      alpha = (a * (running_sum >> 8)) >> 16;
      while(x < steps[k+1].x) {
        alphabuf[x] = alpha;
        x++;
      }
    }
    running_sum += steps[k].delta;
    alpha = (a * (running_sum >> 8)) >> 16;
    while(x < x1) {
      alphabuf[x] = alpha;
      x++;
    }

    x=x0;
    while(alphabuf[x]==0 && x < x1) {
      x++;
    }
    oil_argb_paint_u8 (data->buf + 4*(x-x0), color, alphabuf + x, x1 - x);
  } else {
    alpha = (a * (running_sum >> 8)) >> 16;
    if (alpha)
      art_rgb_run_alpha_2 (linebuf, SWF_COLOR_R (data->color),
          SWF_COLOR_G (data->color), SWF_COLOR_B (data->color),
          alpha, x1 - x0);
  }

  data->buf += data->rowstride;
}

static inline void
art_grey_run_alpha (unsigned char *buf, int alpha, int n)
{
  if (alpha > 0xff)
    alpha = 0xff;
  memset (buf, alpha, n);
}

void
art_grey_svp_alpha_callback (void *callback_data, int y,
    int start, ArtSVPRenderAAStep * steps, int n_steps)
{
  struct swf_svp_render_struct *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  art_u32 running_sum = start;
  int x0, x1;
  int k;
  int alpha;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  if (n_steps > 0) {
    run_x1 = steps[0].x;
    if (run_x1 > x0) {
      alpha = running_sum >> 16;
      art_grey_run_alpha (linebuf, alpha, run_x1 - x0);
    }

    for (k = 0; k < n_steps - 1; k++) {
      running_sum += steps[k].delta;
      run_x0 = run_x1;
      run_x1 = steps[k + 1].x;
      if (run_x1 > run_x0) {
        alpha = running_sum >> 16;
        art_grey_run_alpha (linebuf + (run_x0 - x0), alpha, run_x1 - run_x0);
      }
    }
    running_sum += steps[k].delta;
    if (x1 > run_x1) {
      alpha = running_sum >> 16;
      if (alpha)
        art_grey_run_alpha (linebuf + (run_x1 - x0), alpha, x1 - run_x1);
    }
  } else {
    alpha = running_sum >> 16;
    art_grey_run_alpha (linebuf, alpha, x1 - x0);
  }

  data->buf += data->rowstride;
}

#define WEIGHT (2.0/3.0)

ArtBpath *
swfdec_art_bpath_from_points (GArray * array, SwfdecTransform * trans)
{
  int i;
  ArtBpath *bpath;
  SwfdecShapePoint *points = (SwfdecShapePoint *) array->data;

  bpath = g_malloc (sizeof (ArtBpath) * (array->len + 1));
  for (i = 0; i < array->len; i++) {
    if (points[i].control_x == SWFDEC_SHAPE_POINT_SPECIAL) {
      if (points[i].control_y == SWFDEC_SHAPE_POINT_MOVETO) {
        bpath[i].code = ART_MOVETO_OPEN;
      } else {
        bpath[i].code = ART_LINETO;
      }
      bpath[i].x3 = points[i].to_x * SWF_SCALE_FACTOR;
      bpath[i].y3 = points[i].to_y * SWF_SCALE_FACTOR;
    } else {
      double x, y;

      bpath[i].code = ART_CURVETO;
      x = points[i].control_x * SWF_SCALE_FACTOR;
      y = points[i].control_y * SWF_SCALE_FACTOR;
      bpath[i].x3 = points[i].to_x * SWF_SCALE_FACTOR;
      bpath[i].y3 = points[i].to_y * SWF_SCALE_FACTOR;
      if (i > 0) {
        bpath[i].x1 = WEIGHT * x + (1 - WEIGHT) * bpath[i - 1].x3;
        bpath[i].y1 = WEIGHT * y + (1 - WEIGHT) * bpath[i - 1].y3;
      } else {
        g_assert_not_reached();
      }
      bpath[i].x2 = WEIGHT * x + (1 - WEIGHT) * bpath[i].x3;
      bpath[i].y2 = WEIGHT * y + (1 - WEIGHT) * bpath[i].y3;
    }
  }

  bpath[i].code = ART_END;

  art_bpath_affine_transform_inplace (bpath, trans);

  return bpath;
}

#define apply_affine(x,y,affine) do { \
  double _x = (x); \
  double _y = (y); \
  (x) = _x * (affine)[0] + _y * (affine)[2] + (affine)[4]; \
  (y) = _x * (affine)[1] + _y * (affine)[3] + (affine)[5]; \
}while(0)

void
art_bpath_affine_transform_inplace (ArtBpath * bpath, SwfdecTransform * trans)
{
  int i;

  for (i = 0; bpath[i].code != ART_END; i++) {
    apply_affine (bpath[i].x3, bpath[i].y3, trans->trans);
    if (bpath[i].code == ART_CURVETO) {
      apply_affine (bpath[i].x1, bpath[i].y1, trans->trans);
      apply_affine (bpath[i].x2, bpath[i].y2, trans->trans);
    }
  }
}
