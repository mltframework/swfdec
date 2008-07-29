/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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
#include <math.h>

#include "swfdec_as_date.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_system.h"
#include "swfdec_player_internal.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsDate, swfdec_as_date, SWFDEC_TYPE_AS_OBJECT)

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
}

/*** Helper functions ***/

/* Kind of replacement for gmtime_r, timegm that works the way Flash works */

#define MILLISECONDS_PER_SECOND 1000
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define MONTHS_PER_YEAR 12

#define MILLISECONDS_PER_MINUTE 60000
#define MILLISECONDS_PER_HOUR 3600000
#define MILLISECONDS_PER_DAY 86400000

static const int month_offsets[2][13] = {
  // Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec  Total
  {    0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
  {    0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

typedef struct {
  int milliseconds;
  int seconds;
  int minutes;
  int hours;
  int day_of_month;
  int month;
  int year;

  int day_of_week;
} BrokenTime;

static int
swfdec_as_date_days_in_year (int year)
{
  if (year % 4) {
    return 365;
  } else if (year % 100) {
    return 366;
  } else if (year % 400) {
    return 365;
  } else {
    return 366;
  }
}

#define IS_LEAP(year) (swfdec_as_date_days_in_year ((year)) == 366)

static double
swfdec_as_date_days_since_utc_for_year (double year)
{
  return floor (
      365 * (year - 1970) +
      floor (((year - 1969) / 4.0f)) -
      floor (((year - 1901) / 100.0f)) +
      floor (((year - 1601) / 400.0f))
    );
}

static double
swfdec_as_date_days_from_utc_to_year (double days)
{
  double low, high, pivot;

  low = floor ((days >= 0 ? days / 366.0 : days / 365.0)) + 1970;
  high = ceil ((days >= 0 ? days / 365.0 : days / 366.0)) + 1970;

  while (low < high) {
    pivot = floor ((low + high) / 2.0);

    if (swfdec_as_date_days_since_utc_for_year (pivot) <= days) {
      if (swfdec_as_date_days_since_utc_for_year (pivot + 1) > days) {
	high = low = pivot;
      } else {
	low = pivot + 1;
      }
    } else {
      high = pivot - 1;
    }
  }

  return low;
}

static void
swfdec_as_date_milliseconds_to_brokentime (double milliseconds,
    BrokenTime *brokentime)
{
  double remaining;
  double year;

  g_assert (brokentime != NULL);

  /* special case: hours are calculated from different value */
  if (isfinite (milliseconds)) {
    remaining = floor (milliseconds + 0.5);
  } else {
    remaining = 0;
  }

  remaining = floor (remaining / MILLISECONDS_PER_HOUR);
  brokentime->hours = fmod (remaining, HOURS_PER_DAY);

  /* hours done, on with the rest */
  if (isfinite (milliseconds)) {
    remaining = milliseconds;
  } else {
    remaining = 0;
  }

  brokentime->milliseconds = fmod (remaining, MILLISECONDS_PER_SECOND);
  remaining = floor (remaining / MILLISECONDS_PER_SECOND);

  brokentime->seconds = fmod (remaining, SECONDS_PER_MINUTE);
  remaining = floor (remaining / SECONDS_PER_MINUTE);

  brokentime->minutes = fmod (remaining, MINUTES_PER_HOUR);
  remaining = floor (remaining / MINUTES_PER_HOUR);
  remaining = floor (remaining / HOURS_PER_DAY);

  if (milliseconds < 0) {
    if (brokentime->milliseconds < 0)
      brokentime->milliseconds += MILLISECONDS_PER_SECOND;
    if (brokentime->seconds < 0)
      brokentime->seconds += SECONDS_PER_MINUTE;
    if (brokentime->minutes < 0)
      brokentime->minutes += MINUTES_PER_HOUR;
    if (brokentime->hours < 0)
      brokentime->hours += HOURS_PER_DAY;
  }

  // now remaining == days since 1970

  if (isfinite (milliseconds)) {
    brokentime->day_of_week = fmod ((remaining + 4), 7);
    if (brokentime->day_of_week < 0)
      brokentime->day_of_week += 7;
  } else {
    // special case
    brokentime->day_of_week = 0;
  }

  year = swfdec_as_date_days_from_utc_to_year (remaining);
  brokentime->year = year - 1900;

  remaining -= swfdec_as_date_days_since_utc_for_year (year);
  g_assert (remaining >= 0 && remaining <= 365);

  brokentime->month = 0;
  while (month_offsets[IS_LEAP (year)][brokentime->month + 1] <= remaining)
    brokentime->month++;

  brokentime->day_of_month =
    remaining - month_offsets[IS_LEAP (year)][brokentime->month] + 1;
}

static double
swfdec_as_date_brokentime_to_milliseconds (const BrokenTime *brokentime)
{
  double milliseconds;
  int month, year;

  year = 1900 + brokentime->year;

  milliseconds = brokentime->milliseconds;
  milliseconds += brokentime->seconds * MILLISECONDS_PER_SECOND;
  milliseconds += brokentime->minutes * MILLISECONDS_PER_MINUTE;
  milliseconds += brokentime->hours * MILLISECONDS_PER_HOUR;
  milliseconds += (double)(brokentime->day_of_month - 1) * MILLISECONDS_PER_DAY;

  milliseconds +=
    swfdec_as_date_days_since_utc_for_year (year) * MILLISECONDS_PER_DAY;

  for (month = brokentime->month; month < 0; month += MONTHS_PER_YEAR) {
    milliseconds -=
      (double)month_offsets[IS_LEAP (--year)][MONTHS_PER_YEAR] * MILLISECONDS_PER_DAY;
  }

  for (month = month; month >= MONTHS_PER_YEAR; month -= MONTHS_PER_YEAR) {
    milliseconds +=
      (double)month_offsets[IS_LEAP (year++)][MONTHS_PER_YEAR] * MILLISECONDS_PER_DAY;
  }

  milliseconds += (double)month_offsets[IS_LEAP (year)][month] * MILLISECONDS_PER_DAY;

  return milliseconds;
}

/* Wrappers for swfdec_as_value_to_number because we need both double and int
 * often, and need to generate the right valueOf etc. */

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

/* The functions to query/modify the current time */

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

static double
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
swfdec_as_date_set_milliseconds_utc (SwfdecAsDate *date, double milliseconds)
{
  date->milliseconds = milliseconds;
}

/*static double
swfdec_as_date_get_milliseconds_local (const SwfdecAsDate *date)
{
  g_assert (swfdec_as_date_is_valid (date));

  if (isfinite (date->milliseconds)) {
    return date->milliseconds + (double) date->utc_offset * 60 * 1000;
  } else {
    return 0;
  }
}*/

static void
swfdec_as_date_set_milliseconds_local (SwfdecAsDate *date, double milliseconds)
{
  date->milliseconds =
    milliseconds - (double) date->utc_offset * 60 * 1000;
}

static void
swfdec_as_date_get_brokentime_utc (const SwfdecAsDate *date,
    BrokenTime *brokentime)
{
  g_assert (swfdec_as_date_is_valid (date));

  swfdec_as_date_milliseconds_to_brokentime (date->milliseconds, brokentime);
}

static void
swfdec_as_date_set_brokentime_utc (SwfdecAsDate *date, BrokenTime *brokentime)
{
  date->milliseconds = swfdec_as_date_brokentime_to_milliseconds (brokentime);
}

static void
swfdec_as_date_get_brokentime_local (const SwfdecAsDate *date,
    BrokenTime *brokentime)
{
  g_assert (swfdec_as_date_is_valid (date));

  swfdec_as_date_milliseconds_to_brokentime (
      date->milliseconds + date->utc_offset * 60 * 1000, brokentime);
}

static void
swfdec_as_date_set_brokentime_local (SwfdecAsDate *date, BrokenTime *brokentime)
{
  date->milliseconds = swfdec_as_date_brokentime_to_milliseconds (brokentime) -
    date->utc_offset * 60 * 1000;
}

/* set and get function helpers */

typedef enum {
  FIELD_MILLISECONDS,
  FIELD_SECONDS,
  FIELD_MINUTES,
  FIELD_HOURS,
  FIELD_WEEK_DAYS,
  FIELD_MONTH_DAYS,
  FIELD_MONTHS,
  FIELD_YEAR,
  FIELD_FULL_YEAR
} field_t;

static int field_offsets[] = {
  G_STRUCT_OFFSET (BrokenTime, milliseconds),
  G_STRUCT_OFFSET (BrokenTime, seconds),
  G_STRUCT_OFFSET (BrokenTime, minutes),
  G_STRUCT_OFFSET (BrokenTime, hours),
  G_STRUCT_OFFSET (BrokenTime, day_of_week),
  G_STRUCT_OFFSET (BrokenTime, day_of_month),
  G_STRUCT_OFFSET (BrokenTime, month),
  G_STRUCT_OFFSET (BrokenTime, year),
  G_STRUCT_OFFSET (BrokenTime, year)
};

static int
swfdec_as_date_get_brokentime_value (SwfdecAsDate *date, gboolean utc,
    int field_offset)
{
  BrokenTime brokentime;

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
  BrokenTime brokentime;

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
  SwfdecAsDate *date;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_DATE, &date, "");

  if (!swfdec_as_date_is_valid (date))
    swfdec_as_value_to_number (cx, &argv[0]); // calls valueOf

  if (swfdec_as_date_is_valid (date) && argc > 0)
  {
    gboolean set;
    double milliseconds;
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
	if (d >= 0 && d < 100)
	  number += 1900;
	// fall trough
      case FIELD_FULL_YEAR:
	number -= 1900;
	if (!isfinite (d)) {
	  swfdec_as_date_set_brokentime_value (date, utc, field_offsets[field],
	      cx, 0 - 1900);
	  set = FALSE;
	}
	break;
      case FIELD_MILLISECONDS:
      case FIELD_SECONDS:
      case FIELD_MINUTES:
      case FIELD_HOURS:
      case FIELD_WEEK_DAYS:
      case FIELD_MONTH_DAYS:
	if (!isfinite (d)) {
	  swfdec_as_date_set_invalid (date);
	  set = FALSE;
	}
	break;
      default:
	g_assert_not_reached ();
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

  SWFDEC_AS_VALUE_SET_NUMBER (ret, date->milliseconds);
}

static void
swfdec_as_date_get_field (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret, field_t field,
    gboolean utc)
{
  SwfdecAsDate *date;
  int number;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_DATE, &date, "");

  if (!swfdec_as_date_is_valid (date)) {
    SWFDEC_AS_VALUE_SET_NUMBER (ret, NAN);
    return;
  }

  number = swfdec_as_date_get_brokentime_value (date, utc,
      field_offsets[field]);

  if (field == FIELD_FULL_YEAR)
    number += 1900;

  SWFDEC_AS_VALUE_SET_INT (ret, number);
}

/*** AS CODE ***/

SWFDEC_AS_NATIVE (103, 19, swfdec_as_date_toString)
void
swfdec_as_date_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  static const char *weekday_names[] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  static const char *month_names[] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  SwfdecAsDate *date;
  BrokenTime brokentime;
  char *result;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_DATE, &date, "");

  if (!swfdec_as_date_is_valid (date)) {
    SWFDEC_AS_VALUE_SET_STRING (ret, "Invalid Date");
    return;
  }

  swfdec_as_date_get_brokentime_local (date, &brokentime);

  result = g_strdup_printf ("%s %s %i %02i:%02i:%02i GMT%+03i%02i %i",
      weekday_names[brokentime.day_of_week % 7],
      month_names[brokentime.month % 12],
      brokentime.day_of_month,
      brokentime.hours, brokentime.minutes, brokentime.seconds,
      date->utc_offset / 60, ABS (date->utc_offset % 60),
      1900 + brokentime.year);

  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, result));
}

