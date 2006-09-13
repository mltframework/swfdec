
#ifndef _SWFDEC_SPRITE_H_
#define _SWFDEC_SPRITE_H_

#include "swfdec_types.h"
#include <swfdec_object.h>

G_BEGIN_DECLS
//typedef struct _SwfdecSprite SwfdecSprite;
typedef struct _SwfdecSpriteClass SwfdecSpriteClass;
typedef struct _SwfdecExport SwfdecExport;

//typedef struct _SwfdecSpriteSegment SwfdecSpriteSegment;

#define CLIPEVENT_LOAD			0
#define CLIPEVENT_ENTERFRAME		1
#define CLIPEVENT_MAX			2

struct _SwfdecSpriteSegment
{
  char *name;
  int depth;
  int id;
  int first_index; /* index of first frame that includes us */
  int last_index; /* index of first frame that doesn't include us */
  int clip_depth;
  gboolean stopped;

  cairo_matrix_t transform;
  SwfdecColorTransform color_transform;

  int ran_load;
  SwfdecBuffer *clipevent[CLIPEVENT_MAX];

  int ratio;
};

struct _SwfdecExport {
  char *name;
  int id;
};


#define SWFDEC_TYPE_SPRITE                    (swfdec_sprite_get_type())
#define SWFDEC_IS_SPRITE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SPRITE))
#define SWFDEC_IS_SPRITE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SPRITE))
#define SWFDEC_SPRITE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SPRITE, SwfdecSprite))
#define SWFDEC_SPRITE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SPRITE, SwfdecSpriteClass))

struct _SwfdecSpriteFrame
{
  SwfdecBuffer *sound_chunk;
  SwfdecBuffer *action;
  SwfdecSoundChunk *sound_play;

  swf_color bg_color;
  GList *segments;
};

struct _SwfdecSprite
{
  SwfdecObject object;

  int n_frames;
  int parse_frame;

  SwfdecSpriteFrame *frames;
};

struct _SwfdecSpriteClass
{
  SwfdecObjectClass object_class;

};

GType swfdec_sprite_get_type (void);

void swfdec_sprite_decoder_free (SwfdecObject * object);
int tag_func_define_sprite (SwfdecDecoder * s);
SwfdecSpriteSegment *swfdec_sprite_get_seg (SwfdecSprite * sprite, int depth,
    int frame);
void swfdec_sprite_frame_add_seg (SwfdecSpriteFrame * frame,
    SwfdecSpriteSegment * segnew);
void swfdec_sprite_frame_remove_seg (SwfdecDecoder *s, SwfdecSpriteFrame * frame,
    int layer);
void swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite, SwfdecBuffer * chunk,
    int frame);
void swfdec_sprite_add_action (SwfdecSprite * sprite, SwfdecBuffer * action,
    int frame);
void swfdec_sprite_set_n_frames (SwfdecSprite *sprite, unsigned int n_frames);

SwfdecSpriteSegment *swfdec_spriteseg_new (void);
SwfdecSpriteSegment *swfdec_spriteseg_dup (SwfdecSpriteSegment * seg);
void swfdec_spriteseg_free (SwfdecSpriteSegment * seg);
int swfdec_spriteseg_place_object_2 (SwfdecDecoder * s);
int swfdec_spriteseg_remove_object (SwfdecDecoder * s);
int swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s);

void swf_render_frame (SwfdecDecoder * s, int frame_index);
void swfdec_spriteseg_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg);

int tag_func_export_assets (SwfdecDecoder * s);
SwfdecObject *swfdec_exports_lookup (SwfdecDecoder * s, char *name);

G_END_DECLS
#endif
