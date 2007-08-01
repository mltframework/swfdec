#include <gtk/gtk.h>
#include <math.h>
#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_debugger.h>
#include "swfdec_debug_movies.h"
#include "swfdec_debug_script.h"
#include "swfdec_debug_scripts.h"
#include "swfdec_debug_stack.h"
#include "swfdec_debug_widget.h"
#include "swfdec_player_manager.h"
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>

static void
save_to_svg (GtkWidget *button, SwfdecPlayer *player)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new ("Save current frame as",
      GTK_WINDOW (gtk_widget_get_toplevel (button)), GTK_FILE_CHOOSER_ACTION_SAVE, 
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

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
    swfdec_player_render (player, cr, 0.0, 0.0, 0.0, 0.0);
    cairo_show_page (cr);
    cairo_destroy (cr);
  }
  gtk_widget_destroy (dialog);
}
#endif /* CAIRO_HAS_SVG_SURFACE */

static void
step_clicked_cb (GtkButton *button, SwfdecPlayerManager *manager)
{
  swfdec_player_manager_iterate (manager);
}

static void
step_disable_cb (SwfdecPlayerManager *manager, GParamSpec *pspec, GtkWidget *widget)
{
  gtk_widget_set_sensitive (widget, !swfdec_player_manager_get_interrupted (manager));
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
  gtk_editable_select_region (GTK_EDITABLE (entry), 0, -1);
}

static void
message_display_cb (SwfdecPlayerManager *manager, guint type, const char *message, GtkTextView *view)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (view);
  GtkTextIter iter;
  GtkTextMark *mark;
  const char *tag_name = "output";

  if (type == 0)
    tag_name = "input";
  else if (type == 2)
    tag_name = "error";

  gtk_text_buffer_get_end_iter (buffer, &iter);
  mark = gtk_text_buffer_get_mark (buffer, "end");
  if (mark == NULL)
    mark = gtk_text_buffer_create_mark (buffer, "end", &iter, FALSE);
  if (gtk_text_buffer_get_char_count (buffer) > 0)
    gtk_text_buffer_insert (buffer, &iter, "\n", 1);
  gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, message, -1, tag_name, NULL);
  gtk_text_view_scroll_to_mark (view, mark, 0.0, TRUE, 0.0, 0.0);
}

static void
interrupt_widget_cb (SwfdecPlayerManager *manager, GParamSpec *pspec, SwfdecGtkWidget *widget)
{
  swfdec_gtk_widget_set_interactive (widget,
      !swfdec_player_manager_get_interrupted (manager));
}

static void
select_scripts_cb (SwfdecPlayerManager *manager, GParamSpec *pspec, SwfdecDebugScripts *debug)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  SwfdecDebuggerScript *script;

  if (!swfdec_player_manager_get_interrupted (manager))
    return;

  swfdec_player_manager_get_interrupt (manager, &script, NULL);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW (debug));
  gtk_tree_model_get_iter_first (model, &iter);
  do {
    SwfdecDebuggerScript *cur;
    gtk_tree_model_get (model, &iter, 0, &cur, -1);
    if (cur == script) {
      gtk_tree_selection_select_iter (
	  gtk_tree_view_get_selection (GTK_TREE_VIEW (debug)),
	  &iter);
      return;
    }
  } while (gtk_tree_model_iter_next (model, &iter));
  g_assert_not_reached ();
}

static void
select_script_cb (SwfdecPlayerManager *manager, GParamSpec *pspec, SwfdecDebugScript *debug)
{
  GtkTreePath *path;
  SwfdecDebuggerScript *script;
  guint line;

  if (!swfdec_player_manager_get_interrupted (manager))
    return;

  swfdec_player_manager_get_interrupt (manager, &script, &line);
  swfdec_debug_script_set_script (debug, script);

  path = gtk_tree_path_new_from_indices (line, -1);
  gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (debug)),
      path);
  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (debug), path, NULL, TRUE, 0.0, 0.5);
  gtk_tree_path_free (path);
}

