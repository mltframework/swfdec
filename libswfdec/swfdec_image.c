/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <zlib.h>
#include <string.h>

#include "jpeg_rgb_decoder.h"
#include "swfdec_image.h"
#include "swfdec_cache.h"
#include "swfdec_debug.h"
#include "swfdec_swf_decoder.h"


static void merge_alpha (SwfdecImage * image, unsigned char *image_data,
    unsigned char *alpha);
static void swfdec_image_colormap_decode (SwfdecImage * image,
    unsigned char *dest,
    unsigned char *src, unsigned char *colormap, int colormap_len);

G_DEFINE_TYPE (SwfdecImage, swfdec_image, SWFDEC_TYPE_CACHED)

static void
swfdec_image_unload (SwfdecCached *cached)
{
  SwfdecImage *image = SWFDEC_IMAGE (cached);

  if (image->surface) {
    cairo_surface_destroy (image->surface);
    image->surface = NULL;
  } else if (image->data) {
    g_free (image->data);
  }
  image->data = NULL;
}

static void
swfdec_image_dispose (GObject *object)
{
  SwfdecImage * image = SWFDEC_IMAGE (object);

  if (image->jpegtables) {
    swfdec_buffer_unref (image->jpegtables);
    image->jpegtables = NULL;
  }
  if (image->raw_data) {
    swfdec_buffer_unref (image->raw_data);
    image->raw_data = NULL;
  }

  G_OBJECT_CLASS (swfdec_image_parent_class)->dispose (object);
}

static void
swfdec_image_class_init (SwfdecImageClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecCachedClass *cached_class = SWFDEC_CACHED_CLASS (g_class);

  object_class->dispose = swfdec_image_dispose;

  cached_class->unload = swfdec_image_unload;
}

static void
swfdec_image_init (SwfdecImage * image)
{
}


static void *
zalloc (void *opaque, unsigned int items, unsigned int size)
{
  return g_malloc (items * size);
}

static void
zfree (void *opaque, void *addr)
{
  g_free (addr);
}

static void *
lossless (void *zptr, int zlen, int *plen)
{
  void *data;
  int len;
  z_stream *z;
  int ret;

  z = g_new0 (z_stream, 1);
  z->zalloc = zalloc;
  z->zfree = zfree;
  z->opaque = NULL;

  z->next_in = zptr;
  z->avail_in = zlen;

  data = NULL;
  len = 0;
  ret = inflateInit (z);
  while (z->avail_in > 0) {
    if (z->avail_out == 0) {
      len += 1024;
      data = g_realloc (data, len);
      z->next_out = data + z->total_out;
      z->avail_out += 1024;
    }
    ret = inflate (z, Z_SYNC_FLUSH);
    if (ret != Z_OK)
      break;
  }
  if (ret != Z_STREAM_END) {
    SWFDEC_WARNING ("lossless: ret == %d", ret);
  }

  if (plen)
    (*plen) = z->total_out;

  inflateEnd (z);

  g_free (z);
  return data;
}


int
swfdec_image_jpegtables (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;

  SWFDEC_DEBUG ("swfdec_image_jpegtables");

  s->jpegtables = swfdec_buffer_ref (bits->buffer);
  bits->ptr += bits->buffer->length;

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_bits_jpeg (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecImage *image;

  SWFDEC_LOG ("tag_func_define_bits_jpeg");
  id = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_IMAGE);
  if (!image)
    return SWFDEC_STATUS_OK;

  image->type = SWFDEC_IMAGE_TYPE_JPEG;
  if (!s->jpegtables) {
    SWFDEC_ERROR("No global JPEG tables available");
  } else {
    image->jpegtables = swfdec_buffer_ref (s->jpegtables);
  }
  image->raw_data = swfdec_buffer_ref (bits->buffer);

  bits->ptr += bits->buffer->length - 2;

  return SWFDEC_STATUS_OK;
}

