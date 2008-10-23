/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2008 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_cached_image.h"
#include "swfdec_debug.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecImage, swfdec_image, SWFDEC_TYPE_CHARACTER)

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

  object_class->dispose = swfdec_image_dispose;
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

static gboolean
swfdec_image_validate_size (SwfdecRenderer *renderer, guint width, guint height)
{
  gsize size;

  if (width == 0 || height == 0)
    return FALSE;

  if (renderer) {
    size = swfdec_renderer_get_max_cache_size (renderer);
    size = MIN (size, G_MAXUINT);
  } else {
    size = G_MAXUINT;
  }
  if (size / 4 / width < height) {
    SWFDEC_ERROR ("%ux%u image doesn't fit into %zu bytes of cache", 
	width, height, size);
    return FALSE;
  }

  return TRUE;
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
swfdec_jpeg_decode_argb (SwfdecRenderer *renderer,
    unsigned char *data1, int length1,
    unsigned char *data2, int length2,
    void *outdata, guint *width, guint *height)
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

  if (ret)
    ret = swfdec_image_validate_size (renderer, *width, *height);
  return ret;
}

static cairo_surface_t *
swfdec_image_create_surface_for_data (SwfdecRenderer *renderer, guint8 *data,
    cairo_format_t format, guint width, guint height, guint rowstride)
{
  cairo_surface_t *surface;

  if (renderer) {
    surface = swfdec_renderer_create_for_data (renderer, data, format, width, height, rowstride);
  } else {
    static const cairo_user_data_key_t key;
    surface = cairo_image_surface_create_for_data (data,
	format, width, height, rowstride);
    cairo_surface_set_user_data (surface, &key, data, g_free);
  }
  return surface;
}

static cairo_surface_t * 
swfdec_image_jpeg_load (SwfdecImage *image, SwfdecRenderer *renderer)
{
  gboolean ret;
  guint8 *data;

  if (image->jpegtables) {
    ret = swfdec_jpeg_decode_argb (renderer,
        image->jpegtables->data, image->jpegtables->length,
        image->raw_data->data, image->raw_data->length,
        (void *) &data, &image->width, &image->height);
  } else {
    ret = swfdec_jpeg_decode_argb (renderer,
        image->raw_data->data, image->raw_data->length,
        NULL, 0,
        (void *)&data, &image->width, &image->height);
  }

  if (!ret)
    return NULL;

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);

  return swfdec_image_create_surface_for_data (renderer, data, 
      CAIRO_FORMAT_RGB24, image->width, image->height, 4 * image->width);
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

static cairo_surface_t *
swfdec_image_jpeg2_load (SwfdecImage *image, SwfdecRenderer *renderer)
{
  gboolean ret;
  guint8 *data;

  ret = swfdec_jpeg_decode_argb (renderer, image->raw_data->data, image->raw_data->length,
      NULL, 0,
      (void *)&data, &image->width, &image->height);
  if (!ret)
    return NULL;

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);

  return swfdec_image_create_surface_for_data (renderer, data, 
      CAIRO_FORMAT_RGB24, image->width, image->height, 4 * image->width);
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
merge_alpha (SwfdecImage * image, unsigned char *image_data,
    unsigned char *alpha)
{
  unsigned int x, y;
  unsigned char *p;

  for (y = 0; y < image->height; y++) {
    p = image_data + y * image->width * 4;
    for (x = 0; x < image->width; x++) {
      p[SWFDEC_COLOR_INDEX_ALPHA] = *alpha;
      p[SWFDEC_COLOR_INDEX_RED] = MIN (*alpha, p[SWFDEC_COLOR_INDEX_RED]);
      p[SWFDEC_COLOR_INDEX_GREEN] = MIN (*alpha, p[SWFDEC_COLOR_INDEX_GREEN]);
      p[SWFDEC_COLOR_INDEX_BLUE] = MIN (*alpha, p[SWFDEC_COLOR_INDEX_BLUE]);
      p += 4;
      alpha++;
    }
  }
}

static cairo_surface_t *
swfdec_image_jpeg3_load (SwfdecImage *image, SwfdecRenderer *renderer)
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
    return NULL;

  ret = swfdec_jpeg_decode_argb (renderer,
      buffer->data, buffer->length, NULL, 0,
      (void *)&data, &image->width, &image->height);
  swfdec_buffer_unref (buffer);

  if (!ret)
    return NULL;

  buffer = swfdec_bits_decompress (&bits, -1, image->width * image->height);
  if (buffer) {
    merge_alpha (image, data, buffer->data);
    swfdec_buffer_unref (buffer);
  } else {
    SWFDEC_WARNING ("cannot set alpha channel information, decompression failed");
  }

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);

  return swfdec_image_create_surface_for_data (renderer, data, 
      CAIRO_FORMAT_ARGB32, image->width, image->height, 4 * image->width);
}

