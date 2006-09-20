
#ifndef __SWFDEC_JS_H__
#define __SWFDEC_JS_H__

#include "swfdec_types.h"
#include <js/jsapi.h>

G_BEGIN_DECLS

void		swfdec_js_init			(guint			runtime_size);
void		swfdec_js_init_decoder		(SwfdecDecoder *	s);
gboolean	swfdec_js_script_queue_is_empty (SwfdecDecoder *	s);
void		swfdec_js_finish_decoder	(SwfdecDecoder *	s);
gboolean	swfdec_js_execute_script	(SwfdecDecoder *	s,
						 SwfdecMovieClip *	movie, 
						 JSScript *		script);
gboolean	swfdec_js_run			(SwfdecDecoder *	dec,
						 const char *		s);

void		swfdec_js_add_movieclip_class	(SwfdecDecoder *	dec);
void		swfdec_js_add_movieclip		(SwfdecMovieClip *	movie);

G_END_DECLS

#endif
