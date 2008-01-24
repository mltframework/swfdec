/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include <swfdec/swfdec_color.h>
#include <swfdec/swfdec_event.h>
#include <swfdec/swfdec_graphic.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecSpriteClass SwfdecSpriteClass;
typedef struct _SwfdecSpriteAction SwfdecSpriteAction;
typedef struct _SwfdecExport SwfdecExport;

/* FIXME: It might make sense to event a SwfdecActionBuffer - a subclass of 
 * SwfdecBuffer that carries around a the tag.
 * It might also make more sense to not parse the file into buffers at all
 * and operate on the memory directly.
 */
struct _SwfdecSpriteAction {
  guint				tag;	/* the data tag (see swfdec_tag.h) */
  SwfdecBuffer *		buffer;	/* the buffer for this data (can be NULL) */
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
  guint sound_samples;			/* number of samples in this frame */
  GSList *sound;			/* list of SwfdecSoundChunk events to start playing here */
};

struct _SwfdecSprite
{
  SwfdecGraphic		graphic;

  SwfdecSpriteFrame *	frames;		/* the n_frames different frames */
  guint			n_frames;	/* number of frames in this sprite */
  SwfdecScript *	init_action;	/* action to run when initializing this sprite */
  GArray *		actions;      	/* SwfdecSpriteAction in execution order */

  /* parse state */
  guint			parse_frame;	/* frame we're currently parsing. == n_frames if done parsing */
};

struct _SwfdecSpriteClass
{
  SwfdecGraphicClass	graphic_class;
};

GType swfdec_sprite_get_type (void);

int tag_func_define_sprite (SwfdecSwfDecoder * s, guint tag);
void swfdec_sprite_add_sound_chunk (SwfdecSprite * sprite, guint frame,
    SwfdecBuffer * chunk, int skip, guint n_samples);
void swfdec_sprite_set_n_frames (SwfdecSprite *sprite, guint n_frames, guint rate);
void swfdec_sprite_add_action (SwfdecSprite * sprite, guint tag, SwfdecBuffer *buffer);
gboolean	swfdec_sprite_get_action	(SwfdecSprite *		sprite,
						 guint			n,
						 guint *      		tag,
						 SwfdecBuffer **	buffer);
int		swfdec_sprite_get_frame		(SwfdecSprite *		sprite,
				      		 const char *		label);

int tag_func_set_background_color (SwfdecSwfDecoder * s, guint tag);


G_END_DECLS
#endif
