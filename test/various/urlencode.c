/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "libswfdec/swfdec_loader_internal.h"

typedef struct {
  char *  encoded;
  char *  names[10];
  char *  values[10];
  guint	  n_props;
} Test;
Test tests[] = {
  { "a=b", { "a" }, { "b" }, 1 },
  { "a=b&c=d", { "a", "c" }, { "b", "d" }, 2 },
  { "owned=Your+Mom", { "owned" }, { "Your Mom" }, 1 }
};

#define ERROR(...) G_STMT_START { \
  g_printerr ("ERROR (line %u): ", __LINE__); \
  g_printerr (__VA_ARGS__); \
  g_printerr ("\n"); \
  errors++; \
}G_STMT_END

static guint
run_test_encode (Test *test)
{
  GString *string;
  guint i, errors = 0;

  string = g_string_new ("");
  for (i = 0; i < test->n_props; i++) {
    swfdec_string_append_urlencoded (string, test->names[i], test->values[i]);
  }
  if (!g_str_equal (test->encoded, string->str)) {
    ERROR ("encoded string is \"%s\", but should be \"%s\"", string->str, test->encoded);
  }
  g_string_free (string, TRUE);
  return errors;
}

static guint
run_test_decode (Test *test)
{
  guint i, errors = 0;
  char *name, *value;
  const char *s = test->encoded;

  for (i = 0; i < test->n_props; i++) {
    if (*s == '\0') {
      ERROR ("string only contains %u properties, but should contain %u", i, test->n_props);
      break;
    }
    if (i > 0) {
      if (*s != '&') {
	ERROR ("properties not delimited by &");
      }
      s++;
    }
    if (!swfdec_urldecode_one (s, &name, &value, &s)) {
      ERROR ("could not decode property %u", i);
      continue;
    }
    if (!g_str_equal (name, test->names[i])) {
      ERROR ("names don't match: is %s, should be %s", name, test->names[i]);
    }
    if (!g_str_equal (value, test->values[i])) {
      ERROR ("names don't match: is %s, should be %s", value, test->values[i]);
    }
    g_free (name);
    g_free (value);
  }
  return errors;
}

int
main (int argc, char **argv)
{
  guint i, errors = 0;

  for (i = 0; i < G_N_ELEMENTS (tests); i++) {
    errors += run_test_encode (&tests[i]);
    errors += run_test_decode (&tests[i]);
  }

  g_print ("TOTAL ERRORS: %u\n", errors);
  return errors;
}

