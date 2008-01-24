/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_CHARACTER_H_
#define _SWFDEC_CHARACTER_H_

#include <glib-object.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_rect.h>

G_BEGIN_DECLS
//typedef struct _SwfdecCharacter SwfdecCharacter;
typedef struct _SwfdecCharacterClass SwfdecCharacterClass;

#define SWFDEC_TYPE_CHARACTER                    (swfdec_character_get_type())
#define SWFDEC_IS_CHARACTER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_CHARACTER))
#define SWFDEC_IS_CHARACTER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_CHARACTER))
#define SWFDEC_CHARACTER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_CHARACTER, SwfdecCharacter))
#define SWFDEC_CHARACTER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_CHARACTER, SwfdecCharacterClass))
#define SWFDEC_CHARACTER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_CHARACTER, SwfdecCharacterClass))

struct _SwfdecCharacter
{
  GObject		object;

  int			id;		/* id of this character in the character list */
};

struct _SwfdecCharacterClass
{
  GObjectClass		object_class;
};

GType swfdec_character_get_type (void);
gpointer swfdec_character_new (SwfdecDecoder *dec, GType type);

gboolean swfdec_character_mouse_in (SwfdecCharacter *character,
    double x, double y, int button);
void swfdec_character_render (SwfdecCharacter *character, cairo_t *cr, 
    const SwfdecColorTransform *color, const SwfdecRect *inval, gboolean fill);


G_END_DECLS
#endif