static cairo_surface_t *
swfdec_image_lossless_load (SwfdecImage *image, SwfdecRenderer *renderer)
{
  int format;
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

  SWFDEC_LOG ("format = %d", format);
  SWFDEC_LOG ("width = %d", image->width);
  SWFDEC_LOG ("height = %d", image->height);

  if (!swfdec_image_validate_size (renderer, image->width, image->height))
    return NULL;

  if (format == 3) {
    SwfdecBuffer *buffer;
    guchar *indexed_data;
    guint32 palette[256], *pixels;
    guint i, j;
    guint palette_size;
    guint rowstride = (image->width + 3) & ~3;

    palette_size = swfdec_bits_get_u8 (&bits) + 1;
    SWFDEC_LOG ("palette_size = %d", palette_size);

    data = g_malloc (4 * image->width * image->height);

    if (have_alpha) {
      buffer = swfdec_bits_decompress (&bits, -1, palette_size * 4 + rowstride * image->height);
      if (buffer == NULL) {
	SWFDEC_ERROR ("failed to decompress data");
	memset (data, 0, 4 * image->width * image->height);
	goto out;
      }
      ptr = buffer->data;
      for (i = 0; i < palette_size; i++) {
	palette[i] = SWFDEC_COLOR_COMBINE (ptr[i * 4 + 0], ptr[i * 4 + 1],
	    ptr[i * 4 + 2], ptr[i * 4 + 3]);
      }
      indexed_data = ptr + palette_size * 4;
    } else {
      buffer = swfdec_bits_decompress (&bits, -1, palette_size * 3 + rowstride * image->height);
      if (buffer == NULL) {
	SWFDEC_ERROR ("failed to decompress data");
	memset (data, 0, 4 * image->width * image->height);
	goto out;
      }
      ptr = buffer->data;
      for (i = 0; i < palette_size; i++) {
	palette[i] = SWFDEC_COLOR_COMBINE (ptr[i * 3 + 0],
	    ptr[i * 3 + 1], ptr[i * 3 + 2], 0xFF);
      }
      indexed_data = ptr + palette_size * 3;
    }
    if (palette_size < 256)
      memset (palette + palette_size, 0, (256 - palette_size) * 4);

    pixels = (guint32 *) data;
    for (j = 0; j < (guint) image->height; j++) {
      for (i = 0; i < (guint) image->width; i++) {
	*pixels = palette[indexed_data[i]];
	pixels++;
      }
      indexed_data += rowstride;
    }

    swfdec_buffer_unref (buffer);
  } else if (format == 4) {
    guint i, j;
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
    guint i, j;
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
    data = g_memdup (buffer->data, buffer->length);
    swfdec_buffer_unref (buffer);
  } else {
    SWFDEC_ERROR ("unknown lossless image format %u", format);
    return NULL;
  }

out:
  return swfdec_image_create_surface_for_data (renderer, data,
      have_alpha ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, 
      image->width, image->height, image->width * 4);
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

static cairo_surface_t *
swfdec_image_png_load (SwfdecImage *image, SwfdecRenderer *renderer)
{
  SwfdecBits bits;
  cairo_surface_t *surface;

  swfdec_bits_init (&bits, image->raw_data);
  surface = cairo_image_surface_create_from_png_stream (
      swfdec_image_png_read, &bits);
  if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
    SWFDEC_ERROR ("could not create PNG image: %s", 
	cairo_status_to_string (cairo_surface_status (surface)));
    cairo_surface_destroy (surface);
    return NULL;
  }

  image->width = cairo_image_surface_get_width (surface);
  image->height = cairo_image_surface_get_height (surface);
  if (!swfdec_image_validate_size (renderer, image->width, image->height)) {
    cairo_surface_destroy (surface);
    return NULL;
  }
  if (renderer)
    surface = swfdec_renderer_create_similar (renderer, surface);
  return surface;
}

static gboolean
swfdec_image_find_by_transform (SwfdecCached *cached, gpointer data)
{
  const SwfdecColorTransform *trans = data;
  SwfdecColorTransform ctrans;

  swfdec_cached_image_get_color_transform (SWFDEC_CACHED_IMAGE (cached),
      &ctrans);
  if (trans->mask != ctrans.mask)
    return FALSE;
  if (trans->mask)
    return TRUE;
  return (trans->ra == ctrans.ra && 
      trans->rb == ctrans.rb && 
      trans->ga == ctrans.ga && 
      trans->gb == ctrans.gb && 
      trans->ba == ctrans.ba && 
      trans->bb == ctrans.bb && 
      trans->aa == ctrans.aa && 
      trans->ab == ctrans.ab);
}

