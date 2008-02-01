/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_LOADER_INTERNAL_H_
#define _SWFDEC_LOADER_INTERNAL_H_

#include "swfdec_loader.h"
#include "swfdec_stream_target.h"

G_BEGIN_DECLS

/* swfdec_stream.c */
SwfdecBufferQueue *	swfdec_stream_get_queue		(SwfdecStream *		stream);
const char *		swfdec_stream_describe		(SwfdecStream *		stream);
void			swfdec_stream_close		(SwfdecStream *		stream);
void			swfdec_stream_set_target	(SwfdecStream *		stream,
							 SwfdecStreamTarget *	target);

/* swfdec_loader.c */
void			swfdec_loader_set_data_type	(SwfdecLoader *		loader,
							 SwfdecLoaderDataType	type);

/* swfdec_socket.c */
void			swfdec_socket_send		(SwfdecSocket *		sock,
							 SwfdecBuffer *		buffer);


G_END_DECLS
#endif
