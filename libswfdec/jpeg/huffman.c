
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#include <swf.h>
#include "jpeg_internal.h"
#include "../bits.h"

#define DEBUG printf

/* misc helper function definitions */

static char *sprintbits(char *str, unsigned int bits, int n);


static void huffman_table_dump(HuffmanTable *table)
{
	unsigned int n_bits;
	unsigned int code;
	char str[33];
	int i;

	JPEG_DEBUG(0,"dumping huffman table %p\n",table);
	for(i=0;i<table->length;i++){
		n_bits = table->symbols[i].n_bits;
		code = table->symbols[i].symbol >> (16-n_bits);
		sprintbits(str, code, n_bits);
		JPEG_DEBUG(0,"%s --> %d\n", str, table->symbols[i].value);
	}
}

HuffmanTable *huffman_table_new_jpeg(bits_t *bits)
{
	int total;
	int huffsize[16];
	int i,j,k;
	HuffmanTable *table;
	unsigned int code;

	table = g_new0(HuffmanTable, 1);

	total = 0;
	for(i=0;i<16;i++){
		huffsize[i] = get_u8(bits);
		total += huffsize[i];
	}
	table->length = total;
	table->symbols = g_new0(HuffmanEntry, total);

	for(i=0;i<total;i++){
		table->symbols[i].value = get_u8(bits);
	}

	code = 0;
	k = 0;
	for(i=0;i<16;i++){
		for(j=0;j<huffsize[i];j++){
			table->symbols[k].symbol = code;
			table->symbols[k].mask = 0xffff ^ (0xffff>>(i+1));
			table->symbols[k].n_bits = i + 1;
			code += 0x8000>>i;
			k++;
		}
	}

	huffman_table_dump(table);

	return table;
}

unsigned int huffman_table_decode_jpeg(HuffmanTable *tab, bits_t *bits)
{
	unsigned int code;
	int i;
	char str[33];

	code = peekbits(bits,16);
	for(i=0;i<tab->length;i++){
		if((code&tab->symbols[i].mask) == tab->symbols[i].symbol){
			code = getbits(bits,tab->symbols[i].n_bits);
			sprintbits(str, code, tab->symbols[i].n_bits);
			JPEG_DEBUG(0,"%s --> %d\n", str, tab->symbols[i].value);
			return tab->symbols[i].value;
		}
	}
	printf("huffman sync lost\n");

	return -1;
}

int huffman_table_decode_macroblock(short *block, HuffmanTable *dc_tab,
	HuffmanTable *ac_tab, bits_t *bits)
{
	int r,s,x,rs;
	int k;
	char str[33];

	memset(block,0,sizeof(short)*64);

	s = huffman_table_decode_jpeg(dc_tab, bits);
	if(s<0)return -1;
	x = getbits(bits, s);
	if((x>>(s-1)) == 0){
		x -= (1<<s) - 1;
	}
	JPEG_DEBUG(0,"s=%d (block[0]=%d)\n",s,x);
	block[0] = x;

	for(k=1;k<64;k++){
		rs = huffman_table_decode_jpeg(ac_tab, bits);
		if(rs<0 || bits->ptr >= bits->end){
			JPEG_DEBUG(0,"overrun\n");
			return -1;
		}
		s = rs & 0xf;
		r = rs >> 4;
		if(s==0){
			if(r==15){
				JPEG_DEBUG(0,"r=%d s=%d (skip 16)\n",r,s);
				k+=15;
			}else{
				JPEG_DEBUG(0,"r=%d s=%d (eob)\n",r,s);
				break;
			}
		}else{
			k += r;
			if(k>=64){
				printf("macroblock overrun\n");
				return -1;
			}
			x = getbits(bits, s);
			sprintbits(str, x, s);
			if((x>>(s-1)) == 0){
				x -= (1<<s) - 1;
			}
			block[k] = x;
			JPEG_DEBUG(0,"r=%d s=%d (%s -> block[%d]=%d)\n",r,s,str,k,x);
		}
	}
	return 0;
}

int huffman_table_decode(HuffmanTable *dc_tab, HuffmanTable *ac_tab, bits_t *bits)
{
	short zz[64];
	int ret;
	int i;
	short *q;

	while(bits->ptr < bits->end){
		ret = huffman_table_decode_macroblock(zz, dc_tab, ac_tab, bits);
		if(ret<0)return -1;

		q = zz;
		for(i=0;i<8;i++){
			DEBUG("%3d %3d %3d %3d %3d %3d %3d %3d\n",
				q[0], q[1], q[2], q[3],
				q[4], q[5], q[6], q[7]);
			q+=8;
		}
	}

	return 0;
}

/* misc helper functins */

static char *sprintbits(char *str, unsigned int bits, int n)
{
	int i;
	int bit = 1<<(n-1);

	for(i=0;i<n;i++){
		str[i] = (bits&bit) ? '1' : '0';
		bit>>=1;
	}
	str[i] = 0;

	return str;
}