static cairo_surface_t *
swfdec_image_lookup_surface (SwfdecImage *image, SwfdecRenderer *renderer,
    const SwfdecColorTransform *trans)
{
  if (renderer) {
    SwfdecCached *cached = swfdec_renderer_get_cache (renderer, image, 
	swfdec_image_find_by_transform, (gpointer) trans);
    if (cached) {
      swfdec_cached_use (cached);
      return swfdec_cached_image_get_surface (SWFDEC_CACHED_IMAGE (cached));
    }
  }
  return NULL;
}

cairo_surface_t *
swfdec_image_create_surface (SwfdecImage *image, SwfdecRenderer *renderer)
{
  SwfdecColorTransform trans;
  SwfdecCachedImage *cached;
  cairo_surface_t *surface;

  g_return_val_if_fail (SWFDEC_IS_IMAGE (image), NULL);
  g_return_val_if_fail (renderer == NULL || SWFDEC_IS_RENDERER (renderer), NULL);

  if (image->raw_data == NULL)
    return NULL;

  swfdec_color_transform_init_identity (&trans);
  surface = swfdec_image_lookup_surface (image, renderer, &trans);
  if (surface)
    return surface;

  switch (image->type) {
    case SWFDEC_IMAGE_TYPE_JPEG:
      surface = swfdec_image_jpeg_load (image, renderer);
      break;
    case SWFDEC_IMAGE_TYPE_JPEG2:
      surface = swfdec_image_jpeg2_load (image, renderer);
      break;
    case SWFDEC_IMAGE_TYPE_JPEG3:
      surface = swfdec_image_jpeg3_load (image, renderer);
      break;
    case SWFDEC_IMAGE_TYPE_LOSSLESS:
      surface = swfdec_image_lossless_load (image, renderer);
      break;
    case SWFDEC_IMAGE_TYPE_LOSSLESS2:
      surface = swfdec_image_lossless_load (image, renderer);
      break;
    case SWFDEC_IMAGE_TYPE_PNG:
      surface = swfdec_image_png_load (image, renderer);
      break;
    case SWFDEC_IMAGE_TYPE_UNKNOWN:
    default:
      g_assert_not_reached ();
      break;
  }
  if (surface == NULL) {
    SWFDEC_WARNING ("failed to decode image");
    return NULL;
  }
  if (renderer) {
    /* FIXME: The size is just an educated guess */
    cached = swfdec_cached_image_new (surface, image->width * image->height * 4);
    swfdec_renderer_add_cache (renderer, FALSE, image, SWFDEC_CACHED (cached));
    g_object_unref (cached);
  }

  return surface;
}

cairo_surface_t *
swfdec_image_create_surface_transformed (SwfdecImage *image, SwfdecRenderer *renderer,
    const SwfdecColorTransform *trans)
{
  SwfdecColorTransform mask;
  SwfdecCachedImage *cached;
  cairo_surface_t *surface, *source;
  SwfdecRectangle area;

  g_return_val_if_fail (SWFDEC_IS_IMAGE (image), NULL);
  g_return_val_if_fail (renderer == NULL || SWFDEC_IS_RENDERER (renderer), NULL);
  g_return_val_if_fail (trans != NULL, NULL);
  /* The mask flag is used for caching image surfaces.
   * Instead of masking with images, code should use color patterns */
  g_return_val_if_fail (!swfdec_color_transform_is_mask (trans), NULL);

  surface = swfdec_image_lookup_surface (image, renderer, trans);
  if (surface)
    return surface;
  /* obvious optimization */
  if (swfdec_color_transform_is_identity (trans))
    return swfdec_image_create_surface (image, renderer);

  /* need to create an image surface here, so we can modify it. Will upload later */
  /* NB: we use the mask property here to inidicate an image surface */
  swfdec_color_transform_init_mask (&mask);
  source = swfdec_image_lookup_surface (image, renderer, &mask);
  if (source == NULL) {
    source = swfdec_image_create_surface (image, NULL);
    if (source == NULL)
      return NULL;
    if (renderer) {
      cached = swfdec_cached_image_new (source, image->width * image->height * 4);
      swfdec_cached_image_set_color_transform (cached, &mask);
      swfdec_renderer_add_cache (renderer, FALSE, image, SWFDEC_CACHED (cached));
      g_object_unref (cached);
    }
  }

  area.x = area.y = 0;
  area.width = image->width;
  area.height = image->height;
  surface = swfdec_renderer_transform (renderer, source, trans, &area);

  if (renderer) {
    surface = swfdec_renderer_create_similar (renderer, surface);
    /* FIXME: The size is just an educated guess */
    cached = swfdec_cached_image_new (surface, image->width * image->height * 4);
    swfdec_cached_image_set_color_transform (cached, trans);
    swfdec_renderer_add_cache (renderer, FALSE, image, SWFDEC_CACHED (cached));
    g_object_unref (cached);
  }
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
