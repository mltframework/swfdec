
#include "swfdec_internal.h"

void prerender_layer_button(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object)
{
	SwfdecShape *shape;
	SwfdecObject *obj;
	double save_trans[6];
	SwfdecShapeVec *shapevec;
	SwfdecLayerVec *layervec;
	SwfdecButton *buttons = object->priv;
	int i;

//printf("prerender_layer_button %d [%d,%d,%d]\n",object->id,
//	object->button[0].id, object->button[1].id, object->button[2].id);
	//art_affine_multiply(sprite->transform, layer->transform, s->transform);

	art_affine_copy(save_trans, layer->transform);
	art_affine_multiply(layer->transform, buttons[0].transform, layer->transform);
	if(buttons[0].id){
		obj = swfdec_object_get(s,buttons[0].id);
		if(!obj)return;

		switch(obj->type){
		case SWF_OBJECT_SHAPE:
			shape=obj->priv;
//			if(layer->prerendered)return;
//			layer->prerendered = 1;

			prerender_layer_shape(s,layer,shape);
			for(i=0;i<layer->fills->len;i++){
				shapevec = g_ptr_array_index(shape->fills,i);
				layervec = &g_array_index(layer->fills,SwfdecLayerVec,i);

				layervec->color = shapevec->color;
			}
			for(i=0;i<layer->lines->len;i++){
				shapevec = g_ptr_array_index(shape->lines,i);
				layervec = &g_array_index(layer->lines,SwfdecLayerVec,i);

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

int tag_func_define_button_2(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	int id;
	int flags;
	int offset;
	int condition;
	SwfdecObject *object;
	double trans[6];
	double color_add[4], color_mult[4];
	SwfdecButton *button;
	unsigned char *endptr;

	endptr = bits->ptr + s->tag_len;

	id = get_u16(bits);
	object = swfdec_object_new(s,id);

	button = g_new0(SwfdecButton,3);
	object->type = SWF_OBJECT_BUTTON;
	object->priv = button;

	SWF_DEBUG(0,"  ID: %d\n", object->id);

	flags = get_u8(bits);
	offset = get_u16(bits);

	SWF_DEBUG(0,"  flags = %d\n",flags);
	SWF_DEBUG(0,"  offset = %d\n",offset);

	while(peek_u8(bits)){
		int reserved;
		int hit_test;
		int down;
		int over;
		int up;
		int character;
		int layer;

		syncbits(bits);
		reserved = getbits(bits,4);
		hit_test = getbit(bits);
		down = getbit(bits);
		over = getbit(bits);
		up = getbit(bits);
		character = get_u16(bits);
		layer = get_u16(bits);

		SWF_DEBUG(0,"  reserved = %d\n",reserved);
		if(reserved){
			SWF_DEBUG(4,"reserved is supposed to be 0\n");
		}
		SWF_DEBUG(0,"  hit_test = %d\n",hit_test);
		SWF_DEBUG(0,"  down = %d\n",down);
		SWF_DEBUG(0,"  over = %d\n",over);
		SWF_DEBUG(0,"  up = %d\n",up);
		SWF_DEBUG(0,"  character = %d\n",character);
		SWF_DEBUG(0,"  layer = %d\n",layer);

		SWF_DEBUG(0,"bits->ptr %p\n",bits->ptr);

		//get_art_matrix(bits, trans);
		get_art_matrix(bits, trans);
		syncbits(bits);
		SWF_DEBUG(0,"bits->ptr %p\n",bits->ptr);
		get_art_color_transform(bits, color_mult, color_add);
		syncbits(bits);

		SWF_DEBUG(0,"bits->ptr %p\n",bits->ptr);

		if(up){
			button[0].id = character;
			art_affine_copy(button[0].transform,trans);
			memcpy(button[0].color_mult,color_mult,4*sizeof(double));
			memcpy(button[0].color_add,color_add,4*sizeof(double));
		}
		if(over){
			button[1].id = character;
			art_affine_copy(button[1].transform,trans);
			memcpy(button[1].color_mult,color_mult,4*sizeof(double));
			memcpy(button[1].color_add,color_add,4*sizeof(double));
		}
		if(down){
			button[2].id = character;
			art_affine_copy(button[2].transform,trans);
			memcpy(button[2].color_mult,color_mult,4*sizeof(double));
			memcpy(button[2].color_add,color_add,4*sizeof(double));
		}

	}
	get_u8(bits);

	while(offset!=0){
		offset = get_u16(bits);
		condition = get_u16(bits);

		SWF_DEBUG(0,"  offset = %d\n",offset);
		SWF_DEBUG(0,"  condition = 0x%04x\n",condition);

		get_actions(s,bits);
	}

	return SWF_OK;
}
