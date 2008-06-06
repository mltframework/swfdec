
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <liboil/liboil.h>
#include <liboil/liboil-stdint.h>
#include <cogcompat.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jpeg.h"


extern uint8_t jpeg_standard_tables[];
extern int jpeg_standard_tables_size;

void jpeg_decoder_define_huffman_tables (JpegDecoder * dec);
void jpeg_decoder_define_arithmetic_conditioning (JpegDecoder *dec);
void jpeg_decoder_define_quantization_tables (JpegDecoder *dec);
void jpeg_decoder_define_restart_interval (JpegDecoder *dec);
void jpeg_decoder_start_of_frame (JpegDecoder * dec, int marker);
void jpeg_decoder_start_of_scan (JpegDecoder * dec);


/* misc helper function declarations */

static void jpeg_load_standard_huffman_tables (JpegDecoder * dec);

static void jpeg_decoder_verify_header (JpegDecoder *dec);
static void jpeg_decoder_init_decoder (JpegDecoder *dec);


void
jpeg_decoder_error(JpegDecoder *dec, const char *fmt, ...)
{
  va_list varargs;

  if (dec->error) return;

  va_start (varargs, fmt);
#ifdef HAVE_VASPRINTF
  vasprintf(&dec->error_message, fmt, varargs);
#else
  dec->error_message = malloc(250);
  vsnprintf(dec->error_message, 250 - 1, fmt, varargs);
  dec->error_message[250 - 1] = 0;
#endif
  va_end (varargs);

  dec->error = TRUE;
}

#define jpeg_decoder_error(dec, ...) { \
  COG_ERROR("decoder error: "__VA_ARGS__); \
  jpeg_decoder_error (dec, __VA_ARGS__); \
}

static void
jpeg_decoder_verify_header (JpegDecoder *dec)
{
  int max_quant_table = 0;
  int i;

  if (dec->sof_type != JPEG_MARKER_SOF_0) {
    COG_ERROR("only handle baseline DCT");
    dec->error = TRUE;
  }

  if (dec->width < 1) {
    COG_ERROR("height can't be 0");
    dec->error = TRUE;
  }

  switch (dec->sof_type) {
    case JPEG_MARKER_SOF_0:
      /* baseline DCT */
      max_quant_table = 3;
      if (dec->depth != 8) {
        COG_ERROR("depth must be 8 (%d)", dec->depth);
        dec->error = TRUE;
      }
      break;
    case JPEG_MARKER_SOF_1:
      /* extended DCT */
      max_quant_table = 3;
      if (dec->depth != 8 && dec->depth != 12) {
        COG_ERROR("depth must be 8 or 12 (%d)", dec->depth);
        dec->error = TRUE;
      }
      break;
    case JPEG_MARKER_SOF_2:
      /* progressive DCT */
      max_quant_table = 3;
      if (dec->depth != 8 && dec->depth != 12) {
        COG_ERROR("depth must be 8 or 12 (%d)", dec->depth);
        dec->error = TRUE;
      }
      break;
    case JPEG_MARKER_SOF_3:
      /* lossless DCT */
      max_quant_table = 0;
      if (dec->depth < 2 || dec->depth > 16) {
        COG_ERROR("depth must be between 2 and 16 (%d)", dec->depth);
        dec->error = TRUE;
      }
      break;
    default:
      break;
  }

  if (dec->n_components < 0 || dec->n_components > 255) {
    COG_ERROR("n_components must be in the range 0-255 (%d)",
        dec->n_components);
    dec->error = TRUE;
  }
  if (dec->sof_type == JPEG_MARKER_SOF_2 && dec->n_components > 4) {
    COG_ERROR("n_components must be <= 4 for progressive DCT (%d)",
        dec->n_components);
    dec->error = TRUE;
  }

  for (i = 0; i < dec->n_components; i++) {
    if (dec->components[i].id < 0 || dec->components[i].id > 255) {
      COG_ERROR("component ID out of range");
      dec->error = TRUE;
      break;
    }
    if (dec->components[i].h_sample < 1 || dec->components[i].h_sample > 4 ||
        dec->components[i].v_sample < 1 || dec->components[i].v_sample > 4) {
      COG_ERROR("sample factor(s) for component %d out of range %d %d",
          i, dec->components[i].h_sample, dec->components[i].v_sample);
      dec->error = TRUE;
      break;
    }
    if (dec->components[i].quant_table < 0 ||
        dec->components[i].quant_table > max_quant_table) {
      COG_ERROR("quant table for component %d out of range (%d)",
          i, dec->components[i].quant_table);
      dec->error = TRUE;
      break;
    }
  }
}

