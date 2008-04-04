/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_player.h>
#include <swfdec/swfdec_types.h>

#ifndef _SWFDEC_EVENT_H_
#define _SWFDEC_EVENT_H_

G_BEGIN_DECLS

typedef enum _SwfdecEventType {
  SWFDEC_EVENT_LOAD = 0,
  SWFDEC_EVENT_ENTER = 1,
  SWFDEC_EVENT_UNLOAD = 2,
  SWFDEC_EVENT_MOUSE_MOVE = 3,
  SWFDEC_EVENT_MOUSE_DOWN = 4,
  SWFDEC_EVENT_MOUSE_UP = 5,
  SWFDEC_EVENT_KEY_UP = 6,
  SWFDEC_EVENT_KEY_DOWN = 7,
  SWFDEC_EVENT_DATA = 8,
  SWFDEC_EVENT_INITIALIZE = 9,
  SWFDEC_EVENT_PRESS = 10,
  SWFDEC_EVENT_RELEASE = 11,
  SWFDEC_EVENT_RELEASE_OUTSIDE = 12,
  SWFDEC_EVENT_ROLL_OVER = 13,
  SWFDEC_EVENT_ROLL_OUT = 14,
  SWFDEC_EVENT_DRAG_OVER = 15,
  SWFDEC_EVENT_DRAG_OUT = 16,
  SWFDEC_EVENT_KEY_PRESS = 17,
  SWFDEC_EVENT_CONSTRUCT = 18,
  /* non-loadable events go here */
  SWFDEC_EVENT_CHANGED = 19,
  SWFDEC_EVENT_SCROLL = 20,
} SwfdecEventType;

#define SWFDEC_EVENT_MASK ((1 << SWFDEC_EVENT_CONSTRUCT) - 1)

const char *		swfdec_event_type_get_name	(SwfdecEventType      type);

SwfdecEventList *	swfdec_event_list_new		(SwfdecPlayer *	      player);
SwfdecEventList *	swfdec_event_list_copy		(SwfdecEventList *    list);
void			swfdec_event_list_free		(SwfdecEventList *    list);

void			swfdec_event_list_parse		(SwfdecEventList *    list,
							 SwfdecBits *	      bits,
							 int		      version,
							 guint		      conditions,
							 guint8		      key,
							 const char *	      description);
void			swfdec_event_list_execute	(SwfdecEventList *    list,
							 SwfdecAsObject *     object,
							 guint		      condition,
							 guint8		      key);
gboolean		swfdec_event_list_has_conditions(SwfdecEventList *    list,
							 SwfdecAsObject *     object,
							 guint		      conditions,
							 guint8		      key);
gboolean		swfdec_event_list_has_mouse_events(SwfdecEventList *  list);
							 

G_END_DECLS

#endif
