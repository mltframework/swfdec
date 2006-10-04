#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib-object.h>
#include <swfdec.h>
#include <swfdec_decoder.h>
#include <swfdec_sprite.h>
#include <swfdec_shape.h>
#include <swfdec_text.h>
#include <swfdec_font.h>

static gboolean verbose = FALSE;

static void
dump_sprite (SwfdecSprite *s)
{
  if (!verbose) {
    g_print ("  %u frames\n", s->n_frames);
  } else {
    guint i, j;

    for (i = 0; i < s->n_frames; i++) {
      SwfdecSpriteFrame * frame = &s->frames[i];
      if (frame->actions == NULL)
	continue;
      for (j = 0; j < frame->actions->len; j++) {
	SwfdecSpriteAction *action = 
	  &g_array_index (frame->actions, SwfdecSpriteAction, j);
	switch (action->type) {
	  case SWFDEC_SPRITE_ACTION_SCRIPT:
	    g_print ("   %4u script\n", i);
	  case SWFDEC_SPRITE_ACTION_UPDATE:
	  case SWFDEC_SPRITE_ACTION_REMOVE:
	    break;
	  case SWFDEC_SPRITE_ACTION_ADD:
	    {
	      SwfdecSpriteContent *content = action->data;
	      g_assert (content == content->sequence);
	      g_assert (content->start == i);
	      g_print ("   %4u -%4u %3u", i, content->end, content->depth);
	      if (content->object) {
		g_print (" %s %u", G_OBJECT_TYPE_NAME (content->object), content->object->id);
	      } else {
		g_print (" ---");
	      }
	      if (content->name)
		g_print (" as %s", content->name);
	      g_print ("\n");
	    }
	    break;
	  default:
	    g_assert_not_reached ();
	}
      }
    }
  }
}

static void
dump_path (cairo_path_t *path)
{
  int i;
  cairo_path_data_t *data = path->data;
  const char *name;

  for (i = 0; i < path->num_data; i++) {
    name = NULL;
    switch (data[i].header.type) {
      case CAIRO_PATH_CURVE_TO:
	i += 2;
	name = "curve";
      case CAIRO_PATH_LINE_TO:
	if (!name)
	  name = "line ";
      case CAIRO_PATH_MOVE_TO:
	if (!name)
	  name = "move ";
	i++;
	g_print ("      %s %g %g\n", name, data[i].point.x, data[i].point.y);
	break;
      case CAIRO_PATH_CLOSE_PATH:
	g_print ("      close\n");
	break;
      default:
	g_assert_not_reached ();
	break;
    }
  }
}

static void
print_fill_info (SwfdecShapeVec *shapevec)
{
  switch (shapevec->fill_type) {
    case 0x10:
      g_print ("linear gradient\n");
      break;
    case 0x12:
      g_print ("radial gradient\n");
      break;
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
      g_print ("%s%simage (id %d)\n", shapevec->fill_type % 2 ? "" : "repeating ", 
	  shapevec->fill_type < 0x42 ? "bilinear " : "", shapevec->fill_id);
      break;
    case 0x00:
      g_print ("solid color 0x%08X\n", shapevec->color);
      break;
    default:
      g_print ("unknown fill type %d\n", shapevec->fill_type);
      break;
  }
}

static void
dump_shape(SwfdecShape *shape)
{
  SwfdecShapeVec *shapevec;
  unsigned int i;

  g_print("  lines:\n");
  for(i=0;i<shape->lines->len;i++){
    shapevec = g_ptr_array_index (shape->lines, i);

    g_print("    %d: ", i);
    print_fill_info (shapevec);
    if (verbose)
      dump_path (&shapevec->path);
  }
  g_print("  fills:\n");
  for(i=0;i<shape->fills->len;i++){
    shapevec = g_ptr_array_index (shape->fills, i);

    g_print("    %d: ", i);
    print_fill_info (shapevec);
    if (verbose)
      dump_path (&shapevec->path);
    shapevec = g_ptr_array_index (shape->fills2, i);
    if (verbose)
      dump_path (&shapevec->path);
  }
}