static void
swfdec_image_jpeg_load (SwfdecImage *image)
{
  JpegRGBDecoder *dec;

  dec = jpeg_rgb_decoder_new ();

  jpeg_rgb_decoder_addbits (dec, image->jpegtables->data,
      image->jpegtables->length);
  jpeg_rgb_decoder_addbits (dec, image->raw_data->data + 2,
      image->raw_data->length - 2);
  jpeg_rgb_decoder_parse (dec);
  jpeg_rgb_decoder_get_image_size (dec, &image->width, &image->height);
  if (image->width == 0 || image->height == 0) {
    jpeg_rgb_decoder_free (dec);
    return;
  }
  swfdec_cached_load (SWFDEC_CACHED (image), 4 * image->width * image->height);
  jpeg_rgb_decoder_get_image (dec, &image->data,
      &image->rowstride, NULL, NULL);
  jpeg_rgb_decoder_free (dec);

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);
}

int
tag_func_define_bits_jpeg_2 (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecImage *image;

  id = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_IMAGE);
  if (!image)
    return SWFDEC_STATUS_OK;

  image->type = SWFDEC_IMAGE_TYPE_JPEG2;
  image->raw_data = swfdec_buffer_ref (bits->buffer);

  bits->ptr += bits->buffer->length - 2;

  return SWFDEC_STATUS_OK;
}

static void
swfdec_image_jpeg2_load (SwfdecImage *image)
{
  JpegRGBDecoder *dec;

  dec = jpeg_rgb_decoder_new ();

  jpeg_rgb_decoder_addbits (dec, image->raw_data->data + 2,
      image->raw_data->length - 2);
  jpeg_rgb_decoder_parse (dec);
  jpeg_rgb_decoder_get_image_size (dec, &image->width, &image->height);
  if (image->width == 0 || image->height == 0) {
    jpeg_rgb_decoder_free (dec);
    return;
  }
  swfdec_cached_load (SWFDEC_CACHED (image), 4 * image->width * image->height);
  jpeg_rgb_decoder_get_image (dec, &image->data,
      &image->rowstride, &image->width, &image->height);
  jpeg_rgb_decoder_free (dec);

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);
}

int
tag_func_define_bits_jpeg_3 (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecImage *image;
  unsigned char *endptr;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_IMAGE);
  if (!image)
    return SWFDEC_STATUS_OK;

  image->type = SWFDEC_IMAGE_TYPE_JPEG3;
  image->raw_data = swfdec_buffer_ref (bits->buffer);

  bits->ptr += bits->buffer->length - 2;

  return SWFDEC_STATUS_OK;
}

static void
swfdec_image_jpeg3_load (SwfdecImage *image)
{
  JpegRGBDecoder *dec;
  unsigned char *alpha_data;
  SwfdecBits bits;
  int len;
  int jpeg_length;

  bits.buffer = image->raw_data;
  bits.ptr = image->raw_data->data;
  bits.idx = 0;
  bits.end = bits.ptr + image->raw_data->length;

  bits.ptr += 2;

  jpeg_length = swfdec_bits_get_u32 (&bits);

  dec = jpeg_rgb_decoder_new ();

  jpeg_rgb_decoder_addbits (dec, bits.ptr, jpeg_length);
  jpeg_rgb_decoder_parse (dec);
  jpeg_rgb_decoder_get_image_size (dec, &image->width, &image->height);
  if (image->width == 0 || image->height == 0) {
    jpeg_rgb_decoder_free (dec);
    return;
  }
  swfdec_cached_load (SWFDEC_CACHED (image), 4 * image->width * image->height);
  jpeg_rgb_decoder_get_image (dec, &image->data,
      &image->rowstride, &image->width, &image->height);
  jpeg_rgb_decoder_free (dec);

  bits.ptr += jpeg_length;

  alpha_data = lossless (bits.ptr, bits.end - bits.ptr, &len);

  merge_alpha (image, image->data, alpha_data);
  g_free (alpha_data);

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);
}

static void
merge_alpha (SwfdecImage * image, unsigned char *image_data,
    unsigned char *alpha)
{
  int x, y;
  unsigned char *p;

  for (y = 0; y < image->height; y++) {
    p = image_data + y * image->rowstride;
    for (x = 0; x < image->width; x++) {
      p[SWFDEC_COLOR_INDEX_ALPHA] = *alpha;
      p += 4;
      alpha++;
    }
  }
}

