
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
	SwfdecSpriteSeg *layer;
	SwfdecSpriteSeg *oldlayer;

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

	oldlayer = swfdec_sprite_get_seg(s->parse_sprite,depth,s->parse_sprite->parse_frame);
	if(oldlayer){
		oldlayer->last_frame = s->frame_number;
	}

	layer = swfdec_spriteseg_new();

	layer->depth = depth;
	layer->first_frame = s->frame_number;
	layer->last_frame = 0;

	swfdec_sprite_add_seg(s->main_sprite,layer);

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

	//swfdec_layer_prerender(s,layer);

	return SWF_OK;
}



int art_remove_object(SwfdecDecoder *s)
{
	int depth;
	SwfdecSpriteSeg *seg;
	int id;

	id = get_u16(&s->b);
	depth = get_u16(&s->b);
	seg = swfdec_sprite_get_seg(s->parse_sprite,depth,
		s->parse_sprite->parse_frame);

	seg->last_frame = s->parse_sprite->parse_frame;

	return SWF_OK;
}

int art_remove_object_2(SwfdecDecoder *s)
{
	int depth;
	SwfdecSpriteSeg *seg;

	depth = get_u16(&s->b);
	seg = swfdec_sprite_get_seg(s->parse_sprite,depth,
		s->parse_sprite->parse_frame);

	seg->last_frame = s->parse_sprite->parse_frame;

	return SWF_OK;
}

void swfdec_render_clean(SwfdecRender *render, int frame)
{
	SwfdecLayer *l;
	GList *g, *g_next;

	for(g=g_list_first(render->layers); g; g=g_next){
		g_next = g_list_next(g);
		l = (SwfdecLayer *)g->data;
		if(l->last_frame <= frame + 1){
			render->layers = g_list_delete_link(render->layers,g);
			swfdec_layer_free(l);
		}
	}
}


int art_show_frame(SwfdecDecoder *s)
{
	if(s->no_render){
		s->frame_number++;
		s->parse_sprite->parse_frame++;
		return SWF_OK;
	}

	s->pixels_rendered = 0;

	swf_config_colorspace(s);

	swf_render_frame(s);

	swfdec_render_clean(s->render, s->frame_number - 1);
	
	swfdec_sound_render(s);

	s->frame_number++;
	s->parse_sprite->parse_frame++;

	SWF_DEBUG(0,"pixels_rendered = %d\n",s->pixels_rendered);

	return SWF_IMAGE;
}

void swf_render_frame(SwfdecDecoder *s)
{
	SwfdecSpriteSeg *seg;
	SwfdecLayer *layer;
	SwfdecLayer *oldlayer;
	GList *g;
	int frame;

	SWF_DEBUG(0,"swf_render_frame\n");

	s->drawrect.x0 = 0;
	s->drawrect.x1 = 0;
	s->drawrect.y0 = 0;
	s->drawrect.y1 = 0;
	if(!s->buffer){
		s->buffer = art_new (art_u8, s->stride*s->height);
		swf_invalidate_irect(s,&s->irect);
	}
	if(!s->tmp_scanline){
		if(s->subpixel){
			s->tmp_scanline = malloc(s->width * 3);
		}else{
			s->tmp_scanline = malloc(s->width);
		}
	}

	frame = s->frame_number;
	SWF_DEBUG(1,"rendering frame %d\n",frame);
	for(g=g_list_last(s->main_sprite->layers); g; g=g_list_previous(g)){
		seg = (SwfdecSpriteSeg *)g->data;

		SWF_DEBUG(0,"testing seg %d <= %d < %d\n",
			seg->first_frame,frame,seg->last_frame);
		if(seg->first_frame > frame)continue;
		if(seg->last_frame <= frame)continue;

		oldlayer = swfdec_render_get_layer(s->render, seg->depth, frame - 1);

		SWF_DEBUG(0,"layer %d seg=%p oldlayer=%p\n",seg->depth, seg, oldlayer);

		layer = swfdec_spriteseg_prerender(s,seg,oldlayer);
		if(layer==NULL)continue;

		layer->last_frame = frame + 1;
		if(layer!=oldlayer){
			layer->first_frame = frame;
			swfdec_render_add_layer(s->render, layer);
			if(oldlayer) oldlayer->last_frame = frame;
		}else{
			SWF_DEBUG(0,"cache hit\n");
		}
	}

	for(g=g_list_last(s->render->layers); g; g=g_list_previous(g)){
		layer = (SwfdecLayer *)g->data;
		if(layer->seg->first_frame <= frame-1 &&
		   layer->last_frame == frame){
			SWF_DEBUG(0,"invalidating (%d < %d == %d) %d %d %d %d\n",
				layer->seg->first_frame, frame, layer->last_frame,
				layer->rect.x0,layer->rect.x1,
				layer->rect.y0,layer->rect.y1);
			swf_invalidate_irect(s,&layer->rect);
		}
		if(layer->first_frame == frame){
			swf_invalidate_irect(s,&layer->rect);
		}
	}
	
	if(s->disable_render)return;

	//art_irect_copy(&s->drawrect, &s->irect);
	SWF_DEBUG(0,"inval rect %d %d %d %d\n",s->drawrect.x0,s->drawrect.x1,
		s->drawrect.y0,s->drawrect.y1);

	switch(s->colorspace){
	case SWF_COLORSPACE_RGB565:
		art_rgb565_fillrect(s->buffer,s->stride,s->bg_color,&s->drawrect);
		break;
	case SWF_COLORSPACE_RGB888:
	default:
		art_rgb_fillrect(s->buffer,s->stride,s->bg_color,&s->drawrect);
		break;
	}

	for(g=g_list_last(s->render->layers); g; g=g_list_previous(g)){
		layer = (SwfdecLayer *)g->data;
		SWF_DEBUG(0,"rendering %d < %d <= %d\n",
			layer->seg->first_frame, frame,
			layer->last_frame);
		if(layer->seg->first_frame <= frame &&
		   frame < layer->last_frame){
			swfdec_layer_render(s,layer);
		}
	}
}

