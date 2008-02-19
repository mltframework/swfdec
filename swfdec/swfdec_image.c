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

#include "jpeg.h"
#include "swfdec_image.h"
#include "swfdec_cache.h"
#include "swfdec_debug.h"
#include "swfdec_swf_decoder.h"

static const cairo_user_data_key_t key;

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
  }
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

  g_assert (image->surface == NULL);
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

int
swfdec_image_jpegtables (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecBits *bits = &s->b;

  SWFDEC_DEBUG ("swfdec_image_jpegtables");

  if (s->jpegtables) {
    SWFDEC_FIXME ("duplicate DefineJPEGTables tag. Deleting first one");
    swfdec_buffer_unref (s->jpegtables);
  }
  s->jpegtables = swfdec_bits_get_buffer (bits, -1);

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_bits_jpeg (SwfdecSwfDecoder * s, guint tag)
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
  image->raw_data = swfdec_bits_get_buffer (bits, -1);

  return SWFDEC_STATUS_OK;
}

/**
 * swfdec_jpeg_decode_argb:
 *
 * This is a wrapper around jpeg_decode_argb() that takes two segments,
 * strips off (sometimes bogus) start-of-image and end-of-image codes,
 * concatenates them, and puts new SOI and EOI codes on the resulting
 * buffer.  This makes a real JPEG image out of the crap in SWF files.
 */
static gboolean
swfdec_jpeg_decode_argb (unsigned char *data1, int length1,
    unsigned char *data2, int length2,
    void *outdata, int *width, int *height)
{
  gboolean ret;

  if (data2) {
    unsigned char *tmpdata;
    int tmplength;

    tmplength = length1 + length2;
    tmpdata = g_malloc (tmplength);

    memcpy (tmpdata, data1, length1);
    memcpy (tmpdata + length1, data2, length2);
    ret = jpeg_decode_argb (tmpdata, tmplength, outdata, width, height);

    g_free (tmpdata);
  } else if (data1) {
    ret = jpeg_decode_argb (data1, length1, outdata, width, height);
  } else {
    ret = FALSE;
  }

  return ret;
}

static void
swfdec_image_jpeg_load (SwfdecImage *image)
{
  gboolean ret;
  guint8 *data;

  if (image->jpegtables) {
    ret = swfdec_jpeg_decode_argb (
        image->jpegtables->data, image->jpegtables->length,
        image->raw_data->data, image->raw_data->length,
        (void *) &data, &image->width, &image->height);
  } else {
    ret = swfdec_jpeg_decode_argb (
        image->raw_data->data, image->raw_data->length,
        NULL, 0,
        (void *)&data, &image->width, &image->height);
  }

  if (!ret)
    return;

  swfdec_cached_load (SWFDEC_CACHED (image), 4 * image->width * image->height);
  image->surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_RGB24,
      image->width, image->height, image->width * 4);
  cairo_surface_set_user_data (image->surface, &key, data, g_free);

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);
}

int
tag_func_define_bits_jpeg_2 (SwfdecSwfDecoder * s, guint tag)
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
  image->raw_data = swfdec_bits_get_buffer (bits, -1);

  return SWFDEC_STATUS_OK;
}

static void
swfdec_image_jpeg2_load (SwfdecImage *image)
{
  gboolean ret;
  guint8 *data;

  ret = swfdec_jpeg_decode_argb (image->raw_data->data, image->raw_data->length,
      NULL, 0,
      (void *)&data, &image->width, &image->height);
  if (!ret)
    return;

  swfdec_cached_load (SWFDEC_CACHED (image), 4 * image->width * image->height);
  image->surface = cairo_image_surface_create_for_data (data, CAIRO_FORMAT_RGB24,
      image->width, image->height, image->width * 4);
  cairo_surface_set_user_data (image->surface, &key, data, g_free);

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);
}

int
tag_func_define_bits_jpeg_3 (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecBits *bits = &s->b;
  guint id;
  SwfdecImage *image;

  id = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_IMAGE);
  if (!image)
    return SWFDEC_STATUS_OK;

  image->type = SWFDEC_IMAGE_TYPE_JPEG3;
  image->raw_data = swfdec_bits_get_buffer (bits, -1);

  return SWFDEC_STATUS_OK;
}

