#include <gtk/gtk.h>
#include <math.h>
#include <swfdec.h>
#include <swfdec_buffer.h>
#include <swfdec_widget.h>

static void
view_swf (SwfdecDecoder *dec, double scale, gboolean use_image)
{
  GtkWidget *window, *widget;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  widget = swfdec_widget_new (dec);
  swfdec_widget_set_scale (SWFDEC_WIDGET (widget), scale);
  swfdec_widget_set_use_image (SWFDEC_WIDGET (widget), use_image);
  gtk_container_add (GTK_CONTAINER (window), widget);
  gtk_widget_show_all (window);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  gtk_main ();
}

static void
buffer_free (SwfdecBuffer *buffer, void *priv)
{
  g_free (buffer->data);
}

int main (int argc, char *argv[])
{
  gsize length;
  int ret = 100;
  double scale;
  SwfdecDecoder *s;
  char *contents;
  SwfdecBuffer *buffer;
  GError *error = NULL;
  gboolean use_image = FALSE;

  GOptionEntry options[] = {
    { "scale", 's', 0, G_OPTION_ARG_INT, &ret, "scale factor", "PERCENT" },
    { "image", 'i', 0, G_OPTION_ARG_NONE, &use_image, "use an intermediate image surface for drawing", NULL },
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

  ret = g_file_get_contents (argv[1], &contents, &length, &error);
  if (!ret) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], error->message);
    return 1;
  }

  s = swfdec_decoder_new();
  buffer = swfdec_buffer_new_with_data (contents, length);
  buffer->free = buffer_free;
  ret = swfdec_decoder_add_buffer(s, buffer);

  while (ret != SWF_EOF) {
    ret = swfdec_decoder_parse(s);
    if (ret == SWF_NEEDBITS) {
      swfdec_decoder_eof(s);
    }
    if (ret == SWF_ERROR) {
      g_print("error while parsing\n");
      return 1;
    }
    if (ret == SWF_IMAGE) {
      break;
    }
  }

  view_swf (s, scale, use_image);

  s = NULL;
  return 0;
}

