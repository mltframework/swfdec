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

#include <libswfdec/swfdec_bits.h>
#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_types.h>

#ifndef _SWFDEC_EVENT_H_
#define _SWFDEC_EVENT_H_

G_BEGIN_DECLS

typedef enum _SwfdecEventType {
  SWFDEC_EVENT_LOAD		= (1 << 0),
  SWFDEC_EVENT_ENTER		= (1 << 1),
  SWFDEC_EVENT_UNLOAD		= (1 << 2),
  SWFDEC_EVENT_MOUSE_MOVE	= (1 << 3),
  SWFDEC_EVENT_MOUSE_DOWN	= (1 << 4),
  SWFDEC_EVENT_MOUSE_UP		= (1 << 5),
  SWFDEC_EVENT_KEY_UP		= (1 << 6),
  SWFDEC_EVENT_KEY_DOWN		= (1 << 7),
  SWFDEC_EVENT_DATA		= (1 << 8),
  SWFDEC_EVENT_INITIALIZE	= (1 << 9),
  SWFDEC_EVENT_PRESS		= (1 << 10),
  SWFDEC_EVENT_RELEASE		= (1 << 11),
  SWFDEC_EVENT_RELEASE_OUTSIDE	= (1 << 12),
  SWFDEC_EVENT_ROLL_OVER	= (1 << 13),
  SWFDEC_EVENT_ROLL_OUT		= (1 << 14),
  SWFDEC_EVENT_DRAG_OVER	= (1 << 15),
  SWFDEC_EVENT_DRAG_OUT		= (1 << 16),
  SWFDEC_EVENT_KEY_PRESS	= (1 << 17),
  SWFDEC_EVENT_CONSTRUCT	= (1 << 18)
} SwfdecEventType;

SwfdecEventList *	swfdec_event_list_new		(SwfdecPlayer *	      player);
SwfdecEventList *	swfdec_event_list_copy		(SwfdecEventList *    list);
void			swfdec_event_list_free		(SwfdecEventList *    list);

void			swfdec_event_list_parse		(SwfdecEventList *    list,
							 SwfdecBits *	      bits,
							 int		      version,
							 unsigned int	      conditions,
							 guint8		      key,
							 const char *	      description);
void			swfdec_event_list_execute	(SwfdecEventList *    list,
							 SwfdecScriptable *	scriptable,
							 unsigned int	      condition,
							 guint8		      key);
gboolean		swfdec_event_list_has_conditions(SwfdecEventList *    list,
							 unsigned int	      conditions,
							 guint8		      key);
							 

G_END_DECLS

#endif
