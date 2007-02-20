/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/js/jspubtd.h>

#ifndef _SWFDEC_LISTENER_H_
#define _SWFDEC_LISTENER_H_

G_BEGIN_DECLS


SwfdecListener *	swfdec_listener_new	(SwfdecPlayer *		player);
void			swfdec_listener_free	(SwfdecListener *	listener);
gboolean		swfdec_listener_add	(SwfdecListener *	listener,
						 JSObject *		obj);
void			swfdec_listener_remove	(SwfdecListener *	listener,
						 JSObject *		obj);
void			swfdec_listener_execute	(SwfdecListener *	listener,
						 const char *		event);


G_END_DECLS

#endif
