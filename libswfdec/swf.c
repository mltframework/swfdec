
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>
#include <mad.h>

#include "swf.h"
#include "tags.h"
#include "proto.h"
#include "bits.h"

int swf_parse_header1(swf_state_t *s);
int swf_inflate_init(swf_state_t *s);
int swf_parse_header2(swf_state_t *s);

swf_state_t *swf_init(void)
{
	swf_state_t *s;

	s = malloc(sizeof(*s));
	memset(s,0,sizeof(*s));

	s->bg_color = SWF_COLOR_COMBINE(0xff,0xff,0xff,0xff);

	s->debug = 2;

	art_affine_identity(s->transform);
#if 0
	art_affine_scale(s->transform,0.5,0.5);
	s->transform[4] = 50;
	s->transform[5] = 50;
#endif

	return s;
}

int swf_addbits(swf_state_t *s, unsigned char *bits, int len)
{
	int offset;
	int ret;

	if(s->compressed){
		s->z->next_in = bits;
		s->z->avail_in = len;
		ret = inflate(s->z,Z_SYNC_FLUSH);
		if(ret<0){
			return SWF_ERROR;
		}

		s->parse.end = s->input_data + s->z->total_out;
	}else{
		if(s->parse.ptr){
			offset = (void *)s->parse.ptr - (void *)s->input_data;
		}else{
			offset = 0;
		}

		s->input_data = realloc(s->input_data, s->input_data_len + len);
		memcpy(s->input_data + s->input_data_len, bits, len);
		s->input_data_len += len;

		s->parse.ptr = s->input_data + offset;
		s->parse.end = s->input_data + s->input_data_len;
	}

	return SWF_OK;
	//return swf_parse(s);
}

int swf_parse(swf_state_t *s)
{
	int ret = SWF_OK;
	unsigned char *endptr;

	while(ret==SWF_OK){
		//printf("iter\n");
		s->b = s->parse;

		switch(s->state){
		case SWF_STATE_INIT1:
			/* need to initialize */
			ret = swf_parse_header1(s);
			if(ret != SWF_OK) break;

			s->parse = s->b;
			if(s->compressed){
				swf_inflate_init(s);
			}
			s->state = SWF_STATE_INIT2;
			ret = SWF_OK;
			break;
		case SWF_STATE_INIT2:
			ret = swf_parse_header2(s);
			if(ret == SWF_OK){
				syncbits(&s->b);
				s->parse = s->b;
				s->state = SWF_STATE_PARSETAG;
				{
					ArtIRect rect;
					rect.x0 = 0;
					rect.y0 = 0;
					rect.x1 = s->width;
					rect.y1 = s->height;
					swf_invalidate_irect(s,&rect);
				}
				ret = SWF_CHANGE;
				break;
			}
			break;
		case SWF_STATE_PARSETAG:
			/* we're parsing tags */
			ret = swf_parse_tag(s);
			if(ret != SWF_OK)break;

			if(bits_needbits(&s->b, s->tag_len)){
				ret = SWF_NEEDBITS;
				break;
			}
			endptr = s->b.ptr + s->tag_len;
	
			ret = s->func(s);
			//if(ret != SWF_OK)break;

			syncbits(&s->b);
			if(s->b.ptr < endptr){
				SWF_DEBUG(3,"early parse finish (%d bytes)\n",
					endptr - s->b.ptr);
			}
			if(s->b.ptr > endptr){
				SWF_DEBUG(3,"parse overrun (%d bytes)\n",
					s->b.ptr - endptr);
			}

			s->parse.ptr = endptr;

			if(s->tag==0){
				s->state = SWF_STATE_EOF;
			}
	
			break;
		case SWF_STATE_EOF:
			ret = SWF_EOF;

			break;
		}
	}

	fflush(stdout);

	return ret;
}

#define bits_ret_if_needbits(bits, n_bytes)	if(bits_needbits(bits,n_bytes))return SWF_NEEDBITS

static void *zalloc(void *opaque, unsigned int items, unsigned int size)
{
	return malloc(items*size);
}

static void zfree(void *opaque, void *addr)
{
	free(addr);
}

#if 0
static void dumpbits(bits_t *b)
{
	int i;

	printf("    ");
	for(i=0;i<16;i++){
		printf("%02x ",get_u8(b));
	}
	printf("\n");
}
#endif

