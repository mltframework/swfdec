
#ifndef _SWFDEC_SPRITE_H_
#define _SWFDEC_SPRITE_H_

#include "swfdec_types.h"
#include <swfdec_object.h>
#include <glib-object.h>
#include "swfdec_transform.h"

G_BEGIN_DECLS

//typedef struct _SwfdecSprite SwfdecSprite;
typedef struct _SwfdecSpriteClass SwfdecSpriteClass;
//typedef struct _SwfdecSpriteSegment SwfdecSpriteSegment;

struct _SwfdecSpriteSegment
{
  int depth;
  int id;
  int first_frame;
  int last_frame;

  SwfdecTransform transform;
  SwfdecColorTransform color_transform;

  int ratio;
};


#define SWFDEC_TYPE_SPRITE                    (swfdec_sprite_get_type()) 
#define SWFDEC_IS_SPRITE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SPRITE))
#define SWFDEC_IS_SPRITE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SPRITE))
#define SWFDEC_SPRITE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SPRITE, SwfdecSprite))
#define SWFDEC_SPRITE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SPRITE, SwfdecSpriteClass))

struct _SwfdecSprite {
  SwfdecObject object;

  int n_frames;
  int parse_frame;

  GList *layers;

  SwfdecSoundChunk **sound_chunks;
};

struct _SwfdecSpriteClass {
  SwfdecObjectClass object_class;

};

GType swfdec_sprite_get_type (void);

void swfdec_sprite_decoder_free (SwfdecObject * object);
void swfdec_sprite_render (SwfdecDecoder * s, SwfdecLayer * parent_layer,
    SwfdecObject * parent_object);
int tag_func_define_sprite (SwfdecDecoder * s);
SwfdecSpriteSegment *swfdec_sprite_get_seg (SwfdecSprite * sprite, int depth,
    int frame);
void swfdec_sprite_add_seg (SwfdecSprite * sprite, SwfdecSpriteSegment * segnew);
void swfdec_sprite_add_sound_chunk (SwfdecSprite *sprite, SwfdecSoundChunk *chunk, int frame);

SwfdecSpriteSegment *swfdec_spriteseg_new (void);
SwfdecSpriteSegment *swfdec_spriteseg_dup (SwfdecSpriteSegment * seg);
void swfdec_spriteseg_free (SwfdecSpriteSegment * seg);
int swfdec_spriteseg_place_object_2 (SwfdecDecoder * s);
int swfdec_spriteseg_remove_object (SwfdecDecoder * s);
int swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s);

void swf_render_frame (SwfdecDecoder * s, int frame_index);
SwfdecLayer *swfdec_spriteseg_prerender (SwfdecDecoder * s,
    SwfdecSpriteSegment * seg, SwfdecLayer * oldlayer);
void swfdec_layer_render (SwfdecDecoder * s, SwfdecLayer * layer);



G_END_DECLS

#endif

