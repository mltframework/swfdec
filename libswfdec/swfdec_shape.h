
#ifndef __SWFDEC_SHAPE_H__
#define __SWFDEC_SHAPE_H__

struct swfdec_shape_vec_struct {
	int type;
	int index;
	unsigned int color;
	double width;

	GArray *path;
	int array_len;

	int fill_type;
	int fill_id;
};

struct swfdec_shape_struct {
	GPtrArray *lines;
	GPtrArray *fills;
	GPtrArray *fills2;

	/* used while defining */
	int fills_offset;
	int lines_offset;
	int n_fill_bits;
	int n_line_bits;
	int rgba;
};

int get_shape_rec(bits_t *bits,int n_fill_bits, int n_line_bits);
int tag_func_define_shape(SwfdecDecoder *s);
SwfdecShapeVec *swf_shape_vec_new(void);
int art_define_shape(SwfdecDecoder *s);
int art_define_shape_3(SwfdecDecoder *s);
void swf_shape_add_styles(SwfdecDecoder *s, SwfdecShape *shape, bits_t *bits);
void swf_shape_get_recs(SwfdecDecoder *s, bits_t *bits, SwfdecShape *shape);
int art_define_shape_2(SwfdecDecoder *s);
int tag_func_define_button_2(SwfdecDecoder *s);
int tag_func_define_sprite(SwfdecDecoder *s);
void dump_layers(SwfdecDecoder *s);

void prerender_layer_shape(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecShape *shape);

#endif