SWFDEC_AS_NATIVE (103, 16, swfdec_as_date_getTime)
void
swfdec_as_date_getTime (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_DATE, &date, "");

  SWFDEC_AS_VALUE_SET_NUMBER (ret, date->milliseconds);
}

SWFDEC_AS_NATIVE (103, 18, swfdec_as_date_getTimezoneOffset)
void
swfdec_as_date_getTimezoneOffset (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_DATE, &date, "");

  // reverse of utc_offset
  SWFDEC_AS_VALUE_SET_NUMBER (ret, -(date->utc_offset));
}

// get* functions

SWFDEC_AS_NATIVE (103, 8, swfdec_as_date_getMilliseconds)
void
swfdec_as_date_getMilliseconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MILLISECONDS,
      FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 8, swfdec_as_date_getUTCMilliseconds)
void
swfdec_as_date_getUTCMilliseconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MILLISECONDS,
      TRUE);
}

SWFDEC_AS_NATIVE (103, 7, swfdec_as_date_getSeconds)
void
swfdec_as_date_getSeconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_SECONDS, FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 7, swfdec_as_date_getUTCSeconds)
void
swfdec_as_date_getUTCSeconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_SECONDS, TRUE);
}

SWFDEC_AS_NATIVE (103, 6, swfdec_as_date_getMinutes)
void
swfdec_as_date_getMinutes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MINUTES, FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 6, swfdec_as_date_getUTCMinutes)
void
swfdec_as_date_getUTCMinutes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MINUTES, TRUE);
}

