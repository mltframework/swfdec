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

#define g_print printf

void dump_objects(SwfdecDecoder *s);
void dump_sprite(SwfdecSprite *s);

int main (int argc, char *argv[])
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


void dump_sprite(SwfdecSprite *s)
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

void dump_shape(SwfdecShape *shape)
{
  SwfdecShapeVec *shapevec;
  unsigned int i;

  g_print("  lines:\n");
  for(i=0;i<shape->lines->len;i++){
    shapevec = g_ptr_array_index (shape->lines, i);

    g_print("    %d: n_points=%d fill_id=%d\n", i, shapevec->path->len,
        shapevec->fill_id);
  }
  g_print("  fills:\n");
  for(i=0;i<shape->fills->len;i++){
    shapevec = g_ptr_array_index (shape->fills, i);

    g_print("    %d: n_points=%d fill_id=%d\n", i, shapevec->path->len,
        shapevec->fill_id);
  }
  g_print("  fills1:\n");
  for(i=0;i<shape->fills2->len;i++){
    shapevec = g_ptr_array_index (shape->fills2, i);

    g_print("    %d: n_points=%d fill_id=%d\n", i, shapevec->path->len,
        shapevec->fill_id);
  }

}

void dump_objects(SwfdecDecoder *s)
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
  }

}
