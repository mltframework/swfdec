/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_SWF_INSTANCE_H_
#define _SWFDEC_SWF_INSTANCE_H_

#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_sprite_movie.h>

G_BEGIN_DECLS

//typedef struct _SwfdecSwfInstance SwfdecSwfInstance;
typedef struct _SwfdecSwfInstanceClass SwfdecSwfInstanceClass;

#define SWFDEC_TYPE_SWF_INSTANCE                    (swfdec_swf_instance_get_type())
#define SWFDEC_IS_SWF_INSTANCE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SWF_INSTANCE))
#define SWFDEC_IS_SWF_INSTANCE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SWF_INSTANCE))
#define SWFDEC_SWF_INSTANCE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SWF_INSTANCE, SwfdecSwfInstance))
#define SWFDEC_SWF_INSTANCE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SWF_INSTANCE, SwfdecSwfInstanceClass))

struct _SwfdecSwfInstance
{
  GObject		object;

  SwfdecSpriteMovie * 	movie;		/* the movie responsible for creating this instance */
  guint			parse_frame;	/* next frame to parse */

  SwfdecLoader *	loader;		/* the loader providing data for the decoder */
  SwfdecDecoder *	decoder;	/* decoder that decoded all the stuff used by us */

  GHashTable *		exports;	/* string->SwfdecCharacter mapping of exported characters */
  GHashTable *		export_names;	/* SwfdecCharacter->string mapping of exported characters */
};

struct _SwfdecSwfInstanceClass
{
  GObjectClass		object_class;
};

GType		swfdec_swf_instance_get_type	  	(void);

SwfdecSwfInstance *
		swfdec_swf_instance_new			(SwfdecSpriteMovie *  	movie,
							 SwfdecLoader *		loader);

void		swfdec_swf_instance_advance		(SwfdecSwfInstance *	instance);

gpointer	swfdec_swf_instance_get_export		(SwfdecSwfInstance *	root,
							 const char *		name);
const char *	swfdec_swf_instance_get_export_name    	(SwfdecSwfInstance *	root,
							 SwfdecCharacter *	character);

G_END_DECLS
#endif
