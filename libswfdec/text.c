
#include <stdio.h>

#include "swf.h"
#include "proto.h"
#include "bits.h"



int tag_func_define_font(swf_state_t *s)
{
	int id;
	int i;
	int n_glyphs;
	int offset;
	swf_shape_vec_t *shapevec;
	swf_shape_t *shape;
	swf_object_t *object;
	GArray *array;

	id = get_u16(&s->b);
	object = swf_object_new(s,id);

	offset = get_u16(&s->b);
	n_glyphs = offset/2;

	for(i=1;i<n_glyphs;i++){
		offset = get_u16(&s->b);
	}

	array = g_array_sized_new(TRUE,TRUE,sizeof(swf_shape_t),
			n_glyphs);
	object->priv = array;
	object->type = SWF_OBJECT_FONT;
	g_array_set_size(array,n_glyphs);

	for(i=0;i<n_glyphs;i++){
		shape = &g_array_index(array,swf_shape_t,i);

		shape->fills = g_ptr_array_sized_new(1);
		shapevec = swf_shape_vec_new();
		g_ptr_array_add(shape->fills, shapevec);
		shape->fills2 = g_ptr_array_sized_new(1);
		shapevec = swf_shape_vec_new();
		g_ptr_array_add(shape->fills2, shapevec);
		shape->lines = g_ptr_array_sized_new(1);
		shapevec = swf_shape_vec_new();
		g_ptr_array_add(shape->lines, shapevec);

		//swf_shape_add_styles(s,shape,&s->b);
		syncbits(&s->b);
		shape->n_fill_bits = getbits(&s->b,4);
		SWF_DEBUG(0,"n_fill_bits = %d\n",shape->n_fill_bits);
		shape->n_line_bits = getbits(&s->b,4);
		SWF_DEBUG(0,"n_line_bits = %d\n",shape->n_line_bits);

		swf_shape_get_recs(s,&s->b,shape);
	}
	
	return SWF_OK;
}


static int define_text(swf_state_t *s, int rgba)
{
	bits_t *bits = &s->b;
	int id;
	int rect[4];
	int n_glyph_bits;
	int n_advance_bits;
	swf_text_t *text = NULL;
	swf_text_glyph_t glyph = { 0 };
	swf_object_t *object;
	swf_text_t newtext = { 0 };
	GArray *array;

	id = get_u16(bits);
	object = swf_object_new(s,id);

	array = g_array_new(TRUE,FALSE,sizeof(swf_text_t));
	object->priv = array;
	object->type = SWF_OBJECT_TEXT;

	newtext.color = 0xffffffff;

	get_rect(bits,rect);
	get_art_matrix(bits,object->trans);
	syncbits(bits);
	n_glyph_bits = get_u8(bits);
	n_advance_bits = get_u8(bits);

	//printf("  id = %d\n", id);
	//printf("  n_glyph_bits = %d\n", n_glyph_bits);
	//printf("  n_advance_bits = %d\n", n_advance_bits);

	while(peekbits(bits,8)!=0){
		int type;

		type = getbit(bits);
		if(type==0){
			/* glyph record */
			int n_glyphs;
			int i;

			n_glyphs = getbits(bits,7);
			for(i=0;i<n_glyphs;i++){
				glyph.glyph = getbits(bits,n_glyph_bits);

				g_array_append_val(text->glyphs,glyph);
				glyph.x += getbits(bits,n_advance_bits);
			}
		}else{
			/* state change */
			int reserved;
			int has_font;
			int has_color;
			int has_y_offset;
			int has_x_offset;
			
			reserved = getbits(bits,3);
			has_font = getbit(bits);
			has_color = getbit(bits);
			has_y_offset = getbit(bits);
			has_x_offset = getbit(bits);
			if(has_font){
				newtext.font = get_u16(bits);
				//printf("  font = %d\n",font);
			}
			if(has_color){
				if(rgba){
					newtext.color = get_rgba(bits);
				}else{
					newtext.color = get_color(bits);
				}
				//printf("  color = %08x\n",newtext.color);
			}
			if(has_x_offset){
				glyph.x = get_u16(bits);
				//printf("  x = %d\n",x);
			}
			if(has_y_offset){
				glyph.y = get_u16(bits);
				//printf("  y = %d\n",y);
			}
			if(has_font){
				newtext.height = get_u16(bits);
				//printf("  height = %d\n",height);
			}
			if(has_font || has_color){
				g_array_append_val(array,newtext);
				text = &g_array_index(array,swf_text_t,array->len-1);
				text->glyphs = g_array_new(TRUE,FALSE,
					sizeof(swf_text_glyph_t));
			}
		}
		syncbits(bits);
	}
	get_u8(bits);

	return SWF_OK;
}

int tag_func_define_text(swf_state_t *s)
{
	return define_text(s,0);
}

int tag_func_define_text_2(swf_state_t *s)
{
	return define_text(s,1);
}



