
#include "swfdec_internal.h"


void get_art_color_transform(bits_t *bits, double mult[4], double add[4])
{
	int has_add;
	int has_mult;
	int n_bits;

	syncbits(bits);
	has_add = getbit(bits);
	has_mult = getbit(bits);
	n_bits = getbits(bits,4);
	if(has_mult){
		mult[0] = getsbits(bits,n_bits) * SWF_COLOR_SCALE_FACTOR;
		mult[1] = getsbits(bits,n_bits) * SWF_COLOR_SCALE_FACTOR;
		mult[2] = getsbits(bits,n_bits) * SWF_COLOR_SCALE_FACTOR;
		mult[3] = getsbits(bits,n_bits) * SWF_COLOR_SCALE_FACTOR;
	}else{
		mult[0] = 1.0;
		mult[1] = 1.0;
		mult[2] = 1.0;
		mult[3] = 1.0;
	}
	if(has_add){
		add[0] = getsbits(bits,n_bits);
		add[1] = getsbits(bits,n_bits);
		add[2] = getsbits(bits,n_bits);
		add[3] = getsbits(bits,n_bits);
	}else{
		add[0] = 0;
		add[1] = 0;
		add[2] = 0;
		add[3] = 0;
	}

	//printf("mult %g,%g,%g,%g\n",mult[0],mult[1],mult[2],mult[3]);
	//printf("add %g,%g,%g,%g\n",add[0],add[1],add[2],add[3]);
}

void get_art_matrix(bits_t *bits, double *trans)
{
	int has_scale;
	int n_scale_bits;
	int scale_x = 0;
	int scale_y = 0;
	int has_rotate;
	int n_rotate_bits;
	int rotate_skew0 = 0;
	int rotate_skew1 = 0;
	int n_translate_bits;
	int translate_x;
	int translate_y;

	trans[0] = 1;
	trans[1] = 0;
	trans[2] = 0;
	trans[3] = 1;
	trans[4] = 0;
	trans[5] = 0;

	syncbits(bits);
	has_scale = getbit(bits);
	if(has_scale){
		n_scale_bits = getbits(bits, 5);
		scale_x = getsbits(bits, n_scale_bits);
		scale_y = getsbits(bits, n_scale_bits);

		trans[0] = scale_x * SWF_TRANS_SCALE_FACTOR;
		trans[3] = scale_y * SWF_TRANS_SCALE_FACTOR;
	}
	has_rotate = getbit(bits);
	if(has_rotate){
		n_rotate_bits = getbits(bits, 5);
		rotate_skew0 = getsbits(bits, n_rotate_bits);
		rotate_skew1 = getsbits(bits, n_rotate_bits);

		trans[1] = rotate_skew0 * SWF_TRANS_SCALE_FACTOR;
		trans[2] = rotate_skew1 * SWF_TRANS_SCALE_FACTOR;
	}
	n_translate_bits = getbits(bits, 5);
	translate_x = getsbits(bits, n_translate_bits);
	translate_y = getsbits(bits, n_translate_bits);

	trans[4] = translate_x * SWF_SCALE_FACTOR;
	trans[5] = translate_y * SWF_SCALE_FACTOR;

	//printf("trans=%g,%g,%g,%g,%g,%g\n", trans[0], trans[1], trans[2], trans[3], trans[4], trans[5]);
}

char *get_string(bits_t *bits)
{
	char *s = g_strdup(bits->ptr);

	bits->ptr += strlen(bits->ptr) + 1;

	return s;
}

void get_color_transform(bits_t *bits)
{
	int has_add;
	int has_mult;
	int n_bits;
	int mult_r, mult_g, mult_b, mult_a;
	int add_r, add_g, add_b, add_a;

	syncbits(bits);
	has_add = getbit(bits);
	has_mult = getbit(bits);
	n_bits = getbits(bits,4);
	if(has_mult){
		mult_r = getsbits(bits,n_bits);
		mult_g = getsbits(bits,n_bits);
		mult_b = getsbits(bits,n_bits);
		mult_a = getsbits(bits,n_bits);
		//printf("  mult RGBA = %d,%d,%d,%d\n",mult_r,mult_g,mult_b,mult_a);
	}
	if(has_add){
		add_r = getsbits(bits,n_bits);
		add_g = getsbits(bits,n_bits);
		add_b = getsbits(bits,n_bits);
		add_a = getsbits(bits,n_bits);
		//printf("  add RGBA = %d,%d,%d,%d\n",add_r,add_g,add_b,add_a);
	}
}

