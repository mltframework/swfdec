
#include <swfdec_image.h>
#include <stdio.h>
#include <zlib.h>

#include "swfdec_internal.h"
#include "jpeg_rgb_decoder.h"

static void swfdec_image_base_init (gpointer g_class);
static void swfdec_image_class_init (gpointer g_class, gpointer user_data);
static void swfdec_image_init (GTypeInstance *instance, gpointer g_class);
static void swfdec_image_dispose (GObject *object);

static void jpegdec (SwfdecImage * image, unsigned char *ptr, int len);
static void merge_alpha (SwfdecImage * image, unsigned char *alpha);
static void merge_opaque (SwfdecImage * image);
static void swfdec_image_colormap_decode (SwfdecImage * image,
    unsigned char *src, unsigned char *colormap, int colormap_len);



GType _swfdec_image_type;

static GObjectClass *parent_class = NULL;

GType swfdec_image_get_type (void)
{
  if (!_swfdec_image_type) {
    static const GTypeInfo object_info = {
      sizeof (SwfdecImageClass),
      swfdec_image_base_init,
      NULL,
      swfdec_image_class_init,
      NULL,
      NULL,
      sizeof (SwfdecImage),
      32,
      swfdec_image_init,
      NULL
    };
    _swfdec_image_type = g_type_register_static (SWFDEC_TYPE_OBJECT,
        "SwfdecImage", &object_info, 0);
  }
  return _swfdec_image_type;
}

static void swfdec_image_base_init (gpointer g_class)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

}

static void swfdec_image_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  gobject_class->dispose = swfdec_image_dispose;

  parent_class = g_type_class_peek_parent (gobject_class);

}

static void swfdec_image_init (GTypeInstance *instance, gpointer g_class)
{

}

static void swfdec_image_dispose (GObject *object)
{
  SwfdecImage *image = SWFDEC_IMAGE (object);

  g_free (image->image_data);
}


static void *
zalloc (void *opaque, unsigned int items, unsigned int size)
{
  return malloc (items * size);
}

static void
zfree (void *opaque, void *addr)
{
  free (addr);
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
      data = realloc (data, len);
      z->next_out = data + z->total_out;
      z->avail_out += 1024;
    }
    ret = inflate (z, Z_SYNC_FLUSH);
    if (ret != Z_OK)
      break;
  }
  if (ret != Z_STREAM_END) {
    fprintf (stderr, "lossless: ret == %d\n", ret);
  }

  if (plen)
    (*plen) = z->total_out;

  g_free (z);
  return data;
}


int
swfdec_image_jpegtables (SwfdecDecoder * s)
{
  bits_t *bits = &s->b;

  SWFDEC_LOG ("swfdec_image_jpegtables");

  s->jpegtables = malloc (s->tag_len);
  s->jpegtables_len = s->tag_len;

  memcpy (s->jpegtables, bits->ptr, s->tag_len);
  bits->ptr += s->tag_len;

  return SWF_OK;
}

int
tag_func_define_bits_jpeg (SwfdecDecoder * s)
{

  bits_t *bits = &s->b;
  int id;
  SwfdecImage *image;
  JpegRGBDecoder *dec;

  SWFDEC_LOG ("tag_func_define_bits_jpeg");
  id = get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = g_object_new (SWFDEC_TYPE_IMAGE, NULL);
  SWFDEC_OBJECT (image)->id = id;
  s->objects = g_list_append (s->objects, image);

  dec = jpeg_rgb_decoder_new ();

  jpeg_rgb_decoder_addbits (dec, s->jpegtables, s->jpegtables_len);
  jpeg_rgb_decoder_addbits (dec, bits->ptr, s->tag_len - 2);
  jpeg_rgb_decoder_parse (dec);
  jpeg_rgb_decoder_get_image (dec, (unsigned char **) &image->image_data,
      &image->rowstride, &image->width, &image->height);
  jpeg_rgb_decoder_free (dec);

  merge_opaque (image);

  bits->ptr += s->tag_len - 2;

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);

  return SWF_OK;
}

