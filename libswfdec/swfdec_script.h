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

#ifndef _SWFDEC_SCRIPT_H_
#define _SWFDEC_SCRIPT_H_

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/swfdec_bits.h>
#include <libswfdec/js/jsapi.h>

G_BEGIN_DECLS

//typedef struct _SwfdecScript SwfdecScript;

typedef gboolean (* SwfdecScriptForeachFunc) (gconstpointer bytecode, guint action, 
    const guint8 *data, guint len, gpointer user_data);

/* FIXME: May want to typedef to SwfdecBuffer directly */
struct _SwfdecScript {
  SwfdecBuffer *	buffer;			/* buffer holding the script */
  unsigned int	  	refcount;		/* reference count */
  char *		name;			/* name identifying this script */
  unsigned int		version;		/* version of the script */
  gpointer		debugger;		/* debugger owning us or NULL */
  /* needed by functions */
  SwfdecBuffer *	constant_pool;		/* constant pool action */
};

const char *	swfdec_action_get_name		(guint			action);
guint		swfdec_action_get_from_name	(const char *		name);

SwfdecScript *	swfdec_script_new		(SwfdecBits *		bits,
						 const char *		name,
						 unsigned int	      	version);
SwfdecScript *	swfdec_script_new_for_player  	(SwfdecPlayer *		player,
						 SwfdecBits *		bits,
						 const char *		name,
						 unsigned int	      	version);
void		swfdec_script_ref		(SwfdecScript *		script);
void		swfdec_script_unref		(SwfdecScript *		script);

JSBool		swfdec_script_interpret		(SwfdecScript *		script,
						 JSContext *		cx,
						 jsval *		rval);
jsval		swfdec_script_execute		(SwfdecScript *		script,
						 SwfdecScriptable *	scriptable);

gboolean	swfdec_script_foreach		(SwfdecScript *		script,
						 SwfdecScriptForeachFunc func,
						 gpointer		user_data);
char *		swfdec_script_print_action	(guint			action,
						 const guint8 *		data,
						 guint			len);

G_END_DECLS

#endif
