
#ifndef _JPEG_INTERNAL_H_
#define _JPEG_INTERNAL_H_

#include "jpeg.h"


//#define N_COMPONENTS 256
#define N_COMPONENTS 4

typedef struct{
	unsigned int symbol;
	unsigned int mask;
	int n_bits;
	unsigned char value;
}HuffmanEntry;

typedef struct{
	int length;

	HuffmanEntry *symbols;
}HuffmanTable;

typedef struct{

	int width;
	int height;
	int depth;
	int n_components;
	struct{
		int id;
		int h_subsample;
		int v_subsample;
		int quant_table;
		unsigned char *image;
		int rowstride;
	} components[N_COMPONENTS];

	int quant_table[4][64];
	HuffmanTable *dc_huff_table[4];
	HuffmanTable *ac_huff_table[4];
}JpegDecoder;

typedef struct {
	int length;
	
	int n_components;
	struct {
		int index;
		/* ... */
	}components[N_COMPONENTS];

}JpegScan;


/* huffman.c */

HuffmanTable *huffman_table_new_jpeg(bits_t *bits);
unsigned int huffman_table_decode_jpeg(HuffmanTable *tab, bits_t *bits);
int huffman_table_decode_macroblock(short *block, HuffmanTable *dc_tab,
	HuffmanTable *ac_tab, bits_t *bits);
int huffman_table_decode(HuffmanTable *dc_tab, HuffmanTable *ac_tab,
	bits_t *bits);

/* jpeg.c */

int jpeg_decoder_tag_0xc0(JpegDecoder *dec, unsigned char *bits);
int jpeg_decoder_tag_0xdb(JpegDecoder *dec, unsigned char *bits);
int jpeg_decoder_tag_0xc4(JpegDecoder *dec, unsigned char *ptr);
int jpeg_decoder_tag_0xda(JpegDecoder *dec, unsigned char *ptr);


#endif

