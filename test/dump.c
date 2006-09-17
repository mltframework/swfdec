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

void
dump_sprite(SwfdecSprite *s)
{
#if 0
  GList *layer;
  SwfdecSpriteSegment *seg;

  printf("  n_frames=%d\n", s->n_frames);
  for(layer = g_list_first(s->layers); layer; layer = g_list_next(layer)) {
    seg = layer->data;
    printf("  id %d depth %d [%d,%d] %d\n",
        seg->id,seg->depth,seg->first_frame,seg->last_frame,
        seg->clip_depth);
  }
#endif
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
  g_print ("  %u characters\n", text->glyphs->len);
}

static void
dump_font (SwfdecFont *font)
{
  g_print ("  %u characters\n", font->glyphs->len);
}

static void 
dump_objects(SwfdecDecoder *s)
{
  GList *g;
  SwfdecObject *object;
  GType type;

  for (g=g_list_first(s->objects);g;g = g_list_next(g)) {
    object = g->data;
    type = G_TYPE_FROM_INSTANCE (object);
    printf("%d: %s\n", object->id, g_type_name (type));
    if (SWFDEC_IS_SPRITE(object)){
      dump_sprite(SWFDEC_SPRITE(object));
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
  char *contents;
  gsize length;
  int ret;
  char *fn = "it.swf";
  SwfdecDecoder *s;

  swfdec_init();

  if(argc>=2){
	fn = argv[1];
  }

  ret = g_file_get_contents (fn, &contents, &length, NULL);
  if (!ret) {
    g_print ("dump: file \"%s\" not found\n", fn);
    exit(1);
  }

  s = swfdec_decoder_new();

  ret = swfdec_decoder_add_data(s, (unsigned char *)contents,length);
  printf("%d\n", ret);

  while (ret != SWF_EOF) {
    ret = swfdec_decoder_parse(s);
    printf("parse returned %d\n", ret);
  }

  printf("objects:\n");
  dump_objects(s);

  printf("main sprite:\n");
  dump_sprite(s->main_sprite);

  return 0;
}

