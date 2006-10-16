
#ifndef __SWFDEC_COMPILER_H__
#define __SWFDEC_COMPILER_H__

#include <libswfdec/js/jsapi.h>
#include <libswfdec/swfdec_bits.h>
#include <libswfdec/swfdec_player.h>

G_BEGIN_DECLS

void		swfdec_disassemble	(SwfdecPlayer *	player,
					 JSScript *	script);
JSScript *	swfdec_compile		(SwfdecPlayer *	player,
					 SwfdecBits *	bits);

G_END_DECLS

#endif
