
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


static void printbits(unsigned int bits, int n);


int jpeg_decoder_tag_0xc0(JpegDecoder *dec, bits_t *bits)
{
	int i;
	int length;

	length = get_be_u16(bits);
	bits->end = bits->ptr+length-2;
	
	dec->depth = get_u8(bits);
	dec->height = get_be_u16(bits);
	dec->width = get_be_u16(bits);
	dec->n_components = get_u8(bits);

	DEBUG("frame_length=%d depth=%d height=%d width=%d n_components=%d\n",
		length, dec->depth, dec->height, dec->width,
		dec->n_components);

	for(i=0;i<dec->n_components;i++){
		dec->components[i].id = get_u8(bits);
		dec->components[i].h_subsample = getbits(bits,4);
		dec->components[i].v_subsample = getbits(bits,4);
		dec->components[i].quant_table = get_u8(bits);

		DEBUG("[%d] id=%d h_subsample=%d v_subsample=%d quant_table=%d\n",
			i,
			dec->components[i].id,
			dec->components[i].h_subsample,
			dec->components[i].v_subsample,
			dec->components[i].quant_table);
	}

	if(bits->end != bits->ptr)DEBUG("endptr != bits\n");

	return length;
}

int jpeg_decoder_tag_0xdb(JpegDecoder *dec, bits_t *bits)
{
	int length;
	int pq;
	int tq;
	int i;
	int *q;

	length = get_be_u16(bits);

	pq = getbits(bits,4);
	tq = getbits(bits,4);

	q = dec->quant_table[tq];
	if(pq){
		for(i=0;i<64;i++){
			q[i] = get_be_u16(bits);
		}
	}else{
		for(i=0;i<64;i++){
			q[i] = get_u8(bits);
		}
	}

	DEBUG("quant table index %d:\n",tq);
	for(i=0;i<8;i++){
		DEBUG("%3d %3d %3d %3d %3d %3d %3d %3d\n",
			q[0], q[1], q[2], q[3],
			q[4], q[5], q[6], q[7]);
		q+=8;
	}

	if(bits->end != bits->ptr)DEBUG("endptr != bits\n");

	return length;
}

void generate_code_table(int *huffsize)
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

int jpeg_decoder_tag_0xc4(JpegDecoder *dec, bits_t *bits)
{
	int length;
	int tc;
	int th;
	HuffmanTable *hufftab;

	length = get_be_u16(bits);
	//bits->end = ptr+length;

	tc = getbits(bits,4);
	th = getbits(bits,4);

	DEBUG("huff table index %d:\n",th);
	DEBUG("type %d (%s)\n",tc,tc?"ac":"dc");

	hufftab = huffman_table_new_jpeg(bits);
	if(tc){
		dec->ac_huff_table[th] = hufftab;
	}else{
		dec->dc_huff_table[th] = hufftab;
	}
	
#if 0
	for(i=0;i<16;i++){
		l[i] = *bits++;
	}


	for(i=0;i<16;i++){
		printf("%d:",i);
		for(j=0;j<l[i];j++){
			printf(" %d",*bits);
			bits++;
		}
		printf("\n");
	}

	//generate_code_table(l);
#endif

	if(bits->ptr != bits->end)DEBUG("endptr != bits\n");

	return length;
}

#if 0
static void dumpbits(bits_t *bits)
{
	int i;

	for(i=0;i<8;i++){
		printf("%c",getbit(bits) ? '1' : '0');
	}
	printf("\n");
}
#endif

static void printbits(unsigned int bits, int n)
{
	int i;
	int bit = 1<<(n-1);

	for(i=0;i<n;i++){
		printf("%c",(bits&bit) ? '1' : '0');
		bit>>=1;
	}
}

int jpeg_decoder_tag_0xda(JpegDecoder *dec, bits_t *bits)
{
	int length;
	int i;

	int n_components;
	int component[16];
	int dc_table[16];
	int ac_table[16];
	int spectral_start;
	int spectral_end;
	int approx_high;
	int approx_low;

	length = get_be_u16(bits);
	//bits->end = ptr+length;
	printf("length=%d\n",length);

	n_components = get_u8(bits);
	for(i=0;i<n_components;i++){
		component[i] = get_u8(bits);
		dc_table[i] = getbits(bits,4);
		ac_table[i] = getbits(bits,4);
		syncbits(bits);
	}
	spectral_start = get_u8(bits);
	spectral_end = get_u8(bits);
	approx_high = getbits(bits,4);
	approx_low = getbits(bits,4);
	syncbits(bits);

	if(bits->end != bits->ptr)DEBUG("endptr != bits\n");

	{
	unsigned char *newptr;
	int len;
	int j;

	len = 0;
	j = 0;
	while(1){
		if(bits->ptr[len]==0xff && bits->ptr[len+1]!=0x00){
			break;
		}
		len++;
	}
	printf("entropy length = %d\n", len);
	newptr = malloc(len);
	for(i=0;i<len;i++){
		newptr[j] = bits->ptr[i];
		j++;
		if(bits->ptr[i]==0xff) j++;
	}

	bits->ptr = newptr;
	bits->idx = 0;
	bits->end = newptr + j;
	
#if 1
	huffman_table_decode(dec->dc_huff_table[0],dec->ac_huff_table[0], bits);
#else
	for(i=0;i<j;i++){
		dumpbits(bits);
	}
#endif
	}
	//huffman_table_decode(dec->ac_huff_table[0], bits, NULL);

	return length;
}


