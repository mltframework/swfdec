
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#include <swf.h>
#include "jpeg_internal.h"
#include "../bits.h"

#define unzigzag8x8_s16 unzigzag8x8_s16_ref
#define idct8x8_s16 idct8x8_s16_ref
#define idct8x8_f64 idct8x8_f64_ref
#define conv8x8_f64_s16 conv8x8_f64_s16_ref
#include "unzigzag8x8_s16.h"
#include "idct8x8_s16.h"

#define JPEG_DEBUG(n, format...)	do{ \
	if((n)>=SWF_DEBUG_LEVEL)jpeg_debug((n),format); \
}while(0)
#define JPEG_DEBUG_LEVEL 0


#define JPEG_MARKER_STUFFED		0x00
#define JPEG_MARKER_TEM			0x01
#define JPEG_MARKER_RES			0x02

#define JPEG_MARKER_SOF_0		0xc0
#define JPEG_MARKER_SOF_1		0xc1
#define JPEG_MARKER_SOF_2		0xc2
#define JPEG_MARKER_SOF_3		0xc3
#define JPEG_MARKER_DHT			0xc4
#define JPEG_MARKER_SOF_5		0xc5
#define JPEG_MARKER_SOF_6		0xc6
#define JPEG_MARKER_SOF_7		0xc7
#define JPEG_MARKER_JPG			0xc8
#define JPEG_MARKER_SOF_9		0xc9
#define JPEG_MARKER_SOF_10		0xca
#define JPEG_MARKER_SOF_11		0xcb
#define JPEG_MARKER_DAC			0xcc
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
#define JPEG_MARKER_DQT			0xdb
#define JPEG_MARKER_DNL			0xdc
#define JPEG_MARKER_DRI			0xdd
#define JPEG_MARKER_DHP			0xde
#define JPEG_MARKER_EXP			0xdf
#define JPEG_MARKER_APP(x)		(0xe0 + (x))
#define JPEG_MARKER_JPG_(x)		(0xf0 + (x))
#define JPEG_MARKER_COM			0xfe

#define JPEG_MARKER_JFIF		JPEG_MARKER_APP(0)


/* misc helper function declarations */

static char *sprintbits(char *str, unsigned int bits, int n);
static void dump_block8x8_s16(short *q);
static void dequant8x8_s16(short *dest, short *src, short *mult);
static void addconst8x8_s16(short *dest, short *src, int c);
static void clipconv8x8_u8_s16(unsigned char *dest, int stride, short *src);


int jpeg_decoder_tag_0xc0(JpegDecoder *dec, bits_t *bits)
{
	int i;
	int length;
	int image_size;

	JPEG_DEBUG(0,"start of frame (baseline DCT)\n");

	length = get_be_u16(bits);
	bits->end = bits->ptr+length-2;
	
	dec->depth = get_u8(bits);
	dec->height = get_be_u16(bits);
	dec->width = get_be_u16(bits);
	dec->n_components = get_u8(bits);

	JPEG_DEBUG(0,"frame_length=%d depth=%d height=%d width=%d n_components=%d\n",
		length, dec->depth, dec->height, dec->width,
		dec->n_components);

	for(i=0;i<dec->n_components;i++){
		dec->components[i].id = get_u8(bits);
		dec->components[i].h_subsample = getbits(bits,4);
		dec->components[i].v_subsample = getbits(bits,4);
		dec->components[i].quant_table = get_u8(bits);

		JPEG_DEBUG(0,"[%d] id=%d h_subsample=%d v_subsample=%d quant_table=%d\n",
			i,
			dec->components[i].id,
			dec->components[i].h_subsample,
			dec->components[i].v_subsample,
			dec->components[i].quant_table);

		dec->components[i].rowstride = (dec->width + 7)&~7;
		image_size = ((dec->width + 7)&~7) * ((dec->height + 7)&~7);
		dec->components[i].image = malloc(image_size);
	}

	if(bits->end != bits->ptr)JPEG_DEBUG(0,"endptr != bits\n");

	return length;
}

int jpeg_decoder_tag_0xdb(JpegDecoder *dec, bits_t *bits)
{
	int length;
	int pq;
	int tq;
	int i;
	short *q;

	JPEG_DEBUG(0,"define quantization table\n");

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

	JPEG_DEBUG(0,"quant table index %d:\n",tq);
	for(i=0;i<8;i++){
		JPEG_DEBUG(0,"%3d %3d %3d %3d %3d %3d %3d %3d\n",
			q[0], q[1], q[2], q[3],
			q[4], q[5], q[6], q[7]);
		q+=8;
	}

	if(bits->end != bits->ptr)JPEG_DEBUG(0,"endptr != bits\n");

	return length;
}

