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

#ifndef _SWFDEC_PLAYER_SCRIPTING_H_
#define _SWFDEC_PLAYER_SCRIPTING_H_

#include <glib-object.h>
#include <swfdec/swfdec_player.h>

G_BEGIN_DECLS

/* forward-declared in swfdec-player.h */
/* typedef struct _SwfdecPlayerScripting SwfdecPlayerScripting; */
typedef struct _SwfdecPlayerScriptingClass SwfdecPlayerScriptingClass;

#define SWFDEC_TYPE_PLAYER_SCRIPTING                    (swfdec_player_scripting_get_type())
#define SWFDEC_IS_PLAYER_SCRIPTING(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_PLAYER_SCRIPTING))
#define SWFDEC_IS_PLAYER_SCRIPTING_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_PLAYER_SCRIPTING))
#define SWFDEC_PLAYER_SCRIPTING(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_PLAYER_SCRIPTING, SwfdecPlayerScripting))
#define SWFDEC_PLAYER_SCRIPTING_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_PLAYER_SCRIPTING, SwfdecPlayerScriptingClass))
#define SWFDEC_PLAYER_SCRIPTING_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_PLAYER_SCRIPTING, SwfdecPlayerScriptingClass))

struct _SwfdecPlayerScripting
{
  GObject		object;
};

struct _SwfdecPlayerScriptingClass
{
  /*< private >*/
  GObjectClass		object_class;

  /*< public >*/
  char *		(* js_get_id)	(SwfdecPlayerScripting *scripting,
					 SwfdecPlayer *		player);
  char *		(* js_call)	(SwfdecPlayerScripting *scripting,
					 SwfdecPlayer *         player,
					 const char *		code);
  char *		(* xml_call)	(SwfdecPlayerScripting *scripting,
					 SwfdecPlayer *         player,
					 const char *		xml);
};

GType		swfdec_player_scripting_get_type		(void);


G_END_DECLS
#endif
