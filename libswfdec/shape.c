
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libart_lgpl/libart.h>

#include <mad.h>

#include "swf.h"
#include "tags.h"
#include "proto.h"
#include "bits.h"

static inline void
art_affine_copy(double *dest,double *src)
{
	memcpy(dest,src,sizeof(double)*6);
}

int get_shape_rec(bits_t *bits,int n_fill_bits, int n_line_bits)
{
	int type;
	int state_new_styles;
	int state_line_styles;
	int state_fill_styles1;
	int state_fill_styles0;
	int state_moveto;
	int movebits = 0;
	int movex;
	int movey;
	int fill0style;
	int fill1style;
	int linestyle = 0;

	type = peekbits(bits,6);
	if(type==0){
		getbits(bits,6);
		return 0;
	}

	type = getbits(bits,1);
	printf("   type = %d\n", type);

	if(type == 0){
		state_new_styles = getbits(bits,1);
		state_line_styles = getbits(bits,1);
		state_fill_styles1 = getbits(bits,1);
		state_fill_styles0 = getbits(bits,1);
		state_moveto = getbits(bits,1);

		printf("   state_new_styles = %d\n", state_new_styles);
		printf("   state_line_styles = %d\n", state_line_styles);
		printf("   state_fill_styles1 = %d\n", state_fill_styles1);
		printf("   state_fill_styles0 = %d\n", state_fill_styles0);
		printf("   state_moveto = %d\n", state_moveto);

		if(state_moveto){
			movebits = getbits(bits,5);
			printf("   movebits = %d\n", movebits);
			movex = getsbits(bits,movebits);
			movey = getsbits(bits,movebits);
			printf("   movex = %d\n", movex);
			printf("   movey = %d\n", movey);
		}
		if(state_fill_styles0){
			fill0style = getbits(bits,n_fill_bits);
			printf("   fill0style = %d\n", fill0style);
		}
		if(state_fill_styles1){
			fill1style = getbits(bits,n_fill_bits);
			printf("   fill1style = %d\n", fill1style);
		}
		if(state_line_styles){
			linestyle = getbits(bits,n_line_bits);
			printf("   linestyle = %d\n", linestyle);
		}
		if(state_new_styles){
			printf("***** new styles not implemented\n");
		}


	}else{
		/* edge record */
		int n_bits;
		int edge_flag;

		edge_flag = getbits(bits,1);
		printf("   edge_flag = %d\n",edge_flag);

		if(edge_flag == 0){
			int control_delta_x;
			int control_delta_y;
			int anchor_delta_x;
			int anchor_delta_y;

			n_bits = getbits(bits,4) + 2;
			control_delta_x = getsbits(bits,n_bits);
			control_delta_y = getsbits(bits,n_bits);
			anchor_delta_x = getsbits(bits,n_bits);
			anchor_delta_y = getsbits(bits,n_bits);

			printf("   n_bits = %d\n",n_bits);
			printf("   control_delta = %d,%d\n",
				control_delta_x, control_delta_y);
			printf("   anchor_delta = %d,%d\n",
				anchor_delta_x, anchor_delta_y);
		}else{
			int general_line_flag;
			int delta_x;
			int delta_y;
			int vert_line_flag = 0;

			n_bits = getbits(bits,4) + 2;
			general_line_flag = getbit(bits);
			if(general_line_flag == 1){
				delta_x = getsbits(bits,n_bits);
				delta_y = getsbits(bits,n_bits);
			}else{
				vert_line_flag = getbit(bits);
				if(vert_line_flag == 0){
					delta_x = getsbits(bits,n_bits);
					delta_y = 0;
				}else{
					delta_x = 0;
					delta_y = getsbits(bits,n_bits);
				}
			}
			printf("   general_line_flag = %d\n",general_line_flag);
			if(general_line_flag == 0){
				printf("   vert_line_flag = %d\n",vert_line_flag);
			}
			printf("   n_bits = %d\n", n_bits);
			printf("   delta_x = %d\n", delta_x);
			printf("   delta_y = %d\n", delta_y);
		}
	}

	return 1;
}

