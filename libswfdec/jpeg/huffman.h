
#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#include <liboil/liboil-stdint.h>

#include "bits.h"

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


void huffman_table_dump(HuffmanTable *table);
HuffmanTable *huffman_table_new(void);
void huffman_table_free(HuffmanTable *table);
void huffman_table_add(HuffmanTable *table, uint32_t code, int n_bits,
	int value);
unsigned int huffman_table_decode_jpeg(HuffmanTable *tab, bits_t *bits);
int huffman_table_decode_macroblock(short *block, HuffmanTable *dc_tab,
	HuffmanTable *ac_tab, bits_t *bits);
int huffman_table_decode(HuffmanTable *dc_tab, HuffmanTable *ac_tab, bits_t *bits);


#endif

