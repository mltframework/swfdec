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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_player_scripting.h"
#include "swfdec_debug.h"

/*** GTK-DOC ***/

/**
 * SECTION:SwfdecPlayerScripting
 * @title: SwfdecPlayerScripting
 * @short_description: integration with external scripts
 *
 * The @SwfdecPlayerScripting object is used to implement the ExternalInterface
 * object inside Flash. By providing your own custom implementation of this 
 * object using swfdec_player_set_scripting(), you can allow a Flash file to 
 * call your own custom functions. And you can call functions inside the Flash 
 * player.
 *
 * The functions are emitted using a custom XML format. The same format is used
 * to call functions in the Flash player. FIXME: describe the XML format in use. 
 */

/**
 * SwfdecPlayerScripting:
 *
 * The object you have to create.
 */

/**
 * SwfdecPlayerScriptingClass:
 * @js_get_id: optional function. Returns the string by which the scripting 
 *             object can reference itself in calls to the @js_call function.
 * @js_call: optional function. Emits Javascript code that you have to 
 *           interpret. You must also implement the @js_get_id function to make
 *           this function work. The return value must be in the custom XML 
 *           format.
 * @xml_call: optional function. If the Javascript functions aren't implemented,
 *            an xml format is used to encode the function call and the call is
 *            then done using this function. The return value is to be provided
 *            in the same custom XML format.
 *
 * You have to implement the virtual functions in this object to get a working
 * scripting implementation.
 */

/*** SWFDEC_PLAYER_SCRIPTING ***/

G_DEFINE_ABSTRACT_TYPE (SwfdecPlayerScripting, swfdec_player_scripting, G_TYPE_OBJECT)

static void
swfdec_player_scripting_class_init (SwfdecPlayerScriptingClass *klass)
{
}

static void
swfdec_player_scripting_init (SwfdecPlayerScripting *player_scripting)
{
}

