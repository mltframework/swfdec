/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_AS_CONTEXT_NEW,
  SWFDEC_AS_CONTEXT_RUNNING,
  SWFDEC_AS_CONTEXT_INTERRUPTED,
  SWFDEC_AS_CONTEXT_ABORTED
} SwfdecAsContextState;

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
  SwfdecAsObject *	global;		/* the global object or NULL if not initialized yet. 
					   In SwfdecPlayer is NULL unless a sandbox is in use */
  GRand *		rand;		/* random number generator */
  GTimeVal		start_time;   	/* time this movie started (for GetTime action) */

  /* GC properties */
  gsize			memory_until_gc;/* amount of memory allocations that trigger a GC */

  /* bookkeeping for GC */
  gsize			memory;		/* total memory currently in use */
  gsize			memory_since_gc;/* memory allocated since last GC run */
  GHashTable *		interned_strings;/* string => memory mapping the context manages */
  GHashTable *		objects;	/* all objects the context manages */
  gpointer		strings;	/* all numbers the context manages */
  gpointer		numbers;	/* all numbers the context manages */
  gpointer		movies;		/* all movies the context manages */
  GHashTable *		constant_pools;	/* memory address => SwfdecConstantPool for all gc'ed pools */

  /* execution state */
  unsigned int	      	version;	/* currently active version */
  unsigned int		call_depth;   	/* current depth of call stack (equals length of frame list) */
  SwfdecAsFrame *	frame;		/* topmost stack frame */
  gboolean		exception;	/* whether we are throwing an exception */
  SwfdecAsValue		exception_value; /* value of the exception being thrown, can be anything including undefined */

  /* stack */
  SwfdecAsValue	*	base;		/* stack base */
  SwfdecAsValue	*	end;		/* end of stack */
  SwfdecAsValue	*	cur;		/* pointer to current top of stack */
  SwfdecAsStack *	stack;		/* current stack */

  /* debugging */
  SwfdecAsDebugger *	debugger;	/* debugger (or NULL if none) */
};

struct _SwfdecAsContextClass {
  GObjectClass		object_class;

  /* mark all objects that should not be collected */
  void			(* mark)		(SwfdecAsContext *	context);
  /* overwrite if you want to report a different time than gettimeofday */
  void			(* get_time)		(SwfdecAsContext *      context,
						 GTimeVal *		tv);
  /* overwrite if you want to abort on infinite loops */
  gboolean		(* check_continue)	(SwfdecAsContext *	context);
};

GType		swfdec_as_context_get_type	(void);

void		swfdec_as_context_startup     	(SwfdecAsContext *	context);

gboolean	swfdec_as_context_is_aborted	(SwfdecAsContext *	context);
gboolean	swfdec_as_context_is_constructing
						(SwfdecAsContext *	context);
SwfdecAsFrame *	swfdec_as_context_get_frame	(SwfdecAsContext *	context);
void		swfdec_as_context_get_time	(SwfdecAsContext *	context,
						 GTimeVal *		tv);
const char *	swfdec_as_context_get_string	(SwfdecAsContext *	context,
						 const char *		string);
const char *	swfdec_as_context_give_string	(SwfdecAsContext *	context,
						 char *			string);

void		swfdec_as_context_abort		(SwfdecAsContext *	context,
						 const char *		reason);

void		swfdec_as_context_throw		(SwfdecAsContext *	context,
						 const SwfdecAsValue *	value);
gboolean	swfdec_as_context_catch		(SwfdecAsContext *	context,
						 SwfdecAsValue *	value);

gboolean	swfdec_as_context_try_use_mem	(SwfdecAsContext *	context,
						 gsize			bytes);
void		swfdec_as_context_use_mem	(SwfdecAsContext *	context,
						 gsize			bytes);
void		swfdec_as_context_unuse_mem   	(SwfdecAsContext *	context,
						 gsize			bytes);
void		swfdec_as_value_mark		(SwfdecAsValue *	value);
void		swfdec_as_string_mark		(const char *		string);
void		swfdec_as_context_gc		(SwfdecAsContext *	context);
void		swfdec_as_context_maybe_gc	(SwfdecAsContext *	context);


G_END_DECLS
#endif
