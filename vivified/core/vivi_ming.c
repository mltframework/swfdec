/* Vivified
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

#include "vivi_ming.h"
#include <string.h>

static GString *ming_errors = NULL;

static void
vivi_ming_error (const char *format, ...)
{
  va_list varargs;
  char *s;

  if (ming_errors) {
    g_string_append_c (ming_errors, '\n');
  } else {
    ming_errors = g_string_new ("");
  }
  va_start (varargs, format);
  g_string_append_vprintf (ming_errors, format, varargs);
  s = g_strdup_vprintf (format, varargs);
  va_end (varargs);
}

static char *
vivi_ming_get_error (void)
{
  char *ret;

  if (ming_errors == NULL)
    return g_strdup ("Unknown error");

  ret = g_string_free (ming_errors, FALSE);
  ming_errors = NULL;
  return ret;
}

static void
vivi_ming_clear_error (void)
{
  char *ret;
  
  if (ming_errors != NULL) {
    ret = vivi_ming_get_error ();
    g_free (ret);
  }
}

static void
vivi_ming_init (void)
{
  static gboolean ming_inited = FALSE;

  if (ming_inited)
    return;

  ming_inited = TRUE;

  Ming_init ();
  Ming_useSWFVersion (8);
  Ming_setErrorFunction (vivi_ming_error);
  Ming_setWarnFunction (vivi_ming_error);
}

SwfdecScript *
vivi_ming_compile (const char *code, char **error)
{
  byte *data;
  SWFAction action;
  gssize len;
  SwfdecBuffer *buffer;
  SwfdecScript *script;

  vivi_ming_init ();

  action = newSWFAction (code);
  data = SWFAction_getByteCode (action, &len);
  if (data == NULL || len <= 1) {
    if (error)
      *error = vivi_ming_get_error ();
    script = NULL;
  } else {
    buffer = swfdec_buffer_new (len);
    memcpy (buffer->data, data, len);
    script = swfdec_script_new (buffer, "compiled script", 8);
  }
  vivi_ming_clear_error ();
  return script;
}

