
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zlib.h>
#include <math.h>
#include <string.h>

#include <liboil/liboil.h>
#ifdef HAVE_LIBART
#include "swfdec_render_libart.h"
#endif

#include "swfdec_internal.h"

int swf_parse_header1 (SwfdecDecoder * s);
int swf_inflate_init (SwfdecDecoder * s);
int swf_parse_header2 (SwfdecDecoder * s);


void
swfdec_init (void)
{
  g_type_init ();
}

SwfdecDecoder *
swfdec_decoder_new (void)
{
  SwfdecDecoder *s;

  oil_init ();

  s = g_new0 (SwfdecDecoder, 1);

  s->bg_color = SWF_COLOR_COMBINE (0xff, 0xff, 0xff, 0xff);
  s->colorspace = SWF_COLORSPACE_RGB888;
  swf_config_colorspace (s);

  swfdec_transform_init_identity (&s->transform);

  s->main_sprite = swfdec_object_new (SWFDEC_TYPE_SPRITE);
  s->render = swfdec_render_new ();

  s->flatness = 0.5;

  return s;
}

int
swfdec_decoder_add_data (SwfdecDecoder * s, const unsigned char *data, int length)
{
  SwfdecBuffer *buffer = swfdec_buffer_new();

  buffer->data = (unsigned char *)data;
  buffer->length = length;

  return swfdec_decoder_add_buffer (s, buffer);
}

int
swfdec_decoder_add_buffer (SwfdecDecoder * s, SwfdecBuffer *buffer)
{
  int offset;
  int ret;

  if (s->compressed) {
    s->z->next_in = buffer->data;
    s->z->avail_in = buffer->length;
    ret = inflate (s->z, Z_SYNC_FLUSH);
    if (ret < 0) {
      return SWF_ERROR;
    }

    s->parse.end = s->input_data + s->z->total_out;
    swfdec_buffer_unref(buffer);
  } else {
    if (s->parse.ptr) {
      offset = (void *) s->parse.ptr - (void *) s->input_data;
    } else {
      offset = 0;
    }

    s->input_data = g_realloc (s->input_data, s->input_data_len + buffer->length);
    memcpy (s->input_data + s->input_data_len, buffer->data, buffer->length);
    s->input_data_len += buffer->length;

    swfdec_buffer_unref(buffer);

    s->parse.ptr = s->input_data + offset;
    s->parse.end = s->input_data + s->input_data_len;
  }

  return SWF_OK;
}

void
swfdec_decoder_eof (SwfdecDecoder *s)
{
  if (s->state == SWF_STATE_PARSETAG) {
    s->state = SWF_STATE_EOF;
  }
}

int
swfdec_decoder_parse (SwfdecDecoder * s)
{
  int ret = SWF_OK;
  unsigned char *endptr;

  while (ret == SWF_OK) {
    s->b = s->parse;

    switch (s->state) {
      case SWF_STATE_INIT1:
	/* need to initialize */
	ret = swf_parse_header1 (s);
	if (ret != SWF_OK)
	  break;

	s->parse = s->b;
	if (s->compressed) {
	  swf_inflate_init (s);
	}
	s->state = SWF_STATE_INIT2;
	ret = SWF_OK;
	break;
      case SWF_STATE_INIT2:
	ret = swf_parse_header2 (s);
	if (ret == SWF_OK) {
	  swfdec_bits_syncbits (&s->b);
	  s->parse = s->b;
	  s->state = SWF_STATE_PARSETAG;
          swf_config_colorspace (s);
	  {
	    SwfdecRect rect;

	    rect.x0 = 0;
	    rect.y0 = 0;
	    rect.x1 = s->width;
	    rect.y1 = s->height;
	    swf_invalidate_irect (s, &rect);
	  }
	  ret = SWF_CHANGE;
	  break;
	}
	break;
      case SWF_STATE_PARSETAG:
	/* we're parsing tags */
	ret = swf_parse_tag (s);
	if (ret != SWF_OK)
	  break;

	if (swfdec_bits_needbits (&s->b, s->tag_len)) {
	  ret = SWF_NEEDBITS;
	  break;
	}
	endptr = s->b.ptr + s->tag_len;

	s->parse_sprite = s->main_sprite;
	ret = s->func (s);
	s->parse_sprite = NULL;
	//if(ret != SWF_OK)break;

	swfdec_bits_syncbits (&s->b);
	if (s->b.ptr < endptr) {
	  SWFDEC_WARNING ("early parse finish (%d bytes)", endptr - s->b.ptr);
	  //dumpbits (&s->b);
	}
	if (s->b.ptr > endptr) {
	  SWFDEC_WARNING ("parse overrun (%d bytes)", s->b.ptr - endptr);
	}

	s->parse.ptr = endptr;

	if (s->tag == 0) {
	  s->state = SWF_STATE_EOF;

          SWFDEC_WARNING ("decoded points %d", s->stats_n_points);
	}

	break;
      case SWF_STATE_EOF:
	ret = SWF_EOF;

	break;
    }
  }

  return ret;
}