static void
force_continue (SwfdecPlayerManager *manager, GParamSpec *pspec, gpointer unused)
{
  g_signal_stop_emission_by_name (manager, "notify::interrupted");
  if (swfdec_player_manager_get_interrupted (manager))
    swfdec_player_manager_continue (manager);
}

static void
destroyed_cb (GtkWindow *window, SwfdecPlayerManager *manager)
{
  g_signal_connect (manager, "notify::interrupted", G_CALLBACK (force_continue), NULL);
  if (swfdec_player_manager_get_interrupted (manager))
    swfdec_player_manager_continue (manager);
  g_object_unref (manager);
  gtk_main_quit ();
}

static void
disconnect_all (gpointer from, GObject *object)
{
  g_signal_handlers_disconnect_matched (from, G_SIGNAL_MATCH_DATA,
      0, 0, NULL, NULL, object);
  g_object_weak_unref (G_OBJECT (from), disconnect_all, object);
}

static void
signal_auto_connect (gpointer object, const char *signal, GCallback closure, gpointer data)
{
  g_assert (G_IS_OBJECT (data));

  g_signal_connect (object, signal, closure, data);
  g_object_weak_ref (G_OBJECT (object), disconnect_all, data);
  g_object_weak_ref (G_OBJECT (data), disconnect_all, object);
}

static GtkWidget *
create_movieview (SwfdecPlayer *player)
{
  GtkWidget *treeview;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  SwfdecDebugMovies *movies;

  movies = swfdec_debug_movies_new (player);
  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (movies));
  renderer = gtk_cell_renderer_text_new ();

  column = gtk_tree_view_column_new_with_attributes ("Movie", renderer,
    "text", SWFDEC_DEBUG_MOVIES_COLUMN_NAME, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  column = gtk_tree_view_column_new_with_attributes ("Type", renderer,
    "text", SWFDEC_DEBUG_MOVIES_COLUMN_TYPE, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes ("V", renderer,
    "active", SWFDEC_DEBUG_MOVIES_COLUMN_VISIBLE, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  return treeview;
}

static void
view_swf (SwfdecPlayer *player, gboolean use_image)
{
  SwfdecPlayerManager *manager;
  GtkWidget *window, *widget, *vpaned, *hpaned, *vbox, *hbox, *scroll, *scripts;

  manager = swfdec_player_manager_new (player);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  hpaned = gtk_hpaned_new ();
  gtk_container_add (GTK_CONTAINER (window), hpaned);

  /* left side */
  vpaned = gtk_vpaned_new ();
  gtk_paned_add1 (GTK_PANED (hpaned), vpaned);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_paned_add1 (GTK_PANED (vpaned), scroll);
  widget = create_movieview (player);
  gtk_container_add (GTK_CONTAINER (scroll), widget);

  vbox = gtk_vbox_new (FALSE, 3);
  gtk_paned_add2 (GTK_PANED (vpaned), vbox);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  scripts = swfdec_debug_scripts_new (SWFDEC_DEBUGGER (player));
  gtk_container_add (GTK_CONTAINER (scroll), scripts);
  signal_auto_connect (manager, "notify::interrupted", G_CALLBACK (select_scripts_cb), scripts);

  widget = gtk_toggle_button_new_with_mnemonic ("_Play");
  signal_auto_connect (widget, "toggled", G_CALLBACK (play_toggled_cb), manager);
  signal_auto_connect (manager, "notify::playing", G_CALLBACK (toggle_play_cb), widget);
  gtk_box_pack_end (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
  
  widget = gtk_button_new_from_stock ("_Step");
  signal_auto_connect (widget, "clicked", G_CALLBACK (step_clicked_cb), manager);
  signal_auto_connect (manager, "notify::interrupted", G_CALLBACK (step_disable_cb), widget);
  gtk_box_pack_end (GTK_BOX (vbox), widget, FALSE, TRUE, 0);

#ifdef CAIRO_HAS_SVG_SURFACE
  widget = gtk_button_new_with_mnemonic ("_Save frame");
  g_signal_connect (widget, "clicked", G_CALLBACK (save_to_svg), player);
  gtk_box_pack_end (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
#endif

  /* right side */
  vbox = gtk_vbox_new (FALSE, 3);
  gtk_paned_add2 (GTK_PANED (hpaned), vbox);

  widget = swfdec_debug_widget_new (player);
  if (use_image)
    swfdec_gtk_widget_set_renderer (SWFDEC_GTK_WIDGET (widget), CAIRO_SURFACE_TYPE_IMAGE);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
  signal_auto_connect (manager, "notify::interrupted", G_CALLBACK (interrupt_widget_cb), widget);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, TRUE, TRUE, 0);
  widget = swfdec_debug_script_new (SWFDEC_DEBUGGER (player));
  gtk_container_add (GTK_CONTAINER (scroll), widget);
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (scripts)),
      "changed", G_CALLBACK (select_scripts), widget);
  signal_auto_connect (manager, "notify::interrupted", G_CALLBACK (select_script_cb), widget);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scroll, TRUE, TRUE, 0);
  widget = swfdec_debug_stack_new (manager);
  gtk_container_add (GTK_CONTAINER (scroll), widget);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  widget = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD_CHAR);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)),
      "error", "foreground", "red", "left-margin", 15, NULL);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)),
      "input", "foreground", "dark grey", NULL);
  gtk_text_buffer_create_tag (gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget)),
      "output", "left-margin", 15, NULL);
  signal_auto_connect (manager, "message", G_CALLBACK (message_display_cb), widget);
  gtk_container_add (GTK_CONTAINER (scroll), widget);

  widget = gtk_entry_new ();
  signal_auto_connect (widget, "activate", G_CALLBACK (entry_activate_cb), manager);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
  gtk_widget_grab_focus (widget);

  g_signal_connect (window, "destroy", G_CALLBACK (destroyed_cb), manager);
  gtk_widget_show_all (window);
}

