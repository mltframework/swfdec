
#include <stdio.h>
#include <jpeglib.h>
#include <zlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "swf.h"
#include "proto.h"
#include "bits.h"


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

#if 0
static void *jpegdec(swf_image_t *image, void *ptr, int len)
{
	GdkPixbufLoader *pbl;
	GdkPixbuf *pb;
	GError *error = NULL;

	g_type_init();
	pbl = gdk_pixbuf_loader_new();

	gdk_pixbuf_loader_write(pbl, ptr, len, &error);
	if(error){
		printf("error: %s\n",error->message);
		return NULL;
	}
	gdk_pixbuf_loader_close(pbl, NULL);

	pb = gdk_pixbuf_loader_get_pixbuf(pbl);

	image->width = gdk_pixbuf_get_width(pb);
	image->height = gdk_pixbuf_get_width(pb);
	//image->rowstride = gdk_pixbuf_get_rowstride(pb);
	image->image_data = gdk_pixbuf_get_pixels(pb);

	return NULL;
}
#else
static void *jpegdec(swf_image_t *image, unsigned char *ptr, int len);
#endif

int tag_func_define_bits_jpeg(swf_state_t *s)
{
	tag_func_dumpbits(s);

	return SWF_OK;
}

int tag_func_define_bits_jpeg_2(swf_state_t *s)
{
	bits_t *bits = &s->b;
	int id;
	//unsigned char *endptr = s->b.ptr + s->tag_len;
	int len;
	swf_object_t *obj;
	swf_image_t *image;

	id = get_u16(bits);
	SWF_DEBUG(0,"  id = %d\n",id);
	obj = swf_object_new(s, id);
	image = g_new0(swf_image_t, 1);
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

int tag_func_define_bits_jpeg_3(swf_state_t *s)
{
	bits_t *bits = &s->b;
	int id;
	//unsigned char *endptr = s->b.ptr + s->tag_len;
	int len;
	//int alpha_len;
	//unsigned char *data;
	swf_object_t *obj;
	swf_image_t *image;

	id = get_u16(bits);
	SWF_DEBUG(0,"  id = %d\n",id);
	obj = swf_object_new(s, id);
	image = g_new0(swf_image_t, 1);
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

int tag_func_define_bits_lossless(swf_state_t *s)
{
	bits_t *bits = &s->b;
	int id;
	int format;
	int color_table_size;
	void *ptr;
	int len;
	unsigned char *endptr = bits->ptr + s->tag_len;
	swf_object_t *obj;
	swf_image_t *image;

	id = get_u16(bits);
	SWF_DEBUG(0,"  id = %d\n",id);
	obj = swf_object_new(s, id);
	image = g_new0(swf_image_t, 1);
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

int tag_func_define_bits_lossless_2(swf_state_t *s)
{
	bits_t *bits = &s->b;
	int id;

	id = get_u16(bits);
	
	tag_func_dumpbits(s);

	return SWF_OK;
}



int swfdec_image_render(swf_state_t *s, swf_layer_t *layer)
{
	printf("got here\n");

	//art_irect_intersect(&rect, &s->drawrect, &pobj->rect);

	return SWF_OK;
}

static void *jpegdec(swf_image_t *image, unsigned char *data, int len)
{
	int i;
	int j;
	int tag;
	unsigned char *p;

	i = 0;
	while(i<len){
		while(data[i] != 0xff){ i++; }
		while(data[i] == 0xff){ i++; }
		if(data[i] == 0)continue;

		tag = data[i];
		printf("JPEG: tag %02x\n",tag);
		i++;

		switch(tag){
		case 0xc0:
		case 0xc1:
		case 0xc2:
		case 0xc3:
			image->height = (data[i+3]<<8) | data[i+4];
			image->width = (data[i+5]<<8) | data[i+6];
		default:
		}

	}

	image->image_data = malloc(4*image->width*image->height);
	
	p = image->image_data;
	for(j=0;j<image->height;j++){
	for(i=0;i<image->width;i++){
		p[0] = 64 + 128*((j&1)^(i&1));
		p[1] = 64 + 128*((j&1)^(i&1));
		p[2] = 64 + 128*((j&1)^(i&1));
		p[3] = 255;
		p+=4;
	}
	}

}