int tag_func_define_shape(swf_state_t *s)
{
	bits_t *b = &s->b;
	int id;
	int n_fill_styles;
	int n_line_styles;
	int n_fill_bits;
	int n_line_bits;
	int i;

	id = get_u16(b);
	SWF_DEBUG(0,"  id = %d\n", id);
	printf("  bounds = %s\n",getrect(b));
	syncbits(b);
	n_fill_styles = get_u8(b);
	SWF_DEBUG(0,"  n_fill_styles = %d\n",n_fill_styles);
	for(i=0;i<n_fill_styles;i++){
		get_fill_style(b);
	}
	syncbits(b);
	n_line_styles = get_u8(b);
	SWF_DEBUG(0,"  n_line_styles = %d\n",n_line_styles);
	for(i=0;i<n_line_styles;i++){
		get_line_style(b);
	}
	syncbits(b);
	n_fill_bits = getbits(b,4);
	n_line_bits = getbits(b,4);
	SWF_DEBUG(0,"  n_fill_bits = %d\n",n_fill_bits);
	SWF_DEBUG(0,"  n_line_bits = %d\n",n_line_bits);
	do{
		SWF_DEBUG(0,"  shape_rec:\n");
	}while(get_shape_rec(b,n_fill_bits, n_line_bits));

	syncbits(b);

	return SWF_OK;
}

swf_shape_vec_t *swf_shape_vec_new(void)
{
	swf_shape_vec_t *shapevec;

	shapevec = g_new0(swf_shape_vec_t,1);

	shapevec->path = g_array_new(FALSE,FALSE,sizeof(ArtBpath));

	return shapevec;
}

int art_define_shape(swf_state_t *s)
{
	bits_t *bits = &s->b;
	swf_object_t *object;
	swf_shape_t *shape;
	int id;

	id = get_u16(bits);
	object = swf_object_new(s,id);

	shape = g_new0(swf_shape_t,1);
	object->priv = shape;
	object->type = SWF_OBJECT_SHAPE;
	SWF_DEBUG(0,"  ID: %d\n", object->id);

	getrect(bits);

	shape->fills = g_ptr_array_new();
	shape->fills2 = g_ptr_array_new();
	shape->lines = g_ptr_array_new();

	swf_shape_add_styles(s,shape,bits);

	swf_shape_get_recs(s,bits,shape);

	return SWF_OK;
}

int art_define_shape_3(swf_state_t *s)
{
	bits_t *bits = &s->b;
	swf_object_t *object;
	swf_shape_t *shape;
	int id;

	id = get_u16(bits);
	object = swf_object_new(s,id);

	shape = g_new0(swf_shape_t,1);
	object->priv = shape;
	object->type = SWF_OBJECT_SHAPE;
	SWF_DEBUG(0,"  ID: %d\n", object->id);

	getrect(bits);

	shape->fills = g_ptr_array_new();
	shape->fills2 = g_ptr_array_new();
	shape->lines = g_ptr_array_new();

	shape->rgba = 1;

	swf_shape_add_styles(s,shape,bits);

	swf_shape_get_recs(s,bits,shape);

	return SWF_OK;
}

