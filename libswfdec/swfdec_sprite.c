
#include "swfdec_internal.h"

void prerender_layer_sprite(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object)
{
	SwfdecLayer *l;
	GList *g;
	SwfdecDecoder *sprite = object->priv;

	art_affine_multiply(sprite->transform, layer->transform, s->transform);
	
	layer->frame_number %= sprite->n_frames;

	SWF_DEBUG(0,"prerender_layer_sprite %d frame %d\n",object->id,layer->frame_number);

	for(g=g_list_last(sprite->layers); g; g=g_list_previous(g)){
		l = (SwfdecLayer *)g->data;

		if(l->first_frame > layer->frame_number)continue;
		if(l->last_frame && l->last_frame <= layer->frame_number)continue;
		SWF_DEBUG(0,"prerendering layer %d\n",l->depth);

		swf_layer_prerender(sprite, l);
	}

	layer->prerendered = 0;
}

void render_sprite(SwfdecDecoder *s, SwfdecDecoder *sprite, int frame_number)
{
	SwfdecLayer *layer;
	int i;
	SwfdecLayerVec *layervec;
	GList *g;
	SwfdecObject *object;
	double save_trans[6];

	frame_number %= sprite->n_frames;

	SWF_DEBUG(0,"rendering sprite frame %d of %d\n",frame_number,sprite->n_frames);
	for(g=g_list_last(sprite->layers); g; g=g_list_previous(g)){
		layer = (SwfdecLayer *)g->data;

		if(layer->first_frame > frame_number)continue;
		if(layer->last_frame && layer->last_frame <= frame_number)continue;

art_affine_copy(save_trans, layer->transform);
art_affine_multiply(layer->transform, sprite->transform, layer->transform);
		swf_layer_prerender(s,layer);
art_affine_copy(layer->transform, save_trans);
		object = swfdec_object_get(s,layer->id);
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
				layervec = &g_array_index(layer->fills,SwfdecLayerVec,i);
				swf_layervec_render(s, layervec);
//printf("[%d %d %d %d]\n",layervec->rect.x0,layervec->rect.y0,layervec->rect.x1,layervec->rect.y1);
			}
			for(i=0;i<layer->lines->len;i++){
				layervec = &g_array_index(layer->lines,SwfdecLayerVec,i);
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

