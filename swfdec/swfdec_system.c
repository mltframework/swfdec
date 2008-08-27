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

#include "swfdec_system.h"
#include "swfdec_debug.h"

/*** gtk-doc ***/

/**
 * SECTION:SwfdecSystem
 * @title: SwfdecSystem
 * @short_description: object holding system settings
 * @see_also: SwfdecPlayer
 *
 * This object is used to provide information about the system Swfdec currently 
 * runs on.
 *
 * Almost all of this information can be categorized into three types:
 * Information about the current playback engine like manufacturer or version,
 * information about the current operating system and capabilities of the output
 * capabilities of the System like screen size.
 *
 * The information provided by this object is used by the Actionscript 
 * System.capabilities.Query() function that is usually only run once during 
 * initialization of the Flash player. If you want to set custom properties and
 * have them affect a running #SwfdecPlayer, you should change them before the
 * player gets initialized.
 *
 * Note that the System.capabilites object in Flash provides more functionality
 * than provided by this object. That information can be and is determined 
 * automatically by Swfdec.
 */

/**
 * SwfdecSystem:
 *
 * This is the object used for holding information about the current system. See
 * the introduction for details.
 */

/*** SwfdecSystem ***/

enum {
  PROP_0,
  PROP_DEBUGGER,
  PROP_MANUFACTURER,
  PROP_SERVER_MANUFACTURER,
  PROP_OS,
  PROP_OS_TYPE,
  PROP_PLAYER_TYPE,
  PROP_VERSION,
  PROP_LANGUAGE,
  PROP_SCREEN_WIDTH,
  PROP_SCREEN_HEIGHT,
  PROP_PAR,
  PROP_DPI,
  PROP_COLOR_MODE,
  PROP_UTC_OFFSET
};

G_DEFINE_TYPE (SwfdecSystem, swfdec_system, G_TYPE_OBJECT)

