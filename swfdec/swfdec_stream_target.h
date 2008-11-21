/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef __SWFDEC_STREAM_TARGET_H__
#define __SWFDEC_STREAM_TARGET_H__

#include <swfdec/swfdec.h>

G_BEGIN_DECLS


#define SWFDEC_TYPE_STREAM_TARGET                (swfdec_stream_target_get_type ())
#define SWFDEC_STREAM_TARGET(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_STREAM_TARGET, SwfdecStreamTarget))
#define SWFDEC_IS_STREAM_TARGET(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_STREAM_TARGET))
#define SWFDEC_STREAM_TARGET_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), SWFDEC_TYPE_STREAM_TARGET, SwfdecStreamTargetInterface))

typedef struct _SwfdecStreamTarget SwfdecStreamTarget; /* dummy object */
typedef struct _SwfdecStreamTargetInterface SwfdecStreamTargetInterface;

struct _SwfdecStreamTargetInterface {
  GTypeInterface	parent;

  /* mandatory vfunc */
  SwfdecPlayer *	(* get_player)      	(SwfdecStreamTarget *	target);
  /* optional vfuncs */
  void			(* open)		(SwfdecStreamTarget *   target,
						 SwfdecStream *		stream);
  gboolean		(* parse)		(SwfdecStreamTarget *   target,
						 SwfdecStream *		stream);
  void			(* close)		(SwfdecStreamTarget *   target,
						 SwfdecStream *		stream);
  void			(* error)		(SwfdecStreamTarget *   target,
						 SwfdecStream *		stream);
  void			(* writable)		(SwfdecStreamTarget *	target,
						 SwfdecStream *		stream);
};

GType		swfdec_stream_target_get_type		(void) G_GNUC_CONST;

SwfdecPlayer *	swfdec_stream_target_get_player		(SwfdecStreamTarget *	target);
void		swfdec_stream_target_open		(SwfdecStreamTarget *	target,
							 SwfdecStream *		stream);
gboolean	swfdec_stream_target_parse		(SwfdecStreamTarget *	target,
							 SwfdecStream *		stream);
void		swfdec_stream_target_close		(SwfdecStreamTarget *	target,
							 SwfdecStream *		stream);
void		swfdec_stream_target_error		(SwfdecStreamTarget *	target,
							 SwfdecStream *		stream);
void		swfdec_stream_target_writable		(SwfdecStreamTarget *	target,
							 SwfdecStream *		stream);


G_END_DECLS

#endif /* __SWFDEC_STREAM_TARGET_H__ */
