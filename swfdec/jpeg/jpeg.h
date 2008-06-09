
#ifndef _JPEG_DECODER_H_
#define _JPEG_DECODER_H_

#include <jpeg/jpeg_bits.h>

#include <glib.h>
#include <stdint.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define JPEG_LIMIT_COMPONENTS 256
#define JPEG_LIMIT_SCAN_LIST_LENGTH 10


typedef struct _JpegDecoder JpegDecoder;
typedef struct _JpegQuantTable JpegQuantTable;
typedef struct _HuffmanEntry HuffmanEntry;
typedef struct _HuffmanTable HuffmanTable;

struct _HuffmanEntry {
  unsigned int symbol;
  unsigned int mask;
  int n_bits;
  unsigned char value;
};

struct _HuffmanTable {
  int len;
  HuffmanEntry entries[256];
};

struct _JpegQuantTable {
  int pq;
  int16_t quantizer[64];
};

struct _JpegDecoder {
  int width;
  int height;
  int depth;
  int n_components;
  JpegBits bits;
  int error;
  int strict;
  char *error_message;

  int sof_type;

  int width_blocks;
  int height_blocks;

  int restart_interval;

  unsigned char *data;
  unsigned int data_len;

  struct{
    int id;
    int h_sample;
    int v_sample;
    int quant_table;

    int h_subsample;
    int v_subsample;
    unsigned char *image;
    int rowstride;
  } components[JPEG_LIMIT_COMPONENTS];

  JpegQuantTable quant_tables[4];
  HuffmanTable dc_huff_table[4];
  HuffmanTable ac_huff_table[4];

  int scan_list_length;
  struct{
    int component_index;
    int dc_table;
    int ac_table;
    int quant_table;
    int x;
    int y;
    int offset;
  }scan_list[JPEG_LIMIT_SCAN_LIST_LENGTH];
  int scan_h_subsample;
  int scan_v_subsample;

  /* scan state */
  int x,y;
  int dc[4];
};

#define JPEG_MARKER_STUFFED		0x00
#define JPEG_MARKER_TEM			0x01
#define JPEG_MARKER_RES			0x02

#define JPEG_MARKER_SOF_0		0xc0
#define JPEG_MARKER_SOF_1		0xc1
#define JPEG_MARKER_SOF_2		0xc2
#define JPEG_MARKER_SOF_3		0xc3
#define JPEG_MARKER_DEFINE_HUFFMAN_TABLES		0xc4
#define JPEG_MARKER_SOF_5		0xc5
#define JPEG_MARKER_SOF_6		0xc6
#define JPEG_MARKER_SOF_7		0xc7
#define JPEG_MARKER_JPG			0xc8
#define JPEG_MARKER_SOF_9		0xc9
#define JPEG_MARKER_SOF_10		0xca
#define JPEG_MARKER_SOF_11		0xcb
#define JPEG_MARKER_DEFINE_ARITHMETIC_CONDITIONING	0xcc
#define JPEG_MARKER_SOF_13		0xcd
#define JPEG_MARKER_SOF_14		0xce
#define JPEG_MARKER_SOF_15		0xcf

#define JPEG_MARKER_RST_0		0xd0
#define JPEG_MARKER_RST_1		0xd1
#define JPEG_MARKER_RST_2		0xd2
#define JPEG_MARKER_RST_3		0xd3
#define JPEG_MARKER_RST_4		0xd4
#define JPEG_MARKER_RST_5		0xd5
#define JPEG_MARKER_RST_6		0xd6
#define JPEG_MARKER_RST_7		0xd7

#define JPEG_MARKER_SOI			0xd8
#define JPEG_MARKER_EOI			0xd9
#define JPEG_MARKER_SOS			0xda
#define JPEG_MARKER_DEFINE_QUANTIZATION_TABLES		0xdb
#define JPEG_MARKER_DNL			0xdc
#define JPEG_MARKER_DEFINE_RESTART_INTERVAL		0xdd
#define JPEG_MARKER_DHP			0xde
#define JPEG_MARKER_EXP			0xdf
#define JPEG_MARKER_APP(x)		(0xe0 + (x))
#define JPEG_MARKER_JPG_(x)		(0xf0 + (x))
#define JPEG_MARKER_COMMENT                             0xfe

#define JPEG_MARKER_JFIF		JPEG_MARKER_APP(0)

#define JPEG_MARKER_IS_START_OF_FRAME(x) ((x)>=0xc0 && (x) <= 0xcf && (x)!=0xc4 && (x)!=0xc8 && (x)!=0xcc)
#define JPEG_MARKER_IS_APP(x) ((x)>=0xe0 && (x) <= 0xef)
#define JPEG_MARKER_IS_RESET(x) ((x)>=0xd0 && (x)<=0xd7)



JpegDecoder *jpeg_decoder_new(void);
void jpeg_decoder_free(JpegDecoder *dec);
int jpeg_decoder_addbits(JpegDecoder *dec, unsigned char *data, unsigned int len);
int jpeg_decoder_decode (JpegDecoder *dec);
int jpeg_decoder_get_image_size(JpegDecoder *dec, unsigned int *width, unsigned int *height);
int jpeg_decoder_get_component_size(JpegDecoder *dec, int id,
	int *width, int *height);
int jpeg_decoder_get_component_subsampling(JpegDecoder *dec, int id,
	int *h_subsample, int *v_subsample);
int jpeg_decoder_get_component_ptr(JpegDecoder *dec, int id,
	unsigned char **image, int *rowstride);

uint32_t *jpeg_decoder_get_argb_image (JpegDecoder *dec);
int jpeg_decode_argb (uint8_t *data, int length, uint32_t **image,
    unsigned int *width, unsigned int *height);

void jpeg_decoder_error(JpegDecoder *dec, const char *fmt, ...) G_GNUC_PRINTF (2, 3);

int jpeg_decoder_sof_baseline_dct(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_define_quant_table(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_define_huffman_table(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_sos(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_soi(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_eoi(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_application0(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_application_misc(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_comment(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_restart_interval(JpegDecoder *dec, JpegBits *bits);
int jpeg_decoder_restart(JpegDecoder *dec, JpegBits *bits);
void jpeg_decoder_decode_entropy_segment(JpegDecoder *dec);


void huffman_table_init(HuffmanTable *table);

void huffman_table_dump(HuffmanTable *table);
void huffman_table_add(HuffmanTable *table, uint32_t code, int n_bits,
	int value);
unsigned int huffman_table_decode_jpeg(JpegDecoder *dec, HuffmanTable *tab, JpegBits *bits);
int huffman_table_decode_macroblock(JpegDecoder *dec, short *block, HuffmanTable *dc_tab,
	HuffmanTable *ac_tab, JpegBits *bits);
int huffman_table_decode(JpegDecoder *dec, HuffmanTable *dc_tab, HuffmanTable *ac_tab, JpegBits *bits);


#endif