static void
jpeg_decoder_init_decoder (JpegDecoder *dec)
{
  int max_h_sample = 0;
  int max_v_sample = 0;
  int i;

  /* decoder limitations */
  if (dec->n_components != 3) {
    jpeg_decoder_error(dec, "wrong number of components %d", dec->n_components);
    return;
  }
  if (dec->sof_type != JPEG_MARKER_SOF_0) {
    jpeg_decoder_error(dec, "only handle baseline DCT");
    return;
  }

  for (i=0; i < dec->n_components; i++) {
    max_h_sample = MAX (max_h_sample, dec->components[i].h_sample);
    max_v_sample = MAX (max_v_sample, dec->components[i].v_sample);
  }

  dec->width_blocks =
      (dec->width + 8 * max_h_sample - 1) / (8 * max_h_sample);
  dec->height_blocks =
      (dec->height + 8 * max_v_sample - 1) / (8 * max_v_sample);
  for (i = 0; i < dec->n_components; i++) {
    int rowstride;
    int image_size;

    dec->components[i].h_subsample = max_h_sample /
        dec->components[i].h_sample;
    dec->components[i].v_subsample = max_v_sample /
        dec->components[i].v_sample;

    rowstride = dec->width_blocks * 8 * max_h_sample /
        dec->components[i].h_subsample;
    image_size = rowstride *
        (dec->height_blocks * 8 * max_v_sample /
        dec->components[i].v_subsample);
    dec->components[i].rowstride = rowstride;
    dec->components[i].image = malloc (image_size);
  }
}

#if 0
static char *
sprintbits (char *str, unsigned int bits, int n)
{
  int i;
  int bit = 1 << (n - 1);

  for (i = 0; i < n; i++) {
    str[i] = (bits & bit) ? '1' : '0';
    bit >>= 1;
  }
  str[i] = 0;

  return str;
}

static void
generate_code_table (int *huffsize)
{
  int code;
  int i;
  int j;
  int k;
  char str[33];

  //int l;

  code = 0;
  k = 0;
  for (i = 0; i < 16; i++) {
    for (j = 0; j < huffsize[i]; j++) {
      COG_DEBUG ("huffcode[%d] = %s", k,
          sprintbits (str, code >> (15 - i), i + 1));
      code++;
      k++;
    }
    code <<= 1;
  }

}
#endif

static int
huffman_table_init_jpeg (JpegDecoder *decoder, HuffmanTable *table, JpegBits * bits)
{
  int n_symbols;
  int huffsize[16];
  int i, j, k;
  unsigned int symbol;
  int n = 0;

  huffman_table_init (table);

  /* huffsize[i] is the number of symbols that have length
   * (i+1) bits.  Maximum bit length is 16 bits, so there are
   * 16 entries. */
  n_symbols = 0;
  for (i = 0; i < 16; i++) {
    huffsize[i] = jpeg_bits_get_u8 (bits);
    n++;
    n_symbols += huffsize[i];
  }

  /* Build up the symbol table.  The first symbol is all 0's, with
   * the number of bits determined by the first non-zero entry in
   * huffsize[].  Subsequent symbols with the same bit length are
   * incremented by 1.  Increasing the bit length shifts the
   * symbol 1 bit to the left. */
  symbol = 0;
  k = 0;
  for (i = 0; i < 16; i++) {
    for (j = 0; j < huffsize[i]; j++) {
      huffman_table_add (table, symbol, i + 1, jpeg_bits_get_u8 (bits));
      n++;
      symbol++;
      k++;
    }
    /* This checks that our symbol is actually less than the
     * number of bits we think it is.  This is only triggered
     * for bad huffsize[] arrays. */
    if (symbol >= (1U << (i + 1))) {
      jpeg_decoder_error (decoder, "bad huffsize[] array");
      return -1;
    }

    symbol <<= 1;
  }

  huffman_table_dump (table);

  return n;
}