void swf_shape_add_styles(swf_state_t *s, swf_shape_t *shape, bits_t *bits)
{
	int n_fill_styles;
	int n_line_styles;
	int i;

	syncbits(bits);
	shape->fills_offset = shape->fills->len;
	n_fill_styles = get_u8(bits);
	SWF_DEBUG(0,"   n_fill_styles %d\n",n_fill_styles);
	for(i=0;i<n_fill_styles;i++){
		int fill_style_type;
		swf_shape_vec_t *shapevec;

		SWF_DEBUG(0,"   fill style %d:\n",i);

		shapevec = swf_shape_vec_new();
		g_ptr_array_add(shape->fills2, shapevec);
		shapevec = swf_shape_vec_new();
		g_ptr_array_add(shape->fills, shapevec);

		shapevec->color = SWF_COLOR_COMBINE(0,255,0,255);

		fill_style_type = get_u8(bits);
		SWF_DEBUG(0,"    type 0x%02x\n",fill_style_type);
		if(fill_style_type == 0x00){
			if(shape->rgba){
				shapevec->color = get_rgba(bits);
			}else{
				shapevec->color = get_color(bits);
			}
			SWF_DEBUG(0,"    color %08x\n",shapevec->color);
		}
		if(fill_style_type == 0x10 || fill_style_type == 0x12){
			get_matrix(bits);
			if(shape->rgba){
				shapevec->color = get_gradient_rgba(bits);
			}else{
				shapevec->color = get_gradient(bits);
			}
		}
		if(fill_style_type == 0x40 || fill_style_type == 0x41){
			shapevec->fill_type = fill_style_type;
			shapevec->fill_id = get_u16(bits);
			SWF_DEBUG(0,"   background fill id = %d\n",shapevec->fill_id);

			get_matrix(bits);
		}
	}

	syncbits(bits);
	shape->lines_offset = shape->lines->len;
	n_line_styles = get_u8(bits);
	SWF_DEBUG(0,"   n_line_styles %d\n",n_line_styles);
	for(i=0;i<n_line_styles;i++){
		swf_shape_vec_t *shapevec;

		shapevec = swf_shape_vec_new();
		g_ptr_array_add(shape->lines, shapevec);

		/* FIXME  don't know why some animations use a scale
		 * factor of 0.05.  It might be part of overall scaling.
		 * We hack a correction here.
		 */
		shapevec->width = get_u16(bits);
		//if(shapevec->width>10){
			shapevec->width *= SWF_SCALE_FACTOR;
		//}
		if(shape->rgba){
			shapevec->color = get_rgba(bits);
		}else{
			shapevec->color = get_color(bits);
		}
	}

	syncbits(bits);
	shape->n_fill_bits = getbits(bits,4);
	shape->n_line_bits = getbits(bits,4);
}