int swf_parse_header1(swf_state_t *s)
{
	int sig1,sig2,sig3;

	bits_ret_if_needbits(&s->b, 8);

	sig1 = get_u8(&s->b);
	sig2 = get_u8(&s->b);
	sig3 = get_u8(&s->b);

	if((sig1 != 'F' && sig1 != 'C') || sig2 != 'W' || sig3 != 'S')
		return SWF_ERROR;
	
	s->compressed = (sig1 == 'C');

	s->version = get_u8(&s->b);
	s->length = get_u32(&s->b);

	return SWF_OK;
}

int swf_inflate_init(swf_state_t *s)
{
	z_stream *z;
	char *compressed_data;
	int compressed_len;
	char *data;
	int ret;

	z = g_new0(z_stream,1);
	s->z = z;
	z->zalloc = zalloc;
	z->zfree = zfree;

	compressed_data = s->parse.ptr;
	compressed_len = s->input_data_len -
		((void *)s->parse.ptr - (void *)s->input_data);

	z->next_in = compressed_data;
	z->avail_in = compressed_len;
	z->opaque = NULL;
	data = malloc(s->length);
	z->next_out = data;
	z->avail_out = s->length;

	ret = inflateInit(z);
	printf("inflateInit returned %d\n",ret);
	ret = inflate(z,Z_SYNC_FLUSH);
	printf("inflate returned %d\n",ret);
	printf("total out %d\n",(int)z->total_out);
	printf("total in %d\n",(int)z->total_in);

	free(s->input_data);

	s->input_data = data;
	s->input_data_len = z->total_in;
	s->parse.ptr = data;

	return SWF_OK;
}

int swf_parse_header2(swf_state_t *s)
{
	int rect[4];

	bits_ret_if_needbits(&s->b, 32);

	get_rect(&s->b, rect);
	s->width = rect[1]/20;
	s->height = rect[3]/20;
	s->irect.x0 = 0;
	s->irect.y0 = 0;
	s->irect.x1 = s->width;
	s->irect.y1 = s->height;
	syncbits(&s->b);
	s->rate = get_u16(&s->b)/256.0;
	SWF_DEBUG(4,"rate = %g\n",s->rate);
	s->n_frames = get_u16(&s->b);
	SWF_DEBUG(4,"n_frames = %d\n",s->n_frames);

	return SWF_OK;
}


swf_object_t *swf_object_new(swf_state_t *s, int id)
{
	swf_object_t *obj;

	obj = g_new0(swf_object_t,1);

	obj->id = id;

	s->objects = g_list_append(s->objects, obj);

	return obj;
}


