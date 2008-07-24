/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_GRAPHIC_H_
#define _SWFDEC_GRAPHIC_H_

#include <glib-object.h>
#include <swfdec/swfdec_character.h>
#include <swfdec/swfdec_player.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS


typedef struct _SwfdecGraphicClass SwfdecGraphicClass;

#define SWFDEC_TYPE_GRAPHIC                    (swfdec_graphic_get_type())
#define SWFDEC_IS_GRAPHIC(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_GRAPHIC))
#define SWFDEC_IS_GRAPHIC_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_GRAPHIC))
#define SWFDEC_GRAPHIC(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_GRAPHIC, SwfdecGraphic))
#define SWFDEC_GRAPHIC_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_GRAPHIC, SwfdecGraphicClass))
#define SWFDEC_GRAPHIC_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_GRAPHIC, SwfdecGraphicClass))

struct _SwfdecGraphic
{
  SwfdecCharacter	character;

  SwfdecRect		extents;	/* bounding box that encloses this graphic */
};

struct _SwfdecGraphicClass
{
  SwfdecCharacterClass	character_class;

  /* when creating a movie for this graphic, use this type. Must be a SWFDEC_TYPE_MOVIE */
  GType			movie_type;
  /* optional vfuncs */
  void			(* render)	(SwfdecGraphic *	      	graphic, 
                                         cairo_t *			cr,
					 const SwfdecColorTransform *	trans);
  gboolean		(* mouse_in)	(SwfdecGraphic *      		graphic,
					 double				x,
					 double				y);
};

GType		swfdec_graphic_get_type	(void);

void		swfdec_graphic_render	(SwfdecGraphic *		graphic,
                                         cairo_t *			cr,
					 const SwfdecColorTransform *	trans);
gboolean	swfdec_graphic_mouse_in	(SwfdecGraphic *      		graphic,
					 double				x,
					 double				y);
						

G_END_DECLS
#endif
