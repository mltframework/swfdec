
#ifndef __SWFDEC_BUTTON_H__
#define __SWFDEC_BUTTON_H__

struct swfdec_button_struct{
	SwfdecSpriteSeg *state[3];
};

SwfdecLayer *swfdec_button_prerender(SwfdecDecoder *s,SwfdecSpriteSeg *seg,
	SwfdecObject *object, SwfdecLayer *layer);
void swfdec_button_render(SwfdecDecoder *s,SwfdecLayer *layer,
	SwfdecObject *object);
void swfdec_button_free(SwfdecObject *object);

#endif