struct tag_func_struct {
	char *name;
	int (*func)(swf_state_t *s);
	int flag;
};
struct tag_func_struct tag_funcs[] = {
	[ ST_END		] = { "End",		tag_func_zero,	0 },
	[ ST_SHOWFRAME		] = { "ShowFrame",	art_show_frame,	0 },
	[ ST_DEFINESHAPE	] = { "DefineShape",	art_define_shape,	0 },
	[ ST_FREECHARACTER	] = { "FreeCharacter",	NULL,	0 },
	[ ST_PLACEOBJECT	] = { "PlaceObject",	NULL,	0 },
	[ ST_REMOVEOBJECT	] = { "RemoveObject",	art_remove_object,	0 },
//	[ ST_DEFINEBITS		] = { "DefineBits",	NULL,	0 },
	[ ST_DEFINEBITSJPEG	] = { "DefineBitsJPEG",	NULL,	0 },
	[ ST_DEFINEBUTTON	] = { "DefineButton",	NULL,	0 },
	[ ST_JPEGTABLES		] = { "JPEGTables",	NULL,	0 },
	[ ST_SETBACKGROUNDCOLOR	] = { "SetBackgroundColor",	tag_func_set_background_color,	0 },
	[ ST_DEFINEFONT		] = { "DefineFont",	tag_func_define_font,	0 },
	[ ST_DEFINETEXT		] = { "DefineText",	tag_func_define_text,	0 },
	[ ST_DOACTION		] = { "DoAction",	tag_func_do_action,	0 },
	[ ST_DEFINEFONTINFO	] = { "DefineFontInfo",	tag_func_ignore_quiet,	0 },
	[ ST_DEFINESOUND	] = { "DefineSound",	tag_func_define_sound,	0 },
	[ ST_STARTSOUND		] = { "StartSound",	tag_func_start_sound,	0 },
	[ ST_DEFINEBUTTONSOUND	] = { "DefineButtonSound",	tag_func_define_button_sound,	0 },
	[ ST_SOUNDSTREAMHEAD	] = { "SoundStreamHead",	tag_func_sound_stream_head,	0 },
	[ ST_SOUNDSTREAMBLOCK	] = { "SoundStreamBlock",	tag_func_sound_stream_block,	0 },
	[ ST_DEFINEBITSLOSSLESS	] = { "DefineBitsLossless",	NULL,	0 },
	[ ST_DEFINEBITSJPEG2	] = { "DefineBitsJPEG2",	tag_func_define_bits_jpeg_2,	0 },
	[ ST_DEFINESHAPE2	] = { "DefineShape2",	art_define_shape_2,	0 },
	[ ST_DEFINEBUTTONCXFORM	] = { "DefineButtonCXForm",	NULL,	0 },
	[ ST_PROTECT		] = { "Protect",	tag_func_zero,	0 },
	[ ST_PLACEOBJECT2	] = { "PlaceObject2",	art_place_object_2,	0 },
	[ ST_REMOVEOBJECT2	] = { "RemoveObject2",	art_remove_object_2,	0 },
	[ ST_DEFINESHAPE3	] = { "DefineShape3",	art_define_shape_3,	0 },
	[ ST_DEFINETEXT2	] = { "DefineText2",	tag_func_define_text_2,	0 },
	[ ST_DEFINEBUTTON2	] = { "DefineButton2",	tag_func_define_button_2,	0 },
	[ ST_DEFINEBITSJPEG3	] = { "DefineBitsJPEG3",	NULL,	0 },
	[ ST_DEFINEBITSLOSSLESS2] = { "DefineBitsLossless2",	NULL,	0 },
	[ ST_DEFINEEDITTEXT	] = { "DefineEditText",	NULL,	0 },
	[ ST_DEFINEMOVIE	] = { "DefineMovie",	NULL,	0 },
	[ ST_DEFINESPRITE	] = { "DefineSprite",	tag_func_define_sprite,	0 },
	[ ST_NAMECHARACTER	] = { "NameCharacter",	NULL,	0 },
	[ ST_SERIALNUMBER	] = { "SerialNumber",	NULL,	0 },
	[ ST_GENERATORTEXT	] = { "GeneratorText",	NULL,	0 },
	[ ST_FRAMELABEL		] = { "FrameLabel",	tag_func_frame_label,	0 },
	[ ST_SOUNDSTREAMHEAD2	] = { "SoundStreamHead2",	NULL,	0 },
	[ ST_DEFINEMORPHSHAPE	] = { "DefineMorphShape",	NULL,	0 },
	[ ST_DEFINEFONT2	] = { "DefineFont2",	NULL,	0 },
	[ ST_TEMPLATECOMMAND	] = { "TemplateCommand",	NULL,	0 },
	[ ST_GENERATOR3		] = { "Generator3",	NULL,	0 },
	[ ST_EXTERNALFONT	] = { "ExternalFont",	NULL,	0 },
	[ ST_EXPORTASSETS	] = { "ExportAssets",	NULL,	0 },
	[ ST_IMPORTASSETS	] = { "ImportAssets",	NULL,	0 },
	[ ST_ENABLEDEBUGGER	] = { "EnableDebugger",	NULL,	0 },
	[ ST_MX0		] = { "MX0",	NULL,	0 },
	[ ST_MX1		] = { "MX1",	NULL,	0 },
	[ ST_MX2		] = { "MX2",	NULL,	0 },
	[ ST_MX3		] = { "MX3",	NULL,	0 },
	[ ST_MX4		] = { "MX4",	NULL,	0 },
//	[ ST_REFLEX		] = { "Reflex",	NULL,	0 },
};
static const int n_tag_funcs = sizeof(tag_funcs)/sizeof(tag_funcs[0]);

