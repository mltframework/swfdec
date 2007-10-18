/* Swfdec
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

#ifndef _SWFDEC_RESOURCE_H_
#define _SWFDEC_RESOURCE_H_

#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_flash_security.h>
#include <libswfdec/swfdec_sprite_movie.h>

G_BEGIN_DECLS

//typedef struct _SwfdecResource SwfdecResource;
typedef struct _SwfdecResourceClass SwfdecResourceClass;

#define SWFDEC_TYPE_RESOURCE                    (swfdec_resource_get_type())
#define SWFDEC_IS_RESOURCE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_RESOURCE))
#define SWFDEC_IS_RESOURCE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_RESOURCE))
#define SWFDEC_RESOURCE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_RESOURCE, SwfdecResource))
#define SWFDEC_RESOURCE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_RESOURCE, SwfdecResourceClass))

struct _SwfdecResource
{
  SwfdecFlashSecurity	flash_security;

  SwfdecSpriteMovie * 	movie;		/* the movie responsible for creating this instance */
  guint			parse_frame;	/* next frame to parse */

  SwfdecLoader *	loader;		/* the loader providing data for the decoder */
  SwfdecDecoder *	decoder;	/* decoder that decoded all the stuff used by us */
  char *		variables;	/* extra variables to be set */

  GHashTable *		exports;	/* string->SwfdecCharacter mapping of exported characters */
  GHashTable *		export_names;	/* SwfdecCharacter->string mapping of exported characters */
};

struct _SwfdecResourceClass
{
  SwfdecFlashSecurityClass	flash_security_class;
};

GType		swfdec_resource_get_type	  	(void);

SwfdecResource *
		swfdec_resource_new			(SwfdecSpriteMovie *  	movie,
							 SwfdecLoader *		loader,
							 const char *		variables);

void		swfdec_resource_advance			(SwfdecResource *	instance);

gpointer	swfdec_resource_get_export		(SwfdecResource *	root,
							 const char *		name);
const char *	swfdec_resource_get_export_name    	(SwfdecResource *	root,
							 SwfdecCharacter *	character);

G_END_DECLS
#endif