int
tag_func_define_bits_jpeg_2 (SwfdecDecoder * s)
{
  bits_t *bits = &s->b;
  int id;
  SwfdecImage *image;

  id = get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = g_object_new (SWFDEC_TYPE_IMAGE, NULL);
  SWFDEC_OBJECT (image)->id = id;
  s->objects = g_list_append (s->objects, image);

  jpegdec (image, bits->ptr, s->tag_len - 2);
  merge_opaque (image);

  bits->ptr += s->tag_len - 2;

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);

  return SWF_OK;
}

int
tag_func_define_bits_jpeg_3 (SwfdecDecoder * s)
{
  bits_t *bits = &s->b;
  int id;

  //unsigned char *endptr = s->b.ptr + s->tag_len;
  int len;

  //int alpha_len;
  //unsigned char *data;
  SwfdecImage *image;
  unsigned char *ptr;
  unsigned char *endptr;

  endptr = bits->ptr + s->tag_len;

  id = get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = g_object_new (SWFDEC_TYPE_IMAGE, NULL);
  SWFDEC_OBJECT (image)->id = id;
  s->objects = g_list_append (s->objects, image);

  len = get_u32 (bits);
  SWFDEC_LOG ("  len = %d", len);

  jpegdec (image, bits->ptr, len);

  SWFDEC_LOG ("  width = %d", image->width);
  SWFDEC_LOG ("  height = %d", image->height);

  bits->ptr += len;
  //tag_func_dumpbits(s);

  ptr = lossless (bits->ptr, endptr - bits->ptr, &len);
  bits->ptr = endptr;

  SWFDEC_LOG ("len = %d h x w=%d", len, image->width * image->height);
  g_assert (len == image->width * image->height);

  merge_alpha (image, ptr);

  free (ptr);

  return SWF_OK;
}

static void
merge_alpha (SwfdecImage * image, unsigned char *alpha)
{
  int x, y;
  unsigned char *p;

  for (y = 0; y < image->height; y++) {
    p = image->image_data + y * image->rowstride;
    for (x = 0; x < image->width; x++) {
      p[3] = *alpha++;
      p += 4;
    }
  }
}

static void
merge_opaque (SwfdecImage * image)
{
  int x, y;
  unsigned char *p;

  for (y = 0; y < image->height; y++) {
    p = image->image_data + y * image->rowstride;
    for (x = 0; x < image->width; x++) {
      p[3] = 255;
      p += 4;
    }
  }
}

