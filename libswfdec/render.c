
#include <libart_lgpl/libart.h>
#include <math.h>

#include "swfdec_internal.h"

void swf_invalidate_irect(SwfdecDecoder *s, ArtIRect *rect)
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

int art_place_object_2(SwfdecDecoder *s)
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
	SwfdecLayer *layer;
	SwfdecLayer *oldlayer;

	reserved = getbit(bits);
	has_compose = getbit(bits);
	has_name = getbit(bits);
	has_ratio = getbit(bits);
	has_color_transform = getbit(bits);
	has_matrix = getbit(bits);
	has_character = getbit(bits);
	move = getbit(bits);
	depth = get_u16(bits);

	/* reserved is somehow related to sprites */
	SWF_DEBUG(0,"  reserved = %d\n",reserved);
	if(reserved){
		SWF_DEBUG(4,"  reserved bits non-zero %d\n",reserved);
	}
	SWF_DEBUG(0,"  has_compose = %d\n",has_compose);
	SWF_DEBUG(0,"  has_name = %d\n",has_name);
	SWF_DEBUG(0,"  has_ratio = %d\n",has_ratio);
	SWF_DEBUG(0,"  has_color_transform = %d\n",has_color_transform);
	SWF_DEBUG(0,"  has_matrix = %d\n",has_matrix);
	SWF_DEBUG(0,"  has_character = %d\n",has_character);

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



int art_remove_object(SwfdecDecoder *s)
{
	int depth;
	//SwfdecLayerVec *layervec;
	SwfdecLayer *layer;
	//int i;
	int id;

	id = get_u16(&s->b);
	depth = get_u16(&s->b);
	layer = swf_layer_get(s,depth);

	layer->last_frame = s->frame_number;

	return SWF_OK;
}

int art_remove_object_2(SwfdecDecoder *s)
{
	int depth;
	//SwfdecLayerVec *layervec;
	SwfdecLayer *layer;
	//int i;

	depth = get_u16(&s->b);
	layer = swf_layer_get(s,depth);

	layer->last_frame = s->frame_number;

	return SWF_OK;
}

void swf_clean(SwfdecDecoder *s, int frame)
{
	SwfdecLayer *l;
	GList *g, *g_next;

	for(g=g_list_first(s->layers); g; g=g_next){
		g_next = g_list_next(g);
		l = (SwfdecLayer *)g->data;
		if(l->last_frame && l->last_frame<=frame){
			s->layers = g_list_delete_link(s->layers,g);
			swf_layer_free(l);
		}
	}
}


int art_show_frame(SwfdecDecoder *s)
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

void swf_render_frame(SwfdecDecoder *s)
{
	SwfdecLayer *layer;
	int i;
	SwfdecLayerVec *layervec;
	GList *g;
	SwfdecObject *object;

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
		layer = (SwfdecLayer *)g->data;
		if(layer->last_frame != s->frame_number &&
		   layer->first_frame != s->frame_number)continue;

		//swf_layer_prerender(s,layer);

		SWF_DEBUG(0,"clearing layer %d [%d,%d)\n",layer->depth,
			layer->first_frame,layer->last_frame);
		
		for(i=0;i<layer->fills->len;i++){
			layervec = &g_array_index(layer->fills,SwfdecLayerVec,i);
			swf_invalidate_irect(s,&layervec->rect);
		}
		for(i=0;i<layer->lines->len;i++){
			layervec = &g_array_index(layer->lines,SwfdecLayerVec,i);
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
		layer = (SwfdecLayer *)g->data;

		if(layer->first_frame > s->frame_number)continue;
		if(layer->last_frame && layer->last_frame <= s->frame_number)continue;

		swf_layer_prerender(s,layer);

		object = swfdec_object_get(s,layer->id);
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
				layervec = &g_array_index(layer->fills,SwfdecLayerVec,i);
				swf_layervec_render(s, layervec);
			}
			for(i=0;i<layer->lines->len;i++){
				layervec = &g_array_index(layer->lines,SwfdecLayerVec,i);
				swf_layervec_render(s, layervec);
			}
			break;
		case SWF_OBJECT_IMAGE:
			swfdec_image_render(s, layer);
			break;
		default:
			SWF_DEBUG(4,"swf_render_frame: unknown object type %d\n",object->type);
			break;
		}
	}
}

