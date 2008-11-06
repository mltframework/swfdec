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

#include "swfdec.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_string.h"
#include "swfdec_as_strings.h"
#include "swfdec_audio_decoder.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

SWFDEC_AS_NATIVE (1066, 0, swfdec_system_setClipboard)
void
swfdec_system_setClipboard (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.setClipboard (static)");
}

SWFDEC_AS_NATIVE (2107, 0, swfdec_system_showSettings)
void
swfdec_system_showSettings (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.showSettings (static)");
}

SWFDEC_AS_NATIVE (2107, 1, swfdec_system_get_exactSettings)
void
swfdec_system_get_exactSettings (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.exactSettings (static, get)");
}

SWFDEC_AS_NATIVE (2107, 2, swfdec_system_set_exactSettings)
void
swfdec_system_set_exactSettings (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.exactSettings (static, set)");
}

SWFDEC_AS_NATIVE (2107, 3, swfdec_system_get_useCodepage)
void
swfdec_system_get_useCodepage (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.useCodepage (static, get)");
}

SWFDEC_AS_NATIVE (2107, 4, swfdec_system_set_useCodepage)
void
swfdec_system_set_useCodepage (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.useCodepage (static, set)");
}

static void
swfdec_system_has_audio (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  /* FIXME: allow setting this? */
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
}

static void
swfdec_system_has_streaming_audio (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
}

static void
swfdec_system_has_streaming_video (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  /* FIXME: check if video decoders got compiled in? */
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
}

static void
swfdec_system_has_embedded_video (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  /* FIXME: what's this? */
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
}

static void
swfdec_system_has_mp3 (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  gboolean result = swfdec_audio_decoder_prepare (SWFDEC_AUDIO_CODEC_MP3, 
      swfdec_audio_format_new (44100, 2, TRUE), NULL);

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, result);
}

static void
swfdec_system_has_audio_encoder (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_has_video_encoder (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_has_accessibility (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_has_printing (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_has_screen_broadcast (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_has_screen_playback (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_is_debugger (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, player->priv->system->debugger);
}

static void
swfdec_system_version (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (
	SWFDEC_AS_CONTEXT (player), player->priv->system->version));
}

static void
swfdec_system_manufacturer (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (
	SWFDEC_AS_CONTEXT (player), player->priv->system->manufacturer));
}

static void
swfdec_system_screen_width (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  swfdec_as_value_set_integer (SWFDEC_AS_CONTEXT (player), ret, player->priv->system->screen_width);
}

static void
swfdec_system_screen_height (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  swfdec_as_value_set_integer (SWFDEC_AS_CONTEXT (player), ret, player->priv->system->screen_height);
}

static void
swfdec_system_screen_dpi (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  swfdec_as_value_set_integer (SWFDEC_AS_CONTEXT (player), ret, player->priv->system->dpi);
}

static void
swfdec_system_screen_color (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (
	SWFDEC_AS_CONTEXT (player), player->priv->system->color_mode));
}

static void
swfdec_system_screen_par (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  swfdec_as_value_set_number (SWFDEC_AS_CONTEXT (player), ret, player->priv->system->par);
}

static void
swfdec_system_os (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (
	SWFDEC_AS_CONTEXT (player), player->priv->system->os));
}

static void
swfdec_system_language (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (
	SWFDEC_AS_CONTEXT (player), player->priv->system->language));
}

static void
swfdec_system_has_ime (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_player_type (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_get_string (
	SWFDEC_AS_CONTEXT (player), player->priv->system->player_type));
}

