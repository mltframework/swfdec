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

#ifndef _SWFDEC_TEST_HTTP_REQUEST_H_
#define _SWFDEC_TEST_HTTP_REQUEST_H_

#include <swfdec/swfdec.h>
#include <libsoup/soup.h>

#include "swfdec_test_http_server.h"

G_BEGIN_DECLS


typedef struct _SwfdecTestHTTPRequest SwfdecTestHTTPRequest;
typedef struct _SwfdecTestHTTPRequestClass SwfdecTestHTTPRequestClass;

typedef enum {
  SWFDEC_TEST_HTTP_REQUEST_STATE_WAITING,
  SWFDEC_TEST_HTTP_REQUEST_STATE_SENDING,
  SWFDEC_TEST_HTTP_REQUEST_STATE_SENT
} SwfdecTestHTTPRequestState;

#define SWFDEC_TYPE_TEST_HTTP_REQUEST                    (swfdec_test_http_request_get_type())
#define SWFDEC_IS_TEST_HTTP_REQUEST(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEST_HTTP_REQUEST))
#define SWFDEC_IS_TEST_HTTP_REQUEST_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEST_HTTP_REQUEST))
#define SWFDEC_TEST_HTTP_REQUEST(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEST_HTTP_REQUEST, SwfdecTestHTTPRequest))
#define SWFDEC_TEST_HTTP_REQUEST_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEST_HTTP_REQUEST, SwfdecTestHTTPRequestClass))
#define SWFDEC_TEST_HTTP_REQUEST_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEST_HTTP_REQUEST, SwfdecTestHTTPRequestClass))

struct _SwfdecTestHTTPRequest
{
  SwfdecAsObject		as_object;

  SwfdecTestHTTPServer *	server;
  SoupMessage *			message;

  SwfdecTestHTTPRequestState	state;
  SwfdecAsObject *		headers;
};

struct _SwfdecTestHTTPRequestClass
{
  SwfdecAsObjectClass	as_object_class;
};

GType		swfdec_test_http_request_get_type	(void);

SwfdecAsObject *swfdec_test_http_request_new		(SwfdecAsContext *	context,
							 SwfdecTestHTTPServer *	server,
							 SoupMessage *		message);

G_END_DECLS
#endif
