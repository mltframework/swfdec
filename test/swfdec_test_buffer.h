/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_TEST_BUFFER_H_
#define _SWFDEC_TEST_BUFFER_H_

#include <swfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _SwfdecTestBuffer SwfdecTestBuffer;
typedef struct _SwfdecTestBufferClass SwfdecTestBufferClass;

#define SWFDEC_TYPE_TEST_BUFFER                    (swfdec_test_buffer_get_type())
#define SWFDEC_IS_TEST_BUFFER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEST_BUFFER))
#define SWFDEC_IS_TEST_BUFFER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEST_BUFFER))
#define SWFDEC_TEST_BUFFER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEST_BUFFER, SwfdecTestBuffer))
#define SWFDEC_TEST_BUFFER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEST_BUFFER, SwfdecTestBufferClass))
#define SWFDEC_TEST_BUFFER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEST_BUFFER, SwfdecTestBufferClass))

struct _SwfdecTestBuffer
{
  SwfdecAsRelay		relay;

  SwfdecBuffer *	buffer;
};

struct _SwfdecTestBufferClass
{
  SwfdecAsRelayClass	relay_class;
};

GType	  		swfdec_test_buffer_get_type	(void);

SwfdecTestBuffer *	swfdec_test_buffer_new		(SwfdecAsContext *	context,
							 SwfdecBuffer *		buffer);

SwfdecBuffer *		swfdec_test_buffer_from_args	(SwfdecAsContext *	cx,
				  			 guint			argc,
							 SwfdecAsValue *	argv);

G_END_DECLS
#endif
