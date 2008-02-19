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

#ifndef _SWFDEC_GTK_SYSTEM_H_
#define _SWFDEC_GTK_SYSTEM_H_

#include <swfdec/swfdec.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _SwfdecGtkSystem SwfdecGtkSystem;
typedef struct _SwfdecGtkSystemPrivate SwfdecGtkSystemPrivate;
typedef struct _SwfdecGtkSystemClass SwfdecGtkSystemClass;

#define SWFDEC_TYPE_GTK_SYSTEM                    (swfdec_gtk_system_get_type())
#define SWFDEC_IS_GTK_SYSTEM(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_GTK_SYSTEM))
#define SWFDEC_IS_GTK_SYSTEM_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_GTK_SYSTEM))
#define SWFDEC_GTK_SYSTEM(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_GTK_SYSTEM, SwfdecGtkSystem))
#define SWFDEC_GTK_SYSTEM_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_GTK_SYSTEM, SwfdecGtkSystemClass))
#define SWFDEC_GTK_SYSTEM_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_GTK_SYSTEM, SwfdecGtkSystemClass))

struct _SwfdecGtkSystem
{
  SwfdecSystem	  		system;

  SwfdecGtkSystemPrivate *	priv;
};

struct _SwfdecGtkSystemClass
{
  SwfdecSystemClass     	system_class;
};

GType 		swfdec_gtk_system_get_type    	(void);

SwfdecSystem *	swfdec_gtk_system_new	      	(GdkScreen *		screen);


G_END_DECLS
#endif
