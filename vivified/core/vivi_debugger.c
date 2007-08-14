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

#include "vivi_debugger.h"
#include "vivi_application.h"
/* FIXME: oops */
#include "libswfdec/swfdec_player_internal.h"

enum {
  STEP,
  LAST_SIGNAL
};

G_DEFINE_TYPE (ViviDebugger, vivi_debugger, SWFDEC_TYPE_AS_DEBUGGER)
static guint signals[LAST_SIGNAL] = { 0, };

static gboolean
vivi_accumulate_or (GSignalInvocationHint *ihint, GValue *return_accu, 
    const GValue *handler_return, gpointer data)
{
  if (g_value_get_boolean (handler_return))
    g_value_set_boolean (return_accu, TRUE);
  return TRUE;
}

static void
vivi_debugger_dispose (GObject *object)
{
  //ViviDebugger *debugger = VIVI_DEBUGGER (object);

  G_OBJECT_CLASS (vivi_debugger_parent_class)->dispose (object);
}

static void
vivi_debugger_break (ViviDebugger *debugger)
{
  ViviApplication *app = debugger->app;

  g_assert (app);
  swfdec_player_unlock_soft (app->player);

  app->playback_state = 0;
  app->playback_count = 0;
  app->loop = g_main_loop_new (NULL, FALSE);

  g_main_loop_run (app->loop);

  g_main_loop_unref (app->loop);
  app->loop = NULL;
  swfdec_player_lock_soft (app->player);
}

static void
vivi_debugger_step (SwfdecAsDebugger *debugger, SwfdecAsContext *context)
{
  gboolean retval = FALSE;

  g_signal_emit (debugger, signals[STEP], 0, &retval);

  if (retval)
    vivi_debugger_break (VIVI_DEBUGGER (debugger));
}

static void
vivi_debugger_class_init (ViviDebuggerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsDebuggerClass *debugger_class = SWFDEC_AS_DEBUGGER_CLASS (klass);

  object_class->dispose = vivi_debugger_dispose;

  signals[STEP] = g_signal_new ("step", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, vivi_accumulate_or, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_BOOLEAN, 0);

  debugger_class->step = vivi_debugger_step;
}

static void
vivi_debugger_init (ViviDebugger *debugger)
{
}