static void
swfdec_image_lossless_load (SwfdecImage *image)
{
  int format;
  int color_table_size;
  unsigned char *ptr;
  int len;
  unsigned char *endptr;
  SwfdecBits bits;
  int have_alpha = (image->type == SWFDEC_IMAGE_TYPE_LOSSLESS2);

  bits.buffer = image->raw_data;
  bits.ptr = image->raw_data->data;
  bits.idx = 0;
  bits.end = bits.ptr + image->raw_data->length;
  endptr = bits.ptr + bits.buffer->length;

  bits.ptr += 2;

  format = swfdec_bits_get_u8 (&bits);
  SWFDEC_LOG ("  format = %d", format);
  image->width = swfdec_bits_get_u16 (&bits);
  SWFDEC_LOG ("  width = %d", image->width);
  image->height = swfdec_bits_get_u16 (&bits);
  SWFDEC_LOG ("  height = %d", image->height);
  if (format == 3) {
    color_table_size = swfdec_bits_get_u8 (&bits) + 1;
  } else {
    color_table_size = 0;
  }

  SWFDEC_LOG ("format = %d", format);
  SWFDEC_LOG ("width = %d", image->width);
  SWFDEC_LOG ("height = %d", image->height);
  SWFDEC_LOG ("color_table_size = %d", color_table_size);

  if (image->width == 0 || image->height == 0)
    return;
  swfdec_cached_load (SWFDEC_CACHED (image), 4 * image->width * image->height);
  ptr = lossless (bits.ptr, endptr - bits.ptr, &len);
  bits.ptr = endptr;

  if (format == 3) {
    unsigned char *color_table;
    unsigned char *indexed_data;
    int i;

    image->data = g_malloc (4 * image->width * image->height);
    image->rowstride = image->width * 4;

    color_table = g_malloc (color_table_size * 4);

    if (have_alpha) {
      for (i = 0; i < color_table_size; i++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
        color_table[i * 4 + 0] = ptr[i * 4 + 2];
        color_table[i * 4 + 1] = ptr[i * 4 + 1];
        color_table[i * 4 + 2] = ptr[i * 4 + 0];
        color_table[i * 4 + 3] = ptr[i * 4 + 3];
#else
        color_table[i * 4 + 0] = ptr[i * 4 + 3];
        color_table[i * 4 + 1] = ptr[i * 4 + 0];
        color_table[i * 4 + 2] = ptr[i * 4 + 1];
        color_table[i * 4 + 3] = ptr[i * 4 + 2];
#endif
      }
      indexed_data = ptr + color_table_size * 4;
    } else {
      for (i = 0; i < color_table_size; i++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
        color_table[i * 4 + 0] = ptr[i * 3 + 2];
        color_table[i * 4 + 1] = ptr[i * 3 + 1];
        color_table[i * 4 + 2] = ptr[i * 3 + 0];
        color_table[i * 4 + 3] = 255;
#else
        color_table[i * 4 + 0] = 255;
        color_table[i * 4 + 1] = ptr[i * 3 + 0];
        color_table[i * 4 + 2] = ptr[i * 3 + 1];
        color_table[i * 4 + 3] = ptr[i * 3 + 2];
#endif
      }
      indexed_data = ptr + color_table_size * 3;
    }
    swfdec_image_colormap_decode (image, image->data, indexed_data,
	color_table, color_table_size);

    g_free (color_table);
    g_free (ptr);
  }
  if (format == 4) {
    unsigned char *p = ptr;
    int i, j;
    unsigned int c;
    unsigned char *idata;

    if (have_alpha) {
      SWFDEC_INFO("16bit images aren't allowed to have alpha, ignoring");
      have_alpha = FALSE;
    }

    image->data = g_malloc (4 * image->width * image->height);
    idata = image->data;
    image->rowstride = image->width * 4;

    /* 15 bit packed */
    for (j = 0; j < image->height; j++) {
      for (i = 0; i < image->width; i++) {
        c = p[1] | (p[0] << 8);
        idata[SWFDEC_COLOR_INDEX_BLUE] = (c << 3) | ((c >> 2) & 0x7);
        idata[SWFDEC_COLOR_INDEX_GREEN] = ((c >> 2) & 0xf8) | ((c >> 7) & 0x7);
        idata[SWFDEC_COLOR_INDEX_RED] = ((c >> 7) & 0xf8) | ((c >> 12) & 0x7);
        idata[SWFDEC_COLOR_INDEX_ALPHA] = 0xff;
        p += 2;
        idata += 4;
      }
    }
    g_free (ptr);
  }
  if (format == 5) {
    int i, j;

    image->data = ptr;
    image->rowstride = image->width * 4;

    /* image is stored in 0RGB format.  We use ARGB/BGRA. */
    for (j = 0; j < image->height; j++) {
      for (i = 0; i < image->width; i++) {
	*((guint32 *) ptr) = GUINT32_FROM_BE (*((guint32 *) ptr));
	ptr += 4;
      }
    }
  }
}

