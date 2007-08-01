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
#include "swfdec_loadertarget.h"

G_BEGIN_DECLS

typedef enum {
  SWFDEC_LOADER_STATE_NEW = 0,		/* loader is new and has not been opened yet */
  SWFDEC_LOADER_STATE_OPEN,		/* loader is opened and has got the HTTP headers */
  SWFDEC_LOADER_STATE_READING,		/* loader has read some bytes of data and is still reading */
  SWFDEC_LOADER_STATE_EOF,		/* swfdec_loader_eof() has been called */
  SWFDEC_LOADER_STATE_ERROR		/* loader is in error state */
} SwfdecLoaderState;

SwfdecLoader *		swfdec_loader_load		(SwfdecLoader *		loader,
							 const char *		url,
							 SwfdecLoaderRequest	request,
							 const char *		data,
							 gsize			data_len);
void			swfdec_loader_set_target	(SwfdecLoader *		loader,
							 SwfdecLoaderTarget *	target);
void			swfdec_loader_set_data_type	(SwfdecLoader *		loader,
							 SwfdecLoaderDataType	type);

gboolean		swfdec_urldecode_one		(const char *		string,
							 char **		name,
							 char **		value,
							 const char **		end);
void			swfdec_string_append_urlencoded	(GString *		str,
							 const char *		name,
							 const char *		value);

G_END_DECLS
#endif
