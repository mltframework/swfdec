
#ifndef _SWFDEC_SPRITE_H_
#define _SWFDEC_SPRITE_H_

#include <swfdec_types.h>
#include <swfdec_object.h>
#include <color.h>
#include <swfdec_event.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_SPRITE_ACTION_REMOVE_OBJECT,
  SWFDEC_SPRITE_ACTION_PLACE_OBJECT,
  SWFDEC_SPRITE_ACTION_GET_OBJECT,
  SWFDEC_SPRITE_ACTION_TRANSFORM,
  SWFDEC_SPRITE_ACTION_COLOR_TRANSFORM,
  SWFDEC_SPRITE_ACTION_RATIO,
  SWFDEC_SPRITE_ACTION_NAME,
  SWFDEC_SPRITE_ACTION_CLIP_DEPTH,
  SWFDEC_SPRITE_ACTION_BG_COLOR
} SwfdecSpriteActionType;

typedef struct _SwfdecSpriteClass SwfdecSpriteClass;
typedef struct _SwfdecExport SwfdecExport;
typedef union _SwfdecSpriteAction SwfdecSpriteAction;

union _SwfdecSpriteAction {
  SwfdecSpriteActionType  	type;
  struct {
    SwfdecSpriteActionType	type;
    unsigned int		value[2];
  }				uint;
  struct {
    SwfdecSpriteActionType	type;
    cairo_matrix_t		matrix;
  }				matrix;
  struct {
    SwfdecSpriteActionType	type;
    SwfdecColorTransform	transform;
  }				color;
  struct {
    SwfdecSpriteActionType	type;
    char *			string;
  }				string;
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
  SwfdecSoundChunk *sound_play;

  swf_color bg_color;
  GArray *actions;			/* actions to perform */
  SwfdecEventList *events;		/* events to perform */
  GSList *do_actions;			/* actions queued via DoAction */
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
void swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite, SwfdecBuffer * chunk,
    int frame);
void swfdec_sprite_set_n_frames (SwfdecSprite *sprite, unsigned int n_frames);
void swfdec_sprite_add_script (SwfdecSprite * sprite, int frame, JSScript *script);

int tag_func_set_background_color (SwfdecDecoder * s);
int swfdec_spriteseg_place_object_2 (SwfdecDecoder * s);
int swfdec_spriteseg_remove_object (SwfdecDecoder * s);
int swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s);

int tag_func_export_assets (SwfdecDecoder * s);
SwfdecObject *swfdec_exports_lookup (SwfdecDecoder * s, char *name);

G_END_DECLS
#endif
