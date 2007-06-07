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

#ifndef __SWFDEC_LOADER_TARGET_H__
#define __SWFDEC_LOADER_TARGET_H__

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_decoder.h>

G_BEGIN_DECLS

#define SWFDEC_TYPE_LOADER_TARGET                (swfdec_loader_target_get_type ())
#define SWFDEC_LOADER_TARGET(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_LOADER_TARGET, SwfdecLoaderTarget))
#define SWFDEC_IS_LOADER_TARGET(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_LOADER_TARGET))
#define SWFDEC_LOADER_TARGET_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), SWFDEC_TYPE_LOADER_TARGET, SwfdecLoaderTargetInterface))

typedef struct _SwfdecLoaderTarget SwfdecLoaderTarget; /* dummy object */
typedef struct _SwfdecLoaderTargetInterface SwfdecLoaderTargetInterface;

struct _SwfdecLoaderTargetInterface {
  GTypeInterface	parent;

  /* mandatory vfuncs */
  SwfdecPlayer *	(* get_player)      	(SwfdecLoaderTarget *	target);
  void			(* parse)		(SwfdecLoaderTarget *   target,
						 SwfdecLoader *		loader);
};

GType		swfdec_loader_target_get_type		(void) G_GNUC_CONST;

SwfdecPlayer *	swfdec_loader_target_get_player		(SwfdecLoaderTarget *	target);
void		swfdec_loader_target_parse		(SwfdecLoaderTarget *	target,
							 SwfdecLoader *		loader);

G_END_DECLS

#endif /* __SWFDEC_LOADER_TARGET_H__ */
