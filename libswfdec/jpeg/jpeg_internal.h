
#ifndef _JPEG_INTERNAL_H_
#define _JPEG_INTERNAL_H_

#include "jpeg.h"


#define JPEG_DEBUG(n, format...)	do{ \
	if((n)<=JPEG_DEBUG_LEVEL)jpeg_debug((n),format); \
}while(0)
#define JPEG_DEBUG_LEVEL 4


//#define N_COMPONENTS 256
#define N_COMPONENTS 4

typedef struct huffman_entry_struct HuffmanEntry;
typedef struct huffman_table_struct HuffmanTable;
typedef struct jpeg_scan_struct JpegScan;


struct huffman_entry_struct {
	unsigned int symbol;
	unsigned int mask;
	int n_bits;
	unsigned char value;
};

struct huffman_table_struct {
	int length;

	HuffmanEntry *symbols;
};

struct jpeg_decoder_struct {
	int width;
	int height;
	int depth;
	int n_components;
	bits_t bits;

	struct{
		int id;
		int h_subsample;
		int v_subsample;
		int quant_table;
		unsigned char *image;
		int rowstride;
	} components[N_COMPONENTS];

	short quant_table[4][64];
	HuffmanTable *dc_huff_table[4];
	HuffmanTable *ac_huff_table[4];

	int scan_list_length;
	struct{
		int component_index;
		int dc_table;
		int ac_table;
		int quant_table;
		int x;
		int y;
	}scan_list[10];
	int scan_h_subsample;
	int scan_v_subsample;

	int x,y;
};

struct jpeg_scan_struct {
	int length;
	
	int n_components;
	struct {
		int index;
		int dc_table;
		int ac_table;
	}block_list[10];
};


/* huffman.c */

HuffmanTable *huffman_table_new_jpeg(bits_t *bits);
unsigned int huffman_table_decode_jpeg(HuffmanTable *tab, bits_t *bits);
int huffman_table_decode_macroblock(short *block, HuffmanTable *dc_tab,
	HuffmanTable *ac_tab, bits_t *bits);
int huffman_table_decode(HuffmanTable *dc_tab, HuffmanTable *ac_tab,
	bits_t *bits);
void huffman_table_load_std_jpeg(JpegDecoder *dec);

/* jpeg.c */

int jpeg_decoder_sof_baseline_dct(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_define_quant_table(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_define_huffman_table(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_sos(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_soi(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_eoi(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_application0(JpegDecoder *dec, bits_t *bits);
void jpeg_decoder_decode_entropy_segment(JpegDecoder *dec, bits_t *bits);
void jpeg_debug(int n, const char *format, ... );


#endif