static void
swfdec_system_av_disabled (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_local_file_disabled (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_windowless_disabled (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

static void
swfdec_system_has_tls (SwfdecPlayer *player, SwfdecAsValue *ret)
{
  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
}

/* NB: ordered for the query string order */
const struct {
  const char *	name;
  const char *	server_string;
  void		(* get)		(SwfdecPlayer *player, SwfdecAsValue *ret);
} queries[] = {
  { SWFDEC_AS_STR_hasAudio,		"A",	swfdec_system_has_audio },
  { SWFDEC_AS_STR_hasStreamingAudio,	"SA",	swfdec_system_has_streaming_audio },
  { SWFDEC_AS_STR_hasStreamingVideo,	"SV",	swfdec_system_has_streaming_video },
  { SWFDEC_AS_STR_hasEmbeddedVideo,	"EV",	swfdec_system_has_embedded_video },
  { SWFDEC_AS_STR_hasMP3,		"MP3",	swfdec_system_has_mp3 },
  { SWFDEC_AS_STR_hasAudioEncoder,    	"AE",	swfdec_system_has_audio_encoder },
  { SWFDEC_AS_STR_hasVideoEncoder,    	"VE",	swfdec_system_has_video_encoder },
  { SWFDEC_AS_STR_hasAccessibility,    	"ACC",	swfdec_system_has_accessibility },
  { SWFDEC_AS_STR_hasPrinting,    	"PR",	swfdec_system_has_printing },
  { SWFDEC_AS_STR_hasScreenPlayback,	"SP",	swfdec_system_has_screen_playback },
  { SWFDEC_AS_STR_hasScreenBroadcast,  	"SB",	swfdec_system_has_screen_broadcast },
  { SWFDEC_AS_STR_isDebugger,   	"DEB",	swfdec_system_is_debugger },
  { SWFDEC_AS_STR_version,       	"V",	swfdec_system_version },
  { SWFDEC_AS_STR_manufacturer,       	NULL,	swfdec_system_manufacturer },
  { SWFDEC_AS_STR_screenResolutionX,   	"R",	swfdec_system_screen_width },
  { SWFDEC_AS_STR_screenResolutionY,   	NULL,	swfdec_system_screen_height },
  { SWFDEC_AS_STR_screenDPI,	   	"DP",	swfdec_system_screen_dpi },
  { SWFDEC_AS_STR_screenColor,	   	"COL",	swfdec_system_screen_color },
  { SWFDEC_AS_STR_pixelAspectRatio,    	NULL,	swfdec_system_screen_par },
  { SWFDEC_AS_STR_os,			"OS",	swfdec_system_os },
  { SWFDEC_AS_STR_language,		"L",	swfdec_system_language },
  { SWFDEC_AS_STR_hasIME,		"IME",	swfdec_system_has_ime },
  { SWFDEC_AS_STR_playerType,		"PT",	swfdec_system_player_type },
  { SWFDEC_AS_STR_avHardwareDisable,	"AVD",	swfdec_system_av_disabled },
  { SWFDEC_AS_STR_localFileReadDisable,	"LFD",	swfdec_system_local_file_disabled },
  { SWFDEC_AS_STR_windowlessDisable,	"WD",	swfdec_system_windowless_disabled },
  { SWFDEC_AS_STR_hasTLS,		"TLS",	swfdec_system_has_tls },
};

SWFDEC_AS_NATIVE (11, 0, swfdec_system_query)
void
swfdec_system_query (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (cx);
  SwfdecAsValue val;
  guint i;
  GString *server;

  if (object == NULL) {
    SWFDEC_WARNING ("no this object in Query()");
    return;
  }

  server = g_string_new ("");
  for (i = 0; i < G_N_ELEMENTS (queries); i++) {
    queries[i].get (player, &val);
    swfdec_as_object_set_variable (object, queries[i].name, &val);
    if (queries[i].name == SWFDEC_AS_STR_screenResolutionY) {
      g_string_append_printf (server, "x%d", (int) SWFDEC_AS_VALUE_GET_NUMBER (*&val));
    } else if (queries[i].name == SWFDEC_AS_STR_pixelAspectRatio) {
      char buffer[10];
      g_ascii_formatd (buffer, sizeof (buffer), "%.1f",
	  SWFDEC_AS_VALUE_GET_NUMBER (*&val));
      g_string_append (server, "&AR=");
      g_string_append (server, buffer);
    } else if (queries[i].name == SWFDEC_AS_STR_manufacturer) {
      char *s = swfdec_as_string_escape (cx, player->priv->system->server_manufacturer);
      g_string_append_printf (server, "&M=%s", s);
      g_free (s);
    } else {
      g_assert (queries[i].server_string);
      if (i > 0)
	g_string_append_c (server, '&');
      g_string_append (server, queries[i].server_string);
      g_string_append_c (server, '=');
      if (SWFDEC_AS_VALUE_IS_BOOLEAN (*&val)) {
	g_string_append_c (server, SWFDEC_AS_VALUE_GET_BOOLEAN (*&val) ? 't' : 'f');
      } else if (SWFDEC_AS_VALUE_IS_NUMBER (*&val)) {
	g_string_append_printf (server, "%d", (int) SWFDEC_AS_VALUE_GET_NUMBER (*&val));
      } else if (SWFDEC_AS_VALUE_IS_STRING (*&val)) {
	char *s = swfdec_as_string_escape (cx, SWFDEC_AS_VALUE_GET_STRING (*&val));
	g_string_append (server, s);
	g_free (s);
      } else {
	g_assert_not_reached ();
      }
    }
  }
  SWFDEC_AS_VALUE_SET_STRING (&val, swfdec_as_context_give_string (cx, g_string_free (server, FALSE)));
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_serverString, &val);
}

