
#ifndef __SWFDEC_BUTTON_H__
#define __SWFDEC_BUTTON_H__

struct swfdec_button_struct{
	SwfdecSpriteSeg *state[3];
};

void swfdec_button_prerender(SwfdecDecoder *s,SwfdecLayer *layer,
	SwfdecObject *object);
SwfdecLayer *swfdec_button_prerender_slow(SwfdecDecoder *s,SwfdecSpriteSeg *seg,
	SwfdecObject *object);
void swfdec_button_render(SwfdecDecoder *s,SwfdecLayer *layer,
	SwfdecObject *object);

#endif

