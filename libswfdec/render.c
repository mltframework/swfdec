#include <libart_lgpl/libart.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include "swf.h"
#include "proto.h"
#include "bits.h"

void render_sprite(swf_state_t *s, swf_state_t *sprite, int frame_number);

#define SCALE_FACTOR (1/65536.0)
#define COLOR_SCALE_FACTOR (1/256.0)

static const double flatness = 0.25;

static inline void art_affine_copy(double dst[6],const double src[6])
{
	memcpy(dst,src,sizeof(double)*6);
}

void swf_invalidate_irect(swf_state_t *s, ArtIRect *rect)
{
	if(art_irect_empty(&s->drawrect)){
		art_irect_intersect(&s->drawrect, &s->irect, rect);
	}else{
		ArtIRect tmp1, tmp2;

		art_irect_copy(&tmp1, &s->drawrect);
		art_irect_intersect(&tmp2, &s->irect, rect);
		art_irect_union(&s->drawrect, &tmp1, &tmp2);
	}
}

unsigned int transform_color(unsigned int in, double mult[4], double add[4])
{
	int r, g, b, a;

	r = SWF_COLOR_R(in);
	g = SWF_COLOR_G(in);
	b = SWF_COLOR_B(in);
	a = SWF_COLOR_A(in);

	//SWF_DEBUG(0,"in rgba %d,%d,%d,%d\n",r,g,b,a);

	r = rint((r*mult[0] + add[0]));
	g = rint((g*mult[1] + add[1]));
	b = rint((b*mult[2] + add[2]));
	a = rint((a*mult[3] + add[3]));

	if(r<0)r=0; if(r>255)r=255;
	if(g<0)g=0; if(g>255)g=255;
	if(b<0)b=0; if(b>255)b=255;
	if(a<0)a=0; if(a>255)a=255;

	//SWF_DEBUG(0,"out rgba %d,%d,%d,%d\n",r,g,b,a);

	return SWF_COLOR_COMBINE(r,g,b,a);
}

swf_object_t *swf_object_get(swf_state_t *s, int id)
{
	swf_object_t *object;
	GList *g;

	for(g=g_list_first(s->objects);g;g=g_list_next(g)){
		object = (swf_object_t *)g->data;
		if(object->id == id)return object;
	}
	SWF_DEBUG(2,"object not found (id==%d)\n",id);

	return NULL;
}

swf_layer_t *swf_layer_new(void)
{
	swf_layer_t *layer;

	layer = g_new0(swf_layer_t,1);

	layer->fills = g_array_new(FALSE,FALSE,sizeof(swf_layer_vec_t));
	layer->lines = g_array_new(FALSE,FALSE,sizeof(swf_layer_vec_t));

	return layer;
}

void swf_layer_free(swf_layer_t *layer)
{
	int i;
	swf_layer_vec_t *layervec;

	for(i=0;i<layer->fills->len;i++){
		layervec = &g_array_index(layer->fills,swf_layer_vec_t,i);
		art_svp_free(layervec->svp);
	}
	for(i=0;i<layer->lines->len;i++){
		layervec = &g_array_index(layer->lines,swf_layer_vec_t,i);
		art_svp_free(layervec->svp);
	}
	g_array_free(layer->fills,TRUE);
	g_array_free(layer->lines,TRUE);
}

swf_layer_t *swf_layer_get(swf_state_t *s, int depth)
{
	swf_layer_t *l;
	GList *g;

	for(g=g_list_first(s->layers); g; g=g_list_next(g)){
		l = (swf_layer_t *)g->data;
		if(l->depth == depth && l->first_frame <= s->frame_number-1
		  && (!l->last_frame || l->last_frame > s->frame_number-1))
			return l;
	}

	return NULL;
}

void swf_layer_add(swf_state_t *s, swf_layer_t *lnew)
{
	GList *g;
	swf_layer_t *l;

	for(g=g_list_first(s->layers); g; g=g_list_next(g)){
		l = (swf_layer_t *)g->data;
		if(l->depth < lnew->depth){
			s->layers = g_list_insert_before(s->layers,g,lnew);
			return;
		}
	}

	s->layers = g_list_append(s->layers,lnew);
}

