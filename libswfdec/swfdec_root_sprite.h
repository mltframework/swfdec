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

#ifndef _SWFDEC_ROOT_SPRITE_H_
#define _SWFDEC_ROOT_SPRITE_H_

#include <libswfdec/swfdec_sprite.h>

G_BEGIN_DECLS

typedef struct _SwfdecRootSpriteClass SwfdecRootSpriteClass;
typedef struct _SwfdecRootExportData SwfdecRootExportData;

typedef enum {
  SWFDEC_ROOT_ACTION_EXPORT,		/* contains a SwfdecExportData */
  SWFDEC_ROOT_ACTION_INIT_SCRIPT,	/* contains a SwfdecScript */
} SwfdecRootActionType;

struct _SwfdecRootExportData {
  char *		name;
  SwfdecCharacter *	character;
};

#define SWFDEC_TYPE_ROOT_SPRITE                    (swfdec_root_sprite_get_type())
#define SWFDEC_IS_ROOT_SPRITE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_ROOT_SPRITE))
#define SWFDEC_IS_ROOT_SPRITE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_ROOT_SPRITE))
#define SWFDEC_ROOT_SPRITE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_ROOT_SPRITE, SwfdecRootSprite))
#define SWFDEC_ROOT_SPRITE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_ROOT_SPRITE, SwfdecRootSpriteClass))

struct _SwfdecRootSprite
{
  SwfdecSprite		sprite;

  GArray **		root_actions;	/* n_frames of root actions */
};

struct _SwfdecRootSpriteClass
{
  SwfdecGraphicClass	graphic_class;
};

GType		swfdec_root_sprite_get_type	(void);

int		tag_func_export_assets		(SwfdecSwfDecoder *	s);
int		tag_func_do_init_action		(SwfdecSwfDecoder *	s);


G_END_DECLS
#endif
