
#ifndef __SWFDEC_SPRITE_H__
#define __SWFDEC_SPRITE_H__

#include "swfdec_types.h"

struct swfdec_sprite_struct {
	int n_frames;

	GList *layers;
};

SwfdecSprite *swfdec_sprite_new(void);

void swfdec_sprite_prerender(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object);
void swfdec_sprite_render(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object);

#endif

