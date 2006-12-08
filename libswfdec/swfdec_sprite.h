/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef _SWFDEC_SPRITE_H_
#define _SWFDEC_SPRITE_H_

#include <libswfdec/swfdec_color.h>
#include <libswfdec/swfdec_event.h>
#include <libswfdec/swfdec_graphic.h>
#include <libswfdec/swfdec_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecSpriteClass SwfdecSpriteClass;
typedef struct _SwfdecSpriteAction SwfdecSpriteAction;
typedef struct _SwfdecExport SwfdecExport;

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

#define SWFDEC_TYPE_SPRITE                    (swfdec_sprite_get_type())
#define SWFDEC_IS_SPRITE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SPRITE))
#define SWFDEC_IS_SPRITE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SPRITE))
#define SWFDEC_SPRITE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SPRITE, SwfdecSprite))
#define SWFDEC_SPRITE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SPRITE, SwfdecSpriteClass))

struct _SwfdecSpriteFrame
{
  char *label;				/* name of the frame for "GotoLabel" */

  /* sound */
  SwfdecSound *sound_head;		/* sound head for this frame */
  int sound_skip;			/* samples to skip - maybe even backwards */
  SwfdecBuffer *sound_block;		/* sound chunk to play here or NULL for none */
  unsigned int sound_samples;		/* number of samples in this frame */
  GSList *sound;			/* list of SwfdecSoundChunk events to start playing here */

  /* visuals */
  SwfdecColor bg_color;
  GArray *actions;			/* SwfdecSpriteAction in execution order */
};

struct _SwfdecSprite
{
  SwfdecGraphic		graphic;

  SwfdecPlayer *	player;		/* FIXME: only needed to get the JS Context, I want it gone */
  SwfdecSpriteFrame *	frames;		/* the n_frames different frames */
  unsigned int		n_frames;	/* number of frames in this sprite */

  /* parse state */
  unsigned int		parse_frame;	/* frame we're currently parsing. == n_frames if done parsing */
};

struct _SwfdecSpriteClass
{
  SwfdecGraphicClass	graphic_class;
};

GType swfdec_sprite_get_type (void);

int tag_func_define_sprite (SwfdecSwfDecoder * s);
void swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite, unsigned int frame,
    SwfdecBuffer * chunk, int skip, unsigned int n_samples);
void swfdec_sprite_set_n_frames (SwfdecSprite *sprite, unsigned int n_frames, unsigned int rate);
void swfdec_sprite_add_action (SwfdecSprite * sprite, unsigned int frame, 
    SwfdecSpriteActionType type, gpointer data);
unsigned int swfdec_sprite_get_next_frame (SwfdecSprite *sprite, unsigned int current_frame);
int		swfdec_sprite_get_frame		(SwfdecSprite *		sprite,
				      		 const char *		label);

SwfdecContent *swfdec_content_new (int depth);
void swfdec_content_free (SwfdecContent *content);

int tag_show_frame (SwfdecSwfDecoder * s);
int tag_func_set_background_color (SwfdecSwfDecoder * s);
int swfdec_spriteseg_place_object_2 (SwfdecSwfDecoder * s);
int swfdec_spriteseg_remove_object (SwfdecSwfDecoder * s);
int swfdec_spriteseg_remove_object_2 (SwfdecSwfDecoder * s);


G_END_DECLS
#endif