static int
jpeg_decoder_find_component_by_id (JpegDecoder * dec, int id)
{
  int i;

  for (i = 0; i < dec->n_components; i++) {
    if (dec->components[i].id == id)
      return i;
  }
  COG_DEBUG ("undefined component id %d", id);
  return 0;
}

int
jpeg_decoder_application0 (JpegDecoder * dec, JpegBits * bits)
{
  int length;

  COG_DEBUG ("app0");

  length = get_be_u16 (bits);
  COG_DEBUG ("length=%d", length);

  if (memcmp (bits->ptr, "JFIF", 4) == 0 && bits->ptr[4] == 0) {
    int version;
    int units;
    int x_density;
    int y_density;
    int x_thumbnail;
    int y_thumbnail;

    COG_DEBUG ("JFIF");
    bits->ptr += 5;

    version = get_be_u16 (bits);
    units = get_u8 (bits);
    x_density = get_be_u16 (bits);
    y_density = get_be_u16 (bits);
    x_thumbnail = get_u8 (bits);
    y_thumbnail = get_u8 (bits);

    COG_DEBUG ("version = %04x", version);
    COG_DEBUG ("units = %d", units);
    COG_DEBUG ("x_density = %d", x_density);
    COG_DEBUG ("y_density = %d", y_density);
    COG_DEBUG ("x_thumbnail = %d", x_thumbnail);
    COG_DEBUG ("y_thumbnail = %d", y_thumbnail);

  }

  if (memcmp (bits->ptr, "JFXX", 4) == 0 && bits->ptr[4] == 0) {
    COG_DEBUG ("JFIF extension (not handled)");
    bits->ptr += length - 2;
  }

  return length;
}

int
jpeg_decoder_application_misc (JpegDecoder * dec, JpegBits * bits)
{
  int length;

  COG_DEBUG ("appX");

  length = get_be_u16 (bits);
  COG_DEBUG ("length=%d", length);

  COG_DEBUG ("JPEG application tag X ignored");
  //dumpbits (bits);

  bits->ptr += length - 2;

  return length;
}

int
jpeg_decoder_comment (JpegDecoder * dec, JpegBits * bits)
{
  int length;

  COG_DEBUG ("comment");

  length = get_be_u16 (bits);
  COG_DEBUG ("length=%d", length);

  //dumpbits (bits);

  bits->ptr += length - 2;

  return length;
}

int
jpeg_decoder_restart_interval (JpegDecoder * dec, JpegBits * bits)
{
  int length;

  COG_DEBUG ("comment");

  length = get_be_u16 (bits);
  COG_DEBUG ("length=%d", length);

  dec->restart_interval = get_be_u16 (bits);
  COG_DEBUG ("restart_interval=%d", dec->restart_interval);

  return length;
}

int
jpeg_decoder_restart (JpegDecoder * dec, JpegBits * bits)
{
  COG_DEBUG ("restart");

  return 0;
}