int
swfdec_decoder_free (SwfdecDecoder * s)
{
  GList *g;

  for (g = g_list_first (s->objects); g; g = g_list_next (g)) {
    swfdec_object_unref (SWFDEC_OBJECT (g->data));
  }
  g_list_free (s->objects);

  if (s->buffer)
    g_free (s->buffer);
  if (s->input_data)
    g_free (s->input_data);

  swfdec_object_unref (SWFDEC_OBJECT(s->main_sprite));
  swfdec_render_free (s->render);

  if (s->z) {
    inflateEnd (s->z);
    g_free (s->z);
  }

  if (s->jpegtables) {
    g_free (s->jpegtables);
  }

  if (s->tmp_scanline) {
    g_free (s->tmp_scanline);
  }

  for (g = g_list_first (s->sound_buffers); g; g = g_list_next (g)) {
    swfdec_buffer_unref (g->data);
  }
  g_list_free (s->sound_buffers);

  for (g = g_list_first (s->sound_buffers); g; g = g_list_next (g)) {
    swfdec_buffer_unref (g->data);
  }
  g_list_free (s->stream_sound_buffers);

  g_free (s);

  return SWF_OK;
}

int
swfdec_decoder_get_n_frames (SwfdecDecoder * s, int *n_frames)
{
  if (s->state == SWF_STATE_INIT1 || s->state == SWF_STATE_INIT2) {
    return SWF_ERROR;
  }

  if (n_frames)
    *n_frames = s->n_frames;
  return SWF_OK;
}

int
swfdec_decoder_get_rate (SwfdecDecoder * s, double *rate)
{
  if (s->state == SWF_STATE_INIT1 || s->state == SWF_STATE_INIT2) {
    return SWF_ERROR;
  }

  if (rate)
    *rate = s->rate;
  return SWF_OK;
}

int
swfdec_decoder_get_image (SwfdecDecoder * s, unsigned char **image)
{
  if (s->buffer == NULL) {
    return SWF_ERROR;
  }

  if (image)
    *image = s->buffer;
  s->buffer = NULL;

  return SWF_OK;
}

int
swfdec_decoder_peek_image (SwfdecDecoder * s, unsigned char **image)
{
  if (s->buffer == NULL) {
    return SWF_ERROR;
  }

  if (image)
    *image = s->buffer;

  return SWF_OK;
}

int
swfdec_decoder_get_image_size (SwfdecDecoder * s, int *width, int *height)
{
  if (s->state == SWF_STATE_INIT1 || s->state == SWF_STATE_INIT2) {
    return SWF_ERROR;
  }

  if (width)
    *width = s->width;
  if (height)
    *height = s->height;
  return SWF_OK;
}

int
swfdec_decoder_set_image_size (SwfdecDecoder * s, int width, int height)
{
  //double sw, sh;

  s->width = width;
  s->height = height;

  s->irect.x0 = 0;
  s->irect.y0 = 0;
  s->irect.x1 = s->width;
  s->irect.y1 = s->height;

#if 0
  sw = s->parse_width / s->width;
  sh = s->parse_height / s->height;
  s->scale_factor = (sw < sh) ? sw : sh;

  s->transform[0] = s->scale_factor;
  s->transform[1] = 0;
  s->transform[2] = 0;
  s->transform[3] = s->scale_factor;
  s->transform[4] = 0.5 * (s->width - s->parse_width * s->scale_factor);
  s->transform[5] = 0.5 * (s->height - s->parse_height * s->scale_factor);
#endif
  s->transform.trans[0] = (double)s->width / s->parse_width;
  s->transform.trans[1] = 0;
  s->transform.trans[2] = 0;
  s->transform.trans[3] = (double)s->height / s->parse_height;
  s->transform.trans[4] = 0;
  s->transform.trans[5] = 0;

  return SWF_OK;
}

