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

#include "swfdec_script.h"
#include "swfdec_script_internal.h"
#include "swfdec_as_context.h"
#include "swfdec_as_interpret.h"
#include "swfdec_debug.h"

/**
 * SwfdecScript:
 *
 * This is the object used for code to be executed by Swfdec. Scripts are 
 * independant from the #SwfdecAsContext they are executed in, so you can 
 * execute the same script in multiple contexts.
 */

/* Define this to get SWFDEC_WARN'd about missing properties of objects.
 * This can be useful to find out about unimplemented native properties,
 * but usually just causes a lot of spam. */
//#define SWFDEC_WARN_MISSING_PROPERTIES

/*** SUPPORT FUNCTIONS ***/

char *
swfdec_script_print_action (guint action, const guint8 *data, guint len)
{
  const SwfdecActionSpec *spec = swfdec_as_actions + action;

  if (action & 0x80) {
    if (spec->print == NULL) {
      SWFDEC_ERROR ("action %u 0x%02X %s has no print statement",
	  action, action, spec->name ? spec->name : "Unknown");
      return g_strdup_printf ("erroneous action %s",
	  spec->name ? spec->name : "Unknown");
    }
    return spec->print (action, data, len);
  } else {
    if (spec->name == NULL) {
      SWFDEC_ERROR ("action %u is unknown", action);
      return g_strdup_printf ("unknown Action 0x%02X", action);
    }
    return g_strdup (spec->name);
  }
}

static gboolean
swfdec_script_foreach_internal (SwfdecBits *bits, SwfdecScriptForeachFunc func, gpointer user_data)
{
  guint action, len;
  const guint8 *data;
  gconstpointer bytecode;

  bytecode = bits->ptr;
  while (swfdec_bits_left (bits) && (action = swfdec_bits_get_u8 (bits))) {
    if (action & 0x80) {
      len = swfdec_bits_get_u16 (bits);
      data = bits->ptr;
    } else {
      len = 0;
      data = NULL;
    }
    if (swfdec_bits_skip_bytes (bits, len) != len) {
      SWFDEC_ERROR ("script too short");
      return FALSE;
    }
    if (!func (bytecode, action, data, len, user_data))
      return FALSE;
    bytecode = bits->ptr;
  }
  return TRUE;
}

static gboolean
validate_action (gconstpointer bytecode, guint action, const guint8 *data, guint len, gpointer scriptp)
{
  // TODO: get rid of this function
  return TRUE;
}

/*** PUBLIC API ***/

gboolean
swfdec_script_foreach (SwfdecScript *script, SwfdecScriptForeachFunc func, gpointer user_data)
{
  SwfdecBits bits;

  g_return_val_if_fail (script != NULL, FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  swfdec_bits_init (&bits, script->buffer);
  bits.ptr = script->main;
  return swfdec_script_foreach_internal (&bits, func, user_data);
}

/**
 * swfdec_script_new:
 * @buffer: the #SwfdecBuffer containing the script. This function will take
 *          ownership of the passed in buffer.
 * @name: name of the script for debugging purposes
 * @version: Actionscript version to use in this script
 *
 * Creates a new script for the actionscript provided in @buffer.
 *
 * Returns: a new #SwfdecScript for executing the script i @buffer.
 **/
SwfdecScript *
swfdec_script_new (SwfdecBuffer *buffer, const char *name, guint version)
{
  SwfdecBits bits;
  SwfdecScript *script;

  g_return_val_if_fail (buffer != NULL, NULL);

  swfdec_bits_init (&bits, buffer);
  script = swfdec_script_new_from_bits (&bits, name, version);
  swfdec_buffer_unref (buffer);
  return script;
}

SwfdecScript *
swfdec_script_new_from_bits (SwfdecBits *bits, const char *name, guint version)
{
  SwfdecScript *script;
  SwfdecBuffer *buffer;
  SwfdecBits org;
  guint len;

  g_return_val_if_fail (bits != NULL, NULL);

  org = *bits;
  len = swfdec_bits_left (bits) / 8;
  script = g_new0 (SwfdecScript, 1);
  script->refcount = 1;
  script->name = g_strdup (name ? name : "Unnamed script");
  script->version = version;
  /* by default, a function has 4 registers */
  script->n_registers = 5;
  /* These flags are the default arguments used by scripts read from a file.
   * DefineFunction and friends override this */
  script->flags = SWFDEC_SCRIPT_SUPPRESS_ARGS;

  if (!swfdec_script_foreach_internal (bits, validate_action, script)) {
    swfdec_script_unref (script);
    return NULL;
  }
  len -= swfdec_bits_left (bits) / 8;
  if (len == 0) {
    buffer = swfdec_buffer_new (0);
  } else {
    buffer = swfdec_bits_get_buffer (&org, len);
  }

  script->main = buffer->data;
  script->exit = buffer->data + buffer->length;
  script->buffer = swfdec_buffer_ref (swfdec_buffer_get_super (buffer));
  swfdec_buffer_unref (buffer);
  return script;
}

/**
 * swfdec_script_ref:
 * @script: a script
 *
 * Increases the reference count of the given @script by one.
 *
 * Returns: The @script given as an argument
 **/
SwfdecScript *
swfdec_script_ref (SwfdecScript *script)
{
  g_return_val_if_fail (script != NULL, NULL);
  g_return_val_if_fail (script->refcount > 0, NULL);

  script->refcount++;
  return script;
}

/**
 * swfdec_script_unref:
 * @script: a script
 *
 * Decreases the reference count of the given @script by one. If the count 
 * reaches zero, it will automatically be destroyed.
 **/
void
swfdec_script_unref (SwfdecScript *script)
{
  guint i;

  g_return_if_fail (script != NULL);
  g_return_if_fail (script->refcount > 0);

  script->refcount--;
  if (script->refcount > 0)
    return;

  if (script->buffer)
    swfdec_buffer_unref (script->buffer);
  if (script->constant_pool)
    swfdec_buffer_unref (script->constant_pool);
  g_free (script->name);
  for (i = 0; i < script->n_arguments; i++) {
    g_free (script->arguments[i].name);
  }
  g_free (script->arguments);
  g_free (script);
}

/*** UTILITY FUNCTIONS ***/

const char *
swfdec_action_get_name (guint action)
{
  g_return_val_if_fail (action < 256, NULL);

  return swfdec_as_actions[action].name;
}

guint
swfdec_action_get_from_name (const char *name)
{
  guint i;

  g_return_val_if_fail (name != NULL, 0);

  for (i = 0; i < 256; i++) {
    if (swfdec_as_actions[i].name && g_str_equal (name, swfdec_as_actions[i].name))
      return i;
  }
  return 0;
}


