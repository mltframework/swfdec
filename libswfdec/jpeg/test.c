
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

/* getfile */

void *getfile(char *path, int *n_bytes);

#if 0
static void printbits(unsigned int bits, int n);


int jpeg_decoder_tag_0xc0(JpegDecoder *dec, unsigned char *bits)
{
	int i;
	int frame_length;
	
	frame_length = (bits[0]<<8) | bits[1];
	dec->depth = bits[2];
	dec->height = (bits[3]<<8) | bits[4];
	dec->width = (bits[5]<<8) | bits[6];
	dec->n_components = bits[7];

	DEBUG("frame_length=%d depth=%d height=%d width=%d n_components=%d\n",
		frame_length, dec->depth, dec->height, dec->width,
		dec->n_components);

	for(i=0;i<dec->n_components;i++){
		dec->components[i].id = bits[8+i*3+0];
		dec->components[i].h_subsample = bits[8+i*3+1] >> 4;
		dec->components[i].v_subsample = bits[8+i*3+1] & 0xf;
		dec->components[i].quant_table = bits[8+i*3+2];

		DEBUG("[%d] id=%d h_subsample=%d v_subsample=%d quant_table=%d\n",
			i,
			dec->components[i].id,
			dec->components[i].h_subsample,
			dec->components[i].v_subsample,
			dec->components[i].quant_table);
	}

	return frame_length;
}