static void
swfdec_image_jpeg3_load (SwfdecImage *image)
{
  SwfdecBits bits;
  SwfdecBuffer *buffer;
  int jpeg_length;
  gboolean ret;
  guint8 *data;

  swfdec_bits_init (&bits, image->raw_data);

  jpeg_length = swfdec_bits_get_u32 (&bits);
  buffer = swfdec_bits_get_buffer (&bits, jpeg_length);
  if (buffer == NULL)
    return;

  ret = swfdec_jpeg_decode_argb (buffer->data, buffer->length,
      NULL, 0,
      (void *)&data, &image->width, &image->height);
  swfdec_buffer_unref (buffer);

  if (!ret)
    return;

  swfdec_cached_load (SWFDEC_CACHED (image), 4 * image->width * image->height);

  buffer = swfdec_bits_decompress (&bits, -1, image->width * image->height);
  if (buffer) {
    merge_alpha (image, data, buffer->data);
    swfdec_buffer_unref (buffer);
  } else {
    SWFDEC_WARNING ("cannot set alpha channel information, decompression failed");
  }

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);

  image->surface = cairo_image_surface_create_for_data (data,
      CAIRO_FORMAT_ARGB32, image->width, image->height, image->width * 4);
  cairo_surface_set_user_data (image->surface, &key, data, g_free);
}

