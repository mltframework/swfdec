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

#ifndef __SWFDEC_JS_H__
#define __SWFDEC_JS_H__

#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_movie.h>
#include <libswfdec/js/jsapi.h>

G_BEGIN_DECLS

void		swfdec_js_init			(guint			runtime_size);
void		swfdec_js_init_player		(SwfdecPlayer *		player);
void		swfdec_js_finish_player		(SwfdecPlayer *		player);
gboolean	swfdec_js_execute_script	(SwfdecPlayer *		player,
						 SwfdecMovie *		movie, 
						 JSScript *		script,
						 jsval *		rval);
gboolean	swfdec_js_run			(SwfdecPlayer *		player,
						 const char *		s,
						 jsval *		rval);

void		swfdec_js_add_color		(SwfdecPlayer *		player);
void		swfdec_js_add_globals		(SwfdecPlayer *		player);
void		swfdec_js_add_mouse		(SwfdecPlayer *		player);
void		swfdec_js_add_movieclip_class	(SwfdecPlayer *		player);
void		swfdec_js_add_sound		(SwfdecPlayer *		player);

void		swfdec_js_movie_add_property	(SwfdecMovie *		movie);
void		swfdec_js_movie_remove_property	(SwfdecMovie *		movie);

char *		swfdec_js_slash_to_dot		(const char *		slash_str);
jsval		swfdec_js_eval			(JSContext *		cx,
						 JSObject *		obj,
						 const char *		str,
						 gboolean		ignore_case);

/* support functions */
const char *	swfdec_js_to_string		(JSContext *		cx,
						 jsval			val);

G_END_DECLS

#endif