void swf_layer_del(swf_state_t *s, swf_layer_t *layer)
{
	GList *g;
	swf_layer_t *l;

	for(g=g_list_first(s->layers); g; g=g_list_next(g)){
		l = (swf_layer_t *)g->data;
		if(l == layer){
			s->layers = g_list_delete_link(s->layers,g);
			swf_layer_free(l);
			return;
		}
	}
}

void prerender_layer_shape(swf_state_t *s,swf_layer_t *layer,swf_shape_t *shape);
void prerender_layer_text(swf_state_t *s,swf_layer_t *layer,swf_object_t *object);
void prerender_layer_sprite(swf_state_t *s,swf_layer_t *layer,swf_object_t *object);
void prerender_layer_button(swf_state_t *s,swf_layer_t *layer,swf_object_t *object);

int art_place_object_2(swf_state_t *s)
{
	bits_t *bits = &s->b;
	int reserved;
	int has_compose;
	int has_name;
	int has_ratio;
	int has_color_transform;
	int has_matrix;
	int has_character;
	int move;
	int depth;
	swf_layer_t *layer;
	swf_layer_t *oldlayer;

	reserved = getbit(bits);
	has_compose = getbit(bits);
	has_name = getbit(bits);
	has_ratio = getbit(bits);
	has_color_transform = getbit(bits);
	has_matrix = getbit(bits);
	has_character = getbit(bits);
	move = getbit(bits);
	depth = get_u16(bits);

	SWF_DEBUG(0,"  reserved = %d\n",reserved);
	if(reserved){
		SWF_DEBUG(4,"  reserved bits non-zero %d\n",reserved);
	}

	oldlayer = swf_layer_get(s,depth);
	if(oldlayer){
		oldlayer->last_frame = s->frame_number;
	}

	layer = swf_layer_new();

	layer->depth = depth;
	layer->first_frame = s->frame_number;
	layer->last_frame = 0;

	swf_layer_add(s,layer);

	if(has_character){
		layer->id = get_u16(bits);
		SWF_DEBUG(0,"  id = %d\n",layer->id);
	}else{
		if(oldlayer) layer->id = oldlayer->id;
	}
	if(has_matrix){
		get_art_matrix(bits,layer->transform);
	}else{
		if(oldlayer) art_affine_copy(layer->transform,oldlayer->transform);
	}
	if(has_color_transform){
		get_art_color_transform(bits, layer->color_mult, layer->color_add);
		syncbits(bits);
	}else{
		if(oldlayer){
			memcpy(layer->color_mult,
				oldlayer->color_mult,sizeof(double)*4);
			memcpy(layer->color_add,
				oldlayer->color_add,sizeof(double)*4);
		}else{
			layer->color_mult[0] = 1;
			layer->color_mult[1] = 1;
			layer->color_mult[2] = 1;
			layer->color_mult[3] = 1;
			layer->color_add[0] = 0;
			layer->color_add[1] = 0;
			layer->color_add[2] = 0;
			layer->color_add[3] = 0;
		}
	}
	if(has_ratio){
		layer->ratio = get_u16(bits);
		SWF_DEBUG(0,"  ratio = %d\n",layer->ratio);
	}else{
		if(oldlayer) layer->ratio = oldlayer->ratio;
	}
	if(has_name){
		free(get_string(bits));
	}
	if(has_compose){
		int id;
		id = get_u16(bits);
		SWF_DEBUG(4,"composing with %04x\n",id);
	}

	//swf_layer_prerender(s,layer);

	return SWF_OK;
}

