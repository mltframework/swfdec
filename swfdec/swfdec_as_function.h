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

#ifndef _SWFDEC_AS_FUNCTION_H_
#define _SWFDEC_AS_FUNCTION_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_script.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsFunctionClass SwfdecAsFunctionClass;

#define SWFDEC_TYPE_AS_FUNCTION                    (swfdec_as_function_get_type())
#define SWFDEC_IS_AS_FUNCTION(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_FUNCTION))
#define SWFDEC_IS_AS_FUNCTION_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_FUNCTION))
#define SWFDEC_AS_FUNCTION(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_FUNCTION, SwfdecAsFunction))
#define SWFDEC_AS_FUNCTION_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_FUNCTION, SwfdecAsFunctionClass))
#define SWFDEC_AS_FUNCTION_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_FUNCTION, SwfdecAsFunctionClass))

/* FIXME: do two obejcts, one for scripts and one for native? */
struct _SwfdecAsFunction {
  /*< private >*/
  SwfdecAsObject	object;
};

struct _SwfdecAsFunctionClass {
  SwfdecAsObjectClass	object_class;

  /* return a frame that calls this function or NULL if uncallable */
  SwfdecAsFrame *	(* old_call)			(SwfdecAsFunction *	function);
  /* call this function - see swfdec_as_function_call_full() for meaning of arguments */
  void			(* call)			(SwfdecAsFunction *	function,
							 SwfdecAsObject *	thisp,
							 gboolean		construct,
							 SwfdecAsObject *	super_reference,
							 guint			n_args,
							 const SwfdecAsValue *	args,
							 SwfdecAsValue *	return_value);
};

GType			swfdec_as_function_get_type	(void);

void			swfdec_as_function_old_call	(SwfdecAsFunction *	function,
							 SwfdecAsObject *	thisp,
							 guint			n_args,
							 const SwfdecAsValue *	args,
							 SwfdecAsValue *	return_value);
void			swfdec_as_function_call_full	(SwfdecAsFunction *	function,
							 SwfdecAsObject *	thisp,
							 gboolean		construct,
							 SwfdecAsObject *	super_reference,
							 guint			n_args,
							 const SwfdecAsValue *	args,
							 SwfdecAsValue *	return_value);
static inline void
swfdec_as_function_call (SwfdecAsFunction *function, SwfdecAsObject *thisp, guint n_args, 
    const SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  swfdec_as_function_call_full (function, thisp, FALSE, 
      thisp ? thisp->prototype : SWFDEC_AS_OBJECT (function)->prototype, n_args, args, return_value);
}


G_END_DECLS
#endif
