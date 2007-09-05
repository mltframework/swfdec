/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *                    Pekka Lampila <pekka.lampila@iki.fi>
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

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#include "swfdec_as_date.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsDate, swfdec_as_date, SWFDEC_TYPE_AS_OBJECT)

// forward definations

static void
swfdec_as_date_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret);

/*
 * Class functions
 */

static void
swfdec_as_date_class_init (SwfdecAsDateClass *klass)
{
}

static void
swfdec_as_date_init (SwfdecAsDate *date)
{
  char buffer[16];
  time_t now;
  struct tm *local;

  // FIXME: Smarter way to get the offset?
  // FIXME: DST handling v5 style?
  now = time (NULL);
  local = localtime (&now);

  if (!strftime (buffer, sizeof (buffer), "%z", local) || buffer[0] == 0) {
    date->timezone_offset_minutes = 0;
  } else {
    date->timezone_offset_minutes =
      (strtol (buffer, NULL, 10) / 100) * 60 + strtol (buffer, NULL, 10) % 100;
  }
}

/*** Helper functions ***/

// returns TRUE if d is not Infinite or NAN
static gboolean
swfdec_as_date_value_to_number_and_integer_floor (SwfdecAsContext *context,
    const SwfdecAsValue *value, double *d, int *num)
{
  *d = swfdec_as_value_to_number (context, value);
  if (!isfinite (*d)) {
    *num = 0;
    return FALSE;
  }

  *num = floor (*d);
  return TRUE;
}

// returns TRUE if d is not Infinite or NAN
static gboolean
swfdec_as_date_value_to_number_and_integer (SwfdecAsContext *context,
    const SwfdecAsValue *value, double *d, int *num)
{
  g_assert (d != NULL);
  g_assert (num != NULL);

  // undefined == NAN here, even in version < 7
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (value)) {
    *d = NAN;
  } else {
    *d = swfdec_as_value_to_number (context, value);
  }
  if (!isfinite (*d)) {
    *num = 0;
    return FALSE;
  }

  if (*d < 0) {
    *num = - (guint) fmod (-*d, 4294967296);
  } else {
    *num =  (guint) fmod (*d, 4294967296);
  }
  return TRUE;
}

// returns TRUE with Infinite and -Infinite, because those values should be
// handles like 0 that is returned by below functions
static gboolean
swfdec_as_date_is_valid (const SwfdecAsDate *date)
{
  return !isnan (date->milliseconds);
}

static void
swfdec_as_date_set_invalid (SwfdecAsDate *date)
{
  date->milliseconds = NAN;
}

static gint64
swfdec_as_date_get_milliseconds_utc (const SwfdecAsDate *date)
{
  g_assert (swfdec_as_date_is_valid (date));

  if (isfinite (date->milliseconds)) {
    return date->milliseconds;
  } else {
    return 0;
  }
}

static void
swfdec_as_date_set_milliseconds_utc (SwfdecAsDate *date, gint64 milliseconds)
{
  date->milliseconds = milliseconds;
}

static gint64
swfdec_as_date_get_milliseconds_local (const SwfdecAsDate *date)
{
  g_assert (swfdec_as_date_is_valid (date));

  if (isfinite (date->milliseconds)) {
    return date->milliseconds + date->timezone_offset_minutes * 60 * 1000;
  } else {
    return 0;
  }
}

static void
swfdec_as_date_set_milliseconds_local (SwfdecAsDate *date, gint64 milliseconds)
{
  date->milliseconds =
    milliseconds - date->timezone_offset_minutes * 60 * 1000;
}

static void
swfdec_as_date_get_brokentime_utc (const SwfdecAsDate *date,
    struct tm *brokentime)
{
  time_t seconds;

  g_assert (swfdec_as_date_is_valid (date));

  if (isfinite (date->milliseconds)) {
    seconds = floor (date->milliseconds / 1000);
  } else {
    seconds = 0;
  }

  // FIXME
  if (gmtime_r (&seconds, brokentime) == NULL) {
    seconds = 0;
    gmtime_r (&seconds, brokentime);
  }
}

