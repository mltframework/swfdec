
#include "swfdec_types.h"

#ifndef _SWFDEC_EVENT_H_
#define _SWFDEC_EVENT_H_

G_BEGIN_DECLS

typedef struct _SwfdecEventList SwfdecEventList;

typedef enum {
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

SwfdecEventList *	swfdec_event_list_new		(SwfdecDecoder *      dec);
SwfdecEventList *	swfdec_event_list_copy		(SwfdecEventList *    list);
void			swfdec_event_list_free		(SwfdecEventList *    list);

void			swfdec_event_list_parse		(SwfdecEventList *    list,
							 unsigned int	      conditions,
							 guint8		      key);
void			swfdec_event_list_execute	(SwfdecEventList *    list,
							 unsigned int	      condition,
							 guint8		      key);

G_END_DECLS

#endif
