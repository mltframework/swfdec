
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


#if 0
static void generate_code_table(int *huffsize)
{
	int code;
	int i;
	int j;
	int k;
	//int l;

	code = 0;
	k = 0;
	for(i=0;i<16;i++){
		for(j=0;j<huffsize[i];j++){
			printf("huffcode[%d] = ",k);
			printbits(code>>(15-i), i+1);
			printf("\n");
			code++;
			k++;
		}
		code <<= 1;
	}

}
#endif

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
			//printf("huffcode[%d] = ",k);
			//printbits(code>>(15-i), i+1);
			//printf("\n");

			table->symbols[k].symbol = code;
			table->symbols[k].mask = 0xffff ^ (0xffff>>(i+1));
			table->symbols[k].n_bits = i + 1;
			code += 0x8000>>i;
			k++;
		}
	}

	for(i=0;i<total;i++){
		DEBUG("symbol=0x%04x mask=0x%04x value=%d\n",
			table->symbols[i].symbol,
			table->symbols[i].mask,
			table->symbols[i].value);
	}

	return table;
}

unsigned int huffman_table_decode_jpeg(HuffmanTable *tab, bits_t *bits)
{
	unsigned int code;
	int i;

	code = peekbits(bits,16);
	for(i=0;i<tab->length;i++){
		if((code&tab->symbols[i].mask) == tab->symbols[i].symbol){
			code = getbits(bits,tab->symbols[i].n_bits);
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

	memset(block,0,sizeof(short)*64);

	s = huffman_table_decode_jpeg(dc_tab, bits);
	if(s<0)return -1;
	x = getbits(bits, s);
	if((x>>(s-1)) == 0){
		x -= (1<<s) - 1;
	}
	block[0] = x;

	for(k=1;k<64;k++){
		rs = huffman_table_decode_jpeg(ac_tab, bits);
		if(rs<0)return -1;
		if(bits->ptr >= bits->end)return -1;
		s = rs & 0xf;
		r = rs >> 4;
		if(s==0){
			if(r==15){
				k+=15;
			}else{
				break;
			}
		}else{
			k += r;
			if(k>=64){
				printf("macroblock overrun\n");
				return -1;
			}
			x = getbits(bits, s);
			if((x>>(s-1)) == 0){
				x -= (1<<s) - 1;
			}
			block[k] = x;
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