static void
swfdec_system_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecSystem *system = SWFDEC_SYSTEM (object);
  
  switch (param_id) {
    case PROP_DEBUGGER:
      g_value_set_boolean (value, system->debugger);
      break;
    case PROP_MANUFACTURER:
      g_value_set_string (value, system->manufacturer);
      break;
    case PROP_SERVER_MANUFACTURER:
      g_value_set_string (value, system->server_manufacturer);
      break;
    case PROP_OS:
      g_value_set_string (value, system->os);
      break;
    case PROP_OS_TYPE:
      g_value_set_string (value, system->os_type);
      break;
    case PROP_PLAYER_TYPE:
      g_value_set_string (value, system->player_type);
      break;
    case PROP_VERSION:
      g_value_set_string (value, system->version);
      break;
    case PROP_LANGUAGE:
      g_value_set_string (value, system->language);
      break;
    case PROP_SCREEN_WIDTH:
      g_value_set_uint (value, system->screen_width);
      break;
    case PROP_SCREEN_HEIGHT:
      g_value_set_uint (value, system->screen_height);
      break;
    case PROP_PAR:
      g_value_set_double (value, system->par);
      break;
    case PROP_DPI:
      g_value_set_uint (value, system->dpi);
      break;
    case PROP_COLOR_MODE:
      g_value_set_string (value, system->color_mode);
      break;
    case PROP_UTC_OFFSET:
      g_value_set_int (value, system->utc_offset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_system_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecSystem *system = SWFDEC_SYSTEM (object);
  char *s;

  switch (param_id) {
    case PROP_DEBUGGER:
      system->debugger = g_value_get_boolean (value);
      break;
    case PROP_MANUFACTURER:
      s = g_value_dup_string (value);
      if (s) {
	g_free (system->manufacturer);
	system->manufacturer = s;
      }
      break;
    case PROP_SERVER_MANUFACTURER:
      s = g_value_dup_string (value);
      if (s) {
	g_free (system->server_manufacturer);
	system->server_manufacturer = s;
      }
      break;
    case PROP_OS:
      s = g_value_dup_string (value);
      if (s) {
	g_free (system->os);
	system->os = s;
      }
      break;
    case PROP_OS_TYPE:
      s = g_value_dup_string (value);
      if (s) {
	g_free (system->os_type);
	system->os_type = s;
      }
      break;
    case PROP_PLAYER_TYPE:
      s = g_value_dup_string (value);
      if (s) {
	g_free (system->player_type);
	system->player_type = s;
      }
      break;
    case PROP_VERSION:
      s = g_value_dup_string (value);
      if (s) {
	g_free (system->version);
	system->version = s;
      }
      break;
    case PROP_LANGUAGE:
      s = g_value_dup_string (value);
      if (s) {
	g_free (system->language);
	system->language = s;
      }
      break;
    case PROP_SCREEN_WIDTH:
      system->screen_width = g_value_get_uint (value);
      break;
    case PROP_SCREEN_HEIGHT:
      system->screen_height = g_value_get_uint (value);
      break;
    case PROP_PAR:
      system->par = g_value_get_double (value);
      break;
    case PROP_DPI:
      system->dpi = g_value_get_uint (value);
      break;
    case PROP_COLOR_MODE:
      s = g_value_dup_string (value);
      if (s) {
	g_free (system->color_mode);
	system->color_mode = s;
      }
      break;
    case PROP_UTC_OFFSET:
      system->utc_offset = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_system_finalize (GObject *object)
{
  SwfdecSystem *system = SWFDEC_SYSTEM (object);

  g_free (system->manufacturer);
  g_free (system->server_manufacturer);
  g_free (system->os);
  g_free (system->os_type);
  g_free (system->player_type);
  g_free (system->version);
  g_free (system->language);
  g_free (system->color_mode);

  G_OBJECT_CLASS (swfdec_system_parent_class)->finalize (object);
}

static void
swfdec_system_class_init (SwfdecSystemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = swfdec_system_finalize;
  object_class->get_property = swfdec_system_get_property;
  object_class->set_property = swfdec_system_set_property;

  g_object_class_install_property (object_class, PROP_DEBUGGER,
      g_param_spec_boolean ("debugger", "debugger", "TRUE if this player is supposed to be a debugger",
	  FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_MANUFACTURER,
      g_param_spec_string ("manufacturer", "manufacturer", "string describing the manufacturer of this system",
	  "Macromedia Windows", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_SERVER_MANUFACTURER,
      g_param_spec_string ("server-manufacturer", "server-manufacturer", "manufacturer of this system as used in serverString",
	  "Adobe Windows", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_OS,
      g_param_spec_string ("os", "os", "description of the operating system",
	  "Windows XP", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_OS_TYPE,
      g_param_spec_string ("os-type", "os type", "the operating system type: WIN, LIN or MAC",
	  "WIN", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_PLAYER_TYPE,
      g_param_spec_string ("player-type", "player type", "\"StandAlone\", \"External\", \"PlugIn\" or \"ActiveX\"",
	  "StandAlone", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_VERSION,
      g_param_spec_string ("version", "version", "version string",
	  "WIN 9,0,999,0", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_LANGUAGE,
      g_param_spec_string ("language", "language", "ISO 639-1 language code",
	  "en", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_SCREEN_WIDTH,
      g_param_spec_uint ("screen-width", "screen width", "width of the screen in pixels",
	  0, G_MAXUINT, 1024, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_SCREEN_HEIGHT,
      g_param_spec_uint ("screen-height", "screen height", "height of the screen in pixels",
	  0, G_MAXUINT, 768, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_PAR,
      g_param_spec_double ("pixel-aspect-ratio", "pixel aspect ratio", "the screen's pixel aspect ratio",
	  G_MINDOUBLE, G_MAXDOUBLE, 1.0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_DPI,
      g_param_spec_uint ("dpi", "dpi", "DPI setting of screen",
	  0, G_MAXUINT, 96, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_COLOR_MODE,
      g_param_spec_string ("color-mode", "color mode", "\"color\", \"gray\" or \"bw\"",
	  "color", G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_UTC_OFFSET,
      g_param_spec_int ("utc-offset", "utc offset",
	"Difference between UTC and local timezone in minutes",
	  -12 * 60, 12 * 60, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
swfdec_system_init (SwfdecSystem *system)
{
}

/**
 * swfdec_system_new:
 *
 * Creates a new #SwfdecSystem object using default settings. These settings are 
 * mirroring the most common settings used by a Flash player. Currently this is
 * equivalent to a Flash player running on Windows XP.
 *
 * Returns: a new #SwfdecSystem object
 **/
SwfdecSystem *
swfdec_system_new (void)
{
  return g_object_new (SWFDEC_TYPE_SYSTEM, NULL);
}
