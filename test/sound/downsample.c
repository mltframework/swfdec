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
#include <string.h>
#include <glib.h>

static void
usage (const char *app)
{
  g_print ("usage : %s INFILE OUTFILE\n\n", app);
}

static gboolean
downsample_to_mono (gint16 *dest, const gint16 *src, guint n_samples)
{
  guint i;

  for (i = 0; i < n_samples; i++) {
    if (src[0] != src[1]) {
      g_print ("Info: file is stereo (channel data differs in sample %u\n", i);
      return FALSE;
    }
    *dest = *src;
    dest++;
    src += 2;
  }
  g_print ("Info: conversion stereo => mono done\n");
  return TRUE;
}

static gboolean
downsample_rate_mono (gint16 *dest, const gint16 *src, guint n_samples)
{
  guint i;
  int last;

  last = src[0];
  for (i = 0; i < n_samples; i++) {
    if (src[0] != (last + src[1]) / 2) {
      g_print ("Info: file cannot be downsampled, difference at sample %u\n", i);
      return FALSE;
    }
    last = *dest = src[1];
    dest++;
    src += 2;
  }
  *dest = last;

  return TRUE;
}

static void
convert_le_be (gint16 *data, guint n_samples)
{
  guint i;

  for (i = 0; i < n_samples; i++) {
    data[i] = GINT16_FROM_LE (data[i]);
  }
}

static guint
cut_silence (char *data, guint length, guint steps)
{
  guint i, new;

  for (new = length; new > 0; new--) {
    for (i = 0; i < steps; i++) {
      if (data[new * steps - 1 - i] != 0)
	goto out;
    }
  }

out:
  if (new < length) {
    g_print ("Info: Cut %u zero sample(s) at end of file\n", length - new);
  } else {
    g_print ("Info: No zero samples cut at end of file\n");
  }
  return length - new;
}

int
main (int argc, char **argv)
{
  char *data, *copy;
  gsize length;
  GError *error = NULL;

  if (argc != 3) {
    usage (argv[0]);
    return 0;
  }

  if (!g_file_get_contents (argv[1], &data, &length, &error)) {
    g_printerr ("Error: %s\n", error->message);
    g_error_free (error);
    return 1;
  }
  if (length % 4 != 0) {
    g_printerr ("Error: File size is no multiple of 4\n");
    g_free (data);
    return 1;
  }
  length /= 4;
  copy = g_malloc (length * 2);
  convert_le_be ((void *) data, length * 2);

  if (downsample_to_mono ((void *) copy, (void *) data, length)) {
    guint rate;
    char *tmp = data;
    data = copy;
    copy = tmp;
    for (rate = 44100; rate > 5512 && length % 2 == 0; rate /= 2) {
      if (!downsample_rate_mono ((void *) copy, (void *) data, length / 2))
	break;
      length /= 2;
      g_print ("Info: downsampling from %u => %u successful\n", rate, rate / 2);
      tmp = data;
      data = copy;
      copy = tmp;
    }
    if (length % 2 != 0) {
      g_print ("No more downsampling possible, sample count (%zu) is not multiple of 2\n", length);
    }
    length = cut_silence (data, length,  2);
  } else {
    length *= 2;
    /* FIXME: implement */
    g_assert_not_reached ();
    length = cut_silence (data, length / 2,  4);
  }

  convert_le_be ((void *) data, length);
  length *= 2;
  if (!g_file_set_contents (argv[2], data, length, &error)) {
    g_printerr ("Error: %s\n", error->message);
    g_error_free (error);
    g_free (data);
    return 1;
  }
  g_free (data);
  g_free (copy);
  return 0;
}

