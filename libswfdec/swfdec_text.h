
#ifndef __SWFDEC_TEXT_H__
#define __SWFDEC_TEXT_H__

#include "swfdec_types.h"

struct swfdec_text_struct {
	int font;
	int height;
	unsigned int color;
	GArray *glyphs;
};

struct swfdec_text_glyph_struct {
	int x,y;
	int glyph;
};

int tag_func_define_font(SwfdecDecoder *s);
int tag_func_define_font_2(SwfdecDecoder *s);
int tag_func_define_text(SwfdecDecoder *s);
int tag_func_define_text_2(SwfdecDecoder *s);

void swfdec_text_free(SwfdecObject *object);
void swfdec_font_free(SwfdecObject *object);
SwfdecLayer *swfdec_text_prerender(SwfdecDecoder *s,SwfdecSpriteSeg *seg,
	SwfdecObject *object);
void swfdec_text_render(SwfdecDecoder *s,SwfdecLayer *layer,
	SwfdecObject *object);

#endif

