/* Vivified
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

#include <swfdec/swfdec.h>
#include <vivified/core/vivi_application.h>

#ifndef _VIVI_FUNCTION_H_
#define _VIVI_FUNCTION_H_

G_BEGIN_DECLS


#define VIVI_FUNCTION(name, fun) \
  void fun (SwfdecAsContext *cx, SwfdecAsObject *this, guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval);

void		vivi_function_init_context	(ViviApplication *	app);


G_END_DECLS
#endif
