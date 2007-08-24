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

#include "vivi_application.h"
#include "vivi_function.h"

VIVI_FUNCTION ("reset", vivi_application_as_reset)
void
vivi_application_as_reset (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);

  vivi_application_reset (app);
}

VIVI_FUNCTION ("run", vivi_application_as_run)
void
vivi_application_as_run (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);

  vivi_application_play (app);
}

VIVI_FUNCTION ("stop", vivi_application_as_stop)
void
vivi_application_as_stop (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);

  vivi_application_stop (app);
}

VIVI_FUNCTION ("step", vivi_application_as_step)
void
vivi_application_as_step (SwfdecAsContext *cx, SwfdecAsObject *this, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  int steps;

  if (argc > 0) {
    steps = swfdec_as_value_to_integer (cx, &argv[0]);
    if (steps <= 1)
      steps = 1;
  } else {
    steps = 1;
  }
  vivi_application_step (app, steps);
}

VIVI_FUNCTION ("print", vivi_application_as_print)
void
vivi_application_as_print (SwfdecAsContext *cx, SwfdecAsObject *this, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  const char *s;

  if (argc == 0)
    return;

  s = swfdec_as_value_to_string (cx, &argv[0]);
  vivi_application_output (app, "%s", s);
}

VIVI_FUNCTION ("error", vivi_application_as_error)
void
vivi_application_as_error (SwfdecAsContext *cx, SwfdecAsObject *this, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);
  const char *s;

  if (argc == 0)
    return;

  s = swfdec_as_value_to_string (cx, &argv[0]);
  vivi_application_error (app, "%s", s);
}

VIVI_FUNCTION ("quit", vivi_application_as_quit)
void
vivi_application_as_quit (SwfdecAsContext *cx, SwfdecAsObject *this, 
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviApplication *app = VIVI_APPLICATION (cx);

  vivi_application_quit (app);
}