unsigned int get_color(bits_t *bits)
{
	int r,g,b;

	r = get_u8(bits);
	g = get_u8(bits);
	b = get_u8(bits);

	//printf("   color = %d,%d,%d\n",r,g,b);

	return SWF_COLOR_COMBINE(r,g,b,0xff);
}

unsigned int get_rgba(bits_t *bits)
{
	int r,g,b,a;

	r = get_u8(bits);
	g = get_u8(bits);
	b = get_u8(bits);
	a = get_u8(bits);

	return SWF_COLOR_COMBINE(r,g,b,a);
}

void get_matrix(bits_t *bits)
{
	int has_scale;
	int n_scale_bits;
	int scale_x = 0;
	int scale_y = 0;
	int has_rotate;
	int n_rotate_bits;
	int rotate_skew0 = 0;
	int rotate_skew1 = 0;
	int n_translate_bits;
	int translate_x;
	int translate_y;

	syncbits(bits);
	has_scale = getbit(bits);
	if(has_scale){
		n_scale_bits = getbits(bits, 5);
		scale_x = getbits(bits, n_scale_bits);
		scale_y = getbits(bits, n_scale_bits);
	}
	has_rotate = getbit(bits);
	if(has_rotate){
		n_rotate_bits = getbits(bits, 5);
		rotate_skew0 = getbits(bits, n_rotate_bits);
		rotate_skew1 = getbits(bits, n_rotate_bits);
	}
	n_translate_bits = getbits(bits, 5);
	translate_x = getbits(bits, n_translate_bits);
	translate_y = getbits(bits, n_translate_bits);

	//printf("  scale = %d,%d\n", scale_x, scale_y);
	//printf("  rotate_skew = %d,%d\n", rotate_skew0, rotate_skew1);
	//printf("  translate = %d,%d\n", translate_x, translate_y);
}

SwfdecGradient *get_gradient(bits_t *bits)
{
	SwfdecGradient *grad;
	int n_gradients;
	int i;

	syncbits(bits);
	n_gradients = getbits(bits,8);
	grad = g_malloc(sizeof(SwfdecGradient) +
			sizeof(SwfdecGradientEntry)*(n_gradients-1));
	grad->n_gradients = n_gradients;
	for(i=0;i<n_gradients;i++){
		grad->array[i].ratio = getbits(bits,8);
		grad->array[i].color = get_color(bits);
	}
	return grad;
}

SwfdecGradient *get_gradient_rgba(bits_t *bits)
{
	SwfdecGradient *grad;
	int n_gradients;
	int i;

	syncbits(bits);
	n_gradients = getbits(bits,8);
	grad = g_malloc(sizeof(SwfdecGradient) +
			sizeof(SwfdecGradientEntry)*(n_gradients-1));
	grad->n_gradients = n_gradients;
	for(i=0;i<n_gradients;i++){
		grad->array[i].ratio = getbits(bits,8);
		grad->array[i].color = get_rgba(bits);
	}
	return grad;
}

void get_fill_style(bits_t *bits)
{
	int fill_style_type;
	int id;

	fill_style_type = get_u8(bits);
	//printf("   fill_style_type = 0x%02x\n", fill_style_type);
	if(fill_style_type == 0x00){
		get_color(bits);
	}
	if(fill_style_type == 0x10 || fill_style_type == 0x12){
		get_matrix(bits);
		get_gradient(bits);
	}
	if(fill_style_type == 0x40 || fill_style_type == 0x41){
		id = get_u16(bits);
	}
	if(fill_style_type == 0x40 || fill_style_type == 0x41){
		get_matrix(bits);
	}
	
}

void get_line_style(bits_t *bits)
{
	int width;

	width = get_u16(bits);
	//printf("   width = %d\n", width);
	get_color(bits);
}


void get_rect(bits_t *bits, int *rect)
{
	int nbits = getbits(bits, 5);

	rect[0] = getsbits(bits, nbits);
	rect[1] = getsbits(bits, nbits);
	rect[2] = getsbits(bits, nbits);
	rect[3] = getsbits(bits, nbits);
}

