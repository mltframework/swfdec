/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_TEST_HTTP_SERVER_H_
#define _SWFDEC_TEST_HTTP_SERVER_H_

#include <swfdec/swfdec.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS


typedef struct _SwfdecTestHTTPServer SwfdecTestHTTPServer;
typedef struct _SwfdecTestHTTPServerClass SwfdecTestHTTPServerClass;

#define SWFDEC_TYPE_TEST_HTTP_SERVER                    (swfdec_test_http_server_get_type())
#define SWFDEC_IS_TEST_HTTP_SERVER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEST_HTTP_SERVER))
#define SWFDEC_IS_TEST_HTTP_SERVER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEST_HTTP_SERVER))
#define SWFDEC_TEST_HTTP_SERVER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEST_HTTP_SERVER, SwfdecTestHTTPServer))
#define SWFDEC_TEST_HTTP_SERVER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEST_HTTP_SERVER, SwfdecTestHTTPServerClass))
#define SWFDEC_TEST_HTTP_SERVER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEST_HTTP_SERVER, SwfdecTestHTTPServerClass))

struct _SwfdecTestHTTPServer
{
  SwfdecAsObject	as_object;

  GMainContext *	context;
  SoupServer *		server;
  guint			port;

  GQueue *		messages;	// SoupMessage **
};

struct _SwfdecTestHTTPServerClass
{
  SwfdecAsObjectClass	as_object_class;
};

GType		swfdec_test_http_server_get_type	(void);

SwfdecAsObject *swfdec_test_http_server_new		(SwfdecAsContext *	context,
							 guint			port);
void		swfdec_test_http_server_run		(SwfdecTestHTTPServer *	server);

G_END_DECLS
#endif
