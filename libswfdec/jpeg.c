
#include <stdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "swf.h"
#include "bits.h"

int tag_func_define_bits_jpeg_2(swf_state_t *s)
{
#if 0
	bits_t *bits = &s->b;
	int id;
	int ret;
	GdkPixbufLoader *loader;
	GdkPixbuf *pixbuf;
	GError *error = NULL;

	id = get_u16(bits);
	
	/* strip some stuff */
	get_u16(bits);
	get_u16(bits);

	//loader = gdk_pixbuf_loader_new();
	loader = gdk_pixbuf_loader_new_with_type("jpeg",&error);
	if(error){
		printf("error = %s\n",error->message);
		error = NULL;
	}

	ret = gdk_pixbuf_loader_write(loader, bits->ptr, s->tag_len-6, &error);
	if(error){
		printf("error = %s\n",error->message);
		error = NULL;
	}
	bits->ptr += s->tag_len - 6;

	pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
	printf("pixbuf = %p\n",pixbuf);
#endif

	return SWF_OK;
}