SWFDEC_AS_NATIVE (103, 5, swfdec_as_date_getHours)
void
swfdec_as_date_getHours (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_HOURS, FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 5, swfdec_as_date_getUTCHours)
void
swfdec_as_date_getUTCHours (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_HOURS, TRUE);
}

SWFDEC_AS_NATIVE (103, 4, swfdec_as_date_getDay)
void
swfdec_as_date_getDay (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_WEEK_DAYS,
      FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 4, swfdec_as_date_getUTCDay)
void
swfdec_as_date_getUTCDay (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_WEEK_DAYS,
      TRUE);
}

SWFDEC_AS_NATIVE (103, 3, swfdec_as_date_getDate)
void
swfdec_as_date_getDate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MONTH_DAYS,
      FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 3, swfdec_as_date_getUTCDate)
void
swfdec_as_date_getUTCDate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MONTH_DAYS,
      TRUE);
}

SWFDEC_AS_NATIVE (103, 2, swfdec_as_date_getMonth)
void
swfdec_as_date_getMonth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MONTHS, FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 2, swfdec_as_date_getUTCMonth)
void
swfdec_as_date_getUTCMonth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_MONTHS, TRUE);
}

SWFDEC_AS_NATIVE (103, 1, swfdec_as_date_getYear)
void
swfdec_as_date_getYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_YEAR, FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 1, swfdec_as_date_getUTCYear)
void
swfdec_as_date_getUTCYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_YEAR, TRUE);
}