static int
define_bits_lossless (SwfdecDecoder * s, int have_alpha)
{
  bits_t *bits = &s->b;
  int id;
  int format;
  int color_table_size;
  unsigned char *ptr;
  int len;
  unsigned char *endptr = bits->ptr + s->tag_len;
  SwfdecImage *image;

  id = get_u16 (bits);
  SWFDEC_LOG ("  id = %d", id);

  image = g_object_new (SWFDEC_TYPE_IMAGE, NULL);
  SWFDEC_OBJECT (image)->id = id;
  s->objects = g_list_append (s->objects, image);

  format = get_u8 (bits);
  SWFDEC_LOG ("  format = %d", format);
  image->width = get_u16 (bits);
  SWFDEC_LOG ("  width = %d", image->width);
  image->height = get_u16 (bits);
  SWFDEC_LOG ("  height = %d", image->height);
  if (format == 3) {
    color_table_size = get_u8 (bits) + 1;
  } else {
    color_table_size = 0;
  }

  SWFDEC_LOG ("format = %d", format);
  SWFDEC_LOG ("width = %d", image->width);
  SWFDEC_LOG ("height = %d", image->height);
  SWFDEC_LOG ("color_table_size = %d", color_table_size);

  ptr = lossless (bits->ptr, endptr - bits->ptr, &len);
  bits->ptr = endptr;

  if (format == 3) {
    unsigned char *color_table;
    unsigned char *indexed_data;
    int i;

    image->image_data = malloc (4 * image->width * image->height);
    image->rowstride = image->width * 4;

    if (have_alpha) {
      color_table = ptr;
      indexed_data = ptr + color_table_size * 4;
      swfdec_image_colormap_decode (image, indexed_data,
	  color_table, color_table_size);
    } else {
      color_table = malloc (color_table_size * 4);

      for (i = 0; i < color_table_size; i++) {
	color_table[i * 4 + 0] = ptr[i * 3 + 0];
	color_table[i * 4 + 1] = ptr[i * 3 + 1];
	color_table[i * 4 + 2] = ptr[i * 3 + 2];
	color_table[i * 4 + 3] = 255;
      }
      indexed_data = ptr + color_table_size * 3;
      swfdec_image_colormap_decode (image, indexed_data,
	  color_table, color_table_size);

      free (color_table);
    }

    free (ptr);
  }
  if (format == 4) {
    unsigned char *p = ptr;
    int i, j;
    unsigned int c;
    unsigned char *idata;

    if (have_alpha) {
      SWFDEC_ERROR ("illegal");
    }

    image->image_data = malloc (4 * image->width * image->height);
    idata = image->image_data;
    image->rowstride = image->width * 4;

    /* 15 bit packed */
    for (j = 0; j < image->height; j++) {
      for (i = 0; i < image->width; i++) {
	c = p[0] | (p[1] << 8);
	idata[0] = (c << 3) | ((c >> 2) & 0x7);
	idata[1] = ((c >> 2) & 0xf8) | ((c >> 7) & 0x7);
	idata[2] = ((c >> 7) & 0xf8) | ((c >> 12) & 0x7);
	idata[3] = 0xff;
	p++;
	idata += 4;
      }
    }
    free (ptr);
  }
  if (format == 5) {
    int i, j;

    image->image_data = ptr;
    image->rowstride = image->width * 4;

    if (!have_alpha) {
      /* image is stored in 0RGB format.  We use RGBA. */
      for (j = 0; j < image->height; j++) {
	for (i = 0; i < image->width; i++) {
	  ptr[0] = ptr[3];
	  ptr[1] = ptr[2];
	  ptr[2] = ptr[1];
	  ptr[3] = 255;
	  ptr += 4;
	}
      }
    }
  }


  return SWF_OK;
}

int
tag_func_define_bits_lossless (SwfdecDecoder * s)
{
  return define_bits_lossless (s, FALSE);
}

int
tag_func_define_bits_lossless_2 (SwfdecDecoder * s)
{
  return define_bits_lossless (s, TRUE);
}


#if 0
int
swfdec_image_render (SwfdecDecoder * s, SwfdecLayer * layer, SwfdecObject * obj)
{
  printf ("got here\n");

  //art_irect_intersect(&rect, &s->drawrect, &pobj->rect);

  return SWF_OK;
}
#endif

static void
jpegdec (SwfdecImage * image, unsigned char *data, int len)
{
  JpegRGBDecoder *dec;

  dec = jpeg_rgb_decoder_new ();

  jpeg_rgb_decoder_addbits (dec, data, len);
  jpeg_rgb_decoder_parse (dec);
  jpeg_rgb_decoder_get_image (dec, (unsigned char **) &image->image_data,
      &image->rowstride, &image->width, &image->height);
  jpeg_rgb_decoder_free (dec);

}

static void
swfdec_image_colormap_decode (SwfdecImage * image,
    unsigned char *src, unsigned char *colormap, int colormap_len)
{
  int c;
  int i;
  int j;
  int rowstride;
  unsigned char *dest;

  rowstride = (image->width + 3) & ~0x3;
  //fprintf(stderr,"rowstride %d\n",rowstride);

  dest = image->image_data;

  for (j = 0; j < image->height; j++) {
    for (i = 0; i < image->width; i++) {
      c = src[i];
      if (c >= colormap_len) {
	fprintf (stderr, "colormap index out of range (%d>=%d) (%d,%d)\n",
	    c, colormap_len, i, j);
	dest[0] = 255;
	dest[1] = 0;
	dest[2] = 0;
	dest[3] = 255;
      } else {
	dest[0] = colormap[c * 4 + 2];
	dest[1] = colormap[c * 4 + 1];
	dest[2] = colormap[c * 4 + 0];
	dest[3] = colormap[c * 4 + 3];
      }
      dest += 4;
    }
    src += rowstride;
  }
}
