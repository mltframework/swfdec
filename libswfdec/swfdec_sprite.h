
#ifndef __SWFDEC_SPRITE_H__
#define __SWFDEC_SPRITE_H__

#include "swfdec_types.h"

struct swf_sprite_struct {
	int n_frames;

	/* where we are in global parsing */
	bits_t parse;

	/* temporary state while parsing */
	bits_t b;

	int tag;
	int tag_len;
	int (*func)(SwfdecDecoder *s);

	GList *frames;

	/* rendering state */
	unsigned int bg_color;
	GList *layers;
	ArtIRect irect;
	ArtIRect drawrect;
};

void prerender_layer_sprite(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object);
void render_sprite(SwfdecDecoder *s, SwfdecDecoder *sprite, int frame_number);

#endif

