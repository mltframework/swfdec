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

#ifndef _VIVI_DOCKER_H_
#define _VIVI_DOCKER_H_

#include <gtk/gtk.h>
#include <vivified/dock/vivi_docklet.h>

G_BEGIN_DECLS


typedef struct _ViviDocker ViviDocker;
typedef struct _ViviDockerClass ViviDockerClass;

#define VIVI_TYPE_DOCKER                    (vivi_docker_get_type())
#define VIVI_IS_DOCKER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_DOCKER))
#define VIVI_IS_DOCKER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_DOCKER))
#define VIVI_DOCKER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_DOCKER, ViviDocker))
#define VIVI_DOCKER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_DOCKER, ViviDockerClass))
#define VIVI_DOCKER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_DOCKER, ViviDockerClass))

struct _ViviDocker {
  GtkExpander		bin;
};

struct _ViviDockerClass
{
  GtkExpanderClass	bin_class;

  void			(* request_close)	(ViviDocker *	docker);
};

GType			vivi_docker_get_type   	(void);

GtkWidget *		vivi_docker_new		(ViviDocklet *	docklet);

G_END_DECLS
#endif
