
#include <libart_lgpl/libart.h>
#include <math.h>

#include "swfdec_internal.h"


SwfdecRender *swfdec_render_new(void)
{
	SwfdecRender *render;

	render = g_new0(SwfdecRender, 1);

	return render;
}



SwfdecLayer *swfdec_layer_new(void)
{
	SwfdecLayer *layer;

	layer = g_new0(SwfdecLayer,1);

	layer->fills = g_array_new(FALSE,TRUE,sizeof(SwfdecLayerVec));
	layer->lines = g_array_new(FALSE,TRUE,sizeof(SwfdecLayerVec));

	return layer;
}

void swfdec_layer_free(SwfdecLayer *layer)
{
	int i;
	SwfdecLayerVec *layervec;

	if(!layer){
		g_warning("layer==NULL");
		return;
	}

	for(i=0;i<layer->fills->len;i++){
		layervec = &g_array_index(layer->fills,SwfdecLayerVec,i);
		art_svp_free(layervec->svp);
		if(layervec->compose)g_free(layervec->compose);
	}
	for(i=0;i<layer->lines->len;i++){
		layervec = &g_array_index(layer->lines,SwfdecLayerVec,i);
		art_svp_free(layervec->svp);
		if(layervec->compose)g_free(layervec->compose);
	}
	
	if(layer->sublayers){
		GList *g;
		for(g=g_list_first(layer->sublayers);g;g=g_list_next(g)){
			swfdec_layer_free((SwfdecLayer *)g->data);
		}
		g_list_free(layer->sublayers);
	}

	g_array_free(layer->fills,TRUE);
	g_array_free(layer->lines,TRUE);
	g_free(layer);
}

SwfdecLayer *swfdec_layer_get(SwfdecDecoder *s, int depth)
{
	SwfdecLayer *l;
	GList *g;

	for(g=g_list_first(s->render->layers); g; g=g_list_next(g)){
		l = (SwfdecLayer *)g->data;
		if(l->depth == depth && l->first_frame <= s->frame_number-1
		  && (!l->last_frame || l->last_frame > s->frame_number-1))
			return l;
	}

	return NULL;
}

void swfdec_sprite_add_layer(SwfdecSprite *sprite, SwfdecLayer *lnew)
{
	GList *g;
	SwfdecLayer *l;

	for(g=g_list_first(sprite->layers); g; g=g_list_next(g)){
		l = (SwfdecLayer *)g->data;
		if(l->depth < lnew->depth){
			sprite->layers = g_list_insert_before(sprite->layers,g,lnew);
			return;
		}
	}

	sprite->layers = g_list_append(sprite->layers,lnew);
}

void swfdec_sprite_delete_layer(SwfdecSprite *sprite, SwfdecLayer *layer)
{
	GList *g;
	SwfdecLayer *l;

	for(g=g_list_first(sprite->layers); g; g=g_list_next(g)){
		l = (SwfdecLayer *)g->data;
		if(l == layer){
			sprite->layers = g_list_delete_link(sprite->layers,g);
			swfdec_layer_free(l);
			return;
		}
	}
}

void swfdec_layer_prerender(SwfdecDecoder *s, SwfdecLayer *layer)
{
	SwfdecObject *object;
	SwfdecShape *shape;

	object = swfdec_object_get(s,layer->id);

	if(!object)return;

	switch(object->type){
	case SWF_OBJECT_SHAPE:
		shape = object->priv;

		if(layer->prerendered)return;
		layer->prerendered = 1;

		swfdec_shape_prerender(s,layer,object);
#if 0
		for(i=0;i<layer->fills->len;i++){
			shapevec = g_ptr_array_index(shape->fills,i);
			layervec = &g_array_index(layer->fills,SwfdecLayerVec,i);
	
			layervec->color = transform_color(shapevec->color,
				layer->color_mult, layer->color_add);
		}
		for(i=0;i<layer->lines->len;i++){
			shapevec = g_ptr_array_index(shape->lines,i);
			layervec = &g_array_index(layer->lines,SwfdecLayerVec,i);
	
			layervec->color = transform_color(shapevec->color,
				layer->color_mult, layer->color_add);
		}
#endif
		break;
	case SWF_OBJECT_TEXT:
		if(layer->prerendered)return;
		layer->prerendered = 1;

		swfdec_text_prerender(s,layer,object);
		break;
	case SWF_OBJECT_SPRITE:
		layer->frame_number = s->frame_number - layer->first_frame;
		swfdec_sprite_prerender(s,layer,object);
		break;
	case SWF_OBJECT_BUTTON:
		swfdec_button_prerender(s,layer,object);
		break;
	default:
		SWF_DEBUG(4,"unknown object type\n");
		break;
	}
}

void swfdec_layervec_render(SwfdecDecoder *s, SwfdecLayerVec *layervec)
{
	ArtIRect rect;
	struct swf_svp_render_struct cb_data;

	art_irect_intersect(&rect, &s->drawrect,
		&layervec->rect);
			
	if(art_irect_empty(&rect))return;

#if 0
	art_rgb_fillrect(s->buffer,s->width*3,layervec->color,
		&rect);
#endif
	cb_data.buf = s->buffer + rect.y0*s->stride + rect.x0*s->bytespp;
	cb_data.color = layervec->color;
	cb_data.rowstride = s->stride;
	cb_data.x0 = rect.x0;
	cb_data.x1 = rect.x1;
	cb_data.scanline = s->tmp_scanline;
	cb_data.compose = layervec->compose;
	cb_data.compose_rowstride = layervec->compose_rowstride;
	cb_data.compose_height = layervec->compose_height;
	cb_data.compose_y = rect.y0;
	cb_data.compose_width = layervec->compose_width;

	g_assert(rect.x1 > rect.x0);
	/* This assertion fails occasionally. */
	//g_assert(layervec->svp->n_segs > 0);
	g_assert(layervec->svp->n_segs >= 0);

	if(layervec->svp->n_segs > 0){
		art_svp_render_aa(layervec->svp, rect.x0, rect.y0,
			rect.x1, rect.y1,
			s->callback, &cb_data);
	}
}

