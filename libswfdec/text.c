
#include "swfdec_internal.h"


void swfdec_font_free(SwfdecObject *object)
{
	GArray *array = object->priv;
	SwfdecShape *shape;
	int i;

	for(i=0;i<array->len;i++){
		shape = &g_array_index(array,SwfdecShape,i);
		_swfdec_shape_free(shape);
	}
	g_array_free(array,TRUE);
}

void swfdec_text_free(SwfdecObject *object)
{
	GArray *array = object->priv;

	g_array_free(array,TRUE);
}

int tag_func_define_font(SwfdecDecoder *s)
{
	int id;
	int i;
	int n_glyphs;
	int offset;
	SwfdecShapeVec *shapevec;
	SwfdecShape *shape;
	SwfdecObject *object;
	GArray *array;

	id = get_u16(&s->b);
	object = swfdec_object_new(s,id);

	offset = get_u16(&s->b);
	n_glyphs = offset/2;

	for(i=1;i<n_glyphs;i++){
		offset = get_u16(&s->b);
	}

	array = g_array_sized_new(TRUE,TRUE,sizeof(SwfdecShape),
			n_glyphs);
	object->priv = array;
	object->type = SWF_OBJECT_FONT;
	g_array_set_size(array,n_glyphs);

	for(i=0;i<n_glyphs;i++){
		shape = &g_array_index(array,SwfdecShape,i);

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

static void get_kerning_record(bits_t *bits, int wide_codes)
{
	if(wide_codes){
		get_u16(bits);
		get_u16(bits);
	}else{
		get_u8(bits);
		get_u8(bits);
	}
	get_s16(bits);
}

int tag_func_define_font_2(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	int id;
	GArray *array;
	SwfdecShapeVec *shapevec;
	SwfdecShape *shape;
	SwfdecObject *object;
	int rect[4];

	int has_layout;
	int shift_jis;
	int reserved;
	int ansi;
	int wide_offsets;
	int wide_codes;
	int italic;
	int bold;
	int langcode;
	int font_name_len;
	int n_glyphs;
	int code_table_offset;
	int font_ascent;
	int font_descent;
	int font_leading;
	int kerning_count;
	int i;

	id = get_u16(bits);
	object = swfdec_object_new(s,id);

	has_layout = getbit(bits);
	shift_jis = getbit(bits);
	reserved = getbit(bits);
	ansi = getbit(bits);
	wide_offsets = getbit(bits);
	wide_codes = getbit(bits);
	italic = getbit(bits);
	bold = getbit(bits);

	langcode = get_u8(bits);

	font_name_len = get_u8(bits);
	//font_name = 
	bits->ptr += font_name_len;

	n_glyphs = get_u16(bits);
	if(wide_offsets){
		bits->ptr += 4*n_glyphs;
		code_table_offset = get_u32(bits);
	}else{
		bits->ptr += 2*n_glyphs;
		code_table_offset = get_u16(bits);
	}

	array = g_array_sized_new(TRUE,TRUE,sizeof(SwfdecShape),
			n_glyphs);
	object->priv = array;
	object->type = SWF_OBJECT_FONT;
	g_array_set_size(array,n_glyphs);

	for(i=0;i<n_glyphs;i++){
		shape = &g_array_index(array,SwfdecShape,i);

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
	if(wide_codes){
		bits->ptr += 2*n_glyphs;
	}else{
		bits->ptr += 1*n_glyphs;
	}
	if(has_layout){
		font_ascent = get_s16(bits);
		font_descent = get_s16(bits);
		font_leading = get_s16(bits);
		//font_advance_table = get_s16(bits);
		bits->ptr += 2*n_glyphs;
		//font_bounds = get_s16(bits);
		for(i=0;i<n_glyphs;i++){
			get_rect(bits,rect);
		}
		kerning_count = get_u16(bits);
		for(i=0;i<kerning_count;i++){
			get_kerning_record(bits,wide_codes);
		}
	}

	return SWF_OK;
}

static int define_text(SwfdecDecoder *s, int rgba)
{
	bits_t *bits = &s->b;
	int id;
	int rect[4];
	int n_glyph_bits;
	int n_advance_bits;
	SwfdecText *text = NULL;
	SwfdecTextGlyph glyph = { 0 };
	SwfdecObject *object;
	SwfdecText newtext = { 0 };
	GArray *array;

	id = get_u16(bits);
	object = swfdec_object_new(s,id);

	array = g_array_new(FALSE,TRUE,sizeof(SwfdecText));
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
				text = &g_array_index(array,SwfdecText,array->len-1);
				text->glyphs = g_array_new(FALSE,TRUE,
					sizeof(SwfdecTextGlyph));
			}
		}
		syncbits(bits);
	}
	get_u8(bits);

	return SWF_OK;
}

int tag_func_define_text(SwfdecDecoder *s)
{
	return define_text(s,0);
}

int tag_func_define_text_2(SwfdecDecoder *s)
{
	return define_text(s,1);
}



