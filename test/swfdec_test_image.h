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

#ifndef _SWFDEC_TEST_IMAGE_H_
#define _SWFDEC_TEST_IMAGE_H_

#include <swfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _SwfdecTestImage SwfdecTestImage;
typedef struct _SwfdecTestImageClass SwfdecTestImageClass;

#define SWFDEC_TYPE_TEST_IMAGE                    (swfdec_test_image_get_type())
#define SWFDEC_IS_TEST_IMAGE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEST_IMAGE))
#define SWFDEC_IS_TEST_IMAGE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEST_IMAGE))
#define SWFDEC_TEST_IMAGE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEST_IMAGE, SwfdecTestImage))
#define SWFDEC_TEST_IMAGE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEST_IMAGE, SwfdecTestImageClass))
#define SWFDEC_TEST_IMAGE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEST_IMAGE, SwfdecTestImageClass))

struct _SwfdecTestImage
{
  SwfdecAsRelay		relay;

  cairo_surface_t *	surface;	/* surface or NULL when broken image */
};

struct _SwfdecTestImageClass
{
  SwfdecAsRelayClass	relay_class;
};

GType			swfdec_test_image_get_type	(void);

SwfdecTestImage *	swfdec_test_image_new		(SwfdecAsContext *	context,
							 guint			width,
							 guint			height);

G_END_DECLS
#endif
