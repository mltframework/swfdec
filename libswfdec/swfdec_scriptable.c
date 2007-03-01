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

#include "swfdec_scriptable.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "js/jsapi.h"

G_DEFINE_ABSTRACT_TYPE (SwfdecScriptable, swfdec_scriptable, G_TYPE_OBJECT)

static void
swfdec_scriptable_dispose (GObject *object)
{
  SwfdecScriptable *script = SWFDEC_SCRIPTABLE (object);

  g_assert (script->jsobj == NULL);

  G_OBJECT_CLASS (swfdec_scriptable_parent_class)->dispose (object);
}

static JSObject *
swfdec_scriptable_create_js_object (SwfdecScriptable *scriptable)
{
  SwfdecScriptableClass *klass;
  JSContext *cx;
  JSObject *obj;

  klass = SWFDEC_SCRIPTABLE_GET_CLASS (scriptable);
  g_return_val_if_fail (klass->jsclass != NULL, NULL);
  cx = scriptable->jscx;

  obj = JS_NewObject (cx, (JSClass *) klass->jsclass, NULL, NULL);
  if (obj == NULL) {
    SWFDEC_ERROR ("failed to create JS object for %s %p", 
	G_OBJECT_TYPE_NAME (scriptable), scriptable);
    return NULL;
  }
  SWFDEC_LOG ("created JSObject %p for %s %p", obj, 
      G_OBJECT_TYPE_NAME (scriptable), scriptable);
  g_object_ref (scriptable);
  JS_SetPrivate (cx, obj, scriptable);
  return obj;
}

static void
swfdec_scriptable_class_init (SwfdecScriptableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_scriptable_dispose;

  klass->create_js_object = swfdec_scriptable_create_js_object;
}

static void
swfdec_scriptable_init (SwfdecScriptable *stream)
{
}

/*** PUBLIC API ***/

/**
 * swfdec_scriptable_finalize:
 * @cx: a #JSContext
 * @obj: a #JSObject to finalize
 *
 * This function is intended to be used as the finalizer in the #JSClass used
 * by a scriptable subtype.
 **/
void
swfdec_scriptable_finalize (JSContext *cx, JSObject *obj)
{
  SwfdecScriptable *script;

  script = JS_GetPrivate (cx, obj);
  /* since we also finalize the prototype, not everyone has a private object */
  if (script) {
    g_assert (SWFDEC_IS_SCRIPTABLE (script));
    g_assert (script->jsobj != NULL);

    SWFDEC_LOG ("destroying JSObject %p for %s %p", obj, 
	G_OBJECT_TYPE_NAME (script), script);
    script->jsobj = NULL;
    g_object_unref (script);
  } else {
    SWFDEC_LOG ("destroying JSObject %p without Scriptable (probably a prototype)", obj);
  }
}

JSObject *
swfdec_scriptable_get_object (SwfdecScriptable *scriptable)
{
  SwfdecScriptableClass *klass;

  g_return_val_if_fail (SWFDEC_IS_SCRIPTABLE (scriptable), NULL);

  if (scriptable->jsobj)
    return scriptable->jsobj;
  klass = SWFDEC_SCRIPTABLE_GET_CLASS (scriptable);
  g_return_val_if_fail (klass->create_js_object, NULL);
  scriptable->jsobj = klass->create_js_object (scriptable);

  return scriptable->jsobj;
}

/**
 * swfdec_scriptable_from_jsval:
 * @cx: a #JSContext
 * @val: the jsval to convert
 * @type: type of the object to get.
 *
 * Converts the given value @val to a #SwfdecScriptable, if it represents one.
 * The object must be of @type, otherwise %NULL will be returned.
 *
 * Returns: the scriptable represented by @val or NULL if @val does not 
 *          reference a @scriptable
 **/
gpointer
swfdec_scriptable_from_jsval (JSContext *cx, jsval val, GType type)
{
  SwfdecScriptableClass *klass;
  JSObject *object;

  g_return_val_if_fail (g_type_is_a (type, SWFDEC_TYPE_SCRIPTABLE), NULL);

  if (!JSVAL_IS_OBJECT (val))
    return NULL;
  if (JSVAL_IS_NULL (val))
    return NULL;
  object = JSVAL_TO_OBJECT (val);
  klass = g_type_class_peek (type);
  if (klass == NULL)
    return NULL; /* class doesn't exist -> no object of this type exists */
  if (!JS_InstanceOf (cx, object, klass->jsclass, NULL))
    return NULL;
  return JS_GetPrivate (cx, object);
}

/**
 * swfdec_scriptable_set_variables:
 * @script: a #SwfdecScriptable
 * @variables: variables to set on @script in application-x-www-form-urlencoded 
 *             format
 * 
 * Verifies @variables to be encoded correctly and sets them as string 
 * properties on the JSObject of @script.
 **/
void
swfdec_scriptable_set_variables (SwfdecScriptable *script, const char *variables)
{
  JSObject *object;

  g_return_if_fail (SWFDEC_IS_SCRIPTABLE (script));
  g_return_if_fail (variables != NULL);

  SWFDEC_DEBUG ("setting variables on %p: %s", script, variables);
  object = swfdec_scriptable_get_object (script);
  while (TRUE) {
    char *name, *value;
    JSString *string;
    jsval val;

    if (!swfdec_urldecode_one (variables, &name, &value, &variables)) {
      SWFDEC_WARNING ("variables invalid at \"%s\"", variables);
      break;
    }
    if (*variables != '\0' && *variables != '&') {
      SWFDEC_WARNING ("variables not delimited with & at \"%s\"", variables);
      g_free (name);
      g_free (value);
      break;
    }
    string = JS_NewStringCopyZ (script->jscx, value);
    if (string == NULL) {
      g_free (name);
      g_free (value);
      SWFDEC_ERROR ("could not create string");
      break;
    }
    val = STRING_TO_JSVAL (string);
    if (!JS_SetProperty (script->jscx, object, name, &val)) {
      g_free (name);
      g_free (value);
      SWFDEC_ERROR ("error setting property \"%s\"", name);
      break;
    }
    SWFDEC_DEBUG ("Set variable \"%s\" to \"%s\"", name, value);
    g_free (name);
    g_free (value);
    if (*variables == '\0')
      break;
    variables++;
  }
}