int swf_parse_tag(swf_state_t *s)
{
	unsigned int x;
	bits_t *b = &s->b;
	char *name;

	bits_ret_if_needbits(&s->b, 2);

	x = get_u16(b);
	s->tag = (x>>6)&0x3ff;
	s->tag_len = x&0x3f;
	if(s->tag_len==0x3f){
		bits_ret_if_needbits(&s->b, 4);
		s->tag_len = get_u32(b);
	}

	s->func = NULL;
	name = "";

	if(s->tag >=0 && s->tag < n_tag_funcs){
		s->func = tag_funcs[s->tag].func;
		name = tag_funcs[s->tag].name;
	}

	if(!s->func){
		s->func = tag_func_ignore;
	}

	SWF_DEBUG(0,"tag=%d len=%d name=\"%s\"\n", s->tag, s->tag_len, name);

	return SWF_OK;
}

int tag_func_zero(swf_state_t *s)
{
	return SWF_OK;
}

int tag_func_ignore_quiet(swf_state_t *s)
{
	s->b.ptr += s->tag_len;

	return SWF_OK;
}

int tag_func_ignore(swf_state_t *s)
{
	char *name = "";

	if(s->tag >=0 && s->tag < n_tag_funcs){
		name = tag_funcs[s->tag].name;
	}

	SWF_DEBUG(3,"tag \"%s\" (%d) ignored\n", name, s->tag);

	s->b.ptr += s->tag_len;

	return SWF_OK;
}

int tag_func_dumpbits(swf_state_t *s)
{
	bits_t *b = &s->b;
	int i;

	printf("    ");
	for(i=0;i<16;i++){
		printf("%02x ",get_u8(b));
	}
	printf("\n");

	return SWF_OK;
}

int tag_func_set_background_color(swf_state_t *s)
{
	ArtIRect rect;

	s->bg_color = get_color(&s->b);

	rect.x0 = 0;
	rect.y0 = 0;
	rect.x1 = s->width;
	rect.y1 = s->height;

	swf_invalidate_irect(s,&rect);

	return SWF_OK;
}

int tag_func_frame_label(swf_state_t *s)
{
	free(get_string(&s->b));

	return SWF_OK;
}

int tag_func_place_object_2(swf_state_t *s)
{
	bits_t *bits = &s->b;
	int reserved;
	int has_name;
	int has_ratio;
	int has_color_transform;
	int has_matrix;
	int has_character;
	int move;
	int depth;
	int character_id;
	int ratio;

	reserved = getbits(bits,2);
	has_name = getbit(bits);
	has_ratio = getbit(bits);
	has_color_transform = getbit(bits);
	has_matrix = getbit(bits);
	has_character = getbit(bits);
	move = getbit(bits);
	depth = get_u16(bits);

	printf("  reserved = %d\n",reserved);
	printf("  depth = %d\n",depth);

	if(has_character){
		character_id = get_u16(bits);
		printf("  id = %d\n",character_id);
	}
	if(has_matrix){
		get_matrix(bits);
	}
	if(has_color_transform){
		get_color_transform(bits);
		syncbits(bits);
	}
	if(has_ratio){
		ratio = get_u16(bits);
		printf("  ratio = %d\n",ratio);
	}
	if(has_name){
		free(get_string(bits));
	}

	return SWF_OK;
}

int tag_func_remove_object(swf_state_t *s)
{
	int id;
	int depth;

	id = get_u16(&s->b);
	depth = get_u16(&s->b);

	printf("  id = %d\n",id);
	printf("  depth = %d\n",depth);

	return SWF_OK;
}

int tag_func_remove_object_2(swf_state_t *s)
{
	int depth;

	depth = get_u16(&s->b);

	printf("  depth = %d\n",depth);

	return SWF_OK;
}


void swf_debug(swf_state_t *s, int n, char *format, ...)
{
	va_list args;
	int offset;
	char *name = "unknown";

	if(n<s->debug)return;

	offset = (void *)s->parse.ptr - (void *)s->input_data;
	if(s->tag >=0 && s->tag < n_tag_funcs){
		name = tag_funcs[s->tag].name;
	}
	printf("DEBUG: [%d,%s] ", offset, name);
	va_start(args, format);
	vprintf(format,args);
	va_end(args);
}