void
jpeg_decoder_decode_entropy_segment (JpegDecoder * dec)
{
  JpegBits * bits = &dec->bits;
  JpegBits b2, *bits2 = &b2;
  short block[64];
  short block2[64];
  unsigned char *newptr;
  int len;
  int maxlen;
  int j;
  int i;
  int go;
  int x, y;
  int n;
  int ret;

  len = 0;
  maxlen = jpeg_bits_available (bits) - 1;
  j = 0;
  while (len < maxlen) {
    if (bits->ptr[len] == 0xff && bits->ptr[len + 1] != 0x00) {
      break;
    }
    len++;
  }
  COG_DEBUG ("entropy length = %d", len);

  /* we allocate extra space, since the getbits() code can
   * potentially read past the end of the buffer */
  newptr = malloc (len + 2);
  for (i = 0; i < len; i++) {
    newptr[j] = bits->ptr[i];
    j++;
    if (bits->ptr[i] == 0xff)
      i++;
  }
  bits->ptr += len;

  bits2->ptr = newptr;
  bits2->idx = 0;
  bits2->end = newptr + j;
  newptr[j] = 0;
  newptr[j + 1] = 0;

  dec->dc[0] = dec->dc[1] = dec->dc[2] = dec->dc[3] = 128 * 8;
  go = 1;
  x = dec->x;
  y = dec->y;
  n = dec->restart_interval;
  if (n == 0) n = (1<<26); /* max number of blocks */
  while (go && n-- > 0) {
    for (i = 0; i < dec->scan_list_length; i++) {
      int dc_table_index;
      int ac_table_index;
      int quant_index;
      unsigned char *ptr;
      int component_index;

      COG_DEBUG ("%d,%d: component=%d dc_table=%d ac_table=%d",
          x, y,
          dec->scan_list[i].component_index,
          dec->scan_list[i].dc_table, dec->scan_list[i].ac_table);

      component_index = dec->scan_list[i].component_index;
      dc_table_index = dec->scan_list[i].dc_table;
      ac_table_index = dec->scan_list[i].ac_table;
      quant_index = dec->scan_list[i].quant_table;

      ret = huffman_table_decode_macroblock (dec, block,
          &dec->dc_huff_table[dc_table_index],
          &dec->ac_huff_table[ac_table_index], bits2);
      if (ret < 0) {
        COG_DEBUG ("%d,%d: component=%d dc_table=%d ac_table=%d",
            x, y,
            dec->scan_list[i].component_index,
            dec->scan_list[i].dc_table, dec->scan_list[i].ac_table);
        n = 0;
        break;
      }

      COG_DEBUG ("using quant table %d", quant_index);
      oil_mult8x8_s16 (block2, block, dec->quant_tables[quant_index].quantizer,
          sizeof (short) * 8, sizeof(short) * 8, sizeof (short) * 8);
      dec->dc[component_index] += block2[0];
      block2[0] = dec->dc[component_index];
      oil_unzigzag8x8_s16 (block, sizeof (short) * 8, block2,
          sizeof (short) * 8);
      oil_idct8x8_s16 (block2, sizeof (short) * 8, block, sizeof (short) * 8);
      oil_trans8x8_s16 (block, sizeof (short) * 8, block2, sizeof (short) * 8);

      ptr = dec->components[component_index].image +
          x * dec->components[component_index].h_sample +
          dec->scan_list[i].offset +
          dec->components[component_index].rowstride * y *
          dec->components[component_index].v_sample;

      oil_clipconv8x8_u8_s16 (ptr,
          dec->components[component_index].rowstride,
          block, sizeof (short) * 8);
    }
    x += 8;
    if (x * dec->scan_h_subsample >= dec->width) {
      x = 0;
      y += 8;
    }
    if (y * dec->scan_v_subsample >= dec->height) {
      go = 0;
    }
  }
  dec->x = x;
  dec->y = y;
  free (newptr);
}



JpegDecoder *
jpeg_decoder_new (void)
{
  JpegDecoder *dec;

  oil_init ();

  dec = malloc (sizeof(JpegDecoder));
  memset (dec, 0, sizeof(JpegDecoder));

  jpeg_load_standard_huffman_tables (dec);

  return dec;
}

void
jpeg_decoder_free (JpegDecoder * dec)
{
  int i;

  for (i = 0; i < JPEG_LIMIT_COMPONENTS; i++) {
    if (dec->components[i].image)
      free (dec->components[i].image);
  }

  if (dec->data)
    free (dec->data);

  free (dec);
}

static int
jpeg_decoder_get_marker (JpegDecoder *dec, int *marker)
{
  int a,b;
  JpegBits *bits = &dec->bits;

  if (jpeg_bits_available(bits) < 2) {
    return FALSE;
  }

  a = jpeg_bits_get_u8(bits);
  if (a != 0xff) {
    jpeg_decoder_error(dec, "expected marker, not 0x%02x", a);
    return FALSE;
  }

  do {
    b = jpeg_bits_get_u8 (bits);
  } while (b == 0xff && jpeg_bits_error(bits));

  *marker = b;
  return TRUE;
}