static void
swfdec_as_date_set_brokentime_utc (SwfdecAsDate *date, struct tm *brokentime)
{
  time_t seconds = timegm (brokentime);
  if (isfinite (date->milliseconds)) {
    date->milliseconds -= floor (date->milliseconds / 1000) * 1000;
  } else {
    date->milliseconds = 0;
  }
  date->milliseconds += seconds * 1000;
}

static void
swfdec_as_date_get_brokentime_local (const SwfdecAsDate *date,
    struct tm *brokentime)
{
  time_t seconds;

  g_assert (swfdec_as_date_is_valid (date));

  if (isfinite (date->milliseconds)) {
    seconds =
      floor (date->milliseconds / 1000) + date->timezone_offset_minutes * 60;
  } else {
    seconds = 0;
  }

  // FIXME
  if (gmtime_r (&seconds, brokentime) == NULL) {
    seconds = 0;
    gmtime_r (&seconds, brokentime);
  }
}

static void
swfdec_as_date_set_brokentime_local (SwfdecAsDate *date, struct tm *brokentime)
{
  time_t seconds = timegm (brokentime) - date->timezone_offset_minutes * 60;
  if (isfinite (date->milliseconds)) {
    date->milliseconds -= floor (date->milliseconds / 1000) * 1000;
  } else {
    date->milliseconds = 0;
  }
  date->milliseconds += seconds * 1000;
}

// set and get function helpers

typedef enum {
  FIELD_SECONDS,
  FIELD_MINUTES,
  FIELD_HOURS,
  FIELD_WEEK_DAYS,
  FIELD_MONTH_DAYS,
  FIELD_MONTHS,
  FIELD_YEAR_DAYS,
  FIELD_YEAR,
  FIELD_FULL_YEAR
} field_t;

static int field_offsets[] = {
  G_STRUCT_OFFSET (struct tm, tm_sec),
  G_STRUCT_OFFSET (struct tm, tm_min),
  G_STRUCT_OFFSET (struct tm, tm_hour),
  G_STRUCT_OFFSET (struct tm, tm_wday),
  G_STRUCT_OFFSET (struct tm, tm_mday),
  G_STRUCT_OFFSET (struct tm, tm_mon),
  G_STRUCT_OFFSET (struct tm, tm_yday),
  G_STRUCT_OFFSET (struct tm, tm_year),
  G_STRUCT_OFFSET (struct tm, tm_year)
};

static int
swfdec_as_date_get_brokentime_value (SwfdecAsDate *date, gboolean utc,
    int field_offset)
{
  struct tm brokentime;

  if (utc) {
    swfdec_as_date_get_brokentime_utc (date, &brokentime);
  } else {
    swfdec_as_date_get_brokentime_local (date, &brokentime);
  }

  return G_STRUCT_MEMBER (int, &brokentime, field_offset);
}

static void
swfdec_as_date_set_brokentime_value (SwfdecAsDate *date, gboolean utc,
    int field_offset, SwfdecAsContext *cx, int number)
{
  struct tm brokentime;

  if (utc) {
    swfdec_as_date_get_brokentime_utc (date, &brokentime);
  } else {
    swfdec_as_date_get_brokentime_local (date, &brokentime);
  }

  G_STRUCT_MEMBER (int, &brokentime, field_offset) = number;

  if (utc) {
    swfdec_as_date_set_brokentime_utc (date, &brokentime);
  } else {
    swfdec_as_date_set_brokentime_local (date, &brokentime);
  }
}

