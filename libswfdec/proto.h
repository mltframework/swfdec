
#ifndef _SWF_PROTO_H_
#define _SWF_PROTO_H_

/* art.c */

void art_vpath_dump(FILE *out, ArtVpath *vpath);
void art_bpath_dump(FILE *out, ArtBpath *vpath);
int art_bpath_len(ArtBpath *a);
int art_vpath_len(ArtVpath *a);
ArtBpath *art_bpath_cat(ArtBpath *a, ArtBpath *b);
ArtVpath *art_vpath_cat(ArtVpath *a, ArtVpath *b);
ArtVpath *art_vpath_reverse(ArtVpath *a);
ArtVpath *art_vpath_reverse_free(ArtVpath *a);
int art_affine_inverted(double trans[6]);
void art_rgb_svp_alpha2 (const ArtSVP *svp, int x0, int y0,
	int x1, int y1, art_u32 rgba, art_u8 *buf, int rowstride,
	ArtAlphaGamma *alphagamma);
void art_rgb_fill_run(unsigned char *buf, unsigned char r,
	unsigned char g, unsigned char b, int n);
void art_rgb_run_alpha(unsigned char *buf, unsigned char r,
	unsigned char g, unsigned char b, int alpha, int n);
void art_rgb565_run_alpha(unsigned char *buf, unsigned char r,
	unsigned char g, unsigned char b, int alpha, int n);
void art_rgb565_fill_run(unsigned char *buf, unsigned char r,
	unsigned char g, unsigned char b, int n);
void art_rgb565_fillrect(char *buffer, int stride, unsigned int color, ArtIRect *rect);
void art_rgb_fillrect(char *buffer, int stride, unsigned int color, ArtIRect *rect);
void art_rgb_render_callback(void *data, int y, int start,
	ArtSVPRenderAAStep *steps, int n_steps);
void art_rgb565_svp_aa (const ArtSVP *svp,
		int x0, int y0, int x1, int y1,
		art_u32 fg_color, art_u32 bg_color,
		art_u8 *buf, int rowstride,
		ArtAlphaGamma *alphagamma);
void art_rgb565_svp_alpha (const ArtSVP *svp,
		   int x0, int y0, int x1, int y1,
		   art_u32 rgba,
		   art_u8 *buf, int rowstride,
		   ArtAlphaGamma *alphagamma);
void art_rgb565_svp_alpha_callback (void *callback_data, int y, int start,
	ArtSVPRenderAAStep *steps, int n_steps);
void art_rgb_svp_alpha_callback (void *callback_data, int y, int start,
	ArtSVPRenderAAStep *steps, int n_steps);

/* get.c */

void get_art_color_transform(bits_t *bits, double mult[4], double add[4]);
void get_art_matrix(bits_t *bits, double *trans);
char *get_string(bits_t *bits);
void get_color_transform(bits_t *bits);
unsigned int get_color(bits_t *bits);
unsigned int get_rgba(bits_t *bits);
void get_matrix(bits_t *bits);
unsigned int get_gradient(bits_t *bits);
unsigned int get_gradient_rgba(bits_t *bits);
void get_fill_style(bits_t *bits);
void get_line_style(bits_t *bits);

/* jpeg.c */

int tag_func_define_bits_lossless(swf_state_t *s);
int tag_func_define_bits_lossless_2(swf_state_t *s);
int tag_func_define_bits_jpeg(swf_state_t *s);
int tag_func_define_bits_jpeg_2(swf_state_t *s);
int tag_func_define_bits_jpeg_3(swf_state_t *s);

/* render.c */

void swf_invalidate_irect(swf_state_t *s, ArtIRect *rect);
unsigned int transform_color(unsigned int in, double mult[4], double add[4]);
swf_layer_t *swf_layer_get(swf_state_t *s, int depth);
void swf_layer_del(swf_state_t *s, swf_layer_t *layer);
int art_place_object_2(swf_state_t *s);
int art_remove_object(swf_state_t *s);
int art_remove_object_2(swf_state_t *s);
int art_show_frame(swf_state_t *s);
swf_object_t *swf_object_get(swf_state_t *s, int id);
void swf_render_frame(swf_state_t *s);
void swf_layer_prerender(swf_state_t *s, swf_layer_t *layer);

/* shape.c */

int get_shape_rec(bits_t *bits,int n_fill_bits, int n_line_bits);
int tag_func_define_shape(swf_state_t *s);
void append(struct swf_shape_vec_struct *vec, ArtBpath *b);
int art_define_shape(swf_state_t *s);
int art_define_shape_2(swf_state_t *s);
int art_define_shape_3(swf_state_t *s);
void swf_shape_get_recs(swf_state_t *s, bits_t *bits, swf_shape_t *shape);
swf_shape_vec_t *swf_shape_vec_new(void);
void swf_shape_add_styles(swf_state_t *s, swf_shape_t *shape, bits_t *bits);
int tag_func_define_button_2(swf_state_t *s);
int tag_func_define_sprite(swf_state_t *s);

/* sound.c */

void mp3_decode(swf_object_t *obj);
int tag_func_define_sound(swf_state_t *s);
int tag_func_sound_stream_block(swf_state_t *s);
int tag_func_sound_stream_head(swf_state_t *s);
void get_soundinfo(bits_t *b);
int tag_func_start_sound(swf_state_t *s);
int tag_func_define_button_sound(swf_state_t *s);

/* swf.c */

swf_state_t *swf_init(void);
int swf_addbits(swf_state_t *s, unsigned char *bits, int len);
int swf_parse(swf_state_t *s);
int swf_parse_header(swf_state_t *s);
swf_object_t *swf_object_new(swf_state_t *s, int id);
int swf_parse_tag(swf_state_t *s);
int tag_func_zero(swf_state_t *s);
int tag_func_ignore_quiet(swf_state_t *s);
int tag_func_ignore(swf_state_t *s);
int tag_func_dumpbits(swf_state_t *s);
void get_actions(swf_state_t *s, bits_t *bits);
int tag_func_set_background_color(swf_state_t *s);
int tag_func_frame_label(swf_state_t *s);
int tag_func_do_action(swf_state_t *s);
int tag_func_place_object_2(swf_state_t *s);
int tag_func_remove_object(swf_state_t *s);
int tag_func_remove_object_2(swf_state_t *s);

/* text.c */

int tag_func_define_font(swf_state_t *s);
int tag_func_define_text(swf_state_t *s);
int tag_func_define_text_2(swf_state_t *s);

#endif

