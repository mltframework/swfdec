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
#include <libswfdec/swfdec_as_types.h>
#include <libswfdec/swfdec_types.h>

#ifndef _SWFDEC_LISTENER_H_
#define _SWFDEC_LISTENER_H_

G_BEGIN_DECLS


SwfdecListener *	swfdec_listener_new	(SwfdecAsContext *	context);
void			swfdec_listener_free	(SwfdecListener *	listener);
gboolean		swfdec_listener_add	(SwfdecListener *	listener,
						 SwfdecAsObject *     	obj);
void			swfdec_listener_remove	(SwfdecListener *	listener,
						 SwfdecAsObject *     	obj);
void			swfdec_listener_execute	(SwfdecListener *	listener,
						 const char *		event);
void			swfdec_listener_mark	(SwfdecListener *	listener);


G_END_DECLS

#endif