SWFDEC_AS_NATIVE (103, 0, swfdec_as_date_getFullYear)
void
swfdec_as_date_getFullYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_FULL_YEAR,
      FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 0, swfdec_as_date_getUTCFullYear)
void
swfdec_as_date_getUTCFullYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_get_field (cx, object, argc, argv, ret, FIELD_FULL_YEAR,
      TRUE);
}

// set* functions

SWFDEC_AS_NATIVE (103, 17, swfdec_as_date_setTime)
void
swfdec_as_date_setTime (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsDate *date;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_AS_DATE, &date, "");

  if (argc > 0) {
    swfdec_as_date_set_milliseconds_utc (date,
	trunc (swfdec_as_value_to_number (cx, &argv[0])));
  }

  SWFDEC_AS_VALUE_SET_NUMBER (ret, date->milliseconds);
}

SWFDEC_AS_NATIVE (103, 15, swfdec_as_date_setMilliseconds)
void
swfdec_as_date_setMilliseconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MILLISECONDS,
      FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 15, swfdec_as_date_setUTCMilliseconds)
void
swfdec_as_date_setUTCMilliseconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MILLISECONDS,
      TRUE);
}

SWFDEC_AS_NATIVE (103, 14, swfdec_as_date_setSeconds)
void
swfdec_as_date_setSeconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_SECONDS, FALSE);

  if (argc > 1)
    swfdec_as_date_setMilliseconds (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 128 + 14, swfdec_as_date_setUTCSeconds)