SwfdecLayer *swfdec_spriteseg_prerender(SwfdecDecoder *s, SwfdecSpriteSeg *seg,
	SwfdecLayer *oldlayer)
{
	SwfdecObject *object;

	object = swfdec_object_get(s,seg->id);
	if(!object)return NULL;

	switch(object->type){
	case SWF_OBJECT_SHAPE:
		return swfdec_shape_prerender(s,seg,object,oldlayer);
	case SWF_OBJECT_TEXT:
		return swfdec_text_prerender(s,seg,object,oldlayer);
	case SWF_OBJECT_BUTTON:
		return swfdec_button_prerender(s,seg,object,oldlayer);
	case SWF_OBJECT_SPRITE:
		return swfdec_sprite_prerender(s,seg,object,oldlayer);
	default:
		SWF_DEBUG(4,"unknown object trype\n");
	}

	return NULL;
}

void swfdec_layer_render(SwfdecDecoder *s, SwfdecLayer *layer)
{
	int i;
	SwfdecLayerVec *layervec;
	SwfdecLayer *child_layer;
	GList *g;

#if 0
	/* This rendering order seems to mostly fit the observed behavior
	 * of Macromedia's player.  */
	/* or not */
	for(i=0;i<MAX(layer->fills->len,layer->lines->len);i++){
		if(i<layer->lines->len){
			layervec = &g_array_index(layer->lines, SwfdecLayerVec, i);
			swfdec_layervec_render(s, layervec);
		}
		if(i<layer->fills->len){
			layervec = &g_array_index(layer->fills, SwfdecLayerVec, i);
			swfdec_layervec_render(s, layervec);
		}
	}
#else
	for(i=0;i<layer->fills->len;i++){
		layervec = &g_array_index(layer->fills, SwfdecLayerVec, i);
		swfdec_layervec_render(s, layervec);
	}
	for(i=0;i<layer->lines->len;i++){
		layervec = &g_array_index(layer->lines, SwfdecLayerVec, i);
		swfdec_layervec_render(s, layervec);
	}
#endif
	
	for(g=g_list_first(layer->sublayers);g;g=g_list_next(g)){
		child_layer = (SwfdecLayer *)g->data;
		swfdec_layer_render(s,child_layer);
	}
}

#if 0
void swfdec_layer_render(SwfdecDecoder *s, SwfdecLayer *layer)
{
	SwfdecObject *object;

	object = swfdec_object_get(s,layer->id);
	if(!object)return;

	switch(object->type){
	case SWF_OBJECT_SHAPE:
		swfdec_shape_render(s,layer,object);
		break;
	case SWF_OBJECT_TEXT:
		swfdec_text_render(s,layer,object);
		break;
	case SWF_OBJECT_BUTTON:
		swfdec_button_render(s,layer,object);
		break;
	case SWF_OBJECT_SPRITE:
		swfdec_sprite_render(s,layer,object);
		break;
	default:
		SWF_DEBUG(4,"unknown object trype\n");
	}
}
#endif