static void
swfdec_as_date_set_field (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret, field_t field,
    gboolean utc)
{
  if (!swfdec_as_date_is_valid (SWFDEC_AS_DATE (object)))
    swfdec_as_value_to_number (cx, &argv[0]); // calls valueOf

  if (swfdec_as_date_is_valid (SWFDEC_AS_DATE (object)) && argc > 0)
  {
    SwfdecAsDate *date = SWFDEC_AS_DATE (object);
    gboolean set;
    gint64 milliseconds;
    double d;
    int number;

    set = TRUE;
    swfdec_as_date_value_to_number_and_integer (cx, &argv[0], &d, &number);

    switch (field) {
      case FIELD_MONTHS:
	if (!isfinite (d)) {
	  if (!isnan (d)) {
	    swfdec_as_date_set_brokentime_value (date, utc,
		field_offsets[FIELD_YEAR], cx, 0 - 1900);
	  }
	  swfdec_as_date_set_brokentime_value (date, utc, field_offsets[field],
	      cx, 0);
	  set = FALSE;
	}
	break;
      case FIELD_YEAR:
	// NOTE: Test against double, not the integer
	if (d >= 100 || d < 0)
		number -= 1900;
	// fall trough
      case FIELD_FULL_YEAR:
	if (!isfinite (d)) {
	  swfdec_as_date_set_brokentime_value (date, utc, field_offsets[field],
	      cx, 0 - 1900);
	  set = FALSE;
	}
	break;
      default:
	if (!isfinite (d)) {
	  swfdec_as_date_set_invalid (date);
	  set = FALSE;
	}
	break;
    }

    if (set) {
      swfdec_as_date_set_brokentime_value (date, utc, field_offsets[field], cx,
	  number);
    }

    if (swfdec_as_date_is_valid (date)) {
      milliseconds = swfdec_as_date_get_milliseconds_utc (date);
      if (milliseconds < -8.64e15 || milliseconds > 8.64e15)
	swfdec_as_date_set_invalid (date);
    }
  }

  swfdec_as_date_valueOf (cx, object, 0, NULL, ret);
}

static void
swfdec_as_date_get_field (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret, field_t field,
    gboolean utc)
{
  int number;

  if (!swfdec_as_date_is_valid (SWFDEC_AS_DATE (object))) {
    SWFDEC_AS_VALUE_SET_NUMBER (ret, NAN);
    return;
  }

  number = swfdec_as_date_get_brokentime_value (SWFDEC_AS_DATE (object), utc,
      field_offsets[field]);

  switch (field) {
    case FIELD_FULL_YEAR:
      number += 1900;
      break;
    default:
      break;
  }

  SWFDEC_AS_VALUE_SET_INT (ret, number);
}

/*** AS CODE ***/

static void
swfdec_as_date_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date = SWFDEC_AS_DATE (object);
  char buffer[256];
  struct tm brokentime;

  if (!swfdec_as_date_is_valid (date)) {
    SWFDEC_AS_VALUE_SET_STRING (ret, "Invalid Date");
    return;
  }

  swfdec_as_date_get_brokentime_local (date, &brokentime);
  if (!strftime (buffer, sizeof (buffer), "%a %b %-d %T", &brokentime)) {
    SWFDEC_AS_VALUE_SET_STRING (ret, "Invalid Date");
    return;
  }
  g_snprintf (buffer + strlen (buffer), sizeof (buffer) - strlen (buffer),
      " GMT%+03i%02i", date->timezone_offset_minutes / 60,
      ABS (date->timezone_offset_minutes % 60));
  if (!strftime (buffer + strlen (buffer), sizeof (buffer) - strlen (buffer),
      " %Y", &brokentime)) {
    SWFDEC_AS_VALUE_SET_STRING (ret, "Invalid Date");
    return;
  }

  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (cx, buffer));
}

static void
swfdec_as_date_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date = SWFDEC_AS_DATE (object);

  // milliseconds since epoch, UTC
  // including fractions of milliseconds
  SWFDEC_AS_VALUE_SET_NUMBER (ret, date->milliseconds);
}

static void
swfdec_as_date_getTimezoneOffset (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  // reverse of timezone_offset_minutes
  SWFDEC_AS_VALUE_SET_NUMBER (ret,
      -SWFDEC_AS_DATE (object)->timezone_offset_minutes);
}

// get* functions

static void
swfdec_as_date_getTime (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_valueOf (cx, object, 0, NULL, ret);
}

static void
swfdec_as_date_getMilliseconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date = SWFDEC_AS_DATE (object);
  gint64 milliseconds;

  if (!swfdec_as_date_is_valid (date)) {
    SWFDEC_AS_VALUE_SET_NUMBER (ret, NAN);
    return;
  }

  milliseconds = swfdec_as_date_get_milliseconds_local (date);

  if (milliseconds >= 0 || (milliseconds % 1000 == 0)) {
    SWFDEC_AS_VALUE_SET_INT (ret, milliseconds % 1000);
  } else {
    SWFDEC_AS_VALUE_SET_INT (ret, 1000 + milliseconds % 1000);
  }
}

