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

#include <locale.h>
#include <string.h>
#include <time.h>
#include <gtk/gtk.h>

#include "swfdec_gtk_system.h"

struct _SwfdecGtkSystemPrivate
{
  GdkScreen *		screen;		/* the screen we monitor */
};

/*** gtk-doc ***/

/**
 * SECTION:SwfdecGtkSystem
 * @title: SwfdecGtkSystem
 * @short_description: an improved #SwfdecSystem
 *
 * The #SwfdecGtkSystem automatically determines system values queriable by the
 * Flash player.
 *
 * @see_also: SwfdecSystem
 */

/**
 * SwfdecGtkSystem:
 *
 * The structure for the Swfdec Gtk system contains no public fields.
 */

/*** SWFDEC_GTK_SYSTEM ***/

G_DEFINE_TYPE (SwfdecGtkSystem, swfdec_gtk_system, SWFDEC_TYPE_SYSTEM)

static void
swfdec_gtk_system_dispose (GObject *object)
{
  SwfdecGtkSystem *system = SWFDEC_GTK_SYSTEM (object);

  g_object_unref (system->priv->screen);

  G_OBJECT_CLASS (swfdec_gtk_system_parent_class)->dispose (object);
}

static void
swfdec_gtk_system_class_init (SwfdecGtkSystemClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  g_type_class_add_private (g_class, sizeof (SwfdecGtkSystemPrivate));

  object_class->dispose = swfdec_gtk_system_dispose;
}

static void
swfdec_gtk_system_init (SwfdecGtkSystem * system)
{
  system->priv = G_TYPE_INSTANCE_GET_PRIVATE (system, SWFDEC_TYPE_GTK_SYSTEM, SwfdecGtkSystemPrivate);
}

/*** DETERMINING THE VALUES ***/

static char *
swfdec_gtk_system_get_language (void)
{
  /* we use LC_MESSAGES here, because we want the display language. See 
   * documentation for System.capabilities.language
   */
  char *locale = setlocale (LC_MESSAGES, NULL);
  char *lang;

  if (locale == NULL)
    return g_strdup ("en");

  /* "POSIX" and "C" => "en" */
  if (g_str_equal (locale, "C") || g_str_equal (locale, "POSIX"))
    return g_strdup ("en");

  /* special case chinese */
  if (g_str_has_prefix (locale, "zh_")) {
    lang = g_strndup (locale, strcspn (locale, ".@"));
    lang[2] = '-';
    return lang;
  }

  /* get language part */
  return g_strndup (locale, strcspn (locale, "_.@"));
}

static int
swfdec_gtk_system_get_utc_offset (void)
{
  tzset ();
  return timezone / 60;
}

/*** PUBLIC API ***/

/**
 * swfdec_gtk_system_new:
 * @screen: the GdkScreen to take information from or %NULL for the default
 *
 * Creates a new #SwfdecGtkSystem object.
 *
 * Returns: The new object
 **/
SwfdecSystem *
swfdec_gtk_system_new (GdkScreen *screen)
{
  SwfdecSystem *system;
  char *lang;
  guint dpi;

  if (screen == NULL)
    screen = gdk_screen_get_default ();
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  lang = swfdec_gtk_system_get_language ();
  if (gdk_screen_get_resolution (screen) > 0)
    dpi = gdk_screen_get_resolution (screen);
  else
    dpi = 96;

  system = g_object_new (SWFDEC_TYPE_GTK_SYSTEM, 
      "language", lang, "utc-offset", swfdec_gtk_system_get_utc_offset (),
      "dpi", dpi, "screen-height", gdk_screen_get_height (screen), 
      "screen-width", gdk_screen_get_width (screen), 
      "pixel-aspect-ratio", (double) gdk_screen_get_width (screen) * gdk_screen_get_height_mm (screen)
	  / (gdk_screen_get_width_mm (screen) * gdk_screen_get_height (screen)),
      NULL);
  SWFDEC_GTK_SYSTEM (system)->priv->screen = g_object_ref (screen);

  g_free (lang);

  return system;
}

