#include <gtk/gtk.h>
#include <math.h>
#include <swfdec.h>
#include <swfdec_buffer.h>
#include <swfdec_widget.h>
#include <swfdec_playback.h>

static gpointer playback;

static gboolean
iterate (gpointer dec)
{
  swfdec_decoder_iterate (dec);
  if (playback != NULL) {
    SwfdecBuffer *buffer = swfdec_decoder_render_audio_to_buffer (dec);
    swfdec_playback_write (playback, buffer);
    swfdec_buffer_unref (buffer);
  }
  return TRUE;
}

static void
view_swf (SwfdecDecoder *dec, double scale, gboolean use_image)
{
  GtkWidget *window, *widget;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  widget = swfdec_widget_new (dec);
  swfdec_widget_set_scale (SWFDEC_WIDGET (widget), scale);
  swfdec_widget_set_use_image (SWFDEC_WIDGET (widget), use_image);
  gtk_container_add (GTK_CONTAINER (window), widget);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show_all (window);
}

static void
play_swf (SwfdecDecoder *dec)
{
  if (playback == NULL) {
    guint timeout;
    double rate;

    rate = swfdec_decoder_get_rate (dec);
    if (rate > 0) {
      timeout = g_timeout_add (1000 / rate, iterate, dec);

      gtk_main ();
      g_source_remove (timeout);
    }
  } else {
    gtk_main ();
  }
}

int 
main (int argc, char *argv[])
{
  int ret = 100;
  double scale;
  SwfdecDecoder *s;
  SwfdecBuffer *buffer;
  guint buffer_size = 0;
  GError *error = NULL;
  gboolean use_image = FALSE, no_sound = FALSE;

  GOptionEntry options[] = {
    { "scale", 's', 0, G_OPTION_ARG_INT, &ret, "scale factor", "PERCENT" },
    { "image", 'i', 0, G_OPTION_ARG_NONE, &use_image, "use an intermediate image surface for drawing", NULL },
    { "no-sound", 'n', 0, G_OPTION_ARG_NONE, &no_sound, "don't play sound", NULL },
    { NULL }
  };
  GOptionContext *ctx;

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, "options");
  g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
  g_option_context_parse (ctx, &argc, &argv, &error);
  g_option_context_free (ctx);

  if (error) {
    g_printerr ("Error parsing command line arguments: %s\n", error->message);
    g_error_free (error);
    return 1;
  }

  scale = ret / 100.;
  swfdec_init ();

  if (argc < 2) {
    g_printerr ("Usage: %s [OPTIONS] filename\n", argv[0]);
    return 1;
  }

  buffer = swfdec_buffer_new_from_file (argv[1], &error);
  if (buffer == NULL) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], error->message);
    return 1;
  }

  s = swfdec_decoder_new();
  ret = swfdec_decoder_add_buffer(s, buffer);

  while (ret != SWFDEC_EOF) {
    ret = swfdec_decoder_parse (s);
    if (ret == SWFDEC_NEEDBITS) {
      swfdec_decoder_eof (s);
    }
    if (ret == SWFDEC_ERROR) {
      g_print ("error while parsing\n");
      return 1;
    }
  }

  view_swf (s, scale, use_image);

  if (no_sound) {
    playback = NULL;
  } else {
    playback = swfdec_playback_open (iterate, s, &buffer_size);
  }
  if (buffer_size) {
    SwfdecBuffer *buffer;

    swfdec_decoder_set_latency (s, buffer_size);
    buffer = swfdec_buffer_new ();
    buffer->length = buffer_size * 4;
    buffer->data = g_malloc0 (buffer->length);
    swfdec_decoder_render_audio (s, (gint16 *) buffer->data,
	0, buffer_size);
    swfdec_playback_write (playback, buffer);
    swfdec_buffer_unref (buffer);
  }
  play_swf (s);

  if (playback)
    swfdec_playback_close (playback);

  s = NULL;
  return 0;
}