static void
swfdec_as_date_getUTCMilliseconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date = SWFDEC_AS_DATE (object);
  gint64 milliseconds;

  if (!swfdec_as_date_is_valid (date)) {
    SWFDEC_AS_VALUE_SET_NUMBER (ret, NAN);
    return;
  }

  milliseconds = swfdec_as_date_get_milliseconds_utc (date);

  if (milliseconds >= 0 || (milliseconds % 1000 == 0)) {
    SWFDEC_AS_VALUE_SET_INT (ret, milliseconds % 1000);
  } else {
    SWFDEC_AS_VALUE_SET_INT (ret, 1000 + milliseconds % 1000);
  }
}

static void
swfdec_as_date_getSeconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_SECONDS, FALSE);
}

static void
swfdec_as_date_getUTCSeconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_SECONDS, TRUE);
}

static void
swfdec_as_date_getMinutes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MINUTES, FALSE);
}

static void
swfdec_as_date_getUTCMinutes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MINUTES, TRUE);
}

static void
swfdec_as_date_getHours (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_HOURS, FALSE);
}

static void
swfdec_as_date_getUTCHours (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_HOURS, TRUE);
}

static void
swfdec_as_date_getDay (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_WEEK_DAYS,
      FALSE);
}

static void
swfdec_as_date_getUTCDay (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_WEEK_DAYS,
      TRUE);
}

static void
swfdec_as_date_getDate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MONTH_DAYS,
      FALSE);
}

static void
swfdec_as_date_getUTCDate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MONTH_DAYS,
      TRUE);
}

static void
swfdec_as_date_getMonth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MONTHS, FALSE);
}

static void
swfdec_as_date_getUTCMonth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MONTHS, TRUE);
}

static void
swfdec_as_date_getYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_YEAR, FALSE);
}

static void
swfdec_as_date_getUTCYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_YEAR, TRUE);
}

static void
swfdec_as_date_getFullYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_FULL_YEAR,
      FALSE);
}

static void
swfdec_as_date_getUTCFullYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_FULL_YEAR,
      TRUE);
}

// set* functions

static void
swfdec_as_date_setTime (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (argc > 0) {
    SwfdecAsDate *date = SWFDEC_AS_DATE (object);
    swfdec_as_date_set_milliseconds_utc (date,
	swfdec_as_value_to_integer (cx, &argv[0]));
  }

  swfdec_as_date_valueOf (cx, object, 0, NULL, ret);
}

static void
swfdec_as_date_setMilliseconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date = SWFDEC_AS_DATE (object);

  if (swfdec_as_date_is_valid (date) && argc > 0) {
    gint64 milliseconds = swfdec_as_date_get_milliseconds_local (date);
    milliseconds = milliseconds - milliseconds % 1000 +
      swfdec_as_value_to_integer (cx, &argv[0]);
    swfdec_as_date_set_milliseconds_local (date, milliseconds);
  }

  swfdec_as_date_valueOf (cx, object, 0, NULL, ret);
}

static void
swfdec_as_date_setUTCMilliseconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date = SWFDEC_AS_DATE (object);

  if (swfdec_as_date_is_valid (date) && argc > 0) {
    gint64 milliseconds = swfdec_as_date_get_milliseconds_utc (date);
    milliseconds = milliseconds - milliseconds % 1000 +
      swfdec_as_value_to_integer (cx, &argv[0]);
    swfdec_as_date_set_milliseconds_utc (date, milliseconds);
  }

  swfdec_as_date_valueOf (cx, object, 0, NULL, ret);
}

static void
swfdec_as_date_setSeconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_SECONDS, FALSE);
}

static void
swfdec_as_date_setUTCSeconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_SECONDS, TRUE);
}

static void
swfdec_as_date_setMinutes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MINUTES, FALSE);
}

static void
swfdec_as_date_setUTCMinutes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MINUTES, TRUE);
}

static void
swfdec_as_date_setHours (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_HOURS, FALSE);
}

static void
swfdec_as_date_setUTCHours (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_HOURS, TRUE);
}

static void
swfdec_as_date_setDate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MONTH_DAYS,
      FALSE);
}

static void
swfdec_as_date_setUTCDate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MONTH_DAYS,
      TRUE);
}

