
#include "swfdec_internal.h"

SwfdecSprite *swfdec_sprite_new(void)
{
	SwfdecSprite *sprite;

	sprite = g_new0(SwfdecSprite,1);

	return sprite;
}

void swfdec_sprite_decoder_free(SwfdecObject *object)
{
	SwfdecDecoder *s = object->priv;

	swfdec_sprite_free(s->main_sprite);
	swfdec_render_free(s->render);

	g_free(s);
}

void swfdec_sprite_free(SwfdecSprite *sprite)
{
	GList *g;

	for(g=g_list_first(sprite->layers);g;g=g_list_next(g)){
		g_free(g->data);
	}
	g_list_free(sprite->layers);

	g_free(sprite);
}

SwfdecLayer *swfdec_sprite_prerender(SwfdecDecoder *s,SwfdecSpriteSeg *seg,
	SwfdecObject *object)
{
	SwfdecLayer *layer;
	SwfdecLayer *child_layer;
	GList *g;
	SwfdecDecoder *child_decoder = object->priv;
	SwfdecSprite *sprite = child_decoder->main_sprite;
	SwfdecSpriteSeg *tmpseg;
	SwfdecSpriteSeg *child_seg;
	int frame;

	layer = swfdec_layer_new();
	layer->id = seg->id;
	art_affine_multiply(layer->transform, seg->transform, s->transform);
	//swfdec_render_add_layer(s->render, layer);
	
	layer->frame_number = (s->frame_number - seg->first_frame) % sprite->n_frames;
	frame = layer->frame_number;

	SWF_DEBUG(0,"swfdec_sprite_prerender %d frame %d\n",object->id,layer->frame_number);

	for(g=g_list_last(sprite->layers); g; g=g_list_previous(g)){
		child_seg = (SwfdecSpriteSeg *)g->data;

		if(child_seg->first_frame > frame)continue;
		if(child_seg->last_frame <= frame)continue;
		SWF_DEBUG(0,"prerendering layer %d\n",child_seg->depth);

		tmpseg = swfdec_spriteseg_dup(child_seg);
		art_affine_multiply(tmpseg->transform,
			child_seg->transform, layer->transform);

		child_layer = swfdec_spriteseg_prerender(s, tmpseg);
		layer->sublayers = g_list_append(layer->sublayers, child_layer);

		swfdec_spriteseg_free(tmpseg);
	}

	return layer;
}

#if 0
void swfdec_sprite_render(SwfdecDecoder *s, SwfdecLayer *parent_layer,
	SwfdecObject *parent_object)
{
	SwfdecLayer *layer;
	SwfdecDecoder *s2 = parent_object->priv;
	SwfdecSprite *sprite = s2->main_sprite;
	GList *g;
	SwfdecObject *object;
	double save_trans[6];

	SWF_DEBUG(0,"rendering sprite frame %d of %d\n",
		parent_layer->frame_number,s2->n_frames);
	for(g=g_list_last(sprite->layers); g; g=g_list_previous(g)){
		layer = (SwfdecLayer *)g->data;

		if(layer->first_frame > parent_layer->frame_number)continue;
		if(layer->last_frame <= parent_layer->frame_number)continue;

		art_affine_copy(save_trans, layer->transform);
		art_affine_multiply(layer->transform, s2->transform, layer->transform);
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
#endif

void swfdec_sprite_render(SwfdecDecoder *s, SwfdecLayer *parent_layer,
	SwfdecObject *parent_object)
{
	SwfdecLayer *child_layer;
	SwfdecDecoder *s2 = parent_object->priv;
	GList *g;

	SWF_DEBUG(0,"rendering sprite frame %d of %d\n",
		parent_layer->frame_number,s2->n_frames);
	for(g=g_list_first(parent_layer->sublayers); g; g=g_list_next(g)){
		child_layer = (SwfdecLayer *)g->data;
		if(!child_layer)continue;
		swfdec_layer_render(s,child_layer);
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

	sprite = swfdec_decoder_new();
	object->priv = sprite;
	object->type = SWF_OBJECT_SPRITE;

	sprite->n_frames = get_u16(bits);
	sprite->main_sprite->n_frames = sprite->n_frames;

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

SwfdecSpriteSeg *swfdec_sprite_get_seg(SwfdecSprite *sprite, int depth, int frame)
{
	SwfdecSpriteSeg *seg;
	GList *g;

	for(g=g_list_first(sprite->layers); g; g=g_list_next(g)){
		seg = (SwfdecSpriteSeg *)g->data;
		if(seg->depth==depth && seg->first_frame <= frame
		  && seg->last_frame > frame)
			return seg;
	}

	return NULL;
}

void swfdec_sprite_add_seg(SwfdecSprite *sprite, SwfdecSpriteSeg *segnew)
{
	GList *g;
	SwfdecSpriteSeg *seg;

	for(g=g_list_first(sprite->layers); g; g=g_list_next(g)){
		seg = (SwfdecSpriteSeg *)g->data;
		if(seg->depth < segnew->depth){
			sprite->layers = g_list_insert_before(sprite->layers,
				g, segnew);
			return;
		}
	}

	sprite->layers = g_list_append(sprite->layers, segnew);
}

SwfdecSpriteSeg *swfdec_spriteseg_new(void)
{
	SwfdecSpriteSeg *seg;

	seg = g_new0(SwfdecSpriteSeg, 1);

	return seg;
}

SwfdecSpriteSeg *swfdec_spriteseg_dup(SwfdecSpriteSeg *seg)
{
	SwfdecSpriteSeg *newseg;

	newseg = g_new(SwfdecSpriteSeg,1);
	memcpy(newseg,seg,sizeof(*seg));

	return newseg;
}

void swfdec_spriteseg_free(SwfdecSpriteSeg *seg)
{
	g_free(seg);
}

int swfdec_spriteseg_place_object_2(SwfdecDecoder *s)
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

	oldlayer = swfdec_sprite_get_seg(s->parse_sprite,depth,
		s->parse_sprite->parse_frame);
	if(oldlayer){
		oldlayer->last_frame = s->parse_sprite->parse_frame;
	}

	layer = swfdec_spriteseg_new();

	layer->depth = depth;
	layer->first_frame = s->parse_sprite->parse_frame;
	layer->last_frame = s->parse_sprite->n_frames;

	swfdec_sprite_add_seg(s->parse_sprite,layer);

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

	return SWF_OK;
}

int swfdec_spriteseg_remove_object(SwfdecDecoder *s)
{
	int depth;
	SwfdecSpriteSeg *seg;
	int id;

	id = get_u16(&s->b);
	depth = get_u16(&s->b);
	seg = swfdec_sprite_get_seg(s->parse_sprite,depth,
		s->parse_sprite->parse_frame - 1);

	if(seg){
		seg->last_frame = s->parse_sprite->parse_frame;
	}

	return SWF_OK;
}

int swfdec_spriteseg_remove_object_2(SwfdecDecoder *s)
{
	int depth;
	SwfdecSpriteSeg *seg;

	depth = get_u16(&s->b);
	seg = swfdec_sprite_get_seg(s->parse_sprite,depth,
		s->parse_sprite->parse_frame - 1);

	if(seg){
		seg->last_frame = s->parse_sprite->parse_frame;
	}

	return SWF_OK;
}