void swf_shape_get_recs(swf_state_t *s, bits_t *bits, swf_shape_t *shape)
{
	int x = 0, y = 0;
	int fill0style = 0;
	int fill1style = 0;
	int linestyle = 0;
	int n_vec = 0;
	swf_shape_vec_t *shapevec;
	ArtBpath pt;
	int i;

	while(peekbits(bits,6)!=0){
		int type;
		int n_bits;

		type = getbits(bits,1);

		if(type == 0){
			int state_new_styles = getbits(bits,1);
			int state_line_styles = getbits(bits,1);
			int state_fill_styles1 = getbits(bits,1);
			int state_fill_styles0 = getbits(bits,1);
			int state_moveto = getbits(bits,1);

			if(state_moveto){
				n_bits = getbits(bits,5);
				x = getsbits(bits,n_bits);
				y = getsbits(bits,n_bits);

				SWF_DEBUG(0,"   moveto %d,%d\n", x, y);
			}
			if(state_fill_styles0){
				fill0style = getbits(bits,shape->n_fill_bits);
				SWF_DEBUG(0,"   * fill0style = %d\n",fill0style);
			}
			if(state_fill_styles1){
				fill1style = getbits(bits,shape->n_fill_bits);
				SWF_DEBUG(0,"   * fill1style = %d\n",fill1style);
			}
			if(state_line_styles){
				linestyle = getbits(bits,shape->n_line_bits);
				SWF_DEBUG(0,"   * linestyle = %d\n",linestyle);
			}
			if(state_new_styles){
				swf_shape_add_styles(s,shape,bits);
				SWF_DEBUG(0,"swf_shape_get_recs: new styles\n");
			}
			pt.code = ART_MOVETO_OPEN;
			pt.x3 = x*SWF_SCALE_FACTOR;
			pt.y3 = y*SWF_SCALE_FACTOR;
		}else{
			/* edge record */
			int n_bits;
			int edge_flag;

			edge_flag = getbits(bits,1);

			if(edge_flag == 0){
				double x0,y0;
				double x1,y1;
				double x2,y2;

				x0 = x*SWF_SCALE_FACTOR;
				y0 = y*SWF_SCALE_FACTOR;
				n_bits = getbits(bits,4) + 2;
	
				x += getsbits(bits,n_bits);
				y += getsbits(bits,n_bits);
				SWF_DEBUG(0,"   control %d,%d\n", x, y);
				x1 = x*SWF_SCALE_FACTOR;
				y1 = y*SWF_SCALE_FACTOR;

				x += getsbits(bits,n_bits);
				y += getsbits(bits,n_bits);
				SWF_DEBUG(0,"   anchor %d,%d\n", x, y);
				x2 = x*SWF_SCALE_FACTOR;
				y2 = y*SWF_SCALE_FACTOR;

#define WEIGHT 0.6
				pt.code = ART_CURVETO;
				pt.x1 = WEIGHT*x1 + (1-WEIGHT)*x0;
				pt.y1 = WEIGHT*y1 + (1-WEIGHT)*y0;
				pt.x2 = WEIGHT*x1 + (1-WEIGHT)*x2;
				pt.y2 = WEIGHT*y1 + (1-WEIGHT)*y2;
				pt.x3 = x2;
				pt.y3 = y2;
				n_vec++;
			}else{
				int general_line_flag;
				int vert_line_flag = 0;

				n_bits = getbits(bits,4) + 2;
				general_line_flag = getbit(bits);
				if(general_line_flag == 1){
					x += getsbits(bits,n_bits);
					y += getsbits(bits,n_bits);
				}else{
					vert_line_flag = getbit(bits);
					if(vert_line_flag == 0){
						x += getsbits(bits,n_bits);
					}else{
						y += getsbits(bits,n_bits);
					}
				}
				SWF_DEBUG(0,"   delta %d,%d\n", x, y);

				pt.code = ART_LINETO;
				pt.x3 = x*SWF_SCALE_FACTOR;
				pt.y3 = y*SWF_SCALE_FACTOR;
			}
		}
		if(fill0style){
			shapevec = g_ptr_array_index(shape->fills,
				shape->fills_offset + fill0style-1);
			g_array_append_val(shapevec->path,pt);
		}
		if(fill1style){
			SWF_DEBUG(0,"   using shapevec %d\n",shape->fills_offset);
			shapevec = g_ptr_array_index(shape->fills2,
				shape->fills_offset + fill1style-1);
			g_array_append_val(shapevec->path,pt);
		}
		if(linestyle){
			shapevec = g_ptr_array_index(shape->lines,
				shape->lines_offset + linestyle-1);
			g_array_append_val(shapevec->path,pt);
		}

	}

	getbits(bits,6);
	syncbits(bits);

	pt.code = ART_END;
	for(i=0;i<shape->fills->len;i++){
		shapevec = g_ptr_array_index(shape->fills,i);
		g_array_append_val(shapevec->path,pt);

		shapevec = g_ptr_array_index(shape->fills2,i);
		g_array_append_val(shapevec->path,pt);
	}
	for(i=0;i<shape->lines->len;i++){

		shapevec = g_ptr_array_index(shape->lines,i);
		g_array_append_val(shapevec->path,pt);
	}
}

int art_define_shape_2(swf_state_t *s)
{
	return art_define_shape(s);
}

