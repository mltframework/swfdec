
#ifndef __SWFDEC_COMPILER_H__
#define __SWFDEC_COMPILER_H__

#include "swfdec_types.h"
#include <js/jsapi.h>

G_BEGIN_DECLS

JSScript *	swfdec_compile		(SwfdecDecoder *s);

G_END_DECLS

#endif