int jpeg_decoder_tag_0xdb(JpegDecoder *dec, unsigned char *bits)
{
	int length;
	int pq;
	int tq;
	int i;
	int *q;
	unsigned char *endptr;

	length = (bits[0]<<8) | bits[1];
	endptr = bits+length;
	bits += 2;

	pq = bits[0]>>4;
	tq = bits[0] & 0xf;
	bits++;

	q = dec->quant_table[tq];
	if(pq){
		for(i=0;i<64;i++){
			q[i] = (*bits++)<<8;
			q[i] |= (*bits++);
		}
	}else{
		for(i=0;i<64;i++){
			q[i] |= (*bits++);
		}
	}

	DEBUG("quant table index %d:\n",tq);
	for(i=0;i<8;i++){
		DEBUG("%3d %3d %3d %3d %3d %3d %3d %3d\n",
			q[0], q[1], q[2], q[3],
			q[4], q[5], q[6], q[7]);
		q+=8;
	}

	if(endptr != bits)DEBUG("endptr != bits\n");

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
			printf("huffcode[%d] = ",k);
			printbits(code>>(15-i), i+1);
			printf("\n");

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

int huffman_table_decode1(HuffmanTable *tab, bits_t *bits)
{
	unsigned int code;
	int i;
	unsigned int bit;
	
	code = peekbits(bits,16);

	bit = 0x8000;
#if 0
	for(i=0;i<16;i++){
		printf("%c",bit&code ? '1' : '0');
		bit>>=1;
	}
	printf("\n");
#endif

	for(i=0;i<tab->length;i++){
		if((code&tab->symbols[i].mask) == tab->symbols[i].symbol){
			//printf("%d\n",tab->symbols[i].value);
			break;
		}
	}
	if(i==tab->length){
		printf("huffman lost sync\n");
		return 0;
	}

	code = getbits(bits,tab->symbols[i].n_bits);
	//printbits(code,tab->symbols[i].n_bits);

	return i;
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
	int j;
	int x;
	int k,r,s,rs;
	int z;
	int zz[64];
	int *q;
	int i;

	while(bits->ptr < bits->end){
		memset(zz,0,sizeof(zz));

		j = huffman_table_decode1(dc_tab, bits);
		s = dc_tab->symbols[j].value;
		x = getbits(bits, s);
		//printf(" "); printbits(x, s);
		if((x>>(s-1)) == 0){
			x -= (1<<s) - 1;
		}
		//printf(" --> [%d] %d n=%d\n", j, s, x);
		zz[0] = x;

		for(k=1;k<64;){
			j = huffman_table_decode1(ac_tab, bits);
			rs = ac_tab->symbols[j].value;
			s = rs & 0xf;
			r = rs >> 4;
			//printf(" --> [%d] %d r=%d s=%d ",j,rs,r,s);

			if(s==0){
				if(r == 15){
					//printf("skip 16\n");
					k+=15;
				}else{
					//printf("end\n");
					break;
				}
			}else{
				k+=r;
				z = getbits(bits,s);
				//printbits(z, s);
				if((z>>(s-1)) == 0){
					z -= (1<<s) - 1;
				}
				//printf(" --> %d\n",z);
				zz[k] = z;
				k++;
			}
		}

		q = zz;
		for(i=0;i<8;i++){
			DEBUG("%3d %3d %3d %3d %3d %3d %3d %3d\n",
				q[0], q[1], q[2], q[3],
				q[4], q[5], q[6], q[7]);
			q+=8;
		}
	}

}

#if 0
int huffman_table_decode(HuffmanTable *tab, unsigned char *bits, int *n_bytes)
{
	int bit;
	unsigned int code;
	int i;
	unsigned int fullcode = 0;
	int empty_bits = 0;
	int j = 0;

	fullcode = *bits++;
	fullcode <<= 8;
	fullcode |= *bits++;
	fullcode <<= 8;
	fullcode |= *bits++;
	fullcode <<= 8;
	fullcode |= *bits++;

	while(1){
		code = fullcode >> 16;

		bit = 0x8000;
		for(i=0;i<16;i++){
			printf("%c",bit&code ? '1' : '0');
			bit>>=1;
		}
		printf("\n");

		for(i=0;i<tab->length;i++){
			if((code&tab->symbols[i].mask) == tab->symbols[i].symbol){
				printf("%d: %d\n",j,tab->symbols[i].value);
				break;
			}
		}
		if(i==tab->length){
			printf("huffman lost sync\n");
			return 0;
		}
		j++;
		fullcode <<= i+1;
		empty_bits += i+1;
		while(empty_bits>8){
			empty_bits -= 8;
			fullcode |= (*bits++) << empty_bits;
		}
	}

	return 0;
}
#endif

int jpeg_decoder_tag_0xc4(JpegDecoder *dec, unsigned char *ptr)
{
	bits_t b, *bits = &b;
	int length;
	int tc;
	int th;
	unsigned char *endptr;
	int n_bytes;
	HuffmanTable *hufftab;

	bits->ptr = ptr;
	bits->idx = 0;

	length = get_be_u16(bits);
	bits->end = ptr+length;

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

static void dumpbits(bits_t *bits)
{
	int i;

	for(i=0;i<8;i++){
		printf("%c",getbit(bits) ? '1' : '0');
	}
	printf("\n");
}

static void printbits(unsigned int bits, int n)
{
	int i;
	int bit = 1<<(n-1);

	for(i=0;i<n;i++){
		printf("%c",(bits&bit) ? '1' : '0');
		bit>>=1;
	}
}

int jpeg_decoder_tag_0xda(JpegDecoder *dec, unsigned char *ptr)
{
	bits_t b, *bits = &b;
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

	bits->ptr = ptr;
	bits->idx = 0;

	length = get_be_u16(bits);
	bits->end = ptr+length;
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

#endif


int main(int argc, char *argv[])
{
	bits_t b, *bits = &b;
	unsigned char *data;
	int tag;
	int len;
	int i;
	JpegDecoder *dec;

	dec = g_new0(JpegDecoder,1);

	data = getfile("biglebowski.jpg",&len);
	//data = getfile("32x32.jpg",&len);
	//data = getfile("32x32_2.jpg",&len);
	//data = getfile("16x16.jpg",&len);

	bits->ptr = data;
	bits->idx = 0;
	bits->end = data + len;

	while(bits->ptr < bits->end){
		x = get_u8(bits);
		if(x != 0xff){
			printf("lost sync\n");
		}
		while(x != 0xff){
			x = get_u8(bits);
		}
		while(x == 0xff){
			x = get_u8(bits);
		}
		tag = get_u8(bits);
		if(tag==0)continue;
		printf("tag %02x\n",tag);
		
		b2 = *bits;

		switch(tag){
		case 0xc0: /* baseline DCT */
		case 0xc1: /* extended sequential DCT */
		case 0xc2: /* progressive DCT */
		case 0xc3: /* lossless (sequential) */
			i += jpeg_decoder_tag_0xc0(dec,&b2);
			break;
		case 0xc4: /* define huffman table */
			i += jpeg_decoder_tag_0xc4(dec,&b2);
			break;
		case 0xd8: /* start of image */
			break;
		case 0xd9: /* end of image */
			break;
		case 0xda: /* start of scan */
			i += jpeg_decoder_tag_0xda(dec,&b2);
			break;
		case 0xdb: /* define quantization table */
			i += jpeg_decoder_tag_0xdb(dec,&b2);
			break;
		case 0xe0: /* jpeg extension (JFIF) */
			printf("JFIF\n");
			break;
		case 0x00:
			break;
		}
	}

	return 0;
}





/* getfile */

void *getfile(char *path, int *n_bytes)
{
	int fd;
	struct stat st;
	void *ptr = NULL;
	int ret;

	fd = open(path, O_RDONLY);
	if(!fd)return NULL;

	ret = fstat(fd, &st);
	if(ret<0){
		close(fd);
		return NULL;
	}
	
	ptr = malloc(st.st_size);
	if(!ptr){
		close(fd);
		return NULL;
	}

	ret = read(fd, ptr, st.st_size);
	if(ret!=st.st_size){
		free(ptr);
		close(fd);
		return NULL;
	}

	if(n_bytes)*n_bytes = st.st_size;

	close(fd);
	return ptr;
}