int tag_func_define_button_2(swf_state_t *s)
{
	bits_t *bits = &s->b;
	int id;
	int flags;
	int offset;
	int condition;
	swf_object_t *object;
	double trans[6];
	double color_add[4], color_mult[4];
	swf_button_t *button;
	unsigned char *endptr;

	endptr = bits->ptr + s->tag_len;

	id = get_u16(bits);
	object = swf_object_new(s,id);

	button = g_new0(swf_button_t,3);
	object->type = SWF_OBJECT_BUTTON;
	object->priv = button;

	SWF_DEBUG(0,"  ID: %d\n", object->id);

	flags = get_u8(bits);
	offset = get_u16(bits);

	SWF_DEBUG(0,"  flags = %d\n",flags);
	SWF_DEBUG(0,"  offset = %d\n",offset);

	while(peek_u8(bits)){
		int reserved;
		int hit_test;
		int down;
		int over;
		int up;
		int character;
		int layer;

		syncbits(bits);
		reserved = getbits(bits,4);
		hit_test = getbit(bits);
		down = getbit(bits);
		over = getbit(bits);
		up = getbit(bits);
		character = get_u16(bits);
		layer = get_u16(bits);

		SWF_DEBUG(0,"  reserved = %d\n",reserved);
		if(reserved){
			SWF_DEBUG(4,"reserved is supposed to be 0\n");
		}
		SWF_DEBUG(0,"  hit_test = %d\n",hit_test);
		SWF_DEBUG(0,"  down = %d\n",down);
		SWF_DEBUG(0,"  over = %d\n",over);
		SWF_DEBUG(0,"  up = %d\n",up);
		SWF_DEBUG(0,"  character = %d\n",character);
		SWF_DEBUG(0,"  layer = %d\n",layer);

		SWF_DEBUG(0,"bits->ptr %p\n",bits->ptr);

		//get_art_matrix(bits, trans);
		get_art_matrix(bits, trans);
		syncbits(bits);
		SWF_DEBUG(0,"bits->ptr %p\n",bits->ptr);
		get_art_color_transform(bits, color_mult, color_add);
		syncbits(bits);

		SWF_DEBUG(0,"bits->ptr %p\n",bits->ptr);

		if(up){
			button[0].id = character;
			art_affine_copy(button[0].transform,trans);
			memcpy(button[0].color_mult,color_mult,4*sizeof(double));
			memcpy(button[0].color_add,color_add,4*sizeof(double));
		}
		if(over){
			button[1].id = character;
			art_affine_copy(button[1].transform,trans);
			memcpy(button[1].color_mult,color_mult,4*sizeof(double));
			memcpy(button[1].color_add,color_add,4*sizeof(double));
		}
		if(down){
			button[2].id = character;
			art_affine_copy(button[2].transform,trans);
			memcpy(button[2].color_mult,color_mult,4*sizeof(double));
			memcpy(button[2].color_add,color_add,4*sizeof(double));
		}

	}
	get_u8(bits);

	while(offset!=0){
		offset = get_u16(bits);
		condition = get_u16(bits);

		SWF_DEBUG(0,"  offset = %d\n",offset);
		SWF_DEBUG(0,"  condition = 0x%04x\n",condition);

		get_actions(s,bits);
	}

	return SWF_OK;
}

#undef SWF_DEBUG_LEVEL
#define SWF_DEBUG_LEVEL 0

int tag_func_define_sprite(swf_state_t *s)
{
	bits_t *bits = &s->b;
	int id;
	swf_object_t *object;
	swf_state_t *sprite;
	int ret;

	id = get_u16(bits);
	object = swf_object_new(s,id);

	SWF_DEBUG(0,"  ID: %d\n", object->id);

	sprite = swf_init();
	object->priv = sprite;
	object->type = SWF_OBJECT_SPRITE;

	sprite->n_frames = get_u16(bits);

	sprite->state = SWF_STATE_PARSETAG;
	sprite->no_render = 1;

	sprite->parse = *bits;

	/* massive hack */
	sprite->objects = s->objects;

	ret = swf_parse(sprite);
	SWF_DEBUG(0,"  ret = %d\n", ret);

	*bits = sprite->parse;

	sprite->frame_number = 0;

	//dump_layers(sprite);

	return SWF_OK;
}

void dump_layers(swf_state_t *s)
{
	GList *g;
	swf_layer_t *layer;

	for(g=g_list_last(s->layers); g; g=g_list_previous(g)){
		layer = (swf_layer_t *)g->data;

		printf("  layer %d [%d,%d)\n",layer->depth,
			layer->first_frame, layer->last_frame);
	}
}

