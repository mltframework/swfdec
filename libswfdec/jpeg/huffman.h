
#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#include <glib.h>
#include "bits.h"

typedef struct huffman_entry_struct HuffmanEntry;
typedef GArray HuffmanTable;

struct huffman_entry_struct {
	unsigned int symbol;
	unsigned int mask;
	int n_bits;
	unsigned char value;
};


void huffman_table_dump(HuffmanTable *table);
HuffmanTable *huffman_table_new(void);
void huffman_table_free(HuffmanTable *table);
void huffman_table_add(HuffmanTable *table, guint32 code, gint n_bits,
	gint value);
unsigned int huffman_table_decode_jpeg(HuffmanTable *tab, bits_t *bits);
int huffman_table_decode_macroblock(short *block, HuffmanTable *dc_tab,
	HuffmanTable *ac_tab, bits_t *bits);
int huffman_table_decode(HuffmanTable *dc_tab, HuffmanTable *ac_tab, bits_t *bits);


#endif

