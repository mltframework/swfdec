
#include <math.h>
#include <libart_lgpl/libart.h>

#include "swfdec_internal.h"

static void swfdec_shape_compose(SwfdecDecoder *s, SwfdecLayerVec *layervec,
	SwfdecShapeVec *shapevec);

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

int tag_func_define_shape(SwfdecDecoder *s)
{
	bits_t *b = &s->b;
	int id;
	int n_fill_styles;
	int n_line_styles;
	int n_fill_bits;
	int n_line_bits;
	int rect[4];
	int i;

	id = get_u16(b);
	SWF_DEBUG(0,"  id = %d\n", id);
	printf("  bounds = %s\n","rect");
	get_rect(b,rect);
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

SwfdecShapeVec *swf_shape_vec_new(void)
{
	SwfdecShapeVec *shapevec;

	shapevec = g_new0(SwfdecShapeVec,1);

	shapevec->path = g_array_new(FALSE,TRUE,sizeof(ArtBpath));

	return shapevec;
}

int art_define_shape(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	SwfdecObject *object;
	SwfdecShape *shape;
	int rect[4];
	int id;

	id = get_u16(bits);
	object = swfdec_object_new(s,id);

	shape = g_new0(SwfdecShape,1);
	object->priv = shape;
	object->type = SWF_OBJECT_SHAPE;
	SWF_DEBUG(0,"  ID: %d\n", object->id);

	get_rect(bits,rect);

	shape->fills = g_ptr_array_new();
	shape->fills2 = g_ptr_array_new();
	shape->lines = g_ptr_array_new();

	swf_shape_add_styles(s,shape,bits);

	swf_shape_get_recs(s,bits,shape);

	return SWF_OK;
}

int art_define_shape_3(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	SwfdecObject *object;
	SwfdecShape *shape;
	int rect[4];
	int id;

	id = get_u16(bits);
	object = swfdec_object_new(s,id);

	shape = g_new0(SwfdecShape,1);
	object->priv = shape;
	object->type = SWF_OBJECT_SHAPE;
	SWF_DEBUG(0,"  ID: %d\n", object->id);

	get_rect(bits,rect);

	shape->fills = g_ptr_array_new();
	shape->fills2 = g_ptr_array_new();
	shape->lines = g_ptr_array_new();

	shape->rgba = 1;

	swf_shape_add_styles(s,shape,bits);

	swf_shape_get_recs(s,bits,shape);

	return SWF_OK;
}

void swf_shape_add_styles(SwfdecDecoder *s, SwfdecShape *shape, bits_t *bits)
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
		SwfdecShapeVec *shapevec;

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
			get_art_matrix(bits,shapevec->fill_matrix);
			if(shape->rgba){
				shapevec->color = get_gradient_rgba(bits);
			}else{
				shapevec->color = get_gradient(bits);
			}
		}
		if(fill_style_type == 0x40 || fill_style_type == 0x41){
			shapevec->fill_type = fill_style_type;
			shapevec->fill_id = get_u16(bits);
			SWF_DEBUG(4,"   background fill id = %d (type 0x%02x)\n",
				shapevec->fill_id, fill_style_type);

			if(shapevec->fill_id==65535){
				shapevec->fill_id = 0;
				shapevec->color = SWF_COLOR_COMBINE(0,255,255,255);
			}

			get_art_matrix(bits,shapevec->fill_matrix);
		}
	}

	syncbits(bits);
	shape->lines_offset = shape->lines->len;
	n_line_styles = get_u8(bits);
	SWF_DEBUG(0,"   n_line_styles %d\n",n_line_styles);
	for(i=0;i<n_line_styles;i++){
		SwfdecShapeVec *shapevec;

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

void swf_shape_get_recs(SwfdecDecoder *s, bits_t *bits, SwfdecShape *shape)
{
	int x = 0, y = 0;
	int fill0style = 0;
	int fill1style = 0;
	int linestyle = 0;
	int n_vec = 0;
	SwfdecShapeVec *shapevec;
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

int art_define_shape_2(SwfdecDecoder *s)
{
	return art_define_shape(s);
}

void dump_layers(SwfdecDecoder *s)
{
	GList *g;
	SwfdecLayer *layer;

	for(g=g_list_last(s->main_sprite->layers); g; g=g_list_previous(g)){
		layer = (SwfdecLayer *)g->data;

		printf("  layer %d [%d,%d)\n",layer->depth,
			layer->first_frame, layer->last_frame);
	}
}

void swfdec_shape_prerender(SwfdecDecoder *s,SwfdecLayer *layer,
	SwfdecObject *obj)
{
	SwfdecShape *shape = obj->priv;
	int i;
	SwfdecLayerVec *layervec;
	SwfdecShapeVec *shapevec;
	SwfdecShapeVec *shapevec2;
	double trans[6];

	g_array_set_size(layer->fills, shape->fills->len);
	for(i=0;i<shape->fills->len;i++){
		ArtVpath *vpath,*vpath0,*vpath1;
		ArtBpath *bpath0,*bpath1;

		layervec = &g_array_index(layer->fills,SwfdecLayerVec,i);
		shapevec = g_ptr_array_index(shape->fills,i);
		shapevec2 = g_ptr_array_index(shape->fills2,i);

		art_affine_multiply(trans,layer->transform,s->transform);

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

		layervec->color = transform_color(shapevec->color,
			layer->color_mult, layer->color_add);
		layervec->compose = NULL;
		if(shapevec->fill_id){
			swfdec_shape_compose(s, layervec, shapevec);
		}
	}

	g_array_set_size(layer->lines, shape->lines->len);
	for(i=0;i<shape->lines->len;i++){
		ArtVpath *vpath;
		ArtBpath *bpath;
		double width;
		int half_width;

		layervec = &g_array_index(layer->lines,SwfdecLayerVec,i);
		shapevec = g_ptr_array_index(shape->lines,i);

		art_affine_multiply(trans,layer->transform,s->transform);

		bpath = art_bpath_affine_transform(
			&g_array_index(shapevec->path,ArtBpath,0),
			trans);
		vpath = art_bez_path_to_vec(bpath,s->flatness);
		art_vpath_bbox_irect(vpath, &layervec->rect);

		width = shapevec->width*art_affine_expansion(trans);
		//printf("width=%g\n",width);
		if(width<1)width=1;

		half_width = floor(width*0.5) + 1;
		layervec->rect.x0 -= half_width;
		layervec->rect.y0 -= half_width;
		layervec->rect.x1 += half_width;
		layervec->rect.y1 += half_width;
		layervec->svp = art_svp_vpath_stroke (vpath,
			ART_PATH_STROKE_JOIN_MITER,
			ART_PATH_STROKE_CAP_BUTT,
			width, 1.0, s->flatness);

		art_free(vpath);
		art_free(bpath);
		layervec->color = transform_color(shapevec->color,
			layer->color_mult, layer->color_add);
	}

}

void swfdec_shape_render(SwfdecDecoder *s,SwfdecLayer *layer,
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

static void swfdec_shape_compose(SwfdecDecoder *s, SwfdecLayerVec *layervec,
	SwfdecShapeVec *shapevec)
{
	SwfdecObject *image_object;
	SwfdecImage *image;
	double mat[6];
	double mat0[6];
	int i, j;
	unsigned char *dest;

	image_object = swfdec_object_get(s, shapevec->fill_id);
	if(!image_object)return;

	SWF_DEBUG(4,"swfdec_shape_compose: %d\n", shapevec->fill_id);

	layervec->color = SWF_COLOR_COMBINE(255,0,0,255);

	image = image_object->priv;
	SWF_DEBUG(4,"image %p\n", image->image_data);

	SWF_DEBUG(4,"%g %g %g %g %g %g\n",
		shapevec->fill_matrix[0],
		shapevec->fill_matrix[1],
		shapevec->fill_matrix[2],
		shapevec->fill_matrix[3],
		shapevec->fill_matrix[4],
		shapevec->fill_matrix[5]);

	layervec->compose = g_malloc(s->width * s->height * 4);
	layervec->compose_rowstride = s->width * 4;
	layervec->compose_height = s->height;
	layervec->compose_width = s->width;
	
	for(i=0;i<4;i++){
		mat0[i] = shapevec->fill_matrix[i] / 20.0;
	}
	mat0[4] = shapevec->fill_matrix[4];
	mat0[5] = shapevec->fill_matrix[5];
	art_affine_invert(mat, mat0);
	dest = layervec->compose;
	for(j=0;j<s->height;j++){
	for(i=0;i<s->width;i++){
		int ix,iy;
		double x,y;

		x = mat[0]*i + mat[2]*j + mat[4];
		y = mat[1]*i + mat[3]*j + mat[5];

		ix = x - floor(x/image->width)*image->width;
		iy = y - floor(y/image->height)*image->height;
#if 0
		if(x<0)x=0;
		if(x>image->width-1)x=image->width-1;
		if(y<0)y=0;
		if(y>image->height-1)y=image->height-1;

		ix = x;
		iy = y;
#endif
		dest[0] = image->image_data[ix*4 + iy*image->rowstride + 0];
		dest[1] = image->image_data[ix*4 + iy*image->rowstride + 1];
		dest[2] = image->image_data[ix*4 + iy*image->rowstride + 2];
		dest[3] = 0;
		dest+=4;
	}
	}
}

