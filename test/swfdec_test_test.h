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

#ifndef _SWFDEC_TEST_TEST_H_
#define _SWFDEC_TEST_TEST_H_

#include "swfdec_test_plugin.h"
#include <gmodule.h>
#include <swfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _SwfdecTestTest SwfdecTestTest;
typedef struct _SwfdecTestTestClass SwfdecTestTestClass;

#define SWFDEC_TYPE_TEST_TEST                    (swfdec_test_test_get_type())
#define SWFDEC_IS_TEST_TEST(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEST_TEST))
#define SWFDEC_IS_TEST_TEST_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEST_TEST))
#define SWFDEC_TEST_TEST(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEST_TEST, SwfdecTestTest))
#define SWFDEC_TEST_TEST_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEST_TEST, SwfdecTestTestClass))
#define SWFDEC_TEST_TEST_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEST_TEST, SwfdecTestTestClass))

struct _SwfdecTestTest
{
  SwfdecAsObject	as_object;

  SwfdecTestPlugin	plugin;		/* the plugin we use */
  GModule *		module;		/* module we loaded the plugin from or NULL */
  gboolean		plugin_loaded;	/* the plugin is loaded and ready to rumble */
  gboolean		plugin_quit;	/* the plugin has called quit */
  gboolean		plugin_error;	/* the plugin has called error */

  char *		filename;	/* file the player should be loaded from */
  SwfdecBufferQueue *	trace;		/* all captured trace output */
  SwfdecBufferQueue *	launched;	/* all launched urls */

  GList *		sockets;	/* list of all sockets */
  GList *		pending_sockets;/* last socket handed out or NULL if none given out */
};

struct _SwfdecTestTestClass
{
  SwfdecAsObjectClass	as_object_class;
};

extern char *swfdec_test_plugin_name;

GType		swfdec_test_test_get_type	(void);

void		swfdec_test_plugin_swfdec_new	(SwfdecTestPlugin *	plugin);

G_END_DECLS
#endif