int
swfdec_decoder_set_colorspace (SwfdecDecoder * s, int colorspace)
{
  if (s->state != SWF_STATE_INIT1) {
    return SWF_ERROR;
  }

  if (colorspace != SWF_COLORSPACE_RGB888 &&
      colorspace != SWF_COLORSPACE_RGB565) {
    return SWF_ERROR;
  }

  s->colorspace = colorspace;

  swf_config_colorspace (s);

  return SWF_OK;
}

void
swfdec_decoder_enable_render (SwfdecDecoder * s)
{
  s->disable_render = FALSE;
}

void
swfdec_decoder_disable_render (SwfdecDecoder * s)
{
  s->disable_render = TRUE;
}

#if 0
void *
swfdec_decoder_get_sound_chunk (SwfdecDecoder * s, int *length)
{
  GList *g;
  SwfdecSoundBuffer *buffer;
  void *data;

  g = g_list_first (s->sound_buffers);
  if (!g)
    return NULL;

  buffer = (SwfdecSoundBuffer *) g->data;

  s->sound_buffers = g_list_delete_link (s->sound_buffers, g);

  data = buffer->data;
  if (length)
    *length = buffer->len;

  g_free (buffer);

  return data;
}
#endif

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

#if 0
static void
dumpbits (SwfdecBits * b)
{
  int i;

  printf ("    ");
  for (i = 0; i < 16; i++) {
    printf ("%02x ", swfdec_bits_get_u8 (b));
  }
  printf ("\n");
}
#endif

int
swf_parse_header1 (SwfdecDecoder * s)
{
  int sig1, sig2, sig3;

  if (swfdec_bits_needbits (&s->b, 8))
    return SWF_NEEDBITS;

  sig1 = swfdec_bits_get_u8 (&s->b);
  sig2 = swfdec_bits_get_u8 (&s->b);
  sig3 = swfdec_bits_get_u8 (&s->b);

  if ((sig1 != 'F' && sig1 != 'C') || sig2 != 'W' || sig3 != 'S')
    return SWF_ERROR;

  s->compressed = (sig1 == 'C');

  s->version = swfdec_bits_get_u8 (&s->b);
  s->length = swfdec_bits_get_u32 (&s->b);

  return SWF_OK;
}

int
swf_inflate_init (SwfdecDecoder * s)
{
  z_stream *z;
  char *compressed_data;
  int compressed_len;
  char *data;
  int ret;

  z = g_new0 (z_stream, 1);
  s->z = z;
  z->zalloc = zalloc;
  z->zfree = zfree;

  compressed_data = s->parse.ptr;
  compressed_len = s->input_data_len -
      ((void *) s->parse.ptr - (void *) s->input_data);

  z->next_in = compressed_data;
  z->avail_in = compressed_len;
  z->opaque = NULL;
  data = g_malloc (s->length);
  z->next_out = data;
  z->avail_out = s->length;

  ret = inflateInit (z);
  SWFDEC_DEBUG("inflateInit returned %d",ret);
  ret = inflate (z, Z_SYNC_FLUSH);
  SWFDEC_DEBUG("inflate returned %d",ret);
  SWFDEC_DEBUG("total out %d",(int)z->total_out);
  SWFDEC_DEBUG("total in %d",(int)z->total_in);

  g_free (s->input_data);

  s->input_data = data;
  s->input_data_len = z->total_in;
  s->parse.ptr = data;

  return SWF_OK;
}

int
swf_parse_header2 (SwfdecDecoder * s)
{
  int rect[4];
  double width, height;

  if (swfdec_bits_needbits (&s->b, 32))
    return SWF_NEEDBITS;

  swfdec_bits_get_rect (&s->b, rect);
  width = rect[1] * SWF_SCALE_FACTOR;
  height = rect[3] * SWF_SCALE_FACTOR;
  s->parse_width = width;
  s->parse_height = height;
  if (s->width == 0) {
    s->width = floor (width);
    s->height = floor (height);
    s->scale_factor = 1.0;
    swfdec_transform_init_identity (&s->transform);
  } else {
    double sw, sh;

    sw = s->width / width;
    sh = s->height / height;
    s->scale_factor = (sw < sh) ? sw : sh;

    s->transform.trans[0] = s->scale_factor;
    s->transform.trans[1] = 0;
    s->transform.trans[2] = 0;
    s->transform.trans[3] = s->scale_factor;
    s->transform.trans[4] = 0.5 * (s->width - width * s->scale_factor);
    s->transform.trans[5] = 0.5 * (s->height - height * s->scale_factor);
  }
  s->irect.x0 = 0;
  s->irect.y0 = 0;
  s->irect.x1 = s->width;
  s->irect.y1 = s->height;
  swfdec_bits_syncbits (&s->b);
  s->rate = swfdec_bits_get_u16 (&s->b) / 256.0;
  SWFDEC_LOG("rate = %g", s->rate);
  s->n_frames = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG("n_frames = %d", s->n_frames);

  s->main_sprite->sound_chunks = g_malloc0 (sizeof (gpointer) * s->n_frames);

  s->main_sprite->n_frames = s->n_frames;

  return SWF_OK;
}