static void
swfdec_as_date_setMonth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MONTHS, FALSE);
}

static void
swfdec_as_date_setUTCMonth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MONTHS, TRUE);
}

static void
swfdec_as_date_setYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_YEAR, FALSE);
}

static void
swfdec_as_date_setUTCYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_YEAR, TRUE);
}

static void
swfdec_as_date_setFullYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_FULL_YEAR,
      FALSE);
}

static void
swfdec_as_date_setUTCFullYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_FULL_YEAR,
      TRUE);
}

// Static methods

// gets atleast two args
static void
swfdec_as_date_UTC (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  guint i;
  gint64 milliseconds;
  int year, num;
  double d;
  struct tm brokentime;

  // special case: ignore undefined and everything after it
  for (i = 0; i < argc; i++) {
    if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[i])) {
      argc = i;
      break;
    }
  }

  memset (&brokentime, 0, sizeof (brokentime));

  i = 0;

  if (argc > i) {
    if (swfdec_as_date_value_to_number_and_integer_floor (cx, &argv[i++], &d,
	  &num)) {
      year = num;
    } else {
      // special case: if year is not finite set it to -1900
      year = -1900;
    }
  }

  // if we don't got atleast two values, return undefined
  // do it only here, so valueOf first arg is called
  if (argc < 2) {
    return;
  }

  if (argc > i) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	  &num)) {
      brokentime.tm_mon = num;
    } else {
      // special case: if month is not finite set year to -1900
      year = -1900;
      brokentime.tm_mon = 0;
    }
  }

  if (argc > i) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	  &num)) {
      brokentime.tm_mday = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  } else {
    brokentime.tm_mday = 1;
  }

  if (argc > i) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	  &num)) {
      brokentime.tm_hour = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  }

  if (argc > i) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	  &num)) {
      brokentime.tm_min = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  }

  if (argc > i) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	  &num)) {
      brokentime.tm_sec = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  }

  if (year >= 100) {
    brokentime.tm_year = year - 1900;
  } else {
    brokentime.tm_year = year;
  }

  milliseconds = timegm (&brokentime) * 1000;

  if (argc > i) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	  &num)) {
      milliseconds += num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  }

  SWFDEC_AS_VALUE_SET_INT (ret, milliseconds);
}

// Constructor

static void
swfdec_as_date_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  guint i;
  SwfdecAsDate *date;

  if (!cx->frame->construct) {
    SwfdecAsValue val;
    if (!swfdec_as_context_use_mem (cx, sizeof (SwfdecAsDate)))
      return;
    object = g_object_new (SWFDEC_TYPE_AS_DATE, NULL);
    swfdec_as_object_add (object, cx, sizeof (SwfdecAsDate));
    swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_Date, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      swfdec_as_object_set_constructor (object,
	  SWFDEC_AS_VALUE_GET_OBJECT (&val));
    } else {
      SWFDEC_INFO ("\"Date\" is not an object");
    }
  }

  date = SWFDEC_AS_DATE (object);

  // special case: ignore undefined and everything after it
  for (i = 0; i < argc; i++) {
    if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[i])) {
      argc = i;
      break;
    }
  }

  if (argc == 0) // current time, local
  {
    struct timeval tp;

    gettimeofday (&tp, NULL);
    swfdec_as_date_set_milliseconds_local (date,
	tp.tv_sec * 1000 + tp.tv_usec / 1000);
  }
  else if (argc == 1) // milliseconds from epoch, local
  {
    // need to save directly to keep fractions of a milliseconds
    date->milliseconds = swfdec_as_value_to_number (cx, &argv[0]);
  }
  else // year, month etc. local
  {
    gint64 milliseconds;
    int year, num;
    double d;
    struct tm brokentime;

    date->milliseconds = 0;

    memset (&brokentime, 0, sizeof (brokentime));

    i = 0;

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer_floor (cx, &argv[i++], &d,
	    &num)) {
	year = num;
      } else {
	// special case: if year is not finite set it to -1900
	year = -1900;
      }
    } else {
      year = -1900;
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.tm_mon = num;
      } else {
	// special case: if month is not finite set year to -1900
	year = -1900;
	brokentime.tm_mon = 0;
      }
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.tm_mday = num;
      } else {
	date->milliseconds = d;
      }
    } else {
      brokentime.tm_mday = 1;
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.tm_hour = num;
      } else {
	date->milliseconds = d;
      }
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.tm_min = num;
      } else {
	date->milliseconds = d;
      }
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.tm_sec = num;
      } else {
	date->milliseconds = d;
      }
    }

    if (year >= 100) {
      brokentime.tm_year = year - 1900;
    } else {
      brokentime.tm_year = year;
    }

    milliseconds = timegm (&brokentime) * 1000;

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	milliseconds += num;
      } else {
	date->milliseconds = d;
      }
    }

    if (date->milliseconds == 0)
      swfdec_as_date_set_milliseconds_local (date, milliseconds);
  }

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