int
tag_func_define_bits_lossless (SwfdecSwfDecoder * s)
{
  SwfdecImage *image;
  int id;
  SwfdecBits *bits = &s->b;

  id = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_IMAGE);
  if (!image)
    return SWFDEC_STATUS_OK;

  image->type = SWFDEC_IMAGE_TYPE_LOSSLESS;
  image->raw_data = swfdec_buffer_ref (bits->buffer);

  bits->ptr += bits->buffer->length - 2;
  return SWFDEC_STATUS_OK;
}

int
tag_func_define_bits_lossless_2 (SwfdecSwfDecoder * s)
{
  SwfdecImage *image;
  int id;
  SwfdecBits *bits = &s->b;

  id = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_IMAGE);
  if (!image)
    return SWFDEC_STATUS_OK;

  image->type = SWFDEC_IMAGE_TYPE_LOSSLESS2;
  image->raw_data = swfdec_buffer_ref (bits->buffer);

  bits->ptr += bits->buffer->length - 2;

  return SWFDEC_STATUS_OK;
}

static void
swfdec_image_colormap_decode (SwfdecImage * image,
    unsigned char *dest,
    unsigned char *src, unsigned char *colormap, int colormap_len)
{
  int c;
  int i;
  int j;
  int rowstride;

  rowstride = (image->width + 3) & ~0x3;
  SWFDEC_DEBUG ("rowstride %d", rowstride);

  for (j = 0; j < image->height; j++) {
    for (i = 0; i < image->width; i++) {
      c = src[i];
      if (c >= colormap_len) {
        SWFDEC_ERROR ("colormap index out of range (%d>=%d) (%d,%d)",
            c, colormap_len, i, j);
        dest[0] = 0;
        dest[1] = 0;
        dest[2] = 0;
        dest[3] = 0;
      } else {
	memmove (dest, &colormap[c*4], 4);
      }
      dest += 4;
    }
    src += rowstride;
  }
}

static gboolean
swfdec_image_ensure_loaded (SwfdecImage *image)
{
  if (image->data == NULL) {
    switch (image->type) {
      case SWFDEC_IMAGE_TYPE_JPEG:
	swfdec_image_jpeg_load (image);
	break;
      case SWFDEC_IMAGE_TYPE_JPEG2:
	swfdec_image_jpeg2_load (image);
	break;
      case SWFDEC_IMAGE_TYPE_JPEG3:
	swfdec_image_jpeg3_load (image);
	break;
      case SWFDEC_IMAGE_TYPE_LOSSLESS:
	swfdec_image_lossless_load (image);
	break;
      case SWFDEC_IMAGE_TYPE_LOSSLESS2:
	swfdec_image_lossless_load (image);
	break;
      case SWFDEC_IMAGE_TYPE_UNKNOWN:
      default:
	g_assert_not_reached ();
	break;
    }
    if (image->data == NULL) {
      SWFDEC_WARNING ("failed to load image data");
      return FALSE;
    }
  } else {
    swfdec_cached_use (SWFDEC_CACHED (image));
  }
  return TRUE;
}

