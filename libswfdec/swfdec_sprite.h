
#ifndef __SWFDEC_SPRITE_H__
#define __SWFDEC_SPRITE_H__

#include "swfdec_types.h"

struct swfdec_sprite_struct {
	int n_frames;
	int parse_frame;

	GList *layers;
};

struct swfdec_sprite_seg_struct {
	int depth;
	int id;
	int first_frame;
	int last_frame;

	double transform[6];
	double color_mult[4];
	double color_add[4];

	int ratio;
};

SwfdecSprite *swfdec_sprite_new(void);
void swfdec_sprite_free(SwfdecSprite *object);
void swfdec_sprite_decoder_free(SwfdecObject *object);
void swfdec_sprite_prerender(SwfdecDecoder *s,SwfdecLayer *layer,SwfdecObject *object);
SwfdecLayer *swfdec_sprite_prerender_slow(SwfdecDecoder *s,
	SwfdecSpriteSeg *layer, SwfdecObject *object);
void swfdec_sprite_render(SwfdecDecoder *s, SwfdecLayer *parent_layer,
	SwfdecObject *parent_object);
void swfdec_sprite_render_slow(SwfdecDecoder *s, SwfdecLayer *parent_layer,
	SwfdecObject *parent_object);
int tag_func_define_sprite(SwfdecDecoder *s);
SwfdecSpriteSeg *swfdec_sprite_get_seg(SwfdecSprite *sprite, int depth, int frame);
void swfdec_sprite_add_seg(SwfdecSprite *sprite, SwfdecSpriteSeg *segnew);

SwfdecSpriteSeg *swfdec_spriteseg_new(void);
SwfdecSpriteSeg *swfdec_spriteseg_dup(SwfdecSpriteSeg *seg);
void swfdec_spriteseg_free(SwfdecSpriteSeg *seg);
int swfdec_spriteseg_place_object_2(SwfdecDecoder *s);
int swfdec_spriteseg_remove_object(SwfdecDecoder *s);
int swfdec_spriteseg_remove_object_2(SwfdecDecoder *s);

void swf_render_frame_slow(SwfdecDecoder *s);
SwfdecLayer *swfdec_spriteseg_prerender(SwfdecDecoder *s, SwfdecSpriteSeg *seg);
void swfdec_layer_render(SwfdecDecoder *s, SwfdecLayer *layer);

#endif