void swf_layer_prerender(swf_state_t *s, swf_layer_t *layer)
{
	swf_object_t *object;
	swf_layer_vec_t *layervec;
	swf_shape_vec_t *shapevec;
	int i;
	swf_shape_t *shape;

	object = swf_object_get(s,layer->id);

	if(!object)return;

	switch(object->type){
	case SWF_OBJECT_SHAPE:
		shape = object->priv;

		if(layer->prerendered)return;
		layer->prerendered = 1;

		prerender_layer_shape(s,layer,shape);
		for(i=0;i<layer->fills->len;i++){
			shapevec = g_ptr_array_index(shape->fills,i);
			layervec = &g_array_index(layer->fills,swf_layer_vec_t,i);
	
			layervec->color = transform_color(shapevec->color,
				layer->color_mult, layer->color_add);
		}
		for(i=0;i<layer->lines->len;i++){
			shapevec = g_ptr_array_index(shape->lines,i);
			layervec = &g_array_index(layer->lines,swf_layer_vec_t,i);
	
			layervec->color = transform_color(shapevec->color,
				layer->color_mult, layer->color_add);
		}
		break;
	case SWF_OBJECT_TEXT:
		if(layer->prerendered)return;
		layer->prerendered = 1;

		prerender_layer_text(s,layer,object);
		break;
	case SWF_OBJECT_SPRITE:
		layer->frame_number = s->frame_number - layer->first_frame;
		prerender_layer_sprite(s,layer,object);
		break;
	case SWF_OBJECT_BUTTON:
		prerender_layer_button(s,layer,object);
		break;
	default:
		SWF_DEBUG(4,"unknown object type\n");
		break;
	}
}

