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

#ifndef _SWFDEC_INTERVAL_H_
#define _SWFDEC_INTERVAL_H_

#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_gc_object.h>
#include <swfdec/swfdec_player_internal.h>
#include <swfdec/swfdec_sandbox.h>

G_BEGIN_DECLS

typedef struct _SwfdecInterval SwfdecInterval;
typedef struct _SwfdecIntervalClass SwfdecIntervalClass;

#define SWFDEC_TYPE_INTERVAL                    (swfdec_interval_get_type())
#define SWFDEC_IS_INTERVAL(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_INTERVAL))
#define SWFDEC_IS_INTERVAL_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_INTERVAL))
#define SWFDEC_INTERVAL(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_INTERVAL, SwfdecInterval))
#define SWFDEC_INTERVAL_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_INTERVAL, SwfdecIntervalClass))
#define SWFDEC_INTERVAL_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_INTERVAL, SwfdecIntervalClass))

struct _SwfdecInterval {
  SwfdecGcObject	gc_object;

  SwfdecTimeout		timeout;
  SwfdecSandbox *	sandbox;	/* sandbox we run the script in */
  guint			id;		/* id this interval is identified with */
  guint			msecs;		/* interval in milliseconds */
  gboolean		repeat;		/* TRUE for calling in intervals, FALSE for single-shot */
  /* if calling named function */
  SwfdecAsObject *	object;		/* this object or function to call (depending on fun_name) */
  const char *		fun_name;	/* name of function or NULL if object is function */

  guint			n_args;		/* number of arguments to call function with */
  SwfdecAsValue *     	args;		/* arguments for function */
};

struct _SwfdecIntervalClass {
  SwfdecGcObjectClass	gc_object_class;
};

GType		swfdec_interval_get_type	(void);

guint		swfdec_interval_new_function  	(SwfdecPlayer *		player,
						 guint			msecs,
						 gboolean		repeat,
						 SwfdecAsFunction *	fun,
						 guint			n_args,
						 const SwfdecAsValue *	args);
guint		swfdec_interval_new_object  	(SwfdecPlayer *		player,
						 guint			msecs,
						 gboolean		repeat,
						 SwfdecAsObject *	thisp,
						 const char *		fun_name,
						 guint			n_args,
						 const SwfdecAsValue *	args);
void		swfdec_interval_remove		(SwfdecPlayer *		player,
						 guint			id);


G_END_DECLS
#endif
