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

#ifndef _VIVI_APPLICATION_H_
#define _VIVI_APPLICATION_H_

#include <libswfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _ViviApplication ViviApplication;
typedef struct _ViviApplicationClass ViviApplicationClass;

#define VIVI_TYPE_APPLICATION                    (vivi_application_get_type())
#define VIVI_IS_APPLICATION(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_APPLICATION))
#define VIVI_IS_APPLICATION_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_APPLICATION))
#define VIVI_APPLICATION(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_APPLICATION, ViviApplication))
#define VIVI_APPLICATION_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_APPLICATION, ViviApplicationClass))
#define VIVI_APPLICATION_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_APPLICATION, ViviApplicationClass))

struct _ViviApplication
{
  SwfdecAsContext	context;

  char *		filename;	/* name of the file we play back or NULL if none set yet */
  SwfdecPlayer *	player;		/* the current player */
};

struct _ViviApplicationClass
{
  SwfdecAsContextClass	context_class;
};

GType			vivi_application_get_type   	(void);

ViviApplication *	vivi_application_new		(void);

void			vivi_application_set_filename	(ViviApplication *	app,
							 const char *		filename);
const char *		vivi_application_get_filename	(ViviApplication *	app);
SwfdecPlayer *	      	vivi_application_get_player	(ViviApplication *	app);

void			vivi_application_reset		(ViviApplication *	app);
void			vivi_application_run		(ViviApplication *	app,
							 const char *		command);

G_END_DECLS
#endif