static void
jpeg_decoder_skip (JpegDecoder *dec)
{
  int length;

  length = jpeg_bits_get_u16_be (&dec->bits);
  jpeg_bits_skip (&dec->bits, length - 2);
}

int
jpeg_decoder_decode (JpegDecoder *dec)
{
  JpegBits *bits;
  int marker;

  dec->error = FALSE;

  bits = &dec->bits;

  /* Note: The spec is ambiguous as to whether fill bytes can come
   * before the first marker.  We'll assume yes. */
  if (!jpeg_decoder_get_marker (dec, &marker)) {
    return FALSE;
  }
  if (dec->strict && marker != JPEG_MARKER_SOI) {
    jpeg_decoder_error(dec, "not a JPEG image");
    return FALSE;
  }

  /* Interpret markers up to the start of frame */
  while (!dec->error) {
    if (!jpeg_decoder_get_marker (dec, &marker)) {
      return FALSE;
    }

    if (marker == JPEG_MARKER_DEFINE_HUFFMAN_TABLES) {
      jpeg_decoder_define_huffman_tables (dec);
    } else if (marker == JPEG_MARKER_DEFINE_ARITHMETIC_CONDITIONING) {
      jpeg_decoder_define_arithmetic_conditioning (dec);
    } else if (marker == JPEG_MARKER_DEFINE_QUANTIZATION_TABLES) {
      jpeg_decoder_define_quantization_tables (dec);
    } else if (marker == JPEG_MARKER_DEFINE_RESTART_INTERVAL) {
      jpeg_decoder_define_restart_interval (dec);
    } else if (JPEG_MARKER_IS_APP(marker)) {
      /* FIXME decode app segment */
      jpeg_decoder_skip (dec);
    } else if (marker == JPEG_MARKER_COMMENT) {
      jpeg_decoder_skip (dec);
    } else if (JPEG_MARKER_IS_START_OF_FRAME(marker)) {
      break;
    } else if (marker == JPEG_MARKER_SOI) {
      if (dec->strict) {
        jpeg_decoder_error (dec, "unexpected SOI");
        return FALSE;
      }
    } else if (marker == JPEG_MARKER_EOI) {
      if (dec->strict) {
        jpeg_decoder_error (dec, "unexpected EOI");
        return FALSE;
      }
    } else {
      jpeg_decoder_error(dec, "unexpected marker 0x%02x", marker);
      return FALSE;
    }
  }
  if (dec->error) {
    return FALSE;
  }

  jpeg_decoder_start_of_frame(dec, marker);

  jpeg_decoder_verify_header (dec);
  if (dec->error) {
    return FALSE;
  }

  jpeg_decoder_init_decoder (dec);
  if (dec->error) {
    return FALSE;
  }

  /* In this section, we loop over parse units until we reach the end
   * of the image. */
  while (!dec->error) {
    if (!jpeg_decoder_get_marker (dec, &marker)) {
      return FALSE;
    }

    if (marker == JPEG_MARKER_DEFINE_HUFFMAN_TABLES) {
      jpeg_decoder_define_huffman_tables (dec);
    } else if (marker == JPEG_MARKER_DEFINE_ARITHMETIC_CONDITIONING) {
      jpeg_decoder_define_arithmetic_conditioning (dec);
    } else if (marker == JPEG_MARKER_DEFINE_QUANTIZATION_TABLES) {
      jpeg_decoder_define_quantization_tables (dec);
    } else if (marker == JPEG_MARKER_DEFINE_RESTART_INTERVAL) {
      jpeg_decoder_define_restart_interval (dec);
    } else if (JPEG_MARKER_IS_APP(marker)) {
      jpeg_decoder_skip (dec);
    } else if (marker == JPEG_MARKER_COMMENT) {
      jpeg_decoder_skip (dec);
    } else if (marker == JPEG_MARKER_SOS) {
      jpeg_decoder_start_of_scan (dec);
      if (dec->error) {
        return FALSE;
      }
      jpeg_decoder_decode_entropy_segment (dec);
    } else if (JPEG_MARKER_IS_RESET(marker)) {
      jpeg_decoder_decode_entropy_segment (dec);
    } else if (marker == JPEG_MARKER_SOI) {
      if (dec->strict) {
        jpeg_decoder_error (dec, "unexpected SOI");
        return FALSE;
      }
    } else if (marker == JPEG_MARKER_EOI) {
      if (dec->strict) {
        jpeg_decoder_error (dec, "unexpected EOI");
        return FALSE;
      } else {
        break;
      }
    } else {
      jpeg_decoder_error(dec, "unexpected marker 0x%02x", marker);
      return FALSE;
    }
  }
  if (dec->error) {
    return FALSE;
  }

  return TRUE;
}

