
#ifndef __SWF_H__
#define __SWF_H__

#include <glib.h>
#include <libart_lgpl/libart.h>
#include <zlib.h>

/* see openswf.org */

#define SWF_OK		0
#define SWF_NEEDBITS	1
#define SWF_WAIT	2
#define SWF_ERROR	3
#define SWF_EOF		4
#define SWF_IMAGE	5
#define SWF_CHANGE	6

#define SWF_STATE_INIT1		0
#define SWF_STATE_INIT2		1
#define SWF_STATE_PARSETAG	2
#define SWF_STATE_EOF		3

#define SWF_COLOR_SCALE_FACTOR		(1/256.0)
#define SWF_TRANS_SCALE_FACTOR		(1/63356.0)
#define SWF_SCALE_FACTOR		(1/20.0)

#define SWF_COLORSPACE_RGB888	0
#define SWF_COLORSPACE_RGB565	1

enum {
	SWF_OBJECT_SHAPE,
	SWF_OBJECT_TEXT,
	SWF_OBJECT_FONT,
	SWF_OBJECT_SPRITE,
	SWF_OBJECT_BUTTON,
	SWF_OBJECT_SOUND,
	SWF_OBJECT_IMAGE,
};

typedef unsigned int swf_color;

#define SWF_COLOR_COMBINE(r,g,b,a)	(((r)<<24) | ((g)<<16) | ((b)<<8) | (a))
#define SWF_COLOR_R(x)		(((x)>>24)&0xff)
#define SWF_COLOR_G(x)		(((x)>>16)&0xff)
#define SWF_COLOR_B(x)		(((x)>>8)&0xff)
#define SWF_COLOR_A(x)		((x)&0xff)

#define SWF_DEBUG(n,format...)	do{ \
	if((n)>=SWF_DEBUG_LEVEL)swf_debug(s,(n),format); \
}while(0)
#define SWF_DEBUG_LEVEL 0

typedef struct swf_state_struct swf_state_t;
typedef struct swf_object_struct swf_object_t;
typedef struct swf_shape_struct swf_shape_t;
typedef struct swf_layer_struct swf_layer_t;
typedef struct swf_layer_vec_struct swf_layer_vec_t;
typedef struct swf_text_struct swf_text_t;
typedef struct swf_text_glyph_struct swf_text_glyph_t;
typedef struct swf_button_struct swf_button_t;
typedef struct swf_sound_struct swf_sound_t;
typedef struct swf_image_struct swf_image_t;
typedef struct bits_struct bits_t;

struct bits_struct {
	unsigned char *ptr;
	int idx;
	unsigned char *end;
};

struct swf_state_struct {
	int version;
	int length;
	int width, height;
	double rate;
	int n_frames;
	char *buffer;
	int frame_number;

	void *sound_buffer;
	int sound_len;
	int sound_offset;

	int colorspace;
	int no_render;
	int debug;
	int compressed;

	char *input_data;
	int input_data_len;
	z_stream *z;

	int stride;
	int bytespp;
	void (*callback)(void *,int,int,ArtSVPRenderAAStep *,int);

	double transform[6];

	/* where we are in the top-level state engine */
	int state;

	/* where we are in global parsing */
	bits_t parse;

	/* temporary state while parsing */
	bits_t b;

	/* defined objects */
	GList *objects;

	int tag;
	int tag_len;
	int (*func)(swf_state_t *s);

	swf_object_t *stream_sound_obj;

	/* rendering state */
	unsigned int bg_color;
	GList *layers;
	ArtIRect irect;
	ArtIRect drawrect;
};

struct swf_sprite_struct {
	int n_frames;

	/* where we are in global parsing */
	bits_t parse;

	/* temporary state while parsing */
	bits_t b;

	int tag;
	int tag_len;
	int (*func)(swf_state_t *s);

	GList *frames;

	/* rendering state */
	unsigned int bg_color;
	GList *layers;
	ArtIRect irect;
	ArtIRect drawrect;
};

typedef struct swf_shape_vec_struct swf_shape_vec_t;

struct swf_shape_vec_struct {
	int type;
	int index;
	unsigned int color;
	double width;

	GArray *path;
	int array_len;

	int fill_type;
	int fill_id;
};

struct swf_shape_struct {
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

struct swf_object_struct {
	int id;
	int type;

	double trans[6];

	int number;

	void *priv;
};

struct swf_sound_struct{
	unsigned char *uncompressed_data;
	int format;

	unsigned char *orig_data;
	int orig_len;

	void *mp;

	int n_samples;

	void *sound_buf;
	int sound_len;
};

struct swf_button_struct{
	int id;
	double transform[6];
	double color_mult[4];
	double color_add[4];
};

struct swf_text_struct{
	int font;
	int height;
	unsigned int color;
	GArray *glyphs;
};

struct swf_image_struct{
	int width, height;
	int rowstride;
	char *image_data;
};

struct swf_text_glyph_struct{
	int x,y;
	int glyph;
};

struct swf_layer_vec_struct{
	ArtSVP *svp;
	unsigned int color;
	ArtIRect rect;
};

struct swf_layer_struct{
	int depth;
	int id;
	int first_frame;
	int last_frame;

	int frame_number;

	unsigned int prerendered : 1;

	double transform[6];
	double color_mult[4];
	double color_add[4];
	int ratio;

	GArray *lines;
	GArray *fills;

	swf_image_t *image;
};

struct swf_svp_render_struct{
	unsigned int color;
	unsigned char *buf;
	int rowstride;
	int x0, x1;
};

swf_state_t *swf_init(void);
swf_state_t *swfdec_decoder_new(void);
int swf_addbits(swf_state_t *s, unsigned char *bits, int len);
int swf_parse(swf_state_t *s);
int swf_parse_header(swf_state_t *s);
int swf_parse_tag(swf_state_t *s);
int tag_func_ignore(swf_state_t *s);

void swf_debug(swf_state_t *s, int n, char *format, ...);


#endif