void prerender_layer_text(swf_state_t *s,swf_layer_t *layer,swf_object_t *object)
{
	int i,j;
	swf_text_t *text;
	swf_layer_vec_t *layervec;
	swf_shape_vec_t *shapevec;
	swf_shape_vec_t *shapevec2;
	swf_object_t *fontobj;
	GArray *array;

	array = object->priv;
	for(i=0;i<array->len;i++){
		text = &g_array_index(array,swf_text_t,i);

		fontobj = swf_object_get(s,text->font);
		if(fontobj == NULL)continue;

		for(j=0;j<text->glyphs->len;j++){
			ArtVpath *vpath,*vpath0,*vpath1;
			ArtBpath *bpath0,*bpath1;
			swf_text_glyph_t *glyph;
			swf_shape_t *shape;
			double trans[6];
			double pos[6];

			glyph = &g_array_index(text->glyphs,swf_text_glyph_t,j);

			shape = &g_array_index((GArray *)fontobj->priv,
				swf_shape_t,glyph->glyph);
			art_affine_translate(pos,
				glyph->x * SWF_SCALE_FACTOR,
				glyph->y * SWF_SCALE_FACTOR);
			pos[0] = text->height*0.001;
			pos[3] = text->height*0.001;
			art_affine_multiply(trans,pos,object->trans);
			art_affine_multiply(trans,trans,layer->transform);
			art_affine_multiply(trans,trans,s->transform);

			layer->fills = g_array_set_size(layer->fills,layer->fills->len + 1);
			layervec = &g_array_index(layer->fills,swf_layer_vec_t,layer->fills->len - 1);

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
			vpath0 = art_bez_path_to_vec(bpath0,flatness);
			vpath1 = art_bez_path_to_vec(bpath1,flatness);
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

void prerender_layer_shape(swf_state_t *s,swf_layer_t *layer,swf_shape_t *shape)
{
	int i;
	swf_layer_vec_t *layervec;
	swf_shape_vec_t *shapevec;
	swf_shape_vec_t *shapevec2;
	double trans[6];

	g_array_set_size(layer->fills, shape->fills->len);
	for(i=0;i<shape->fills->len;i++){
		ArtVpath *vpath,*vpath0,*vpath1;
		ArtBpath *bpath0,*bpath1;

		layervec = &g_array_index(layer->fills,swf_layer_vec_t,i);
		shapevec = g_ptr_array_index(shape->fills,i);
		shapevec2 = g_ptr_array_index(shape->fills2,i);

		art_affine_multiply(trans,layer->transform,s->transform);

		bpath0 = art_bpath_affine_transform(
			&g_array_index(shapevec->path,ArtBpath,0),
			trans);
		bpath1 = art_bpath_affine_transform(
			&g_array_index(shapevec2->path,ArtBpath,0),
			trans);
		vpath0 = art_bez_path_to_vec(bpath0,flatness);
		vpath1 = art_bez_path_to_vec(bpath1,flatness);
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

	g_array_set_size(layer->lines, shape->lines->len);
	for(i=0;i<shape->lines->len;i++){
		ArtVpath *vpath;
		ArtBpath *bpath;
		double width;
		int half_width;

		layervec = &g_array_index(layer->lines,swf_layer_vec_t,i);
		shapevec = g_ptr_array_index(shape->lines,i);

		art_affine_multiply(trans,layer->transform,s->transform);

		bpath = art_bpath_affine_transform(
			&g_array_index(shapevec->path,ArtBpath,0),
			trans);
		vpath = art_bez_path_to_vec(bpath,flatness);
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
			width, 1.0, flatness);

		art_free(vpath);
		art_free(bpath);
	}

}

void prerender_layer_sprite(swf_state_t *s,swf_layer_t *layer,swf_object_t *object)
{
	swf_layer_t *l;
	GList *g;
	swf_state_t *sprite = object->priv;

	art_affine_multiply(sprite->transform, layer->transform, s->transform);
	
	layer->frame_number %= sprite->n_frames;

	SWF_DEBUG(0,"prerender_layer_sprite %d frame %d\n",object->id,layer->frame_number);

	for(g=g_list_last(sprite->layers); g; g=g_list_previous(g)){
		l = (swf_layer_t *)g->data;

		if(l->first_frame > layer->frame_number)continue;
		if(l->last_frame && l->last_frame <= layer->frame_number)continue;
		SWF_DEBUG(0,"prerendering layer %d\n",l->depth);

		swf_layer_prerender(sprite, l);
	}

	layer->prerendered = 0;
}

void prerender_layer_button(swf_state_t *s,swf_layer_t *layer,swf_object_t *object)
{
	swf_shape_t *shape;
	swf_object_t *obj;
	double save_trans[6];
	swf_shape_vec_t *shapevec;
	swf_layer_vec_t *layervec;
	swf_button_t *buttons = object->priv;
	int i;

//printf("prerender_layer_button %d [%d,%d,%d]\n",object->id,
//	object->button[0].id, object->button[1].id, object->button[2].id);
	//art_affine_multiply(sprite->transform, layer->transform, s->transform);

	art_affine_copy(save_trans, layer->transform);
	art_affine_multiply(layer->transform, buttons[0].transform, layer->transform);
	if(buttons[0].id){
		obj = swf_object_get(s,buttons[0].id);
		if(!obj)return;

		switch(obj->type){
		case SWF_OBJECT_SHAPE:
			shape=obj->priv;
//			if(layer->prerendered)return;
//			layer->prerendered = 1;

			prerender_layer_shape(s,layer,shape);
			for(i=0;i<layer->fills->len;i++){
				shapevec = g_ptr_array_index(shape->fills,i);
				layervec = &g_array_index(layer->fills,swf_layer_vec_t,i);

				layervec->color = shapevec->color;
			}
			for(i=0;i<layer->lines->len;i++){
				shapevec = g_ptr_array_index(shape->lines,i);
				layervec = &g_array_index(layer->lines,swf_layer_vec_t,i);

				layervec->color = shapevec->color;
			}

			break;
		case SWF_OBJECT_TEXT:
			prerender_layer_text(s,layer,obj);
			break;
		case SWF_OBJECT_SPRITE:
			//printf("sprite\n");
			//layer->type = 1;
			layer->frame_number = s->frame_number - layer->first_frame;
			prerender_layer_sprite(s,layer,obj);
			break;
		default:
			SWF_DEBUG(4,"prerender_layer_button: object type not handled %d\n",obj->type);
			break;
		}
	}
	art_affine_copy(layer->transform, save_trans);
}

int art_remove_object(swf_state_t *s)
{
	int depth;
	//swf_layer_vec_t *layervec;
	swf_layer_t *layer;
	//int i;
	int id;

	id = get_u16(&s->b);
	depth = get_u16(&s->b);
	layer = swf_layer_get(s,depth);

	layer->last_frame = s->frame_number;

	return SWF_OK;
}

int art_remove_object_2(swf_state_t *s)
{
	int depth;
	//swf_layer_vec_t *layervec;
	swf_layer_t *layer;
	//int i;

	depth = get_u16(&s->b);
	layer = swf_layer_get(s,depth);

	layer->last_frame = s->frame_number;

	return SWF_OK;
}

static inline void swf_layervec_render(swf_state_t *s, swf_layer_vec_t *layervec)
{
	ArtIRect rect;
	struct swf_svp_render_struct cb_data;

	art_irect_intersect(&rect, &s->drawrect,
		&layervec->rect);
			
	//if(art_irect_empty(&rect))return;

#if 0
	art_rgb_fillrect(s->buffer,s->width*3,layervec->color,
		&rect);
#endif
	cb_data.buf = s->buffer + rect.y0*s->stride + rect.x0*s->bytespp;
	cb_data.color = layervec->color;
	cb_data.rowstride = s->stride;
	cb_data.x0 = rect.x0;
	cb_data.x1 = rect.x1;
	art_svp_render_aa(layervec->svp, rect.x0, rect.y0,
		rect.x1, rect.y1,
		s->callback, &cb_data);
}

void swf_config_colorspace(swf_state_t *s)
{
	switch(s->colorspace){
	case SWF_COLORSPACE_RGB565:
		s->stride = s->width * 2;
		s->bytespp = 2;
		s->callback = art_rgb565_svp_alpha_callback;
		break;
	case SWF_COLORSPACE_RGB888:
	default:
		s->stride = s->width * 3;
		s->bytespp = 3;
		s->callback = art_rgb_svp_alpha_callback;
		break;
	}
}

void swf_clean(swf_state_t *s, int frame)
{
	swf_layer_t *l;
	GList *g, *g_next;

	for(g=g_list_first(s->layers); g; g=g_next){
		g_next = g_list_next(g);
		l = (swf_layer_t *)g->data;
		if(l->last_frame && l->last_frame<=frame){
			s->layers = g_list_delete_link(s->layers,g);
			swf_layer_free(l);
		}
	}
}


int art_show_frame(swf_state_t *s)
{
	if(s->no_render){
		s->frame_number++;
		return SWF_OK;
	}

	swf_config_colorspace(s);

	swf_render_frame(s);

	swf_clean(s,s->frame_number);

	s->frame_number++;

	return SWF_IMAGE;
}

void swf_render_frame(swf_state_t *s)
{
	swf_layer_t *layer;
	int i;
	swf_layer_vec_t *layervec;
	GList *g;
	swf_object_t *object;

	SWF_DEBUG(0,"swf_render_frame\n");

	if(!s->buffer){
		s->buffer = art_new (art_u8, s->stride*s->height);
	}

	if(!s->sound_buffer){
		s->sound_len = 4*2*44100;
		s->sound_buffer = malloc(s->sound_len);
		s->sound_offset = 0;

		memset(s->sound_buffer,0,s->sound_len);
	}

	s->drawrect.x0 = 0;
	s->drawrect.y0 = 0;
	s->drawrect.x1 = 0;
	s->drawrect.y1 = 0;

#if 0
	for(g=g_list_last(s->layers); g; g=g_list_previous(g)){
		layer = (swf_layer_t *)g->data;
		if(layer->last_frame != s->frame_number &&
		   layer->first_frame != s->frame_number)continue;

		//swf_layer_prerender(s,layer);

		SWF_DEBUG(0,"clearing layer %d [%d,%d)\n",layer->depth,
			layer->first_frame,layer->last_frame);
		
		for(i=0;i<layer->fills->len;i++){
			layervec = &g_array_index(layer->fills,swf_layer_vec_t,i);
			swf_invalidate_irect(s,&layervec->rect);
		}
		for(i=0;i<layer->lines->len;i++){
			layervec = &g_array_index(layer->lines,swf_layer_vec_t,i);
			swf_invalidate_irect(s,&layervec->rect);
		}
	}

	switch(s->colorspace){
	case SWF_COLORSPACE_RGB565:
		art_rgb565_fillrect(s->buffer,s->stride,s->bg_color,&s->drawrect);
		break;
	case SWF_COLORSPACE_RGB888:
	default:
		art_rgb_fillrect(s->buffer,s->stride,s->bg_color,&s->drawrect);
		break;
	}
#else
	s->drawrect = s->irect;
#endif

	switch(s->colorspace){
	case SWF_COLORSPACE_RGB565:
		art_rgb565_fillrect(s->buffer,s->stride,s->bg_color,&s->drawrect);
		break;
	case SWF_COLORSPACE_RGB888:
	default:
		art_rgb_fillrect(s->buffer,s->stride,s->bg_color,&s->drawrect);
		break;
	}

	for(g=g_list_last(s->layers); g; g=g_list_previous(g)){
		layer = (swf_layer_t *)g->data;

		if(layer->first_frame > s->frame_number)continue;
		if(layer->last_frame && layer->last_frame <= s->frame_number)continue;

		swf_layer_prerender(s,layer);

		object = swf_object_get(s,layer->id);
		if(!object){
			SWF_DEBUG(4,"lost object\n");
			continue;
		}

		SWF_DEBUG(0,"rendering layer %d (id = %d, type = %d)\n",layer->depth,layer->id,object->type);

		switch(object->type){
		case SWF_OBJECT_SPRITE:
			layer->frame_number = s->frame_number - layer->first_frame;
			render_sprite(s,object->priv,layer->frame_number);
			break;
		case SWF_OBJECT_TEXT:
		case SWF_OBJECT_SHAPE:
		case SWF_OBJECT_BUTTON:
			for(i=0;i<layer->fills->len;i++){
				layervec = &g_array_index(layer->fills,swf_layer_vec_t,i);
				swf_layervec_render(s, layervec);
			}
			for(i=0;i<layer->lines->len;i++){
				layervec = &g_array_index(layer->lines,swf_layer_vec_t,i);
				swf_layervec_render(s, layervec);
			}
			break;
		default:
			SWF_DEBUG(4,"swf_render_frame: unknown object type %d\n",object->type);
			break;
		}
	}
}

void render_sprite(swf_state_t *s, swf_state_t *sprite, int frame_number)
{
	swf_layer_t *layer;
	int i;
	swf_layer_vec_t *layervec;
	GList *g;
	swf_object_t *object;
	double save_trans[6];

	frame_number %= sprite->n_frames;

	SWF_DEBUG(0,"rendering sprite frame %d of %d\n",frame_number,sprite->n_frames);
	for(g=g_list_last(sprite->layers); g; g=g_list_previous(g)){
		layer = (swf_layer_t *)g->data;

		if(layer->first_frame > frame_number)continue;
		if(layer->last_frame && layer->last_frame <= frame_number)continue;

art_affine_copy(save_trans, layer->transform);
art_affine_multiply(layer->transform, sprite->transform, layer->transform);
		swf_layer_prerender(s,layer);
art_affine_copy(layer->transform, save_trans);
		object = swf_object_get(s,layer->id);
		if(!object){
			/* WTF! */
			SWF_DEBUG(4,"lost object!\n");
			continue;
		}

		SWF_DEBUG(0,"rendering layer %d (id = %d, type = %d)\n",layer->depth,layer->id,object->type);

		switch(object->type){
		case SWF_OBJECT_SPRITE:
			render_sprite(s,object->priv, layer->frame_number);
			break;
		case SWF_OBJECT_TEXT:
		case SWF_OBJECT_SHAPE:
		case SWF_OBJECT_BUTTON:
			SWF_DEBUG(0,"fills = %d, lines = %d\n",
				layer->fills->len,layer->lines->len);
			for(i=0;i<layer->fills->len;i++){
				layervec = &g_array_index(layer->fills,swf_layer_vec_t,i);
				swf_layervec_render(s, layervec);
//printf("[%d %d %d %d]\n",layervec->rect.x0,layervec->rect.y0,layervec->rect.x1,layervec->rect.y1);
			}
			for(i=0;i<layer->lines->len;i++){
				layervec = &g_array_index(layer->lines,swf_layer_vec_t,i);
				swf_layervec_render(s, layervec);
//printf("[%d %d %d %d]\n",layervec->rect.x0,layervec->rect.y0,layervec->rect.x1,layervec->rect.y1);
			}
			break;
		default:
			SWF_DEBUG(4,"render_sprite: unknown object type %d\n",object->type);
			break;
		}
	}
}

