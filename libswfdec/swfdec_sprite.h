
#ifndef __SWFDEC_SPRITE_H__
#define __SWFDEC_SPRITE_H__

#include "swfdec_types.h"

struct swfdec_sprite_struct {
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

void swfdec_sprite_prerender(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object);
void swfdec_sprite_render(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object);

#endif