void
swfdec_as_date_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsObject *date, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  date = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_Date, 0, swfdec_as_date_construct, 0));
  swfdec_as_native_function_set_construct_type (
      SWFDEC_AS_NATIVE_FUNCTION (date), SWFDEC_TYPE_AS_DATE);
  if (!date)
    return;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsDate)))
    return;
  proto = g_object_new (SWFDEC_TYPE_AS_DATE, NULL);
  swfdec_as_object_add (proto, context, sizeof (SwfdecAsDate));
  /* set the right properties on the Date object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (date, SWFDEC_AS_STR_prototype, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
  swfdec_as_object_set_variable (date, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (date, SWFDEC_AS_STR_UTC, 0,
      swfdec_as_date_UTC, 1);
  /* set the right properties on the Date.prototype object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, date);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, 0,
      swfdec_as_date_toString, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_valueOf, 0,
      swfdec_as_date_valueOf, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getTime, 0,
      swfdec_as_date_getTime, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getTimezoneOffset, 0,
      swfdec_as_date_getTimezoneOffset, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getMilliseconds, 0,
      swfdec_as_date_getMilliseconds, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCMilliseconds, 0,
      swfdec_as_date_getUTCMilliseconds, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getSeconds, 0,
      swfdec_as_date_getSeconds, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCSeconds, 0,
      swfdec_as_date_getUTCSeconds, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getMinutes, 0,
      swfdec_as_date_getMinutes, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCMinutes, 0,
      swfdec_as_date_getUTCMinutes, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getHours, 0,
      swfdec_as_date_getHours, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCHours, 0,
      swfdec_as_date_getUTCHours, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getDay, 0,
      swfdec_as_date_getDay, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCDay, 0,
      swfdec_as_date_getUTCDay, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getDate, 0,
      swfdec_as_date_getDate, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCDate, 0,
      swfdec_as_date_getUTCDate, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getMonth, 0,
      swfdec_as_date_getMonth, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCMonth, 0,
      swfdec_as_date_getUTCMonth, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getYear, 0,
      swfdec_as_date_getYear, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCYear, 0,
      swfdec_as_date_getUTCYear, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getFullYear, 0,
      swfdec_as_date_getFullYear, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getUTCFullYear, 0,
      swfdec_as_date_getUTCFullYear, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setTime, 0,
      swfdec_as_date_setTime, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setMilliseconds, 0,
      swfdec_as_date_setMilliseconds, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setUTCMilliseconds, 0,
      swfdec_as_date_setUTCMilliseconds, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setSeconds, 0,
      swfdec_as_date_setSeconds, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setUTCSeconds, 0,
      swfdec_as_date_setUTCSeconds, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setMinutes, 0,
      swfdec_as_date_setMinutes, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setUTCMinutes, 0,
      swfdec_as_date_setUTCMinutes, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setHours, 0,
      swfdec_as_date_setHours, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setUTCHours, 0,
      swfdec_as_date_setUTCHours, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setDate, 0,
      swfdec_as_date_setDate, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setUTCDate, 0,
      swfdec_as_date_setUTCDate, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setMonth, 0,
      swfdec_as_date_setMonth, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setUTCMonth, 0,
      swfdec_as_date_setUTCMonth, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setYear, 0,
      swfdec_as_date_setYear, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setUTCYear, 0,
      swfdec_as_date_setUTCYear, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setFullYear, 0,
      swfdec_as_date_setFullYear, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_setUTCFullYear, 0,
      swfdec_as_date_setUTCFullYear, 0);
}
