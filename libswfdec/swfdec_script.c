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
#include "swfdec_as_context.h"
#include "swfdec_as_interpret.h"
#include "swfdec_debug.h"
#include "swfdec_debugger.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include "swfdec_decoder.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite.h"
#include "swfdec_sprite_movie.h"

/* Define this to get SWFDEC_WARN'd about missing properties of objects.
 * This can be useful to find out about unimplemented native properties,
 * but usually just causes a lot of spam. */
//#define SWFDEC_WARN_MISSING_PROPERTIES

/*** CONSTANT POOLS ***/

SwfdecConstantPool *
swfdec_constant_pool_new_from_action (const guint8 *data, guint len)
{
  guint8 *next;
  guint i, n;
  GPtrArray *pool;

  if (len < 2) {
    SWFDEC_ERROR ("constant pool too small");
    return NULL;
  }
  n = GUINT16_FROM_LE (*((guint16*) data));
  data += 2;
  len -= 2;
  pool = g_ptr_array_sized_new (n);
  g_ptr_array_set_size (pool, n);
  for (i = 0; i < n; i++) {
    next = memchr (data, 0, len);
    if (next == NULL) {
      SWFDEC_ERROR ("not enough strings available");
      g_ptr_array_free (pool, TRUE);
      return NULL;
    }
    next++;
    g_ptr_array_index (pool, i) = (gpointer) data;
    len -= next - data;
    data = next;
  }
  if (len != 0) {
    SWFDEC_WARNING ("constant pool didn't consume whole buffer (%u bytes leftover)", len);
  }
  return pool;
}

void
swfdec_constant_pool_attach_to_context (SwfdecConstantPool *pool, SwfdecAsContext *context)
{
  guint i;

  g_return_if_fail (pool != NULL);
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  for (i = 0; i < pool->len; i++) {
    g_ptr_array_index (pool, i) = (gpointer) swfdec_as_context_get_string (context, 
	g_ptr_array_index (pool, i));
  }
}

guint
swfdec_constant_pool_size (SwfdecConstantPool *pool)
{
  return pool->len;
}

const char *
swfdec_constant_pool_get (SwfdecConstantPool *pool, guint i)
{
  g_assert (i < pool->len);
  return g_ptr_array_index (pool, i);
}

void
swfdec_constant_pool_free (SwfdecConstantPool *pool)
{
  g_ptr_array_free (pool, TRUE);
}

#if 0
/* FIXME: this is a bit hacky */
static SwfdecBuffer *
swfdec_constant_pool_get_area (SwfdecScript *script, SwfdecConstantPool *pool)
{
  guint8 *start;
  SwfdecBuffer *buffer;
  guint len;

  if (pool->len == 0)
    return NULL;
  start = (guint8 *) g_ptr_array_index (pool, 0) - 5;
  buffer = script->buffer;
  if (start < buffer->data) {
    /* DefineFunction inside DefineFunction */
    g_assert (buffer->parent != NULL);
    buffer = buffer->parent;
    g_assert (start >= buffer->data);
  }
  g_assert (start + 3 < buffer->data + buffer->length);
  g_assert (*start == 0x88);
  len = 3 + (start[1] | start[2] << 8);
  g_assert (start + len < buffer->data + buffer->length);
  return swfdec_buffer_new_subbuffer (buffer, start - buffer->data, len);
}
#endif

/*** SUPPORT FUNCTIONS ***/

static void
swfdec_script_add_to_player (SwfdecScript *script, SwfdecPlayer *player)
{
  if (SWFDEC_IS_DEBUGGER (player)) {
    swfdec_debugger_add_script (SWFDEC_DEBUGGER (player), script);
    script->debugger = player;
  }
}

char *
swfdec_script_print_action (guint action, const guint8 *data, guint len)
{
  const SwfdecActionSpec *spec = swfdec_as_actions + action;

  if (action & 0x80) {
    if (spec->print == NULL) {
      SWFDEC_ERROR ("action %u %s has no print statement",
	  action, spec->name ? spec->name : "Unknown");
      return NULL;
    }
    return spec->print (action, data, len);
  } else {
    if (spec->name == NULL) {
      SWFDEC_ERROR ("action %u is unknown", action);
      return NULL;
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

/*** PUBLIC API ***/

gboolean
swfdec_script_foreach (SwfdecScript *script, SwfdecScriptForeachFunc func, gpointer user_data)
{
  SwfdecBits bits;

  g_return_val_if_fail (script != NULL, FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  swfdec_bits_init (&bits, script->buffer);
  return swfdec_script_foreach_internal (&bits, func, user_data);
}

static gboolean
validate_action (gconstpointer bytecode, guint action, const guint8 *data, guint len, gpointer scriptp)
{
  SwfdecScript *script = scriptp;
  int version = SWFDEC_AS_EXTRACT_SCRIPT_VERSION (script->version);

  /* warn if there's no function to execute this opcode */
  if (swfdec_as_actions[action].exec[version] == NULL) {
    SWFDEC_ERROR ("no function for %3u 0x%02X %s in v%u", action, action,
	swfdec_as_actions[action].name ? swfdec_as_actions[action].name : "Unknown",
	script->version);
  }
  /* we might want to do stuff here for certain actions */
#if 0
  {
    char *foo = swfdec_script_print_action (action, data, len);
    if (foo == NULL)
      return FALSE;
    g_print ("%s\n", foo);
  }
#endif
  return TRUE;
}

SwfdecScript *
swfdec_script_new_for_player (SwfdecPlayer *player, SwfdecBits *bits, 
    const char *name, guint version)
{
  SwfdecScript *script;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (bits != NULL, NULL);

  script = swfdec_script_new (bits, name, version);
  if (script)
    swfdec_script_add_to_player (script, player);
  return script;
}

SwfdecScript *
swfdec_script_new (SwfdecBits *bits, const char *name, guint version)
{
  SwfdecScript *script;
  const guchar *start;
  
  g_return_val_if_fail (bits != NULL, NULL);

  if (version < SWFDEC_AS_MIN_SCRIPT_VERSION) {
    SWFDEC_ERROR ("swfdec version %u doesn't support scripts", version);
    return NULL;
  }

  start = bits->ptr;
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
    /* assign a random buffer here so we have something to unref */
    script->buffer = bits->buffer;
    swfdec_buffer_ref (script->buffer);
    swfdec_script_unref (script);
    return NULL;
  }
  script->buffer = swfdec_buffer_new_subbuffer (bits->buffer, start - bits->buffer->data,
      bits->ptr - start);
  return script;
}

SwfdecScript *
swfdec_script_ref (SwfdecScript *script)
{
  g_return_val_if_fail (script != NULL, NULL);
  g_return_val_if_fail (script->refcount > 0, NULL);

  script->refcount++;
  return script;
}

void
swfdec_script_unref (SwfdecScript *script)
{
  g_return_if_fail (script != NULL);
  g_return_if_fail (script->refcount > 0);

  script->refcount--;
  if (script->refcount == 1 && script->debugger) {
    script->debugger = NULL;
    swfdec_debugger_remove_script (script->debugger, script);
    return;
  }
  if (script->refcount > 0)
    return;

  swfdec_buffer_unref (script->buffer);
  if (script->constant_pool)
    swfdec_buffer_unref (script->constant_pool);
  g_free (script->name);
  g_free (script->preloads);
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