void
swfdec_as_date_setUTCSeconds (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_SECONDS, TRUE);

  if (argc > 1)
    swfdec_as_date_setUTCMilliseconds (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 13, swfdec_as_date_setMinutes)
void
swfdec_as_date_setMinutes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MINUTES, FALSE);

  if (argc > 1)
    swfdec_as_date_setSeconds (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 128 + 13, swfdec_as_date_setUTCMinutes)
void
swfdec_as_date_setUTCMinutes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MINUTES, TRUE);

  if (argc > 1)
    swfdec_as_date_setUTCSeconds (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 12, swfdec_as_date_setHours)
void
swfdec_as_date_setHours (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_HOURS, FALSE);

  if (argc > 1)
    swfdec_as_date_setMinutes (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 128 + 12, swfdec_as_date_setUTCHours)
void
swfdec_as_date_setUTCHours (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_HOURS, TRUE);

  if (argc > 1)
    swfdec_as_date_setUTCMinutes (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 11, swfdec_as_date_setDate)
void
swfdec_as_date_setDate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MONTH_DAYS,
      FALSE);
}

SWFDEC_AS_NATIVE (103, 128 + 11, swfdec_as_date_setUTCDate)
void
swfdec_as_date_setUTCDate (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MONTH_DAYS,
      TRUE);
}

SWFDEC_AS_NATIVE (103, 10, swfdec_as_date_setMonth)
void
swfdec_as_date_setMonth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MONTHS, FALSE);

  if (argc > 1)
    swfdec_as_date_setDate (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 128 + 10, swfdec_as_date_setUTCMonth)
void
swfdec_as_date_setUTCMonth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_MONTHS, TRUE);

  if (argc > 1)
    swfdec_as_date_setUTCDate (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 20, swfdec_as_date_setYear)
void
swfdec_as_date_setYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_YEAR, FALSE);

  if (argc > 1)
    swfdec_as_date_setMonth (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 9, swfdec_as_date_setFullYear)
void
swfdec_as_date_setFullYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_FULL_YEAR,
      FALSE);

  if (argc > 1)
    swfdec_as_date_setMonth (cx, object, argc - 1, argv + 1, ret);
}

SWFDEC_AS_NATIVE (103, 128 + 9, swfdec_as_date_setUTCFullYear)
void
swfdec_as_date_setUTCFullYear (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  swfdec_as_date_set_field (cx, object, argc, argv, ret, FIELD_FULL_YEAR,
      TRUE);

  if (argc > 1)
    swfdec_as_date_setUTCMonth (cx, object, argc - 1, argv + 1, ret);
}

// Static methods

SWFDEC_AS_NATIVE (103, 257, swfdec_as_date_UTC)
void
swfdec_as_date_UTC (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  guint i;
  int year, num;
  double d;
  BrokenTime brokentime;

  // special case: ignore undefined and everything after it
  for (i = 0; i < argc; i++) {
    if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[i])) {
      argc = i;
      break;
    }
  }

  memset (&brokentime, 0, sizeof (brokentime));

  if (argc > 0) {
    if (swfdec_as_date_value_to_number_and_integer_floor (cx, &argv[0], &d,
	  &num)) {
      year = num;
    } else {
      // special case: if year is not finite set it to -1900
      year = -1900;
    }
  } else {
    return;
  }

  // if we don't got atleast two values, return undefined
  // do it only here, so valueOf first arg is called
  if (argc < 2) 
    return;

  if (swfdec_as_date_value_to_number_and_integer (cx, &argv[1], &d,
	&num)) {
    brokentime.month = num;
  } else {
    // special case: if month is not finite set year to -1900
    year = -1900;
    brokentime.month = 0;
  }

  if (argc > 2) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[2], &d,
	  &num)) {
      brokentime.day_of_month = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  } else {
    brokentime.day_of_month = 1;
  }

  if (argc > 3) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[3], &d,
	  &num)) {
      brokentime.hours = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  }

  if (argc > 4) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[4], &d,
	  &num)) {
      brokentime.minutes = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  }

  if (argc > 5) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[5], &d,
	  &num)) {
      brokentime.seconds = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  }

  if (year >= 100) {
    brokentime.year = year - 1900;
  } else {
    brokentime.year = year;
  }

  if (argc > 6) {
    if (swfdec_as_date_value_to_number_and_integer (cx, &argv[6], &d,
	  &num)) {
      brokentime.milliseconds = num;
    } else {
      SWFDEC_AS_VALUE_SET_NUMBER (ret, d);
      return;
    }
  }

  SWFDEC_AS_VALUE_SET_NUMBER (ret,
      swfdec_as_date_brokentime_to_milliseconds (&brokentime));
}