static void
swfdec_image_premultiply (guint8 *data, guint n)
{
  guint i;

  for (i = 0; i < n; i++, data += 4) {
    if (data[SWFDEC_COLOR_INDEX_ALPHA] == 0xFF)
      continue;
    if (data[SWFDEC_COLOR_INDEX_ALPHA] == 0) {
      data[SWFDEC_COLOR_INDEX_RED] = data[SWFDEC_COLOR_INDEX_GREEN] = data[SWFDEC_COLOR_INDEX_BLUE] = 0;
    } else {
      data[SWFDEC_COLOR_INDEX_RED] = (guint) data[SWFDEC_COLOR_INDEX_RED] * data[SWFDEC_COLOR_INDEX_ALPHA] / 255;
      data[SWFDEC_COLOR_INDEX_GREEN] = (guint) data[SWFDEC_COLOR_INDEX_GREEN] * data[SWFDEC_COLOR_INDEX_ALPHA] / 255;
      data[SWFDEC_COLOR_INDEX_BLUE] = (guint) data[SWFDEC_COLOR_INDEX_BLUE] * data[SWFDEC_COLOR_INDEX_ALPHA] / 255;
    }
  }
}

static gboolean
swfdec_image_has_alpha (SwfdecImage *image)
{
  return image->type == SWFDEC_IMAGE_TYPE_LOSSLESS2 ||
      image->type == SWFDEC_IMAGE_TYPE_JPEG3;
}

cairo_surface_t *
swfdec_image_create_surface (SwfdecImage *image)
{
  static const cairo_user_data_key_t key;

  g_return_val_if_fail (SWFDEC_IS_IMAGE (image), NULL);

  if (!swfdec_image_ensure_loaded (image))
    return NULL;
  if (image->surface) {
    cairo_surface_reference (image->surface);
    return image->surface;
  }

  if (swfdec_image_has_alpha (image)) {
    cairo_surface_t *surface;
    guint8 *data;
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 
	image->width, image->height);
    /* FIXME: only works if rowstride == image->width * 4 */
    data = cairo_image_surface_get_data (surface);
    memcpy (data, image->data, image->width * image->height * 4);
    swfdec_image_premultiply (data, image->width * image->height);
    return surface;
  } else {
    image->surface = cairo_image_surface_create_for_data (image->data,
	CAIRO_FORMAT_RGB24, image->width, image->height, image->rowstride);
    cairo_surface_set_user_data (image->surface, &key, image->data, g_free);
    cairo_surface_reference (image->surface);
    return image->surface;
  }
}

cairo_surface_t *
swfdec_image_create_surface_transformed (SwfdecImage *image, const SwfdecColorTransform *trans)
{
  static const cairo_user_data_key_t key;
  cairo_surface_t *surface;
  guint8 *tdata;
  const guint8 *sdata;
  guint i, n;
  gboolean has_alpha = FALSE;

  g_return_val_if_fail (SWFDEC_IS_IMAGE (image), NULL);
  g_return_val_if_fail (trans != NULL, NULL);

  /* obvious optimization */
  if (swfdec_color_transform_is_identity (trans))
    return swfdec_image_create_surface (image);

  if (!swfdec_image_ensure_loaded (image))
    return NULL;

  tdata = g_try_malloc (image->width * image->height * 4);
  if (!tdata) {
    SWFDEC_ERROR ("failed to allocate memory for transformed image");
    return NULL;
  }
  sdata = image->data;
  n = image->width * image->height;
  for (i = 0; i < n; i++) {
    ((guint32 *) tdata)[i] = swfdec_color_apply_transform (((guint32 *) sdata)[i], trans);
    has_alpha |= tdata[4 * i + SWFDEC_COLOR_INDEX_ALPHA] != 0xFF;
  }
  if (has_alpha)
    swfdec_image_premultiply (tdata, n);
  surface = cairo_image_surface_create_for_data (tdata,
      has_alpha ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, 
      image->width, image->height, image->width * 4);
  cairo_surface_set_user_data (surface, &key, tdata, g_free);
  return surface;
}
