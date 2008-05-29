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

#include <swfdec/swfdec_player.h>
#include <swfdec/swfdec_sandbox.h>
#include <swfdec/swfdec_sprite_movie.h>

G_BEGIN_DECLS

//typedef struct _SwfdecResource SwfdecResource;
typedef struct _SwfdecResourceClass SwfdecResourceClass;

#define SWFDEC_TYPE_RESOURCE                    (swfdec_resource_get_type())
#define SWFDEC_IS_RESOURCE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_RESOURCE))
#define SWFDEC_IS_RESOURCE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_RESOURCE))
#define SWFDEC_RESOURCE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_RESOURCE, SwfdecResource))
#define SWFDEC_RESOURCE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_RESOURCE, SwfdecResourceClass))

typedef enum {
  SWFDEC_RESOURCE_NEW = 0,	      	/* no loader set yet, only the call to _load() was done */
  SWFDEC_RESOURCE_REQUESTED,		/* the URL has been requested, the request was ok, ->loader is set */
  SWFDEC_RESOURCE_OPENED,		/* onLoadStart has been called */
  SWFDEC_RESOURCE_COMPLETE,		/* onLoadComplete has been called */
  SWFDEC_RESOURCE_DONE			/* onLoadInit has been called, clip_loader is unset */
} SwfdecResourceState;

struct _SwfdecResource
{
  SwfdecAsObject      	object;

  guint			version;	/* version of this resource */
  SwfdecSandbox *	sandbox;	/* sandbox this resource belongs to (only NULL for a short time on very first loader) */
  SwfdecSpriteMovie * 	movie;		/* the movie responsible for creating this instance */

  SwfdecLoader *	loader;		/* the loader providing data for the decoder */
  SwfdecDecoder *	decoder;	/* decoder in use or NULL if broken file */
  char *		variables;	/* extra variables to be set */

  GHashTable *		exports;	/* string->SwfdecCharacter mapping of exported characters */
  GHashTable *		export_names;	/* SwfdecCharacter->string mapping of exported characters */

  /* only used while loading */
  SwfdecResourceState	state;		/* state we're in (for determining callbacks */
  char *		target;		/* target path we use for signalling */
  SwfdecMovieClipLoader *clip_loader;	/* loader that gets notified about load events */
  SwfdecSandbox *	clip_loader_sandbox; /* sandbox used for events on the clip loader */
};

struct _SwfdecResourceClass
{
  SwfdecAsObjectClass 	object_class;
};

GType		swfdec_resource_get_type	  	(void);

SwfdecResource *swfdec_resource_new			(SwfdecPlayer *		player,
							 SwfdecLoader *		loader,
							 const char *		variables);

gboolean	swfdec_resource_emit_on_load_init	(SwfdecResource *	resource);
void		swfdec_resource_add_export		(SwfdecResource *	instance,
							 SwfdecCharacter *	character,
							 const char * 		name);
gpointer	swfdec_resource_get_export		(SwfdecResource *	root,
							 const char *		name);
const char *	swfdec_resource_get_export_name    	(SwfdecResource *	root,
							 SwfdecCharacter *	character);

void		swfdec_resource_load			(SwfdecPlayer *		player,
							 const char *		target,
							 const char *		url,
							 SwfdecBuffer *		buffer,
							 SwfdecMovieClipLoader *loader,
							 gboolean		target_is_movie);


G_END_DECLS
#endif
