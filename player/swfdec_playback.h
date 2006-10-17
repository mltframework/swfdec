
#ifndef _SWFDEC_PLAYBACK_H_
#define _SWFDEC_PLAYBACK_H_

#include <libswfdec/swfdec.h>

G_BEGIN_DECLS

gpointer	swfdec_playback_open	(GSourceFunc	callback,
					 gpointer	data,
					 guint		usecs_per_frame,
					 guint *	buffer_size);

void		swfdec_playback_write	(gpointer	sound,
					 SwfdecBuffer *	buffer);
void		swfdec_playback_close	(gpointer	sound);

G_END_DECLS
#endif