/* handle markers */

void
jpeg_decoder_define_huffman_tables (JpegDecoder * dec)
{
  JpegBits *bits = &dec->bits;
  int length;
  int tc;
  int th;
  int x;
  HuffmanTable *hufftab;

  COG_DEBUG ("define huffman tables");

  length = jpeg_bits_get_u16_be (bits);
  if (length < 2) {
    jpeg_decoder_error(dec, "length too short");
    return;
  }
  length -= 2;

  while (length > 0) {
    x = jpeg_bits_get_u8 (bits);
    length--;

    tc = x >> 4;
    th = x & 0xf;

    COG_DEBUG ("huff table type %d (%s) idx %d", tc, tc ? "ac" : "dc", th);
    if (tc > 1 || th > 3) {
      jpeg_decoder_error(dec, "huffman table type or idx out of range");
      return;
    }

    if (tc) {
      hufftab = &dec->ac_huff_table[th];
      length -= huffman_table_init_jpeg (dec, hufftab, bits);
    } else {
      hufftab = &dec->dc_huff_table[th];
      length -= huffman_table_init_jpeg (dec, hufftab, bits);
    }
    if (dec->error) {
      return;
    }
  }
  if (length < 0) {
    jpeg_decoder_error(dec, "huffman table overran available bytes");
    return;
  }
}

void
jpeg_decoder_define_quantization_tables (JpegDecoder *dec)
{
  JpegBits *bits = &dec->bits;
  JpegQuantTable *table;
  int length;
  int pq;
  int tq;
  int i;

  COG_INFO ("define quantization table");

  length = jpeg_bits_get_u16_be (bits);
  if (length < 2) {
    jpeg_decoder_error(dec, "length too short");
    return;
  }
  length -= 2;

  while (length > 0) {
    int x;
    
    x = jpeg_bits_get_u8 (bits);
    length--;
    pq = x >> 4;
    tq = x & 0xf;

    if (pq > 1) {
      jpeg_decoder_error (dec, "bad pq value");
      return;
    }
    if (tq > 3) {
      jpeg_decoder_error (dec, "bad tq value");
      return;
    }

    table = &dec->quant_tables[tq];
    if (pq) {
      for (i = 0; i < 64; i++) {
        table->quantizer[i] = jpeg_bits_get_u16_be (bits);
        length -= 2;
      }
    } else {
      for (i = 0; i < 64; i++) {
        table->quantizer[i] = jpeg_bits_get_u8 (bits);
        length -= 1;
      }
    }
  }
  if (length < 0) {
    jpeg_decoder_error(dec, "quantization table overran available bytes");
    return;
  }
}

void
jpeg_decoder_define_restart_interval (JpegDecoder *dec)
{
  JpegBits *bits = &dec->bits;
  int length;

  length = jpeg_bits_get_u16_be (bits);
  if (length != 4) {
    jpeg_decoder_error(dec, "length supposed to be 4 (%d)", length);
    return;
  }

  /* FIXME this needs to be checked somewhere */
  dec->restart_interval = jpeg_bits_get_u16_be (bits);
}

void
jpeg_decoder_define_arithmetic_conditioning (JpegDecoder *dec)
{
  /* we don't handle arithmetic coding, so skip it */
  jpeg_decoder_skip (dec);
}

