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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "vivi_window.h"

enum {
  PROP_0,
  PROP_APP,
  PROP_MOVIE
};

G_DEFINE_TYPE (ViviWindow, vivi_window, GTK_TYPE_WINDOW)

static void
vivi_window_app_notify (ViviApplication *app, GParamSpec *pspec, ViviWindow *window)
{
  if (g_str_equal (pspec->name, "player")) {
    vivi_window_set_movie (window, NULL);
  } else if (g_str_equal (pspec->name, "filename")) {
    const char *filename = vivi_application_get_filename (app);

    if (filename == NULL)
      filename = "Vivified";
    gtk_window_set_title (GTK_WINDOW (window), filename);
  }
}

static void
vivi_window_set_application (ViviWindow *window, ViviApplication *app)
{
  g_return_if_fail (VIVI_IS_WINDOW (window));
  g_return_if_fail (app == NULL || VIVI_IS_APPLICATION (app));

  vivi_window_set_movie (window, NULL);
  if (window->app) {
    g_signal_handlers_disconnect_by_func (window->app, vivi_window_app_notify, window);
    g_object_unref (window->app);
  }
  window->app = app;
  if (app) {
    g_object_ref (app);
    g_signal_connect (app, "notify", G_CALLBACK (vivi_window_app_notify), window);
  }
  g_object_notify (G_OBJECT (window), "application");
}

static void
vivi_window_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  ViviWindow *window = VIVI_WINDOW (object);
  
  switch (param_id) {
    case PROP_APP:
      g_value_set_object (value, window->app);
      break;
    case PROP_MOVIE:
      g_value_set_object (value, window->movie);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_window_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  ViviWindow *window = VIVI_WINDOW (object);
  
  switch (param_id) {
    case PROP_APP:
      vivi_window_set_application (window, VIVI_APPLICATION (g_value_get_object (value)));
      break;
    case PROP_MOVIE:
      vivi_window_set_movie (window, SWFDEC_MOVIE (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_window_dispose (GObject *object)
{
  ViviWindow *window = VIVI_WINDOW (object);

  vivi_window_set_application (window, NULL);
  vivi_window_set_movie (window, NULL);

  G_OBJECT_CLASS (vivi_window_parent_class)->dispose (object);
}

static void
vivi_window_class_init (ViviWindowClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = vivi_window_dispose;
  object_class->get_property = vivi_window_get_property;
  object_class->set_property = vivi_window_set_property;

  g_object_class_install_property (object_class, PROP_APP,
      g_param_spec_object ("application", "application", "application that is playing",
	  VIVI_TYPE_APPLICATION, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MOVIE,
      g_param_spec_object ("movie", "movie", "selected movie",
	  SWFDEC_TYPE_MOVIE, G_PARAM_READWRITE));
}

static void
vivi_window_init (ViviWindow *window)
{
}

static void
try_grab_focus (GtkWidget *widget, gpointer unused)
{
  if (GTK_IS_ENTRY (widget))
    gtk_widget_grab_focus (widget);
  else if (GTK_IS_CONTAINER (widget))
    gtk_container_foreach (GTK_CONTAINER (widget), try_grab_focus, NULL);
}

GtkWidget *
vivi_window_new (ViviApplication *app)
{
  GtkWidget *window, *box, *paned, *widget;
  GtkBuilder *builder;
  GError *error = NULL;

  g_return_val_if_fail (VIVI_IS_APPLICATION (app), NULL);

  window = g_object_new (VIVI_TYPE_WINDOW, "application", app, 
      "type", GTK_WINDOW_TOPLEVEL, "default-width", 600, "default-height", 450,
      NULL);

  builder = gtk_builder_new ();
  if (!gtk_builder_add_from_file (builder, "vivi_player.xml", &error) ||
      !gtk_builder_add_from_file (builder, "vivi_command_line.xml", &error) ||
      !gtk_builder_add_from_file (builder, "vivi_movies.xml", &error))
    g_error ("%s", error->message);
  gtk_builder_connect_signals (builder, app);

  gtk_window_set_default_size (GTK_WINDOW (window), 600, 450);
  paned = gtk_hpaned_new ();
  gtk_paned_set_position (GTK_PANED (paned), 200);
  gtk_container_add (GTK_CONTAINER (window), paned);

  box = vivi_vdock_new ();
  gtk_paned_add2 (GTK_PANED (paned), box);
  widget = GTK_WIDGET (gtk_builder_get_object (builder, "player"));
  g_object_set (widget, "application", app, NULL);
  vivi_vdock_add (VIVI_VDOCK (box), widget);
  widget = GTK_WIDGET (gtk_builder_get_object (builder, "command-line"));
  g_object_set (widget, "application", app, NULL);
  vivi_vdock_add (VIVI_VDOCK (box), widget);
  gtk_container_foreach (GTK_CONTAINER (widget), try_grab_focus, NULL);

  box = vivi_vdock_new ();
  gtk_paned_add1 (GTK_PANED (paned), box);
  widget = GTK_WIDGET (gtk_builder_get_object (builder, "movies"));
  g_object_set (widget, "application", app, NULL);
  vivi_vdock_add (VIVI_VDOCK (box), widget);

  g_object_unref (builder);

  return window;
}

ViviApplication *
vivi_window_get_application (ViviWindow *window)
{
  g_return_val_if_fail (VIVI_IS_WINDOW (window), NULL);

  return window->app;
}

void
vivi_window_set_movie (ViviWindow *window, SwfdecMovie *movie)
{
  g_return_if_fail (VIVI_IS_WINDOW (window));
  g_return_if_fail (movie == NULL || SWFDEC_IS_MOVIE (movie));

  if (window->movie == movie)
    return;

  if (window->movie) {
    g_object_unref (window->movie);
  }
  window->movie = movie;
  if (window->movie) {
    g_object_ref (movie);
  }
  g_object_notify (G_OBJECT (window), "movie");
}

