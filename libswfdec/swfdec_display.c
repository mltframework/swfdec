
#include "swfdec_internal.h"

int tag_func_place_object_2(SwfdecDecoder *s)
{
	bits_t *bits = &s->b;
	int reserved;
	int has_unknown;
	int has_name;
	int has_ratio;
	int has_color_transform;
	int has_matrix;
	int has_character;
	int move;
	int depth;
	int character_id;
	int ratio;

	reserved = getbit(bits);
	has_unknown = getbit(bits);
	has_name = getbit(bits);
	has_ratio = getbit(bits);
	has_color_transform = getbit(bits);
	has_matrix = getbit(bits);
	has_character = getbit(bits);
	move = getbit(bits);
	depth = get_u16(bits);

	printf("  reserved = %d\n",reserved);
	printf("  depth = %d\n",depth);

	if(has_character){
		character_id = get_u16(bits);
		printf("  id = %d\n",character_id);
	}
	if(has_matrix){
		get_matrix(bits);
	}
	if(has_color_transform){
		get_color_transform(bits);
		syncbits(bits);
	}
	if(has_ratio){
		ratio = get_u16(bits);
		printf("  ratio = %d\n",ratio);
	}
	if(has_name){
		free(get_string(bits));
	}

	return SWF_OK;
}

int tag_func_remove_object(SwfdecDecoder *s)
{
	int id;
	int depth;

	id = get_u16(&s->b);
	depth = get_u16(&s->b);

	printf("  id = %d\n",id);
	printf("  depth = %d\n",depth);

	return SWF_OK;
}

int tag_func_remove_object_2(SwfdecDecoder *s)
{
	int depth;

	depth = get_u16(&s->b);

	printf("  depth = %d\n",depth);

	return SWF_OK;
}

