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

#ifndef _SWFDEC_AS_SCRIPT_FUNCTION_H_
#define _SWFDEC_AS_SCRIPT_FUNCTION_H_

#include <swfdec/swfdec_as_function.h>
#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_sandbox.h>
#include <swfdec/swfdec_script.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsScriptFunction SwfdecAsScriptFunction;
typedef struct _SwfdecAsScriptFunctionClass SwfdecAsScriptFunctionClass;

#define SWFDEC_TYPE_AS_SCRIPT_FUNCTION                    (swfdec_as_script_function_get_type())
#define SWFDEC_IS_AS_SCRIPT_FUNCTION(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_SCRIPT_FUNCTION))
#define SWFDEC_IS_AS_SCRIPT_FUNCTION_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_SCRIPT_FUNCTION))
#define SWFDEC_AS_SCRIPT_FUNCTION(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_SCRIPT_FUNCTION, SwfdecAsScriptFunction))
#define SWFDEC_AS_SCRIPT_FUNCTION_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_SCRIPT_FUNCTION, SwfdecAsScriptFunctionClass))
#define SWFDEC_AS_SCRIPT_FUNCTION_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_SCRIPT_FUNCTION, SwfdecAsScriptFunctionClass))

/* FIXME: do two obejcts, one for scripts and one for native? */
struct _SwfdecAsScriptFunction {
  SwfdecAsFunction	function;

  /* for script script_functions */
  SwfdecScript *	script;		/* script being executed or NULL when native */
  GSList *		scope_chain;  	/* scope this script_function was defined in */
  SwfdecAsObject *	target;		/* target this object was defined in or NULL if in init script */
  SwfdecSandbox *	sandbox;	/* sandbox this function was defined in or NULL if don't care */
};

struct _SwfdecAsScriptFunctionClass {
  SwfdecAsFunctionClass	function_class;
};

GType			swfdec_as_script_function_get_type	(void);

SwfdecAsFunction *	swfdec_as_script_function_new		(SwfdecAsObject *	target,
								 const GSList *		scope_chain,
								 SwfdecScript *		script);


G_END_DECLS
#endif
