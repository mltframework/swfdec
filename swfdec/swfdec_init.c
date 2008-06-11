/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_GST
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include "swfdec_audio_decoder_gst.h"
#endif
#include <liboil/liboil.h>

#include "swfdec_audio_decoder_adpcm.h"
#include "swfdec_audio_decoder_uncompressed.h"
#include "swfdec_debug.h"

/**
 * swfdec_init:
 *
 * Initializes the Swfdec library.
 **/
void
swfdec_init (void)
{
  static gboolean _inited = FALSE;
  const char *s;

  if (_inited)
    return;

  _inited = TRUE;

  /* initialize all types */
  if (!g_thread_supported ())
    g_thread_init (NULL);
  g_type_init ();
  oil_init ();
#ifdef HAVE_GST
  gst_init (NULL, NULL);
  gst_pb_utils_init ();
#endif

  /* setup debugging */
  s = g_getenv ("SWFDEC_DEBUG");
  if (s && s[0]) {
    char *end;
    int level;

    level = strtoul (s, &end, 0);
    if (end[0] == 0) {
      swfdec_debug_set_level (level);
    }
  }

  /* Setup audio and video decoders. 
   * NB: The order is important! */
  swfdec_audio_decoder_register (SWFDEC_TYPE_AUDIO_DECODER_UNCOMPRESSED);
  swfdec_audio_decoder_register (SWFDEC_TYPE_AUDIO_DECODER_ADPCM);
#ifdef HAVE_GST
  swfdec_audio_decoder_register (SWFDEC_TYPE_AUDIO_DECODER_GST);
#endif
}

