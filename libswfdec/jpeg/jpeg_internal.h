
#ifndef _JPEG_INTERNAL_H_
#define _JPEG_INTERNAL_H_

#include "jpeg.h"
#include "huffman.h"
#include "bits.h"
#include "jpeg_debug.h"



//#define N_COMPONENTS 256
#define JPEG_N_COMPONENTS 4

typedef struct jpeg_scan_struct JpegScan;

#define JPEG_ENTROPY_SEGMENT 0x0001
#define JPEG_LENGTH 0x0002

struct jpeg_decoder_struct {
	int width;
	int height;
	int depth;
	int n_components;
	bits_t bits;

	int width_blocks;
	int height_blocks;

	int restart_interval;

	unsigned char *data;
	unsigned int data_len;

	struct{
		int id;
		int h_oversample;
		int v_oversample;
		int h_subsample;
		int v_subsample;
		int quant_table;
		unsigned char *image;
		int rowstride;
	} components[JPEG_N_COMPONENTS];

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
		int offset;
	}scan_list[10];
	int scan_h_subsample;
	int scan_v_subsample;

	/* scan state */
	int x,y;
	int dc[4];
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


/* jpeg.c */

int jpeg_decoder_sof_baseline_dct(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_define_quant_table(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_define_huffman_table(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_sos(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_soi(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_eoi(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_application0(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_application_misc(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_comment(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_restart_interval(JpegDecoder *dec, bits_t *bits);
int jpeg_decoder_restart(JpegDecoder *dec, bits_t *bits);
void jpeg_decoder_decode_entropy_segment(JpegDecoder *dec, bits_t *bits);


#endif


