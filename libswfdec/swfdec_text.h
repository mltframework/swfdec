
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
int tag_func_define_text(SwfdecDecoder *s);
int tag_func_define_text_2(SwfdecDecoder *s);

#endif

