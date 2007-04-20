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

#ifndef _SWFDEC_AS_INTERPRET_H_
#define _SWFDEC_AS_INTERPRET_H_

#include <libswfdec/swfdec_as_types.h>

G_BEGIN_DECLS

/* defines minimum and maximum versions for which we have seperate scripts */
#define SWFDEC_AS_MIN_SCRIPT_VERSION 3
#define SWFDEC_AS_MAX_SCRIPT_VERSION 7
#define SWFDEC_AS_EXTRACT_SCRIPT_VERSION(v) MIN ((v) - SWFDEC_AS_MIN_SCRIPT_VERSION, SWFDEC_AS_MAX_SCRIPT_VERSION - SWFDEC_AS_MIN_SCRIPT_VERSION)

typedef void (* SwfdecActionExec) (SwfdecAsContext *cx, guint action, const guint8 *data, guint len);
typedef struct {
  const char *		name;		/* name identifying the action */
  char *		(* print)	(guint action, const guint8 *data, guint len);
  int			remove;		/* values removed from stack or -1 for dynamic */
  int			add;		/* values added to the stack or -1 for dynamic */
  SwfdecActionExec	exec[SWFDEC_AS_MAX_SCRIPT_VERSION - SWFDEC_AS_MIN_SCRIPT_VERSION + 1];
					/* array is for version 3, 4, 5, 6, 7+ */
} SwfdecActionSpec;

extern const SwfdecActionSpec swfdec_as_actions[256];

G_END_DECLS
#endif
