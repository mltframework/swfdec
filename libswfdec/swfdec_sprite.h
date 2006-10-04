
#ifndef _SWFDEC_SPRITE_H_
#define _SWFDEC_SPRITE_H_

#include <swfdec_types.h>
#include <swfdec_object.h>
#include <color.h>
#include <swfdec_event.h>

G_BEGIN_DECLS

typedef struct _SwfdecSpriteClass SwfdecSpriteClass;
typedef struct _SwfdecSpriteAction SwfdecSpriteAction;
typedef struct _SwfdecExport SwfdecExport;

struct _SwfdecSpriteContent {
  SwfdecObject *	object;		/* object to display or NULL (FIXME: wanna allow NULL here?) */
  unsigned int	      	depth;		/* at which depth to display */
  unsigned int		clip_depth;	/* clip depth of object */
  unsigned int		ratio;
  cairo_matrix_t	transform;
  SwfdecColorTransform	color_transform;
  char *		name;
  SwfdecEventList *	events;

  SwfdecSpriteContent *	sequence;	/* first element in sequence this content belongs to */
  /* NB: the next two elements are only filled for the sequence leader */
  guint			start;		/* first frame that contains this sequence */
  guint			end;		/* first frame that does not contain this sequence anymore */
};

typedef enum {
  SWFDEC_SPRITE_ACTION_SCRIPT,		/* contains an action only */
  SWFDEC_SPRITE_ACTION_ADD,		/* contains a SwfdecSpriteContent */
  SWFDEC_SPRITE_ACTION_REMOVE,		/* contains a depth */
  SWFDEC_SPRITE_ACTION_UPDATE		/* contains a SwfdecSpriteContent */
} SwfdecSpriteActionType;

struct _SwfdecSpriteAction {
  SwfdecSpriteActionType	type;
  gpointer			data;
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
  char *name;				/* name of the frame for "GotoFrameLabel" */

  /* sound */
  SwfdecSound *sound_head;		/* sound head for this frame */
  int sound_skip;			/* samples to skip - maybe even backwards */
  SwfdecBuffer *sound_block;		/* sound chunk to play here or NULL for none */
  GSList *sound;			/* list of SwfdecSoundChunk events to start playing here */

  /* visuals */
  swf_color bg_color;
  GArray *actions;			/* SwfdecSpriteAction in execution order */
};

struct _SwfdecSprite
{
  SwfdecObject object;

  unsigned int n_frames;
  unsigned int parse_frame;

  SwfdecSpriteFrame *frames;
};

struct _SwfdecSpriteClass
{
  SwfdecObjectClass object_class;

};

GType swfdec_sprite_get_type (void);

void swfdec_sprite_decoder_free (SwfdecObject * object);
int tag_func_define_sprite (SwfdecDecoder * s);
void swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite, int frame,
    SwfdecBuffer * chunk, int skip);
void swfdec_sprite_set_n_frames (SwfdecSprite *sprite, unsigned int n_frames);
void swfdec_sprite_add_action (SwfdecSprite * sprite, unsigned int frame, 
    SwfdecSpriteActionType type, gpointer data);

SwfdecSpriteContent *swfdec_sprite_content_new (unsigned int depth);
void swfdec_sprite_content_free (SwfdecSpriteContent *content);

int tag_show_frame (SwfdecDecoder * s);
int tag_func_set_background_color (SwfdecDecoder * s);
int swfdec_spriteseg_place_object_2 (SwfdecDecoder * s);
int swfdec_spriteseg_remove_object (SwfdecDecoder * s);
int swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s);

int tag_func_export_assets (SwfdecDecoder * s);
SwfdecObject *swfdec_exports_lookup (SwfdecDecoder * s, char *name);

G_END_DECLS
#endif
