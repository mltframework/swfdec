#include <gtk/gtk.h>
#include <math.h>
#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_debugger.h>
#include "swfdec_debug_script.h"
#include "swfdec_debug_scripts.h"
#include "swfdec_player_manager.h"
#include "swfdec_widget.h"
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>

static void
save_to_svg (GtkWidget *button, SwfdecPlayer *player)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new ("Save current frame as",
      GTK_WINDOW (gtk_widget_get_toplevel (button)), GTK_FILE_CHOOSER_ACTION_SAVE, 
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    int w, h;
    cairo_t *cr;
    cairo_surface_t *surface;

    swfdec_player_get_image_size (player, &w, &h);
    if (w == 0 || h == 0) {
      return;
    }
    surface = cairo_svg_surface_create (
	gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)),
	w, h);
    cr = cairo_create (surface);
    cairo_surface_destroy (surface);
    swfdec_player_render (player, cr, NULL);
    cairo_show_page (cr);
    cairo_destroy (cr);
  }
  gtk_widget_destroy (dialog);
}
#endif /* CAIRO_HAS_SVG_SURFACE */

static void
step_clicked_cb (gpointer manager)
{
  swfdec_player_manager_iterate (manager);
}

static void
select_scripts (GtkTreeSelection *select, SwfdecDebugScript *script)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (select, &model, &iter)) {
    SwfdecDebuggerScript *dscript;
    gtk_tree_model_get (model, &iter, 0, &dscript, -1);
    swfdec_debug_script_set_script (script, dscript);
  } else {
    swfdec_debug_script_set_script (script, NULL);
  }
}

static void
toggle_play_cb (SwfdecPlayerManager *manager, GParamSpec *pspec, GtkToggleButton *button)
{
  gtk_toggle_button_set_active (button,
      swfdec_player_manager_get_playing (manager));
}

static void
play_toggled_cb (GtkToggleButton *button, SwfdecPlayerManager *manager)
{
  swfdec_player_manager_set_playing (manager,
      gtk_toggle_button_get_active (button));
}

static void
entry_activate_cb (GtkEntry *entry, SwfdecPlayerManager *manager)
{
  const char *text = gtk_entry_get_text (entry);

  if (text[0] == '\0')
    return;

  swfdec_player_manager_execute (manager, text);
  gtk_entry_set_text (entry, "");
}

static void
message_display_cb (SwfdecPlayerManager *manager, guint type, const char *message, GtkTextView *view)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
  GtkTextIter iter;
  const char *tag_name = "output";

  if (type == 0)
    tag_name = "input";
  else if (type == 2)
    tag_name = "error";

  gtk_text_buffer_get_end_iter (buffer, &iter);
  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, message, -1, tag_name, NULL);
  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, "\n", 1, tag_name, NULL);
}

static void
view_swf (SwfdecPlayer *player, double scale, gboolean use_image)
{
  SwfdecPlayerManager *manager;
  GtkWidget *window, *widget, *hpaned, *vbox, *scroll, *scripts;

  manager = swfdec_player_manager_new (player);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  hpaned = gtk_hpaned_new ();
  gtk_container_add (GTK_CONTAINER (window), hpaned);

  /* left side */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_paned_add1 (GTK_PANED (hpaned), vbox);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  scripts = swfdec_debug_scripts_new (player);
  gtk_container_add (GTK_CONTAINER (scroll), scripts);

  widget = gtk_toggle_button_new_with_mnemonic ("_Play");
  g_signal_connect (widget, "toggled", G_CALLBACK (play_toggled_cb), manager);
  g_signal_connect (manager, "notify::playing", G_CALLBACK (toggle_play_cb), widget);
  gtk_box_pack_end (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
  
  widget = gtk_button_new_from_stock ("_Step");
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (step_clicked_cb), manager);
  gtk_box_pack_end (GTK_BOX (vbox), widget, FALSE, TRUE, 0);

#ifdef CAIRO_HAS_SVG_SURFACE
  widget = gtk_button_new_with_mnemonic ("_Save frame");
  g_signal_connect (widget, "clicked", G_CALLBACK (save_to_svg), player);
  gtk_box_pack_end (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
#endif

  /* right side */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_paned_add2 (GTK_PANED (hpaned), vbox);

  widget = swfdec_widget_new (player);
  swfdec_widget_set_scale (SWFDEC_WIDGET (widget), scale);
  swfdec_widget_set_use_image (SWFDEC_WIDGET (widget), use_image);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  widget = swfdec_debug_script_new (player);
  gtk_container_add (GTK_CONTAINER (scroll), widget);
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (scripts)),
      "changed", G_CALLBACK (select_scripts), widget);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  widget = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)),
      "error", "foreground", "red", "left-margin", 15, NULL);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)),
      "input", "foreground", "dark grey", NULL);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)),
      "output", "left-margin", 15, NULL);
  g_signal_connect (manager, "message", G_CALLBACK (message_display_cb), widget);
  gtk_container_add (GTK_CONTAINER (scroll), widget);

  widget = gtk_entry_new ();
  g_signal_connect (widget, "activate", G_CALLBACK (entry_activate_cb), manager);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
  gtk_widget_grab_focus (widget);

  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  gtk_widget_show_all (window);

  gtk_main ();

  g_object_unref (manager);
}

int 
main (int argc, char *argv[])
{
  int ret = 0;
  double scale;
  SwfdecLoader *loader;
  SwfdecPlayer *player;
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

  loader = swfdec_loader_new_from_file (argv[1], &error);
  if (loader == NULL) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], error->message);
    g_error_free (error);
    return 1;
  }
  player = swfdec_player_new_with_debugger ();
  swfdec_player_set_loader (player, loader);
  if (swfdec_player_get_rate (player) == 0) {
    g_printerr ("File \"%s\" is not a SWF file\n", argv[1]);
    g_object_unref (player);
    player = NULL;
    return 1;
  }

  view_swf (player, scale, use_image);

  player = NULL;
  return 0;
}

