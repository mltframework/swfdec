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

#ifndef _SWFDEC_SCRIPT_H_
#define _SWFDEC_SCRIPT_H_

#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_buffer.h>

G_BEGIN_DECLS


SwfdecScript *	swfdec_script_new			(SwfdecBuffer *		buffer,
							 const char *		name,
							 guint			version);
SwfdecScript *	swfdec_script_ref			(SwfdecScript *		script);
void		swfdec_script_unref			(SwfdecScript *		script);

guint		swfdec_script_get_version		(SwfdecScript *		script);

G_END_DECLS

#endif
