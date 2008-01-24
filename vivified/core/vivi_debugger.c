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
#include "vivi_marshal.h"
/* FIXME: oops */
#include "swfdec/swfdec_player_internal.h"

enum {
  ADD,
  REMOVE,
  STEP,
  ENTER_FRAME,
  LEAVE_FRAME,
  SET_VARIABLE,
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
  SwfdecPlayer *player = app->player;

  g_assert (app);
  if (app->playback_state == VIVI_APPLICATION_EXITING)
    return;
  if (app->playback_state == VIVI_APPLICATION_PLAYING) {
    app->playback_count--;
    if (app->playback_count > 0)
      return;
  }
  swfdec_player_unlock_soft (player);

  app->playback_state = 0;
  app->playback_count = 0;
  app->loop = g_main_loop_new (NULL, FALSE);
  g_object_notify (G_OBJECT (app), "interrupted");
  vivi_application_check (app);

  g_main_loop_run (app->loop);

  g_main_loop_unref (app->loop);
  app->loop = NULL;
  g_object_notify (G_OBJECT (app), "interrupted");
  vivi_application_check (app);
  swfdec_player_lock_soft (player);
}

static void
vivi_debugger_add (SwfdecAsDebugger *debugger, SwfdecAsContext *context, 
    SwfdecAsObject *object)
{
  gboolean retval = FALSE;

  g_signal_emit (debugger, signals[ADD], 0, object, &retval);

  if (retval)
    vivi_debugger_break (VIVI_DEBUGGER (debugger));
}

static void
vivi_debugger_remove (SwfdecAsDebugger *debugger, SwfdecAsContext *context, 
    SwfdecAsObject *object)
{
  gboolean retval = FALSE;

  g_signal_emit (debugger, signals[REMOVE], 0, object, &retval);

  if (retval)
    vivi_debugger_break (VIVI_DEBUGGER (debugger));
}

static void
vivi_debugger_step (SwfdecAsDebugger *debugger, SwfdecAsContext *context)
{
  gboolean retval = FALSE;

  g_signal_emit (debugger, signals[STEP], 0, &retval);

  if (!retval) {
    ViviApplication *app = VIVI_DEBUGGER (debugger)->app;

    if (app->playback_state == VIVI_APPLICATION_STEPPING) {
      app->playback_count--;
      if (app->playback_count == 0)
	retval = TRUE;
    }
  }
  if (retval)
    vivi_debugger_break (VIVI_DEBUGGER (debugger));
}

static void
vivi_debugger_enter_frame (SwfdecAsDebugger *debugger, SwfdecAsContext *context, SwfdecAsFrame *frame)
{
  gboolean retval = FALSE;

  g_signal_emit (debugger, signals[ENTER_FRAME], 0, frame, &retval);

  if (retval)
    vivi_debugger_break (VIVI_DEBUGGER (debugger));
}

static void
vivi_debugger_leave_frame (SwfdecAsDebugger *debugger, SwfdecAsContext *context, 
    SwfdecAsFrame *frame, const SwfdecAsValue *ret)
{
  gboolean retval = FALSE;

  g_signal_emit (debugger, signals[LEAVE_FRAME], 0, frame, ret, &retval);

  if (retval)
    vivi_debugger_break (VIVI_DEBUGGER (debugger));
}

static void
vivi_debugger_set_variable (SwfdecAsDebugger *debugger, SwfdecAsContext *context, 
    SwfdecAsObject *object, const char *variable, const SwfdecAsValue *value)
{
  gboolean retval = FALSE;

  g_signal_emit (debugger, signals[SET_VARIABLE], 0, object, variable, value, &retval);

  if (retval)
    vivi_debugger_break (VIVI_DEBUGGER (debugger));
}

static void
vivi_debugger_class_init (ViviDebuggerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsDebuggerClass *debugger_class = SWFDEC_AS_DEBUGGER_CLASS (klass);

  object_class->dispose = vivi_debugger_dispose;

  signals[ADD] = g_signal_new ("add", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, vivi_accumulate_or, NULL, vivi_marshal_BOOLEAN__OBJECT,
      G_TYPE_BOOLEAN, 1, SWFDEC_TYPE_AS_OBJECT);
  signals[REMOVE] = g_signal_new ("remove", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, vivi_accumulate_or, NULL, vivi_marshal_BOOLEAN__OBJECT,
      G_TYPE_BOOLEAN, 1, SWFDEC_TYPE_AS_OBJECT);
  signals[STEP] = g_signal_new ("step", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, vivi_accumulate_or, NULL, vivi_marshal_BOOLEAN__VOID,
      G_TYPE_BOOLEAN, 0);
  signals[ENTER_FRAME] = g_signal_new ("enter-frame", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, vivi_accumulate_or, NULL, vivi_marshal_BOOLEAN__OBJECT,
      G_TYPE_BOOLEAN, 1, SWFDEC_TYPE_AS_FRAME);
  signals[LEAVE_FRAME] = g_signal_new ("leave-frame", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, vivi_accumulate_or, NULL, vivi_marshal_BOOLEAN__OBJECT_POINTER,
      G_TYPE_BOOLEAN, 2, SWFDEC_TYPE_AS_FRAME, G_TYPE_POINTER);
  signals[SET_VARIABLE] = g_signal_new ("set-variable", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, vivi_accumulate_or, NULL, vivi_marshal_BOOLEAN__OBJECT_STRING_POINTER,
      G_TYPE_BOOLEAN, 3, SWFDEC_TYPE_AS_OBJECT, G_TYPE_STRING, G_TYPE_POINTER);

  debugger_class->add = vivi_debugger_add;
  debugger_class->remove = vivi_debugger_remove;
  debugger_class->step = vivi_debugger_step;
  debugger_class->enter_frame = vivi_debugger_enter_frame;
  debugger_class->leave_frame = vivi_debugger_leave_frame;
  debugger_class->set_variable = vivi_debugger_set_variable;
}

static void
vivi_debugger_init (ViviDebugger *debugger)
{
}

