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

#ifndef _SWFDEC_SCRIPT_INTERNAL_H_
#define _SWFDEC_SCRIPT_INTERNAL_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_constant_pool.h>

G_BEGIN_DECLS

typedef struct _SwfdecScriptArgument SwfdecScriptArgument;

typedef enum {
  SWFDEC_SCRIPT_PRELOAD_THIS = (1 << 0),
  SWFDEC_SCRIPT_SUPPRESS_THIS = (1 << 1),
  SWFDEC_SCRIPT_PRELOAD_ARGS = (1 << 2),
  SWFDEC_SCRIPT_SUPPRESS_ARGS = (1 << 3),
  SWFDEC_SCRIPT_PRELOAD_SUPER = (1 << 4),
  SWFDEC_SCRIPT_SUPPRESS_SUPER = (1 << 5),
  SWFDEC_SCRIPT_PRELOAD_ROOT = (1 << 6),
  SWFDEC_SCRIPT_PRELOAD_PARENT = (1 << 7),
  SWFDEC_SCRIPT_PRELOAD_GLOBAL = (1 << 8)
} SwfdecScriptFlag;

typedef gboolean (* SwfdecScriptForeachFunc) (gconstpointer bytecode, guint action, 
    const guint8 *data, guint len, gpointer user_data);

/* FIXME: May want to typedef to SwfdecBuffer directly */
struct _SwfdecScript {
  SwfdecBuffer *	buffer;			/* buffer holding the script */
  const guint8 *	main;			/* entry point for script */
  const guint8 *	exit;			/* exit point for script */
  guint		 	refcount;		/* reference count */
  char *		name;			/* name identifying this script */
  guint			version;		/* version of the script */
  guint			n_registers;		/* number of registers */
  SwfdecBuffer *	constant_pool;		/* constant pool action */
  guint			flags;			/* SwfdecScriptFlags */
  guint			n_arguments;  		/* number of arguments */
  SwfdecScriptArgument *arguments;		/* arguments or NULL if none */
};

struct _SwfdecScriptArgument {
  char *		name;			/* name of the argument (not GC'ed) */
  guint			preload;		/* preload slot to preload to or 0 */
};

const char *	swfdec_action_get_name		(guint			action);
guint		swfdec_action_get_from_name	(const char *		name);

SwfdecScript *	swfdec_script_new_from_bits   		(SwfdecBits *			bits,
							 const char *			name,
							 guint				version);

gboolean	swfdec_script_foreach			(SwfdecScript *			script,
							 SwfdecScriptForeachFunc	func,
							 gpointer			user_data);

G_END_DECLS

#endif