void generate_code_table(int *huffsize)
{
	int code;
	int i;
	int j;
	int k;
	char str[33];
	//int l;

	code = 0;
	k = 0;
	for(i=0;i<16;i++){
		for(j=0;j<huffsize[i];j++){
			JPEG_DEBUG(0,"huffcode[%d] = %s\n",k,
				sprintbits(str,code>>(15-i), i+1));
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

	JPEG_DEBUG(0,"define huffman table\n");

	length = get_be_u16(bits);
	//bits->end = ptr+length;

	tc = getbits(bits,4);
	th = getbits(bits,4);

	JPEG_DEBUG(0,"huff table index %d:\n",th);
	JPEG_DEBUG(0,"type %d (%s)\n",tc,tc?"ac":"dc");

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

	if(bits->ptr != bits->end)JPEG_DEBUG(0,"endptr != bits\n");

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

int jpeg_decoder_find_component_by_id(JpegDecoder *dec, int id)
{
	int i;
	for(i=0;i<dec->n_components;i++){
		if(dec->components[i].id == id)return i;
	}
	JPEG_DEBUG(0,"undefined component id\n");
	return 0;
}

int jpeg_decoder_tag_0xda(JpegDecoder *dec, bits_t *bits)
{
	int length;
	int i;

	int n_components;
	int spectral_start;
	int spectral_end;
	int approx_high;
	int approx_low;
	int n;

	JPEG_DEBUG(0,"start of scan\n");

	length = get_be_u16(bits);
	//bits->end = ptr+length;
	JPEG_DEBUG(0,"length=%d\n",length);

	n_components = get_u8(bits);
	n = 0;
	dec->scan_h_subsample = 0;
	dec->scan_v_subsample = 0;
	for(i=0;i<n_components;i++){
		int component_id;
		int dc_table;
		int ac_table;
		int x;
		int y;
		int index;
		int h_subsample;
		int v_subsample;
		int quant_index;

		component_id = get_u8(bits);
		dc_table = getbits(bits,4);
		ac_table = getbits(bits,4);
		index = jpeg_decoder_find_component_by_id(dec, component_id);

		h_subsample = dec->components[index].h_subsample;
		v_subsample = dec->components[index].v_subsample;
		quant_index = dec->components[index].quant_table;

		for(y=0;y<dec->components[index].v_subsample;y++){
		for(x=0;x<dec->components[index].h_subsample;x++){
			dec->scan_list[n].component_index = index;
			dec->scan_list[n].dc_table = dc_table;
			dec->scan_list[n].ac_table = ac_table;
			dec->scan_list[n].quant_table = quant_index;
			dec->scan_list[n].x = x;
			dec->scan_list[n].y = y;
			n++;
		}}

		dec->scan_h_subsample = MAX(dec->scan_h_subsample,h_subsample);
		dec->scan_v_subsample = MAX(dec->scan_v_subsample,v_subsample);

		syncbits(bits);

		JPEG_DEBUG(0,"component %d: index=%d dc_table=%d ac_table=%d n=%d\n",
			component_id, index, dc_table, ac_table, n);
	}
	dec->scan_list_length = n;

	spectral_start = get_u8(bits);
	spectral_end = get_u8(bits);
	JPEG_DEBUG(0,"spectral range [%d,%d]\n",spectral_start,spectral_end);
	approx_high = getbits(bits,4);
	approx_low = getbits(bits,4);
	JPEG_DEBUG(0,"approx range [%d,%d]\n",approx_low, approx_high);
	syncbits(bits);

	if(bits->end != bits->ptr)JPEG_DEBUG(0,"endptr != bits\n");

	return length;
}

void jpeg_decoder_decode_entropy_segment(JpegDecoder *dec, bits_t *bits)
{
	bits_t b2, *bits2 = &b2;
	short block[64];
	short block2[64];
	unsigned char *newptr;
	int len;
	int j;
	int i;
	int go;
	int x,y;
	int dc[4];

	len = 0;
	j = 0;
	while(1){
		if(bits->ptr[len]==0xff && bits->ptr[len+1]!=0x00){
			break;
		}
		len++;
	}
	JPEG_DEBUG(0,"entropy length = %d\n", len);

	newptr = malloc(len);
	for(i=0;i<len;i++){
		newptr[j] = bits->ptr[i];
		j++;
		if(bits->ptr[i]==0xff) i++;
	}
	bits->ptr += len;

	bits2->ptr = newptr;
	bits2->idx = 0;
	bits2->end = newptr + j;
	
	go = 1;
	x = 0;
	y = 0;
	dc[0] = dc[1] = dc[2] = dc[3] = 128;
	while(go){
	for(i=0;i<dec->scan_list_length;i++){
		int dc_table_index;
		int ac_table_index;
		int quant_index;
		unsigned char *ptr;
		int component_index;

		JPEG_DEBUG(0,"%d,%d: component=%d dc_table=%d ac_table=%d\n",
			x,y,
			dec->scan_list[i].component_index,
			dec->scan_list[i].dc_table,
			dec->scan_list[i].ac_table);

		component_index = dec->scan_list[i].component_index;
		dc_table_index = dec->scan_list[i].dc_table;
		ac_table_index = dec->scan_list[i].ac_table;
		quant_index = dec->scan_list[i].quant_table;

		huffman_table_decode_macroblock(block,
			dec->dc_huff_table[dc_table_index],
			dec->ac_huff_table[ac_table_index], bits2);

		dc[component_index] += block[0];
		block[0] = 0;
		JPEG_DEBUG(0,"using quant table %d\n", quant_index);
		dequant8x8_s16(block2, block, dec->quant_table[quant_index]);
		unzigzag8x8_s16(block, block2);
		idct8x8_s16(block2, block, sizeof(short)*8, sizeof(short)*8);
		addconst8x8_s16(block, block2, dc[component_index]);

		dump_block8x8_s16(block);

		ptr = dec->components[component_index].image + x +
			dec->scan_list[i].x * 8 +
			dec->components[component_index].rowstride *
				(y + dec->scan_list[i].y * 8);

		clipconv8x8_u8_s16(ptr,
			dec->components[component_index].rowstride,
			block);
	}
		x += dec->scan_h_subsample * 8;
		if(x >= dec->width){
			x = 0;
			y += dec->scan_v_subsample * 8;
		}
		if(y >= dec->height){
			go = 0;
		}
	}
}



JpegDecoder *jpeg_decoder_new(void)
{
	JpegDecoder *dec;

	dec = g_new0(JpegDecoder,1);

	return dec;
}

int jpeg_decoder_addbits(JpegDecoder *dec, unsigned char *data, unsigned int len)
{
	dec->bits.ptr = data;
	dec->bits.idx = 0;
	dec->bits.end = data + len;

	return 0;
}

int jpeg_decoder_get_component(JpegDecoder *dec, int id,
	unsigned char **image, int *rowstride, int *width, int *height)
{
	int i;

	i = jpeg_decoder_find_component_by_id(dec,id);
	if(image)*image = dec->components[i].image;
	if(rowstride)*rowstride = dec->components[i].rowstride;
	if(width)*width = dec->width;
	if(height)*height = dec->height;

	return 0;
}

int jpeg_decoder_parse(JpegDecoder *dec)
{
	bits_t *bits = &dec->bits;
	bits_t b2;
	unsigned int x;
	unsigned int tag;

	while(bits->ptr < bits->end){
		x = get_u8(bits);
		if(x != 0xff){
			JPEG_DEBUG(0,"lost sync\n");
			while(x != 0xff){
				x = get_u8(bits);
			}
		}
		while(x == 0xff){
			x = get_u8(bits);
		}
		tag = x;
		if(tag==0)continue;
		JPEG_DEBUG(0,"tag %02x\n",tag);
		
		b2 = *bits;

		switch(tag){
		case JPEG_MARKER_SOF_0: /* baseline DCT */
			jpeg_decoder_tag_0xc0(dec,&b2);
			break;
		case JPEG_MARKER_DHT: /* define huffman table */
			jpeg_decoder_tag_0xc4(dec,&b2);
			break;
		case JPEG_MARKER_SOI: /* start of image */
			break;
		case JPEG_MARKER_EOI: /* end of image */
			break;
		case JPEG_MARKER_SOS: /* start of scan */
			jpeg_decoder_tag_0xda(dec,&b2);
			jpeg_decoder_decode_entropy_segment(dec, &b2);
			break;
		case JPEG_MARKER_DQT: /* define quantization table */
			jpeg_decoder_tag_0xdb(dec,&b2);
			break;
		case JPEG_MARKER_JFIF: /* jpeg extension (JFIF) */
			JPEG_DEBUG(0,"JFIF\n");
			break;
		default:
			JPEG_DEBUG(0,"unhandled or illegal JPEG marker (0x%02x)\n",tag);
		}
		syncbits(&b2);
		bits->ptr = b2.ptr;
	}

	return 0;
}


int jpeg_decoder_verbose_level;

void jpeg_debug(int n, const char *format, ... )
{
	va_list args;

	if(n>jpeg_decoder_verbose_level)return;

	printf("JPEG_DEBUG: ");
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
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

static void dump_block8x8_s16(short *q)
{
	int i;

	for(i=0;i<8;i++){
		JPEG_DEBUG(0,"%3d %3d %3d %3d %3d %3d %3d %3d\n",
			q[0], q[1], q[2], q[3],
			q[4], q[5], q[6], q[7]);
		q+=8;
	}
}

static void dequant8x8_s16(short *dest, short *src, short *mult)
{
	int i;

	for(i=0;i<64;i++){
		dest[i] = src[i] * mult[i];
	}
}

static void addconst8x8_s16(short *dest, short *src, int c)
{
	int i;

	for(i=0;i<64;i++){
		dest[i] = src[i] + c;
	}
}

static void clipconv8x8_u8_s16(unsigned char *dest, int stride, short *src)
{
	int i,j;
	int x;

	for(i=0;i<8;i++){
		for(j=0;j<8;j++){
			x = *src++;
			if(x<0)x=0;
			if(x>255)x=255;
			dest[j] = x;
		}
		dest += stride;
	}
}

