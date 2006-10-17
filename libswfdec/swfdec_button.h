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

#ifndef _SWFDEC_BUTTON_H_
#define _SWFDEC_BUTTON_H_

#include <libswfdec/swfdec_graphic.h>
#include <libswfdec/color.h>
#include <libswfdec/swfdec_event.h>

G_BEGIN_DECLS
//typedef struct _SwfdecButton SwfdecButton;
typedef struct _SwfdecButtonClass SwfdecButtonClass;
typedef struct _SwfdecButtonRecord SwfdecButtonRecord;

#define SWFDEC_TYPE_BUTTON                    (swfdec_button_get_type())
#define SWFDEC_IS_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BUTTON))
#define SWFDEC_IS_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BUTTON))
#define SWFDEC_BUTTON(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BUTTON, SwfdecButton))
#define SWFDEC_BUTTON_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BUTTON, SwfdecButtonClass))

/* these values have to be kept in line with record parsing */
typedef enum {
  SWFDEC_BUTTON_UP = (1 << 0),
  SWFDEC_BUTTON_OVER = (1 << 1),
  SWFDEC_BUTTON_DOWN = (1 << 2),
  SWFDEC_BUTTON_HIT = (1 << 3)
} SwfdecButtonState;

/* these values have to be kept in line with condition parsing */
typedef enum {
  SWFDEC_BUTTON_IDLE_TO_OVER_UP = (1 << 0),
  SWFDEC_BUTTON_OVER_UP_TO_IDLE = (1 << 1),
  SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN = (1 << 2),
  SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP = (1 << 3),
  SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN = (1 << 4),
  SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN = (1 << 5),
  SWFDEC_BUTTON_OUT_DOWN_TO_IDLE = (1 << 6),
  SWFDEC_BUTTON_IDLE_TO_OVER_DOWN = (1 << 7),
  SWFDEC_BUTTON_OVER_DOWN_TO_IDLE = (1 << 8)
} SwfdecButtonCondition;

struct _SwfdecButtonRecord
{
  SwfdecButtonState	states;
  SwfdecGraphic *	graphic;
  cairo_matrix_t	transform;
  SwfdecColorTransform	color_transform;
};

struct _SwfdecButton
{
  SwfdecGraphic		graphic;

  gboolean		menubutton;	/* treat as menubutton */

  GArray *		records;	/* the contained objects */
  SwfdecEventList *	events;		/* the events triggered by this button */
};

struct _SwfdecButtonClass
{
  SwfdecGraphicClass	graphic_class;
};

GType		swfdec_button_get_type	(void);


G_END_DECLS
#endif
