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

#ifndef _SWFDEC_SYSTEM_H_
#define _SWFDEC_SYSTEM_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _SwfdecSystem SwfdecSystem;
typedef struct _SwfdecSystemClass SwfdecSystemClass;

#define SWFDEC_TYPE_SYSTEM                    (swfdec_system_get_type())
#define SWFDEC_IS_SYSTEM(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SYSTEM))
#define SWFDEC_IS_SYSTEM_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SYSTEM))
#define SWFDEC_SYSTEM(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SYSTEM, SwfdecSystem))
#define SWFDEC_SYSTEM_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SYSTEM, SwfdecSystemClass))
#define SWFDEC_SYSTEM_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_SYSTEM, SwfdecSystemClass))

struct _SwfdecSystem
{
  GObject		object;

  /*< private >*/
  /* player engine */
  gboolean		debugger;	/* TRUE if this engine is debugging */
  char *		manufacturer;	/* note that this includes OS information */
  char *		server_manufacturer; /* manufacturer as reported in serverString */
  char *		os;		/* supposed to identify the operating system */
  char *		os_type;	/* usually WIN, LIN or MAC */
  char *		player_type;	/* "StandAlone", "External", "PlugIn" or "ActiveX" */
  char *		version;	/* string of type "os_type MAJOR.MINOR.MACRO.MICRO" */
  
  /* system */
  char *		language;	/* ISO 639-1 language code */

  /* screen */
  guint			screen_width;	/* width of screen */
  guint			screen_height;	/* height of screen */
  double		par;		/* pixel aspect ratio */
  guint			dpi;		/* dpi setting */
  char *		color_mode;	/* "color", "gray" or "bw" */

  /* date */
  int			utc_offset;	/* difference between UTC and local timezeon in minutes */

};

struct _SwfdecSystemClass
{
  GObjectClass		object_class;
};

GType		swfdec_system_get_type		(void);

SwfdecSystem *	swfdec_system_new		(void);

G_END_DECLS
#endif
