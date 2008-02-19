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

#ifndef _SWFDEC_BUTTON_H_
#define _SWFDEC_BUTTON_H_

#include <swfdec/swfdec_graphic.h>
#include <swfdec/swfdec_color.h>

G_BEGIN_DECLS
//typedef struct _SwfdecButton SwfdecButton;
typedef struct _SwfdecButtonClass SwfdecButtonClass;

#define SWFDEC_TYPE_BUTTON                    (swfdec_button_get_type())
#define SWFDEC_IS_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BUTTON))
#define SWFDEC_IS_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BUTTON))
#define SWFDEC_BUTTON(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BUTTON, SwfdecButton))
#define SWFDEC_BUTTON_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BUTTON, SwfdecButtonClass))

/* these values have to be kept in line with record parsing */
typedef enum {
  SWFDEC_BUTTON_INIT = -1,
  SWFDEC_BUTTON_UP = 0,
  SWFDEC_BUTTON_OVER = 1,
  SWFDEC_BUTTON_DOWN = 2,
  SWFDEC_BUTTON_HIT = 3
} SwfdecButtonState;

/* these values have to be kept in line with condition parsing */
typedef enum {
  SWFDEC_BUTTON_IDLE_TO_OVER_UP = 0,
  SWFDEC_BUTTON_OVER_UP_TO_IDLE = 1,
  SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN = 2,
  SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP = 3,
  SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN = 4,
  SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN = 5,
  SWFDEC_BUTTON_OUT_DOWN_TO_IDLE = 6,
  SWFDEC_BUTTON_IDLE_TO_OVER_DOWN = 7,
  SWFDEC_BUTTON_OVER_DOWN_TO_IDLE = 8
} SwfdecButtonCondition;

struct _SwfdecButton {
  SwfdecGraphic		graphic;	/* graphic->extents is used for HIT area extents only */

  gboolean		menubutton;	/* treat as menubutton */

  GSList *		records;	/* the contained objects */
  SwfdecEventList *	events;		/* the events triggered by this button */
  SwfdecSoundChunk *	sounds[4];    	/* for meaning of index see DefineButtonSound */
};

struct _SwfdecButtonClass
{
  SwfdecGraphicClass	graphic_class;
};

GType		swfdec_button_get_type	  	(void);

int		tag_func_define_button		(SwfdecSwfDecoder *	s,
						 guint			tag);
int		tag_func_define_button_2	(SwfdecSwfDecoder *	s,
						 guint			tag);

G_END_DECLS
#endif