static void
dump_text (SwfdecText *text)
{
  guint i;
  gunichar2 uni[text->glyphs->len];
  char *s;

  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);
    uni[i] = g_array_index (glyph->font->glyphs, SwfdecFontEntry, glyph->glyph).value;
    if (uni[i] == 0)
      goto fallback;
  }
  s = g_utf16_to_utf8 (uni, text->glyphs->len, NULL, NULL, NULL);
  if (s == NULL)
    goto fallback;
  g_print ("  text: %s\n", s);
  g_free (s);
  return;

fallback:
  g_print ("  %u characters\n", text->glyphs->len);
}

static void
dump_font (SwfdecFont *font)
{
  unsigned int i;
  if (font->name)
    g_print ("  %s\n", font->name);
  g_print ("  %u characters\n", font->glyphs->len);
  if (verbose) {
    for (i = 0; i < font->glyphs->len; i++) {
      gunichar2 c = g_array_index (font->glyphs, SwfdecFontEntry, i).value;
      char *s;
      if (c == 0 || (s = g_utf16_to_utf8 (&c, 1, NULL, NULL, NULL)) == NULL) {
	g_print (" ");
      } else {
	g_print ("%s ", s);
	g_free (s);
      }
    }
    g_print ("\n");
  }
}

static void 
dump_objects(SwfdecDecoder *s)
{
  GList *g;
  SwfdecObject *object;
  GType type;

  for (g = g_list_last (s->characters); g; g = g->prev) {
    object = g->data;
    type = G_TYPE_FROM_INSTANCE (object);
    printf("%d: %s\n", object->id, g_type_name (type));
    if (SWFDEC_IS_SPRITE (object)){
      dump_sprite (SWFDEC_SPRITE (object));
    }
    if (SWFDEC_IS_SHAPE(object)){
      dump_shape(SWFDEC_SHAPE(object));
    }
    if (SWFDEC_IS_TEXT (object)){
      dump_text (SWFDEC_TEXT (object));
    }
    if (SWFDEC_IS_FONT (object)){
      dump_font (SWFDEC_FONT (object));
    }
  }
}

int
main (int argc, char *argv[])
{
  int ret;
  char *fn = "it.swf";
  SwfdecDecoder *s;
  SwfdecBuffer *buffer;
  GError *error = NULL;
  GOptionEntry options[] = {
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "bew verbose", NULL },
    { NULL }
  };
  GOptionContext *ctx;

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, "options");
  g_option_context_parse (ctx, &argc, &argv, &error);
  g_option_context_free (ctx);
  if (error) {
    g_printerr ("Error parsing command line arguments: %s\n", error->message);
    g_error_free (error);
    return 1;
  }

  swfdec_init();

  if(argc>=2){
	fn = argv[1];
  }

  buffer = swfdec_buffer_new_from_file (fn, NULL);
  if (!buffer) {
    g_print ("dump: file \"%s\" not found\n", fn);
    return 1;
  }

  s = swfdec_decoder_new();

  ret = swfdec_decoder_add_buffer (s, buffer);
  //printf("%d\n", ret);

  while (ret != SWFDEC_EOF) {
    ret = swfdec_decoder_parse (s);
    if (ret == SWFDEC_NEEDBITS) {
      swfdec_decoder_eof(s);
    }
    if (ret == SWFDEC_ERROR) {
      g_printerr ("Failed to parse file. Parser returned %d\n", ret);
      return 1;
    }
  }

  g_print ("file:\n");
  g_print ("  version: %d\n", s->version);
  g_print ("objects:\n");
  dump_objects(s);

  g_print ("main sprite:\n");
  dump_sprite(s->main_sprite);
  g_object_unref (s);
  s = NULL;

  return 0;
}