struct tag_func_struct
{
  char *name;
  int (*func) (SwfdecDecoder * s);
  int flag;
};
struct tag_func_struct tag_funcs[] = {
  [ST_END] = {"End", tag_func_zero, 0},
  [ST_SHOWFRAME] = {"ShowFrame", tag_show_frame, 0},
  [ST_DEFINESHAPE] = {"DefineShape", tag_define_shape, 0},
  [ST_FREECHARACTER] = {"FreeCharacter", NULL, 0},
  [ST_PLACEOBJECT] = {"PlaceObject", NULL, 0},
  [ST_REMOVEOBJECT] = {"RemoveObject", swfdec_spriteseg_remove_object, 0},
//      [ ST_DEFINEBITS         ] = { "DefineBits",     NULL,   0 },
  [ST_DEFINEBITSJPEG] = {"DefineBitsJPEG", tag_func_define_bits_jpeg, 0},
  [ST_DEFINEBUTTON] = {"DefineButton", NULL, 0},
  [ST_JPEGTABLES] = {"JPEGTables", swfdec_image_jpegtables, 0},
  [ST_SETBACKGROUNDCOLOR] =
      {"SetBackgroundColor", tag_func_set_background_color, 0},
  [ST_DEFINEFONT] = {"DefineFont", tag_func_define_font, 0},
  [ST_DEFINETEXT] = {"DefineText", tag_func_define_text, 0},
  [ST_DOACTION] = {"DoAction", tag_func_do_action, 0},
  [ST_DEFINEFONTINFO] = {"DefineFontInfo", tag_func_ignore_quiet, 0},
  [ST_DEFINESOUND] = {"DefineSound", tag_func_define_sound, 0},
  [ST_STARTSOUND] = {"StartSound", tag_func_start_sound, 0},
  [ST_DEFINEBUTTONSOUND] =
      {"DefineButtonSound", tag_func_define_button_sound, 0},
  [ST_SOUNDSTREAMHEAD] = {"SoundStreamHead", tag_func_sound_stream_head, 0},
  [ST_SOUNDSTREAMBLOCK] = {"SoundStreamBlock", tag_func_sound_stream_block, 0},
  [ST_DEFINEBITSLOSSLESS] =
      {"DefineBitsLossless", tag_func_define_bits_lossless, 0},
  [ST_DEFINEBITSJPEG2] = {"DefineBitsJPEG2", tag_func_define_bits_jpeg_2, 0},
  [ST_DEFINESHAPE2] = {"DefineShape2", tag_define_shape_2, 0},
  [ST_DEFINEBUTTONCXFORM] = {"DefineButtonCXForm", NULL, 0},
  [ST_PROTECT] = {"Protect", tag_func_zero, 0},
  [ST_PLACEOBJECT2] = {"PlaceObject2", swfdec_spriteseg_place_object_2, 0},
  [ST_REMOVEOBJECT2] = {"RemoveObject2", swfdec_spriteseg_remove_object_2, 0},
  [ST_DEFINESHAPE3] = {"DefineShape3", tag_define_shape_3, 0},
  [ST_DEFINETEXT2] = {"DefineText2", tag_func_define_text_2, 0},
  [ST_DEFINEBUTTON2] = {"DefineButton2", tag_func_define_button_2, 0},
  [ST_DEFINEBITSJPEG3] = {"DefineBitsJPEG3", tag_func_define_bits_jpeg_3, 0},
  [ST_DEFINEBITSLOSSLESS2] =
      {"DefineBitsLossless2", tag_func_define_bits_lossless_2, 0},
  [ST_DEFINEEDITTEXT] = {"DefineEditText", NULL, 0},
  [ST_DEFINEMOVIE] = {"DefineMovie", NULL, 0},
  [ST_DEFINESPRITE] = {"DefineSprite", tag_func_define_sprite, 0},
  [ST_NAMECHARACTER] = {"NameCharacter", NULL, 0},
  [ST_SERIALNUMBER] = {"SerialNumber", NULL, 0},
  [ST_GENERATORTEXT] = {"GeneratorText", NULL, 0},
  [ST_FRAMELABEL] = {"FrameLabel", tag_func_frame_label, 0},
  [ST_SOUNDSTREAMHEAD2] = {"SoundStreamHead2", NULL, 0},
  [ST_DEFINEMORPHSHAPE] = {"DefineMorphShape", NULL, 0},
  [ST_DEFINEFONT2] = {"DefineFont2", tag_func_define_font_2, 0},
  [ST_TEMPLATECOMMAND] = {"TemplateCommand", NULL, 0},
  [ST_GENERATOR3] = {"Generator3", NULL, 0},
  [ST_EXTERNALFONT] = {"ExternalFont", NULL, 0},
  [ST_EXPORTASSETS] = {"ExportAssets", NULL, 0},
  [ST_IMPORTASSETS] = {"ImportAssets", NULL, 0},
  [ST_ENABLEDEBUGGER] = {"EnableDebugger", NULL, 0},
  [ST_MX0] = {"MX0", NULL, 0},
  [ST_MX1] = {"MX1", NULL, 0},
  [ST_MX2] = {"MX2", NULL, 0},
  [ST_MX3] = {"MX3", NULL, 0},
  [ST_MX4] = {"MX4", NULL, 0},
//      [ ST_REFLEX             ] = { "Reflex", NULL,   0 },
};
static const int n_tag_funcs = sizeof (tag_funcs) / sizeof (tag_funcs[0]);

