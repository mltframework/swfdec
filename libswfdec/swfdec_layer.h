
#ifndef __SWFDEC_LAYER_H__
#define __SWFDEC_LAYER_H__

#include "swfdec_types.h"

struct swfdec_layer_vec_struct{
	ArtSVP *svp;
	unsigned int color;
	ArtIRect rect;

	unsigned char *compose;
	unsigned int compose_rowstride;
	unsigned int compose_height;
	unsigned int compose_width;
};

struct swfdec_layer_struct{
	int depth;
	int id;
	int first_frame;
	int last_frame;

	int frame_number;

	unsigned int prerendered : 1;

	double transform[6];
	double color_mult[4];
	double color_add[4];
	int ratio;

	GArray *lines;
	GArray *fills;

	//swf_image_t *image;
};

SwfdecLayer *swfdec_layer_new(void);
void swfdec_layer_free(SwfdecLayer *layer);
SwfdecLayer *swfdec_layer_get(SwfdecDecoder *s, int depth);
void swfdec_sprite_add_layer(SwfdecSprite *s, SwfdecLayer *lnew);
void swfdec_sprite_del_layer(SwfdecSprite *s, SwfdecLayer *layer);
void swfdec_layer_prerender(SwfdecDecoder *s, SwfdecLayer *layer);
void swfdec_layervec_render(SwfdecDecoder *s, SwfdecLayerVec *layervec);

#endif

