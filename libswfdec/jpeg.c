
#include <zlib.h>

#include "swfdec_internal.h"
#include "jpeg_rgb_decoder.h"

static void jpegdec(SwfdecImage *image, unsigned char *ptr, int len);

static void *zalloc(void *opaque, unsigned int items, unsigned int size)
{
	return malloc(items*size);
}

static void zfree(void *opaque, void *addr)
{
	free(addr);
}

static void *lossless(void *zptr, int zlen, int *plen)
{
	void *data;
	int len;
	z_stream *z;
	int ret;

	z = g_new0(z_stream, 1);
	z->zalloc = zalloc;
	z->zfree = zfree;
	z->opaque = NULL;

	z->next_in = zptr;
	z->avail_in = zlen;

	data = NULL;
	len = 0;
	ret = inflateInit(z);
	while(z->avail_in > 0){
		if(z->avail_out == 0){
			len += 1024;
			data = realloc(data,len);
			z->next_out = data + z->total_out;
			z->avail_out += 1024;
		}
		ret = inflate(z,Z_SYNC_FLUSH);
		if(ret!=Z_OK)break;
	}

	if(plen)(*plen) = z->total_out;
	return data;
}


int tag_func_define_bits_jpeg(SwfdecDecoder *s)
{
	tag_func_dumpbits(s);

	return SWF_OK;
}

int tag_func_define_bits_jpeg_2(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	int id;
	SwfdecObject *obj;
	SwfdecImage *image;

	id = get_u16(bits);
	SWF_DEBUG(0,"  id = %d\n",id);
	obj = swfdec_object_new(s, id);
	image = g_new0(SwfdecImage, 1);
	obj->priv = image;
	obj->type = SWF_OBJECT_IMAGE;
	
	jpegdec(image, bits->ptr, s->tag_len - 2);
	bits->ptr += s->tag_len - 2;

	SWF_DEBUG(0,"  width = %d\n", image->width);
	SWF_DEBUG(0,"  height = %d\n", image->height);

	return SWF_OK;
}

int tag_func_define_bits_jpeg_3(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	int id;
	//unsigned char *endptr = s->b.ptr + s->tag_len;
	int len;
	//int alpha_len;
	//unsigned char *data;
	SwfdecObject *obj;
	SwfdecImage *image;

	id = get_u16(bits);
	SWF_DEBUG(0,"  id = %d\n",id);
	obj = swfdec_object_new(s, id);
	image = g_new0(SwfdecImage, 1);
	obj->priv = image;
	obj->type = SWF_OBJECT_IMAGE;
	
	len = get_u32(bits);
	SWF_DEBUG(0,"  len = %d\n",len);

	jpegdec(image, bits->ptr, len);

	SWF_DEBUG(0,"  width = %d\n", image->width);
	SWF_DEBUG(0,"  height = %d\n", image->height);

	bits->ptr += len;
	//tag_func_dumpbits(s);

	return SWF_OK;
}

int tag_func_define_bits_lossless(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	int id;
	int format;
	int color_table_size;
	void *ptr;
	int len;
	unsigned char *endptr = bits->ptr + s->tag_len;
	SwfdecObject *obj;
	SwfdecImage *image;

	id = get_u16(bits);
	SWF_DEBUG(0,"  id = %d\n",id);
	obj = swfdec_object_new(s, id);
	image = g_new0(SwfdecImage, 1);
	obj->priv = image;
	obj->type = SWF_OBJECT_IMAGE;

	format = get_u8(bits);
	SWF_DEBUG(0,"  format = %d\n",format);
	image->width = get_u16(bits);
	SWF_DEBUG(0,"  width = %d\n",image->width);
	image->height = get_u16(bits);
	SWF_DEBUG(0,"  height = %d\n",image->height);
	if(format==3){
		color_table_size = get_u8(bits) + 1;
	}else{
		color_table_size = 0;
	}
	
	//printf("format = %d\n", format);
	//printf("width = %d\n", width);
	//printf("height = %d\n", height);
	//printf("color_table_size = %d\n", color_table_size);

	ptr = lossless(bits->ptr, endptr - bits->ptr, &len);
	bits->ptr = endptr;

	//printf("len=%d h*w=%d\n",len,width*height);

	if(format==3){	
		unsigned char *ctab = ptr;
		unsigned char *p = ptr + color_table_size * 3;
		int c;
		int i;
		int j;
		unsigned char *idata;
		int rowstride;

#if 0
		for(i=0;i<color_table_size;i++){
			printf("rgb %d %d %d\n",
				ctab[i*3+0],ctab[i*3+1],ctab[i*3+2]);
		}
#endif

		image->image_data = malloc(4*image->width*image->height);
 		idata = image->image_data;

		rowstride = ((image->width - 1)|3) + 1;

		for(j=0;j<image->height;j++){
		p = ptr + color_table_size * 3 + rowstride * j;
		for(i=0;i<image->width;i++){
			c = *p;
			if(c>=color_table_size){
				SWF_DEBUG(4,"colormap index out of range\n");
				c = 0;
			}
			idata[0] = ctab[c*3 + 0];
			idata[1] = ctab[c*3 + 1];
			idata[2] = ctab[c*3 + 2];
			idata[3] = 0xff;
			p++;
			idata += 4;
		}
		}

		free(ptr);
	}
	if(format==4){
		unsigned char *p = ptr;
		int i,j;
		unsigned int c;
		unsigned char *idata;

		image->image_data = malloc(4*image->width*image->height);
 		idata = image->image_data;

		/* 16 bit packed */
		/* FIXME assume this means RGB-555 */
		printf("Warning: assumption about colorspace\n");

		for(j=0;j<image->height;j++){
		for(i=0;i<image->width;i++){
			c = p[0] | (p[1]<<8);
			idata[0] = ((c>>7)&0xf8) | ((c>>12)&0x7);
			idata[1] = ((c>>2)&0xf8) | ((c>>7)&0x7);
			idata[2] = (c<<3) | ((c>>2)&0x7);
			idata[3] = 0xff;
			p++;
			idata += 4;
		}
		}
		free(ptr);
	}
	if(format==5){
		image->image_data = ptr;
	}


	return SWF_OK;
}

int tag_func_define_bits_lossless_2(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	int id;

	id = get_u16(bits);
	
	tag_func_dumpbits(s);

	return SWF_OK;
}



int swfdec_image_render(SwfdecDecoder *s, SwfdecLayer *layer, SwfdecObject *obj)
{
	printf("got here\n");

	//art_irect_intersect(&rect, &s->drawrect, &pobj->rect);

	return SWF_OK;
}

static void jpegdec(SwfdecImage *image, unsigned char *data, int len)
{
	JpegRGBDecoder *dec;

	dec = jpeg_rgb_decoder_new();

	jpeg_rgb_decoder_addbits(dec, data, len);
	jpeg_rgb_decoder_parse(dec);
	jpeg_rgb_decoder_get_image(dec, (unsigned char **)&image->image_data,
		&image->rowstride, &image->width, &image->height);
	jpeg_rgb_decoder_free(dec);

}

