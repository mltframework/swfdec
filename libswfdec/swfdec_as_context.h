/* SwfdecAs
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

#ifndef _SWFDEC_AS_CONTEXT_H_
#define _SWFDEC_AS_CONTEXT_H_

#include <glib-object.h>
#include <libswfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_AS_CONTEXT_RUNNING,
  SWFDEC_AS_CONTEXT_INTERRUPTED,
  SWFDEC_AS_CONTEXT_ABORTED
} SwfdecAsContextState;

#define SWFDEC_AS_GC_MARK (1 << 0)		/* only valid during GC */
#define SWFDEC_AS_GC_ROOT (1 << 1)		/* for objects: rooted, for strings: static */

typedef struct _SwfdecAsContextClass SwfdecAsContextClass;

#define SWFDEC_TYPE_AS_CONTEXT                    (swfdec_as_context_get_type())
#define SWFDEC_IS_AS_CONTEXT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_CONTEXT))
#define SWFDEC_IS_AS_CONTEXT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_CONTEXT))
#define SWFDEC_AS_CONTEXT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_CONTEXT, SwfdecAsContext))
#define SWFDEC_AS_CONTEXT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_CONTEXT, SwfdecAsContextClass))
#define SWFDEC_AS_CONTEXT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_CONTEXT, SwfdecAsContextClass))

struct _SwfdecAsContext {
  GObject		object;

  SwfdecAsContextState	state;		/* our current state */

  /* bookkeeping for GC */
  gsize			memory;		/* memory currently in use */
  GHashTable *		strings;	/* string=>memory mapping the context manages */
  GHashTable *		objects;	/* all objects the context manages */

  /* execution state */
  unsigned int	      	version;	/* currently active version */
  SwfdecAsFrame *	frame;		/* topmost stack frame */
};

struct _SwfdecAsContextClass {
  GObjectClass		object_class;
};

GType		swfdec_as_context_get_type	(void);

SwfdecAsContext *swfdec_as_context_new		(void);

const char *	swfdec_as_context_get_string	(SwfdecAsContext *	context,
						 const char *		string);

#define swfdec_as_context_abort_oom(context) swfdec_as_context_abort (context, "Out of memory")
void		swfdec_as_context_abort		(SwfdecAsContext *	context,
						 const char *		reason);

gboolean	swfdec_as_context_use_mem     	(SwfdecAsContext *	context, 
						 gsize			len);
void		swfdec_as_context_unuse_mem   	(SwfdecAsContext *	context,
						 gsize			len);
void		swfdec_as_object_mark		(SwfdecAsObject *	object);
void		swfdec_as_value_mark		(SwfdecAsValue *	value);
void		swfdec_as_string_mark		(const char *		string);
void		swfdec_as_context_gc		(SwfdecAsContext *	context);

void		swfdec_as_context_return	(SwfdecAsContext *	context,
						 SwfdecAsValue *	retval);

G_END_DECLS
#endif
