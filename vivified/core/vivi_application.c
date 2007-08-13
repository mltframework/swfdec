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

#include <libswfdec-gtk/swfdec-gtk.h>
#include "vivi_application.h"
#include "vivi_function.h"
#include "vivi_ming.h"

typedef enum {
  VIVI_APPLICATION_STOPPED,
  VIVI_APPLICATION_PLAYING,
  VIVI_APPLICATION_STEPPING,
} ViviApplicationPlayback;

enum {
  MESSAGE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_FILENAME,
  PROP_PLAYER
};

G_DEFINE_TYPE (ViviApplication, vivi_application, SWFDEC_TYPE_AS_CONTEXT)
static guint signals[LAST_SIGNAL] = { 0, };

static void
vivi_application_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  ViviApplication *app = VIVI_APPLICATION (object);
  
  switch (param_id) {
    case PROP_FILENAME:
      g_value_set_string (value, app->filename);
      break;
    case PROP_PLAYER:
      g_value_set_object (value, app->player);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_application_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  ViviApplication *app = VIVI_APPLICATION (object);

  switch (param_id) {
    case PROP_FILENAME:
      vivi_application_set_filename (app, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_application_dispose (GObject *object)
{
  ViviApplication *app = VIVI_APPLICATION (object);

  g_object_unref (app->player);

  G_OBJECT_CLASS (vivi_application_parent_class)->dispose (object);
}

static void
vivi_application_class_init (ViviApplicationClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_application_dispose;
  object_class->get_property = vivi_application_get_property;
  object_class->set_property = vivi_application_set_property;

  g_object_class_install_property (object_class, PROP_FILENAME,
      g_param_spec_string ("filename", "filename", "name of file to play",
	  NULL, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PLAYER,
      g_param_spec_object ("player", "player", "Flash player in use",
	  SWFDEC_TYPE_PLAYER, G_PARAM_READABLE));

  signals[MESSAGE] = g_signal_new ("message", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT_POINTER, /* FIXME */
      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);
}

static void
vivi_application_init (ViviApplication *app)
{
  app->player = swfdec_gtk_player_new (NULL);
}

ViviApplication *
vivi_application_new (void)
{
  ViviApplication *app;

  app = g_object_new (VIVI_TYPE_APPLICATION, NULL);
  swfdec_as_context_startup (SWFDEC_AS_CONTEXT (app), 8);
  vivi_function_init_context (app);
  return app;
}

void
vivi_application_init_player (ViviApplication *app)
{
  SwfdecLoader *loader;

  g_return_if_fail (VIVI_IS_APPLICATION (app));

  g_print ("init\n");
  if (app->player_inited)
    return;
  
  if (app->filename == NULL) {
    vivi_application_error (app, "no file set to play.");
    return;
  }

  g_print ("really init\n");
  loader = swfdec_file_loader_new (app->filename);
  swfdec_player_set_loader (app->player, loader);
  app->player_inited = TRUE;
}

void
vivi_application_reset (ViviApplication *app)
{
  g_return_if_fail (VIVI_IS_APPLICATION (app));

  g_object_unref (app->player);
  app->player = swfdec_gtk_player_new (NULL);
  app->player_inited = FALSE;
}

void
vivi_application_set_filename (ViviApplication *app, const char *filename)
{
  g_return_if_fail (VIVI_IS_APPLICATION (app));
  g_return_if_fail (filename != NULL);

  g_free (app->filename);
  app->filename = g_strdup (filename);
  vivi_application_reset (app);
  g_object_notify (G_OBJECT (app), "filename");
}

const char *
vivi_application_get_filename (ViviApplication *app)
{
  g_return_val_if_fail (VIVI_IS_APPLICATION (app), NULL);

  return app->filename;
}

SwfdecPlayer *
vivi_application_get_player (ViviApplication *app)
{
  g_return_val_if_fail (VIVI_IS_APPLICATION (app), NULL);

  return app->player;
}

static gboolean
vivi_application_step_forward (gpointer appp)
{
  ViviApplication *app = appp;
  guint next_event;

  next_event = swfdec_player_get_next_event (app->player);
  swfdec_player_advance (app->player, next_event);

  return FALSE;
}

static void
vivi_application_check (ViviApplication *app)
{
  gboolean is_playing = swfdec_gtk_player_get_playing (SWFDEC_GTK_PLAYER (app->player));
  gboolean is_breakpoint = app->loop != NULL;

  switch (app->playback_state) {
    case VIVI_APPLICATION_STOPPED:
      if (is_playing)
	swfdec_gtk_player_set_playing (SWFDEC_GTK_PLAYER (app->player), FALSE);
      break;
    case VIVI_APPLICATION_PLAYING:
      if (!is_playing)
	swfdec_gtk_player_set_playing (SWFDEC_GTK_PLAYER (app->player), TRUE);
      if (is_breakpoint)
	g_main_loop_quit (app->loop);
      break;
    case VIVI_APPLICATION_STEPPING:
      if (is_playing)
	swfdec_gtk_player_set_playing (SWFDEC_GTK_PLAYER (app->player), FALSE);
      if (is_breakpoint) {
	g_main_loop_quit (app->loop);
      } else {
	/* FIXME: sanely handle this */
	g_idle_add_full (-100, vivi_application_step_forward, app, NULL);
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
}

void
vivi_applciation_execute (ViviApplication *app, const char *command)
{
  SwfdecAsValue val;
  SwfdecAsObject *object;
  SwfdecScript *script;
  char *error = NULL;

  g_return_if_fail (VIVI_IS_APPLICATION (app));
  g_return_if_fail (command != NULL);

  vivi_application_input (app, command);
  script = vivi_ming_compile (command, &error);
  if (script == NULL) {
    vivi_application_error (app, error);
    g_free (error);
    return;
  }
  object = SWFDEC_AS_CONTEXT (app)->global;
  swfdec_as_object_get_variable (object, 
      swfdec_as_context_get_string (SWFDEC_AS_CONTEXT (app), "Commands"),
      &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    object = SWFDEC_AS_VALUE_GET_OBJECT (&val);
  swfdec_as_object_run (object, script);
  swfdec_script_unref (script);
  vivi_application_check (app);
}

void
vivi_application_send_message (ViviApplication *app,
    ViviMessageType type, const char *format, ...)
{
  va_list args;
  char *msg;

  g_return_if_fail (VIVI_IS_APPLICATION (app));
  g_return_if_fail (format != NULL);

  va_start (args, format);
  msg = g_strdup_vprintf (format, args);
  va_end (args);
  g_signal_emit (app, signals[MESSAGE], 0, (guint) type, msg);
  g_free (msg);
}

void
vivi_application_play (ViviApplication *app)
{
  g_return_if_fail (VIVI_IS_APPLICATION (app));

  app->playback_state = VIVI_APPLICATION_PLAYING;
  app->playback_count = 1;
}

void
vivi_application_step (ViviApplication *app, guint n_times)
{
  g_return_if_fail (VIVI_IS_APPLICATION (app));

  app->playback_state = VIVI_APPLICATION_STEPPING;
  app->playback_count = n_times;
}