void
jpeg_decoder_start_of_frame (JpegDecoder * dec, int marker)
{
  JpegBits *bits = &dec->bits;
  int i;
  int length;

  COG_INFO ("start of frame");

  dec->sof_type = marker;

  length = jpeg_bits_get_u16_be (bits);

  if (jpeg_bits_available(bits) < length) {
    jpeg_decoder_error(dec, "not enough data for start_of_frame (%d < %d)",
        length, jpeg_bits_available(bits));
    return;
  }

  dec->depth = jpeg_bits_get_u8 (bits);
  dec->height = jpeg_bits_get_u16_be (bits);
  dec->width = jpeg_bits_get_u16_be (bits);
  dec->n_components = jpeg_bits_get_u8 (bits);

  COG_DEBUG (
      "frame_length=%d depth=%d height=%d width=%d n_components=%d", length,
      dec->depth, dec->height, dec->width, dec->n_components);

  if (dec->n_components * 3 + 8 != length) {
    jpeg_decoder_error(dec, "inconsistent header");
    return;
  }

  for (i = 0; i < dec->n_components; i++) {
    dec->components[i].id = get_u8 (bits);
    dec->components[i].h_sample = getbits (bits, 4);
    dec->components[i].v_sample = getbits (bits, 4);
    dec->components[i].quant_table = get_u8 (bits);

    COG_DEBUG (
        "[%d] id=%d h_sample=%d v_sample=%d quant_table=%d", i,
        dec->components[i].id, dec->components[i].h_sample,
        dec->components[i].v_sample, dec->components[i].quant_table);
  }
}

void
jpeg_decoder_start_of_scan (JpegDecoder * dec)
{
  JpegBits *bits = &dec->bits;
  int length;
  int i;
  int spectral_start;
  int spectral_end;
  int approx_high;
  int approx_low;
  int n;
  int tmp;
  int n_components;

  COG_DEBUG ("start of scan");

  length = jpeg_bits_get_u16_be (bits);
  COG_DEBUG ("length=%d", length);

  n_components = jpeg_bits_get_u8 (bits);
  n = 0;
  dec->scan_h_subsample = 0;
  dec->scan_v_subsample = 0;
  for (i = 0; i < n_components; i++) {
    int component_id;
    int dc_table;
    int ac_table;
    int x;
    int y;
    int idx;
    int h_subsample;
    int v_subsample;
    int quant_index;

    component_id = jpeg_bits_get_u8 (bits);
    tmp = jpeg_bits_get_u8 (bits);
    dc_table = tmp >> 4;
    ac_table = tmp & 0xf;
    idx = jpeg_decoder_find_component_by_id (dec, component_id);

    h_subsample = dec->components[idx].h_sample;
    v_subsample = dec->components[idx].v_sample;
    quant_index = dec->components[idx].quant_table;

    for (y = 0; y < v_subsample; y++) {
      for (x = 0; x < h_subsample; x++) {
        dec->scan_list[n].component_index = idx;
        dec->scan_list[n].dc_table = dc_table;
        dec->scan_list[n].ac_table = ac_table;
        dec->scan_list[n].quant_table = quant_index;
        dec->scan_list[n].x = x;
        dec->scan_list[n].y = y;
        dec->scan_list[n].offset =
            y * 8 * dec->components[idx].rowstride + x * 8;
        n++;
        if (n > JPEG_LIMIT_SCAN_LIST_LENGTH) {
          jpeg_decoder_error(dec, "scan list too long");
          return;
        }
      }
    }

    dec->scan_h_subsample = MAX (dec->scan_h_subsample, h_subsample);
    dec->scan_v_subsample = MAX (dec->scan_v_subsample, v_subsample);

    COG_DEBUG ("component %d: idx=%d dc_table=%d ac_table=%d n=%d",
        component_id, idx, dc_table, ac_table, n);
  }
  dec->scan_list_length = n;


  spectral_start = jpeg_bits_get_u8 (bits);
  spectral_end = jpeg_bits_get_u8 (bits);
  COG_DEBUG ("spectral range [%d,%d]", spectral_start, spectral_end);
  tmp = jpeg_bits_get_u8 (bits);
  approx_high = tmp >> 4;
  approx_low = tmp & 0xf;
  COG_DEBUG ("approx range [%d,%d]", approx_low, approx_high);

  dec->x = 0;
  dec->y = 0;
  dec->dc[0] = dec->dc[1] = dec->dc[2] = dec->dc[3] = 128 * 8;
}












