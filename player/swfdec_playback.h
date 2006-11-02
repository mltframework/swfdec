
#ifndef _SWFDEC_PLAYBACK_H_
#define _SWFDEC_PLAYBACK_H_

#include <libswfdec/swfdec.h>

G_BEGIN_DECLS

gpointer	swfdec_playback_open	(SwfdecPlayer *	player);

void		swfdec_playback_close	(gpointer	sound);

G_END_DECLS
#endif