static void
do_break_cb (SwfdecDebugger *debugger, SwfdecDebuggerScript *script, gpointer unused)
{
  /* no need tobreak on scripts that don't do anything, so no special case needed */
  if (script->n_commands > 0)
    swfdec_debugger_set_breakpoint (debugger, script, 0);
}

static gboolean
add_variables (gpointer player)
{
  const char *variables = g_object_get_data (player, "variables");
  SwfdecLoader *loader = g_object_get_data (player, "loader");

  swfdec_player_set_loader_with_variables (player, loader, variables);
  if (!swfdec_player_is_initialized (player)) {
    g_printerr ("File \"%s\" is not a file Swfdec can play\n", 
	swfdec_url_get_url (swfdec_loader_get_url (loader)));
    g_object_unref (player);
    gtk_main_quit ();
    return FALSE;
  }
  return FALSE;
}

int 
main (int argc, char *argv[])
{
  gboolean do_break = FALSE;
  SwfdecLoader *loader;
  SwfdecPlayer *player;
  GError *error = NULL;
  gboolean use_image = FALSE;
  char *variables = NULL;

  GOptionEntry options[] = {
    { "image", 'i', 0, G_OPTION_ARG_NONE, &use_image, "use an intermediate image surface for drawing", NULL },
    { "break", 'b', 0, G_OPTION_ARG_NONE, &do_break, "break at the beginning of every script", NULL },
    { "variables", 'v', 0, G_OPTION_ARG_STRING, &variables, "variables to pass to player", "VAR=NAME[&VAR=NAME..]" },
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

  swfdec_init ();

  if (argc < 2) {
    g_printerr ("Usage: %s [OPTIONS] filename\n", argv[0]);
    return 1;
  }

  loader = swfdec_file_loader_new (argv[1]);
  if (loader->error) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], loader->error);
    g_object_unref (loader);
    return 1;
  }
  player = swfdec_debugger_new ();
  if (do_break)
    g_signal_connect (player, "script-added", G_CALLBACK (do_break_cb), NULL);
  view_swf (player, use_image);
  g_object_set_data (G_OBJECT (player), "loader", loader);
  g_object_set_data (G_OBJECT (player), "variables", variables);
  g_idle_add_full (G_PRIORITY_HIGH, add_variables, player, NULL);

  gtk_main ();

  player = NULL;
  return 0;
}