int
jpeg_decoder_addbits (JpegDecoder * dec, unsigned char *data, unsigned int len)
{
  unsigned int offset;

  offset = dec->bits.ptr - dec->data;

  dec->data = realloc (dec->data, dec->data_len + len);
  memcpy (dec->data + dec->data_len, data, len);
  dec->data_len += len;

  dec->bits.ptr = dec->data + offset;
  dec->bits.end = dec->data + dec->data_len;

  return 0;
}

int
jpeg_decoder_get_image_size (JpegDecoder * dec, unsigned int *width, unsigned int *height)
{
  if (width)
    *width = (unsigned) dec->width;
  if (height)
    *height = (unsigned) dec->height;

  return 0;
}

int
jpeg_decoder_get_component_ptr (JpegDecoder * dec, int id,
    unsigned char **image, int *rowstride)
{
  int i;

  i = jpeg_decoder_find_component_by_id (dec, id);
  if (image)
    *image = dec->components[i].image;
  if (rowstride)
    *rowstride = dec->components[i].rowstride;

  return 0;
}

int
jpeg_decoder_get_component_size (JpegDecoder * dec, int id,
    int *width, int *height)
{
  int i;

  /* subsampling sizes are rounded up */

  i = jpeg_decoder_find_component_by_id (dec, id);
  if (width)
    *width = (dec->width - 1) / dec->components[i].h_subsample + 1;
  if (height)
    *height = (dec->height - 1) / dec->components[i].v_subsample + 1;

  return 0;
}

int
jpeg_decoder_get_component_subsampling (JpegDecoder * dec, int id,
    int *h_subsample, int *v_subsample)
{
  int i;

  i = jpeg_decoder_find_component_by_id (dec, id);
  if (h_subsample)
    *h_subsample = dec->components[i].h_subsample;
  if (v_subsample)
    *v_subsample = dec->components[i].v_subsample;

  return 0;
}

#if 0
int
jpeg_decoder_parse (JpegDecoder * dec)
{
  JpegBits *bits = &dec->bits;
  JpegBits b2;
  unsigned int x;
  unsigned int tag;
  int i;

  while (bits->ptr < bits->end) {
    x = get_u8 (bits);
    if (x != 0xff) {
      int n = 0;

      while (x != 0xff) {
        x = get_u8 (bits);
        n++;
      }
      OIL_DEBUG ("lost sync, skipped %d bytes", n);
    }
    while (x == 0xff) {
      x = get_u8 (bits);
    }
    tag = x;
    OIL_DEBUG ("tag %02x", tag);

    b2 = *bits;

    for (i = 0; i < n_jpeg_markers - 1; i++) {
      if (tag == jpeg_markers[i].tag) {
        break;
      }
    }
    OIL_DEBUG ("tag: %s", jpeg_markers[i].name);
    if (jpeg_markers[i].func) {
      jpeg_markers[i].func (dec, &b2);
    } else {
      OIL_DEBUG ("unhandled or illegal JPEG marker (0x%02x)", tag);
      dumpbits (&b2);
    }
    if (jpeg_markers[i].flags & JPEG_ENTROPY_SEGMENT) {
      jpeg_decoder_decode_entropy_segment (dec, &b2);
    }
    syncbits (&b2);
    bits->ptr = b2.ptr;
  }

  return 0;
}
#endif


/* misc helper functins */

static void
jpeg_load_standard_huffman_tables (JpegDecoder * dec)
{
  JpegBits b, *bits = &b;

  bits->ptr = jpeg_standard_tables;
  bits->idx = 0;
  bits->end = jpeg_standard_tables + jpeg_standard_tables_size;

  huffman_table_init_jpeg (dec, &dec->dc_huff_table[0], bits);
  huffman_table_init_jpeg (dec, &dec->ac_huff_table[0], bits);
  huffman_table_init_jpeg (dec, &dec->dc_huff_table[1], bits);
  huffman_table_init_jpeg (dec, &dec->ac_huff_table[1], bits);
}



