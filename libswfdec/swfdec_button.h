
#ifndef __SWFDEC_BUTTON_H__
#define __SWFDEC_BUTTON_H__

struct swfdec_button_struct{
	struct{
		int id;
		double transform[6];
		double color_mult[4];
		double color_add[4];
	}state[3];
};

void swfdec_button_prerender(SwfdecDecoder *s,SwfdecLayer *layer,
	SwfdecObject *object);
void swfdec_button_render(SwfdecDecoder *s,SwfdecLayer *layer,
	SwfdecObject *object);

#endif