static void
merge_alpha (SwfdecImage * image, unsigned char *image_data,
    unsigned char *alpha)
{
  int x, y;
  unsigned char *p;

  for (y = 0; y < image->height; y++) {
    p = image_data + y * image->width * 4;
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
  guint color_table_size;
  unsigned char *ptr;
  SwfdecBits bits;
  guint8 *data;
  int have_alpha = (image->type == SWFDEC_IMAGE_TYPE_LOSSLESS2);

  swfdec_bits_init (&bits, image->raw_data);

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

  if (format == 3) {
    SwfdecBuffer *buffer;
    unsigned char *indexed_data;
    guint i;
    guint rowstride = (image->width + 3) & ~3;

    data = g_malloc (4 * image->width * image->height);

    if (have_alpha) {
      buffer = swfdec_bits_decompress (&bits, -1, color_table_size * 4 + rowstride * image->height);
      if (buffer == NULL) {
	SWFDEC_ERROR ("failed to decompress data");
	memset (data, 0, 4 * image->width * image->height);
	goto out;
      }
      ptr = buffer->data;
      for (i = 0; i < color_table_size; i++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	guint8 tmp = ptr[i * 4 + 0];
        ptr[i * 4 + 0] = ptr[i * 4 + 2];
        ptr[i * 4 + 2] = tmp;
#else
        guint8 tmp = ptr[i * 4 + 3];
        ptr[i * 4 + 3] = ptr[i * 4 + 2];
        ptr[i * 4 + 2] = ptr[i * 4 + 1];
        ptr[i * 4 + 1] = ptr[i * 4 + 0];
        ptr[i * 4 + 0] = tmp;
#endif
      }
      indexed_data = ptr + color_table_size * 4;
    } else {
      buffer = swfdec_bits_decompress (&bits, -1, color_table_size * 3 + rowstride * image->height);
      if (buffer == NULL) {
	SWFDEC_ERROR ("failed to decompress data");
	memset (data, 0, 4 * image->width * image->height);
	goto out;
      }
      ptr = buffer->data;
      for (i = color_table_size - 1; i < color_table_size; i--) {
	guint8 color[3];
	color[0] = ptr[i * 3 + 0];
	color[1] = ptr[i * 3 + 1];
	color[2] = ptr[i * 3 + 2];
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
        ptr[i * 4 + 0] = color[2];
        ptr[i * 4 + 1] = color[1];
	ptr[i * 4 + 2] = color[0];
        ptr[i * 4 + 3] = 255;
#else
        ptr[i * 4 + 0] = 255;
        ptr[i * 4 + 1] = color[0];
        ptr[i * 4 + 2] = color[1];
        ptr[i * 4 + 3] = color[2];
#endif
      }
      indexed_data = ptr + color_table_size * 3;
    }
    swfdec_image_colormap_decode (image, data, indexed_data,
	ptr, color_table_size);

    swfdec_buffer_unref (buffer);
  } else if (format == 4) {
    int i, j;
    guint c;
    unsigned char *idata;
    SwfdecBuffer *buffer;

    if (have_alpha) {
      SWFDEC_INFO("16bit images aren't allowed to have alpha, ignoring");
      have_alpha = FALSE;
    }

    buffer = swfdec_bits_decompress (&bits, -1, 2 * ((image->width + 1) & ~1) * image->height);
    data = g_malloc (4 * image->width * image->height);
    idata = data;
    if (buffer == NULL) {
      SWFDEC_ERROR ("failed to decompress data");
      memset (data, 0, 4 * image->width * image->height);
      goto out;
    }
    ptr = buffer->data;

    /* 15 bit packed */
    for (j = 0; j < image->height; j++) {
      for (i = 0; i < image->width; i++) {
        c = ptr[1] | (ptr[0] << 8);
        idata[SWFDEC_COLOR_INDEX_BLUE] = (c << 3) | ((c >> 2) & 0x7);
        idata[SWFDEC_COLOR_INDEX_GREEN] = ((c >> 2) & 0xf8) | ((c >> 7) & 0x7);
        idata[SWFDEC_COLOR_INDEX_RED] = ((c >> 7) & 0xf8) | ((c >> 12) & 0x7);
        idata[SWFDEC_COLOR_INDEX_ALPHA] = 0xff;
        ptr += 2;
        idata += 4;
      }
      if (image->width & 1)
	ptr += 2;
    }
    swfdec_buffer_unref (buffer);
  } else if (format == 5) {
    SwfdecBuffer *buffer;
    int i, j;
    guint32 *p;

    buffer = swfdec_bits_decompress (&bits, -1, 4 * image->width * image->height);
    if (buffer == NULL) {
      SWFDEC_ERROR ("failed to decompress data");
      data = g_malloc0 (4 * image->width * image->height);
      goto out;
    }
    data = buffer->data;
    p = (void *) data;
    /* image is stored in 0RGB format.  We use ARGB/BGRA. */
    for (j = 0; j < image->height; j++) {
      for (i = 0; i < image->width; i++) {
	*p = GUINT32_FROM_BE (*p);
	p++;
      }
    }
    image->surface = cairo_image_surface_create_for_data (data, 
	have_alpha ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, 
	image->width, image->height, image->width * 4);
    cairo_surface_set_user_data (image->surface, &key, buffer, 
	(cairo_destroy_func_t) swfdec_buffer_unref);
    return;
  } else {
    SWFDEC_ERROR ("unknown lossless image format %u", format);
    return;
  }

out:
  image->surface = cairo_image_surface_create_for_data (data, 
      have_alpha ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, 
      image->width, image->height, image->width * 4);
  cairo_surface_set_user_data (image->surface, &key, data, g_free);
}

int
tag_func_define_bits_lossless (SwfdecSwfDecoder * s, guint tag)
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
  image->raw_data = swfdec_bits_get_buffer (bits, -1);

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_bits_lossless_2 (SwfdecSwfDecoder * s, guint tag)
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
  image->raw_data = swfdec_bits_get_buffer (bits, -1);

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
      if (colormap_len < 256 && c == 255) {
        dest[0] = 0;
        dest[1] = 0;
        dest[2] = 0;
        dest[3] = 0;
      } else if (c >= colormap_len) {
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

static cairo_status_t
swfdec_image_png_read (void *bitsp, unsigned char *data, unsigned int length)
{
  SwfdecBits *bits = bitsp;
  const guint8 *ptr;

  ptr = bits->ptr;
  if (swfdec_bits_skip_bytes (bits, length) != length)
    return CAIRO_STATUS_READ_ERROR;

  memcpy (data, ptr, length);
  return CAIRO_STATUS_SUCCESS;
}

static void
swfdec_image_png_load (SwfdecImage *image)
{
  SwfdecBits bits;

  swfdec_bits_init (&bits, image->raw_data);
  image->surface = cairo_image_surface_create_from_png_stream (
      swfdec_image_png_read, &bits);
  image->width = cairo_image_surface_get_width (image->surface);
  image->height = cairo_image_surface_get_height (image->surface);
  swfdec_cached_load (SWFDEC_CACHED (image), image->height *
      cairo_image_surface_get_stride (image->surface));
}

cairo_surface_t *
swfdec_image_create_surface (SwfdecImage *image)
{
  if (image->raw_data == NULL)
    return NULL;

  if (image->surface == NULL) {
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
      case SWFDEC_IMAGE_TYPE_PNG:
	swfdec_image_png_load (image);
	break;
      case SWFDEC_IMAGE_TYPE_UNKNOWN:
      default:
	g_assert_not_reached ();
	break;
    }
    if (image->surface == NULL) {
      SWFDEC_WARNING ("failed to decode image");
      return NULL;
    }
  } else {
    swfdec_cached_use (SWFDEC_CACHED (image));
  }
  return cairo_surface_reference (image->surface);
}

cairo_surface_t *
swfdec_image_create_surface_transformed (SwfdecImage *image, const SwfdecColorTransform *trans)
{
  cairo_surface_t *surface, *source;
  guint32 *tdata;
  const guint32 *sdata;
  guint i, n;
  gboolean has_alpha = FALSE;

  g_return_val_if_fail (SWFDEC_IS_IMAGE (image), NULL);
  g_return_val_if_fail (trans != NULL, NULL);

  /* obvious optimization */
  if (swfdec_color_transform_is_identity (trans))
    return swfdec_image_create_surface (image);

  source = swfdec_image_create_surface (image);
  tdata = g_try_malloc (image->width * image->height * 4);
  if (!tdata) {
    SWFDEC_ERROR ("failed to allocate memory for transformed image");
    cairo_surface_destroy (source);
    return NULL;
  }
  /* FIXME: This code assumes a rowstride of 4 * width */
  /* FIXME: This code assumes an alignment of 4 */
  sdata = (void *) cairo_image_surface_get_data (source);
  n = image->width * image->height;
  for (i = 0; i < n; i++) {
    tdata[i] = swfdec_color_apply_transform_premultiplied (sdata[i], trans);
    /* optimization: check for alpha channel to speed up compositing */
    has_alpha |= SWFDEC_COLOR_A (tdata[i]) != 0xFF;
  }
  cairo_surface_destroy (source);
  surface = cairo_image_surface_create_for_data ((unsigned char *) tdata,
      has_alpha ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, 
      image->width, image->height, image->width * 4);
  cairo_surface_set_user_data (surface, &key, tdata, g_free);
  return surface;
}

/* NB: must be at least SWFDEC_DECODER_DETECT_LENGTH bytes */
SwfdecImageType
swfdec_image_detect (const guint8 *data)
{
  g_return_val_if_fail (data != NULL, SWFDEC_IMAGE_TYPE_UNKNOWN);

  if (data[0] == 0xFF && data[1] == 0xD8)
    return SWFDEC_IMAGE_TYPE_JPEG2;
  else if (data[0] == 0x89 && data[1] == 'P' &&
      data[2] == 'N' && data[3] == 'G')
    return SWFDEC_IMAGE_TYPE_PNG;
  else
    return SWFDEC_IMAGE_TYPE_UNKNOWN;
}

SwfdecImage *
swfdec_image_new (SwfdecBuffer *buffer)
{
  SwfdecImage *image;
  SwfdecImageType type;

  g_return_val_if_fail (buffer != NULL, NULL);

  /* check type of the image */
  if (buffer->length < 4)
    goto fail;
  type = swfdec_image_detect (buffer->data);
  if (type == SWFDEC_IMAGE_TYPE_UNKNOWN)
    goto fail;

  image = g_object_new (SWFDEC_TYPE_IMAGE, NULL);
  image->type = type;
  image->raw_data = buffer;

  return image;

fail:
  swfdec_buffer_unref (buffer);
  return NULL;
}