int
swf_parse_tag (SwfdecDecoder * s)
{
  unsigned int x;
  SwfdecBits *b = &s->b;
  char *name;

  SWFDEC_DEBUG ("parsing at %d (%d left)",
      (char *)s->parse.ptr - s->input_data,
      s->parse.end - s->parse.ptr);

  if (swfdec_bits_needbits (&s->b, 2))
    return SWF_NEEDBITS;

  x = swfdec_bits_get_u16 (b);
  s->tag = (x >> 6) & 0x3ff;
  s->tag_len = x & 0x3f;
  if (s->tag_len == 0x3f) {
    if (swfdec_bits_needbits (&s->b, 4))
      return SWF_NEEDBITS;
    s->tag_len = swfdec_bits_get_u32 (b);
  }

  s->func = NULL;
  name = "";

  if (s->tag >= 0 && s->tag < n_tag_funcs) {
    s->func = tag_funcs[s->tag].func;
    name = tag_funcs[s->tag].name;
  }

  if (!s->func) {
    s->func = tag_func_ignore;
  }

  SWFDEC_DEBUG ("tag=%d len=%d name=\"%s\"", s->tag, s->tag_len, name);

  return SWF_OK;
}

int
tag_func_zero (SwfdecDecoder * s)
{
  return SWF_OK;
}

int
tag_func_ignore_quiet (SwfdecDecoder * s)
{
  s->b.ptr += s->tag_len;

  return SWF_OK;
}

int
tag_func_ignore (SwfdecDecoder * s)
{
  char *name = "";

  if (s->tag >= 0 && s->tag < n_tag_funcs) {
    name = tag_funcs[s->tag].name;
  }

  SWFDEC_WARNING ("tag \"%s\" (%d) ignored", name, s->tag);

  s->b.ptr += s->tag_len;

  return SWF_OK;
}

int
tag_func_dumpbits (SwfdecDecoder * s)
{
  SwfdecBits *b = &s->b;

  SWFDEC_DEBUG ("%02x %02x %02x %02x %02x %02x %02x %02x",
    swfdec_bits_get_u8 (b), swfdec_bits_get_u8 (b),
    swfdec_bits_get_u8 (b), swfdec_bits_get_u8 (b),
    swfdec_bits_get_u8 (b), swfdec_bits_get_u8 (b),
    swfdec_bits_get_u8 (b), swfdec_bits_get_u8 (b));

  return SWF_OK;
}

int
tag_func_frame_label (SwfdecDecoder * s)
{
  g_free (swfdec_bits_get_string (&s->b));

  return SWF_OK;
}


