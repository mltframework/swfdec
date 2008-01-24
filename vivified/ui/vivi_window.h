/* Vivi
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _VIVI_WINDOW_H_
#define _VIVI_WINDOW_H_

#include <vivified/dock/vivified-dock.h>
#include <vivified/core/vivified-core.h>
/* FIXME */
#include <swfdec/swfdec_movie.h>

G_BEGIN_DECLS

typedef struct _ViviWindow ViviWindow;
typedef struct _ViviWindowClass ViviWindowClass;

#define VIVI_TYPE_WINDOW                    (vivi_window_get_type())
#define VIVI_IS_WINDOW(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_WINDOW))
#define VIVI_IS_WINDOW_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_WINDOW))
#define VIVI_WINDOW(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_WINDOW, ViviWindow))
#define VIVI_WINDOW_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_WINDOW, ViviWindowClass))

struct _ViviWindow
{
  GtkWindow     	window;

  ViviApplication *	app;		/* application we are displaying */

  SwfdecMovie *		movie;		/* the currently active movie or NULL if none */
};

struct _ViviWindowClass
{
  GtkWindowClass	window_class;
};

GType			vivi_window_get_type		(void);

GtkWidget *		vivi_window_new			(ViviApplication *	app);

ViviApplication *	vivi_window_get_application	(ViviWindow *		window);
void			vivi_window_set_movie		(ViviWindow *		window,
							 SwfdecMovie *		movie);
SwfdecMovie *		vivi_window_get_movie		(ViviWindow *		window);


G_END_DECLS
#endif
