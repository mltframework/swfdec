
#ifndef __SWFDEC_JS_H__
#define __SWFDEC_JS_H__

#include "swfdec_types.h"
#include <js/jsapi.h>

G_BEGIN_DECLS

void		swfdec_js_init			(guint		runtime_size);
void		swfdec_js_init_decoder		(SwfdecDecoder *s);
void		swfdec_js_finish_decoder	(SwfdecDecoder *s);

void		swfdec_js_add_movieclip		(SwfdecDecoder *s);

G_END_DECLS

#endif
