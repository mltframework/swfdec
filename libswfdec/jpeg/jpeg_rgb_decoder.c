
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "jpeg_rgb_internal.h"
#include "jpeg.h"


static void convert (JpegRGBDecoder * rgbdec);

static void imagescale2h_u8 (unsigned char *dest, int d_rowstride,
    unsigned char *src, int src_rowstride, int width, int height);
static void imagescale2v_u8 (unsigned char *dest, int d_rowstride,
    unsigned char *src, int src_rowstride, int width, int height);
static void imagescale2h2v_u8 (unsigned char *dest, int d_rowstride,
    unsigned char *src, int src_rowstride, int width, int height);
static void scanlinescale2_u8 (unsigned char *dest, unsigned char *src,
    int len);


JpegRGBDecoder *
jpeg_rgb_decoder_new (void)
{
  JpegRGBDecoder *rgbdec;

  rgbdec = g_new0 (JpegRGBDecoder, 1);

  rgbdec->dec = jpeg_decoder_new ();

  return rgbdec;
}

void
jpeg_rgb_decoder_free (JpegRGBDecoder * rgbdec)
{
  int i;

  jpeg_decoder_free (rgbdec->dec);

  for (i = 0; i < 3; i++) {
    if (rgbdec->component[i].alloc) {
      g_free (rgbdec->component[i].image);
    }
  }

  g_free (rgbdec);
}

int
jpeg_rgb_decoder_addbits (JpegRGBDecoder * dec, unsigned char *data,
    unsigned int len)
{
  return jpeg_decoder_addbits (dec->dec, data, len);
}

int
jpeg_rgb_decoder_parse (JpegRGBDecoder * dec)
{
  return jpeg_decoder_parse (dec->dec);
}

int
jpeg_rgb_decoder_get_image (JpegRGBDecoder * rgbdec,
    unsigned char **image, int *rowstride, int *width, int *height)
{
  int i;

  jpeg_decoder_get_image_size (rgbdec->dec, &rgbdec->width, &rgbdec->height);
  for (i = 0; i < 3; i++) {
    jpeg_decoder_get_component_ptr (rgbdec->dec, i + 1,
        &rgbdec->component[i].image, &rgbdec->component[i].rowstride);
    jpeg_decoder_get_component_subsampling (rgbdec->dec, i + 1,
        &rgbdec->component[i].h_subsample, &rgbdec->component[i].v_subsample);
    rgbdec->component[i].alloc = 0;
    if (rgbdec->component[i].h_subsample > 1 ||
        rgbdec->component[i].v_subsample > 1) {
      unsigned char *dest;

      dest = g_malloc (rgbdec->width * rgbdec->height);
      if (rgbdec->component[i].v_subsample > 1) {
        if (rgbdec->component[i].h_subsample > 1) {
          imagescale2h2v_u8 (dest,
              rgbdec->width,
              rgbdec->component[i].image,
              rgbdec->component[i].rowstride, rgbdec->width, rgbdec->height);
        } else {
          imagescale2v_u8 (dest,
              rgbdec->width,
              rgbdec->component[i].image,
              rgbdec->component[i].rowstride, rgbdec->width, rgbdec->height);
        }
      } else {
        imagescale2h_u8 (dest,
            rgbdec->width,
            rgbdec->component[i].image,
            rgbdec->component[i].rowstride, rgbdec->width, rgbdec->height);
      }
      rgbdec->component[i].alloc = 1;
      rgbdec->component[i].image = dest;
      rgbdec->component[i].rowstride = rgbdec->width;
      rgbdec->component[i].h_subsample = 1;
      rgbdec->component[i].v_subsample = 1;
    }
  }

  rgbdec->image = g_malloc (rgbdec->width * rgbdec->height * 4);

  convert (rgbdec);

  if (image)
    *image = rgbdec->image;
  if (rowstride)
    *rowstride = rgbdec->width * 4;
  if (width)
    *width = rgbdec->width;
  if (height)
    *height = rgbdec->height;

  return 0;
}


/* from the JFIF spec */
#if 0
#define CONVERT(r, g, b, y, u, v) G_STMT_START{ \
  (r) = CLAMP((y) + 1.402*((v)-128),0,255); \
  (g) = CLAMP((y) - 0.34414*((u)-128) - 0.71414*((v)-128),0,255); \
  (b) = CLAMP((y) + 1.772*((u)-128),0,255); \
}G_STMT_END
#else
#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)	  ((int) ((x) * (1<<SCALEBITS) + 0.5))
#define CONVERT(r, g, b, y, u, v) G_STMT_START{ \
    int cb = (u) - 128;\
    int cr = (v) - 128;\
    int cy = (y) << SCALEBITS;\
    int r_add = cy + FIX(1.40200) * cr + ONE_HALF;\
    int g_add = cy - FIX(0.34414) * cb - FIX(0.71414) * cr + ONE_HALF;\
    int b_add = cy + FIX(1.77200) * cb + ONE_HALF;\
    r_add = CLAMP (r_add, 0, 255 << SCALEBITS);\
    g_add = CLAMP (g_add, 0, 255 << SCALEBITS);\
    b_add = CLAMP (b_add, 0, 255 << SCALEBITS);\
    r = r_add >> SCALEBITS; \
    g = g_add >> SCALEBITS; \
    b = b_add >> SCALEBITS; \
}G_STMT_END

#endif

static void
convert (JpegRGBDecoder * rgbdec)
{
  int x, y;
  unsigned char *yp;
  unsigned char *up;
  unsigned char *vp;
  unsigned char *rgbp;

  rgbp = rgbdec->image;
  yp = rgbdec->component[0].image;
  up = rgbdec->component[1].image;
  vp = rgbdec->component[2].image;
  for (y = 0; y < rgbdec->height; y++) {
    for (x = 0; x < rgbdec->width; x++) {
#if G_LITTLE_ENDIAN == G_BYTE_ORDER
      CONVERT (rgbp[2], rgbp[1], rgbp[0], yp[x], up[x], vp[x]);
      rgbp[3] = 0xFF;
#else
      rgbp[0] = 0xFF;
      CONVERT (rgbp[1], rgbp[2], rgbp[3], yp[x], up[x], vp[x]);
#endif
      rgbp += 4;
    }
    yp += rgbdec->component[0].rowstride;
    up += rgbdec->component[1].rowstride;
    vp += rgbdec->component[2].rowstride;
  }
}


static void
imagescale2h_u8 (unsigned char *dest, int d_rowstride,
    unsigned char *src, int s_rowstride, int width, int height)
{
  int i;

  for (i = 0; i < height; i++) {
    scanlinescale2_u8 (dest + i * d_rowstride, src + i * s_rowstride, width);
  }

}

static void
imagescale2v_u8 (unsigned char *dest, int d_rowstride,
    unsigned char *src, int s_rowstride, int width, int height)
{
  int i;

  for (i = 0; i < height; i++) {
    memcpy (dest + i * d_rowstride, src + (i / 2) * s_rowstride, width);
  }

}

static void
imagescale2h2v_u8 (unsigned char *dest, int d_rowstride,
    unsigned char *src, int s_rowstride, int width, int height)
{
  int i;

  for (i = 0; i < height; i++) {
    scanlinescale2_u8 (dest + i * d_rowstride,
        src + (i / 2) * s_rowstride, width);
  }

}

static void
scanlinescale2_u8 (unsigned char *dest, unsigned char *src, int len)
{
  int i;

  for (i = 0; i < len; i++) {
    dest[i] = src[i / 2];
  }
}
