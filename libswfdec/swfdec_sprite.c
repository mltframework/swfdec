
#include "swfdec_internal.h"

void swfdec_sprite_prerender(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object)
{
	SwfdecLayer *l;
	GList *g;
	SwfdecDecoder *sprite = object->priv;

	art_affine_multiply(sprite->transform, layer->transform, s->transform);
	
	layer->frame_number %= sprite->n_frames;

	SWF_DEBUG(0,"swfdec_sprite_prerender %d frame %d\n",object->id,layer->frame_number);

	for(g=g_list_last(sprite->layers); g; g=g_list_previous(g)){
		l = (SwfdecLayer *)g->data;

		if(l->first_frame > layer->frame_number)continue;
		if(l->last_frame && l->last_frame <= layer->frame_number)continue;
		SWF_DEBUG(0,"prerendering layer %d\n",l->depth);

		swfdec_layer_prerender(sprite, l);
	}

	layer->prerendered = 0;
}

void swfdec_sprite_render(SwfdecDecoder *s, SwfdecLayer *parent_layer,
	SwfdecObject *parent_object)
{
	SwfdecLayer *layer;
	SwfdecDecoder *sprite = parent_object->priv;
	GList *g;
	SwfdecObject *object;
	double save_trans[6];

	SWF_DEBUG(0,"rendering sprite frame %d of %d\n",
		parent_layer->frame_number,sprite->n_frames);
	for(g=g_list_last(sprite->layers); g; g=g_list_previous(g)){
		layer = (SwfdecLayer *)g->data;

		if(layer->first_frame > parent_layer->frame_number)continue;
		if(layer->last_frame && layer->last_frame <=
			parent_layer->frame_number)continue;

		art_affine_copy(save_trans, layer->transform);
		art_affine_multiply(layer->transform, sprite->transform, layer->transform);
		swfdec_layer_prerender(s,layer);
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
			swfdec_sprite_render(s,layer,object);
			break;
		case SWF_OBJECT_TEXT:
			swfdec_text_render(s,layer,object);
			break;
		case SWF_OBJECT_SHAPE:
			swfdec_shape_render(s,layer,object);
			break;
		case SWF_OBJECT_BUTTON:
			swfdec_button_render(s,layer,object);
			break;
		default:
			SWF_DEBUG(4,"render_sprite: unknown object type %d\n",object->type);
			break;
		}
	}
}

int tag_func_define_sprite(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	int id;
	SwfdecObject *object;
	SwfdecDecoder *sprite;
	int ret;

	id = get_u16(bits);
	object = swfdec_object_new(s,id);

	SWF_DEBUG(0,"  ID: %d\n", object->id);

	sprite = swf_init();
	object->priv = sprite;
	object->type = SWF_OBJECT_SPRITE;

	sprite->n_frames = get_u16(bits);

	sprite->state = SWF_STATE_PARSETAG;
	sprite->no_render = 1;

	sprite->width = s->width;
	sprite->height = s->height;
	memcpy(sprite->transform, s->transform, sizeof(double)*6);

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

