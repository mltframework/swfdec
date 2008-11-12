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

#ifndef _SWFDEC_CONSTANT_POOL_H_
#define _SWFDEC_CONSTANT_POOL_H_

#include <swfdec/swfdec.h>

G_BEGIN_DECLS


#define SWFDEC_IS_CONSTANT_POOL(pool) ((pool) != NULL)

typedef struct _SwfdecConstantPool SwfdecConstantPool;


SwfdecConstantPool *	swfdec_constant_pool_new	(SwfdecAsContext *	context,
							 SwfdecBuffer *		buffer,
							 guint			version);
SwfdecConstantPool *	swfdec_constant_pool_ref      	(SwfdecConstantPool *	pool);
void			swfdec_constant_pool_unref     	(SwfdecConstantPool *	pool);
gboolean		swfdec_constant_pool_collect	(SwfdecConstantPool *	pool);

guint			swfdec_constant_pool_size	(SwfdecConstantPool *	pool);
const char *	  	swfdec_constant_pool_get	(SwfdecConstantPool *	pool,
							 guint			i);
SwfdecBuffer *		swfdec_constant_pool_get_buffer	(SwfdecConstantPool *	pool);


G_END_DECLS

#endif