void swfdec_text_prerender(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object)
{
	int i,j;
	SwfdecText *text;
	SwfdecLayerVec *layervec;
	SwfdecShapeVec *shapevec;
	SwfdecShapeVec *shapevec2;
	SwfdecObject *fontobj;
	GArray *array;

	array = object->priv;
	for(i=0;i<array->len;i++){
		text = &g_array_index(array,SwfdecText,i);

		fontobj = swfdec_object_get(s,text->font);
		if(fontobj == NULL)continue;

		for(j=0;j<text->glyphs->len;j++){
			ArtVpath *vpath,*vpath0,*vpath1;
			ArtBpath *bpath0,*bpath1;
			SwfdecTextGlyph *glyph;
			SwfdecShape *shape;
			double trans[6];
			double pos[6];

			glyph = &g_array_index(text->glyphs,SwfdecTextGlyph,j);

			shape = &g_array_index((GArray *)fontobj->priv,
				SwfdecShape,glyph->glyph);
			art_affine_translate(pos,
				glyph->x * SWF_SCALE_FACTOR,
				glyph->y * SWF_SCALE_FACTOR);
			pos[0] = text->height * SWF_TEXT_SCALE_FACTOR;
			pos[3] = text->height * SWF_TEXT_SCALE_FACTOR;
			art_affine_multiply(trans,pos,object->trans);
			art_affine_multiply(trans,trans,layer->transform);
			art_affine_multiply(trans,trans,s->transform);

			layer->fills = g_array_set_size(layer->fills,layer->fills->len + 1);
			layervec = &g_array_index(layer->fills,SwfdecLayerVec,layer->fills->len - 1);

			shapevec = g_ptr_array_index(shape->fills,0);
			shapevec2 = g_ptr_array_index(shape->fills2,0);
			layervec->color = transform_color(
				text->color,
				layer->color_mult, layer->color_add);

			bpath0 = art_bpath_affine_transform(
				&g_array_index(shapevec->path,ArtBpath,0),
				trans);
			bpath1 = art_bpath_affine_transform(
				&g_array_index(shapevec2->path,ArtBpath,0),
				trans);
			vpath0 = art_bez_path_to_vec(bpath0,s->flatness);
			vpath1 = art_bez_path_to_vec(bpath1,s->flatness);
			if(art_affine_inverted(layer->transform)){
				vpath0 = art_vpath_reverse_free(vpath0);
			}else{
				vpath1 = art_vpath_reverse_free(vpath1);
			}
			vpath = art_vpath_cat(vpath0,vpath1);
			art_vpath_bbox_irect(vpath, &layervec->rect);
			layervec->svp = art_svp_from_vpath (vpath);
	
			art_free(bpath0);
			art_free(bpath1);
			art_free(vpath0);
			art_free(vpath1);
			art_free(vpath);
		}
	}
}

SwfdecLayer *swfdec_text_prerender_slow(SwfdecDecoder *s,SwfdecSpriteSeg *seg,
	SwfdecObject *object)
{
	int i,j;
	SwfdecText *text;
	SwfdecLayerVec *layervec;
	SwfdecShapeVec *shapevec;
	SwfdecShapeVec *shapevec2;
	SwfdecObject *fontobj;
	SwfdecLayer *layer;
	GArray *array;

	layer = swfdec_layer_new();
	layer->id = seg->id;
	art_affine_multiply(layer->transform,seg->transform,s->transform);

	array = object->priv;
	for(i=0;i<array->len;i++){
		text = &g_array_index(array,SwfdecText,i);

		fontobj = swfdec_object_get(s,text->font);
		if(fontobj == NULL)continue;

		for(j=0;j<text->glyphs->len;j++){
			ArtVpath *vpath,*vpath0,*vpath1;
			ArtBpath *bpath0,*bpath1;
			SwfdecTextGlyph *glyph;
			SwfdecShape *shape;
			double glyph_trans[6];
			double trans[6];
			double pos[6];

			glyph = &g_array_index(text->glyphs,SwfdecTextGlyph,j);

			shape = &g_array_index((GArray *)fontobj->priv,
				SwfdecShape,glyph->glyph);
			art_affine_translate(pos,
				glyph->x * SWF_SCALE_FACTOR,
				glyph->y * SWF_SCALE_FACTOR);
			pos[0] = text->height * SWF_TEXT_SCALE_FACTOR;
			pos[3] = text->height * SWF_TEXT_SCALE_FACTOR;
			art_affine_multiply(glyph_trans,pos,object->trans);
			art_affine_multiply(trans,glyph_trans,layer->transform);

			layer->fills = g_array_set_size(layer->fills,layer->fills->len + 1);
			layervec = &g_array_index(layer->fills,SwfdecLayerVec,layer->fills->len - 1);

			shapevec = g_ptr_array_index(shape->fills,0);
			shapevec2 = g_ptr_array_index(shape->fills2,0);
			layervec->color = transform_color(
				text->color,
				seg->color_mult, seg->color_add);

			bpath0 = art_bpath_affine_transform(
				&g_array_index(shapevec->path,ArtBpath,0),
				trans);
			bpath1 = art_bpath_affine_transform(
				&g_array_index(shapevec2->path,ArtBpath,0),
				trans);
			vpath0 = art_bez_path_to_vec(bpath0,s->flatness);
			vpath1 = art_bez_path_to_vec(bpath1,s->flatness);
			if(art_affine_inverted(trans)){
				vpath0 = art_vpath_reverse_free(vpath0);
			}else{
				vpath1 = art_vpath_reverse_free(vpath1);
			}
			vpath = art_vpath_cat(vpath0,vpath1);
			art_vpath_bbox_irect(vpath, &layervec->rect);
			layervec->svp = art_svp_from_vpath (vpath);
	
			art_free(bpath0);
			art_free(bpath1);
			art_free(vpath0);
			art_free(vpath1);
			art_free(vpath);
		}
	}

	return layer;
}

void swfdec_text_render(SwfdecDecoder *s,SwfdecLayer *layer,
	SwfdecObject *object)
{
	int i;
	SwfdecLayerVec *layervec;

	for(i=0;i<layer->fills->len;i++){
		layervec = &g_array_index(layer->fills, SwfdecLayerVec, i);
		swfdec_layervec_render(s, layervec);
	}
	for(i=0;i<layer->lines->len;i++){
		layervec = &g_array_index(layer->lines, SwfdecLayerVec, i);
		swfdec_layervec_render(s, layervec);
	}
}

