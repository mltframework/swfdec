
#ifndef __SWFDEC_LAYER_H__
#define __SWFDEF_LAYER_H__

#include "swfdec_types.h"

struct swfdec_layer_vec_struct{
	ArtSVP *svp;
	unsigned int color;
	ArtIRect rect;
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

#endif

