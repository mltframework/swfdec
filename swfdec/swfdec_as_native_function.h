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

#ifndef _SWFDEC_AS_NATIVE_FUNCTION_H_
#define _SWFDEC_AS_NATIVE_FUNCTION_H_

#include <swfdec/swfdec_as_function.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsNativeFunction SwfdecAsNativeFunction;
typedef struct _SwfdecAsNativeFunctionClass SwfdecAsNativeFunctionClass;

#define SWFDEC_TYPE_AS_NATIVE_FUNCTION                    (swfdec_as_native_function_get_type())
#define SWFDEC_IS_AS_NATIVE_FUNCTION(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_NATIVE_FUNCTION))
#define SWFDEC_IS_AS_NATIVE_FUNCTION_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_NATIVE_FUNCTION))
#define SWFDEC_AS_NATIVE_FUNCTION(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_NATIVE_FUNCTION, SwfdecAsNativeFunction))
#define SWFDEC_AS_NATIVE_FUNCTION_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_NATIVE_FUNCTION, SwfdecAsNativeFunctionClass))
#define SWFDEC_AS_NATIVE_FUNCTION_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_NATIVE_FUNCTION, SwfdecAsNativeFunctionClass))

/* FIXME: do two obejcts, one for scripts and one for native? */
struct _SwfdecAsNativeFunction {
  /*< private >*/
  SwfdecAsFunction	function;

  SwfdecAsNative	native;		/* native call or NULL when script */
  char *		name;		/* function name */
};

struct _SwfdecAsNativeFunctionClass {
  SwfdecAsFunctionClass	function_class;
};

GType		swfdec_as_native_function_get_type	(void);

SwfdecAsFunction *swfdec_as_native_function_new	(SwfdecAsContext *	context,
						 const char *		name,
						 SwfdecAsNative		native);

gboolean	swfdec_as_native_function_check	(SwfdecAsContext *	cx,
						 SwfdecAsObject *	object,
						 GType			type,
						 gpointer *		result,
						 guint			argc,
						 SwfdecAsValue *	argv,
						 const char *	      	args,
						 ...);
gboolean	swfdec_as_native_function_checkv(SwfdecAsContext *	cx,
						 SwfdecAsObject *	object,
						 GType			type,
						 gpointer *		result,
						 guint			argc,
						 SwfdecAsValue *	argv,
						 const char *	      	args,
						 va_list		varargs);
#define SWFDEC_AS_CHECK(type,result,...) G_STMT_START {\
  if (!swfdec_as_native_function_check (cx, object, type, (gpointer) result, argc, argv, __VA_ARGS__)) \
    return; \
}G_STMT_END


G_END_DECLS
#endif
