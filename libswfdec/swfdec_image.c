
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
static void swfdec_image_jpeg_load (SwfdecHandle *handle);
static void swfdec_image_jpeg2_load (SwfdecHandle *handle);
static void swfdec_image_jpeg3_load (SwfdecHandle *handle);

G_DEFINE_TYPE (SwfdecImage, swfdec_image, SWFDEC_TYPE_CHARACTER)

static void
swfdec_image_dispose (GObject *object)
{
  SwfdecImage * image = SWFDEC_IMAGE (object);

  if (image->handle) {
    swfdec_handle_free (image->handle);
    image->handle = NULL;
  }
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
  image->handle = swfdec_handle_new (swfdec_image_jpeg_load,
      (SwfdecHandleFreeFunc)g_free, image);
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
swfdec_image_jpeg_load (SwfdecHandle *handle)
{
  SwfdecImage *image = swfdec_handle_get_private (handle);
  JpegRGBDecoder *dec;
  unsigned char *image_data;

  dec = jpeg_rgb_decoder_new ();

  jpeg_rgb_decoder_addbits (dec, image->jpegtables->data,
      image->jpegtables->length);
  jpeg_rgb_decoder_addbits (dec, image->raw_data->data + 2,
      image->raw_data->length - 2);
  jpeg_rgb_decoder_parse (dec);
  jpeg_rgb_decoder_get_image (dec, &image_data,
      &image->rowstride, &image->width, &image->height);
  jpeg_rgb_decoder_free (dec);

  swfdec_handle_set_data (handle, image_data);
  swfdec_handle_add_size (handle, image->rowstride * image->height);

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
  image->handle = swfdec_handle_new (swfdec_image_jpeg2_load,
      (SwfdecHandleFreeFunc)g_free, image);
  image->raw_data = swfdec_buffer_ref (bits->buffer);

  bits->ptr += bits->buffer->length - 2;

  return SWFDEC_STATUS_OK;
}

static void
swfdec_image_jpeg2_load (SwfdecHandle *handle)
{
  SwfdecImage *image = swfdec_handle_get_private (handle);
  JpegRGBDecoder *dec;
  unsigned char *image_data;

  dec = jpeg_rgb_decoder_new ();

  jpeg_rgb_decoder_addbits (dec, image->raw_data->data + 2,
      image->raw_data->length - 2);
  jpeg_rgb_decoder_parse (dec);
  jpeg_rgb_decoder_get_image (dec, &image_data,
      &image->rowstride, &image->width, &image->height);
  jpeg_rgb_decoder_free (dec);

  swfdec_handle_set_data (handle, image_data);
  swfdec_handle_add_size (handle, image->width * image->height * 4);

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
  image->handle = swfdec_handle_new (swfdec_image_jpeg3_load,
      (SwfdecHandleFreeFunc)g_free, image);
  image->raw_data = swfdec_buffer_ref (bits->buffer);

  bits->ptr += bits->buffer->length - 2;

  return SWFDEC_STATUS_OK;
}

static void
swfdec_image_jpeg3_load (SwfdecHandle *handle)
{
  SwfdecImage *image = swfdec_handle_get_private (handle);
  JpegRGBDecoder *dec;
  unsigned char *image_data;
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
  jpeg_rgb_decoder_get_image (dec, &image_data,
      &image->rowstride, &image->width, &image->height);
  jpeg_rgb_decoder_free (dec);

  bits.ptr += jpeg_length;

  alpha_data = lossless (bits.ptr, bits.end - bits.ptr, &len);

  merge_alpha (image, image_data, alpha_data);
  g_free (alpha_data);

  swfdec_handle_set_data (handle, image_data);
  swfdec_handle_add_size (handle, image->width * image->height * 4);

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
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      p[3] = *alpha;
#else
      p[0] = *alpha;
#endif
      p += 4;
      alpha++;
    }
  }
}

static void
swfdec_image_lossless_load (SwfdecHandle *handle)
{
  SwfdecImage *image = swfdec_handle_get_private (handle);
  int format;
  int color_table_size;
  unsigned char *ptr;
  int len;
  unsigned char *endptr;
  SwfdecBits bits;
  unsigned char *image_data = NULL;
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

  ptr = lossless (bits.ptr, endptr - bits.ptr, &len);
  bits.ptr = endptr;

  if (format == 3) {
    unsigned char *color_table;
    unsigned char *indexed_data;
    int i;

    image_data = g_malloc (4 * image->width * image->height);
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
    swfdec_image_colormap_decode (image, image_data, indexed_data,
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
    }

    image_data = g_malloc (4 * image->width * image->height);
    idata = image_data;
    image->rowstride = image->width * 4;

    /* 15 bit packed */
    for (j = 0; j < image->height; j++) {
      for (i = 0; i < image->width; i++) {
        c = p[1] | (p[0] << 8);
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
        idata[0] = (c << 3) | ((c >> 2) & 0x7);
        idata[1] = ((c >> 2) & 0xf8) | ((c >> 7) & 0x7);
        idata[2] = ((c >> 7) & 0xf8) | ((c >> 12) & 0x7);
        idata[3] = 0xff;
#else
        idata[3] = (c << 3) | ((c >> 2) & 0x7);
        idata[2] = ((c >> 2) & 0xf8) | ((c >> 7) & 0x7);
        idata[1] = ((c >> 7) & 0xf8) | ((c >> 12) & 0x7);
        idata[0] = 0xff;
#endif
        p += 2;
        idata += 4;
      }
    }
    g_free (ptr);
  }
  if (format == 5) {
    int i, j;

    image_data = ptr;
    image->rowstride = image->width * 4;

    if (!have_alpha) {
      /* image is stored in 0RGB format.  We use ARGB/BGRA. */
      for (j = 0; j < image->height; j++) {
        for (i = 0; i < image->width; i++) {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
          ptr[0] = ptr[3];
          ptr[1] = ptr[2];
          ptr[2] = ptr[1];
          ptr[3] = 255;
#endif
          ptr += 4;
        }
      }
    }
  }

  swfdec_handle_set_data (handle, image_data);
  swfdec_handle_add_size (handle, image->width * image->height * 4);
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
  image->handle = swfdec_handle_new (swfdec_image_lossless_load,
      (SwfdecHandleFreeFunc)g_free, image);
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
  image->handle = swfdec_handle_new (swfdec_image_lossless_load,
      (SwfdecHandleFreeFunc)g_free, image);
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
