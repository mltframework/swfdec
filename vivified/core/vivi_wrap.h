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

#ifndef _VIVI_WRAP_H_
#define _VIVI_WRAP_H_

#include <swfdec/swfdec.h>
#include <vivified/core/vivi_application.h>

G_BEGIN_DECLS


typedef struct _ViviWrap ViviWrap;
typedef struct _ViviWrapClass ViviWrapClass;

#define VIVI_TYPE_WRAP                    (vivi_wrap_get_type())
#define VIVI_IS_WRAP(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_WRAP))
#define VIVI_IS_WRAP_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_WRAP))
#define VIVI_WRAP(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_WRAP, ViviWrap))
#define VIVI_WRAP_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_WRAP, ViviWrapClass))
#define VIVI_WRAP_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_WRAP, ViviWrapClass))

struct _ViviWrap
{
  SwfdecAsObject	object;

  SwfdecAsObject *	wrap;	  	/* the object we wrap */
};

struct _ViviWrapClass
{
  SwfdecAsObjectClass	object_class;
};

GType			vivi_wrap_get_type   	(void);

SwfdecAsObject *	vivi_wrap_object	(ViviApplication *	app,
						 SwfdecAsObject *	object);
void			vivi_wrap_value		(ViviApplication *	app,
						 SwfdecAsValue *	dest,
						 const SwfdecAsValue *	src);

G_END_DECLS
#endif