// Constructor

SWFDEC_AS_CONSTRUCTOR (103, 256, swfdec_as_date_construct, swfdec_as_date_get_type)
void
swfdec_as_date_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  guint i;
  SwfdecAsDate *date;

  if (!cx->frame->construct) {
    SwfdecAsValue val;
    object = g_object_new (SWFDEC_TYPE_AS_DATE, "context", cx, NULL);
    swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_Date, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      swfdec_as_object_set_constructor (object,
	  SWFDEC_AS_VALUE_GET_OBJECT (&val));
    } else {
      SWFDEC_INFO ("\"Date\" is not an object");
    }
  }

  date = SWFDEC_AS_DATE (object);

  /* FIXME: find a general solution here */
  if (SWFDEC_IS_PLAYER (swfdec_gc_object_get_context (date))) {
    date->utc_offset =
      SWFDEC_PLAYER (swfdec_gc_object_get_context (date))->priv->system->utc_offset;
  }

  // don't accept arguments when not constructing
  if (!cx->frame->construct)
    argc = 0;

  // special case: ignore undefined and everything after it
  for (i = 0; i < argc; i++) {
    if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[i])) {
      argc = i;
      break;
    }
  }

  if (argc == 0) // current time, local
  {
    GTimeVal tv;

    swfdec_as_context_get_time (cx, &tv);
    /* Use millisecond granularity here. Otherwise the value returned by 
     * getTime() or toString() has a decimal point which breaks Flash files.
     */
    swfdec_as_date_set_milliseconds_utc (date,
	tv.tv_sec * 1000.0 + tv.tv_usec / 1000);
  }
  else if (argc == 1) // milliseconds from epoch, local
  {
    // need to save directly to keep fractions of a milliseconds
    date->milliseconds = swfdec_as_value_to_number (cx, &argv[0]);
  }
  else // year, month etc. local
  {
    int year, num;
    double d;
    BrokenTime brokentime;

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
	brokentime.month = num;
      } else {
	// special case: if month is not finite set year to -1900
	year = -1900;
	brokentime.month = 0;
      }
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.day_of_month = num;
      } else {
	date->milliseconds += d;
      }
    } else {
      brokentime.day_of_month = 1;
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.hours = num;
      } else {
	date->milliseconds += d;
      }
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.minutes = num;
      } else {
	date->milliseconds += d;
      }
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.seconds = num;
      } else {
	date->milliseconds += d;
      }
    }

    if (year >= 100) {
      brokentime.year = year - 1900;
    } else {
      brokentime.year = year;
    }

    if (argc > i) {
      if (swfdec_as_date_value_to_number_and_integer (cx, &argv[i++], &d,
	    &num)) {
	brokentime.milliseconds += num;
      } else {
	date->milliseconds += d;
      }
    }

    if (date->milliseconds == 0) {
      swfdec_as_date_set_milliseconds_local (date,
	  swfdec_as_date_brokentime_to_milliseconds (&brokentime));
    }
  }

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

SwfdecAsObject *
swfdec_as_date_new (SwfdecAsContext *context, double milliseconds,
    int utc_offset)
{
  SwfdecAsObject *ret;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  ret = g_object_new (SWFDEC_TYPE_AS_DATE, "context", context, NULL);
  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_Date, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    swfdec_as_object_set_constructor (ret, SWFDEC_AS_VALUE_GET_OBJECT (&val));

  SWFDEC_AS_DATE (ret)->milliseconds = milliseconds;
  SWFDEC_AS_DATE (ret)->utc_offset = utc_offset;

  return ret;
}
