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


void dump_sprite(SwfdecSprite *s);

int main (int argc, char *argv[])
{
  char *contents;
  int length;
  int ret;
  char *fn = "it.swf";
  SwfdecDecoder *s;

  g_type_init();

  if(argc>=2){
	fn = argv[1];
  }

  ret = g_file_get_contents (fn, &contents, &length, NULL);
  if (!ret) {
    exit(1);
  }

  s = swfdec_decoder_new();

  ret = swfdec_decoder_add_data(s,contents,length);
  printf("%d\n", ret);

  while (ret != SWF_EOF) {
    ret = swfdec_decoder_parse(s);
    printf("parse returned %d\n", ret);
  }

  dump_sprite(s->main_sprite);

  return 0;
}


void dump_sprite(SwfdecSprite *s)
{
  GList *layer;
  SwfdecSpriteSegment *seg;

  for(layer = g_list_first(s->layers); layer; layer = g_list_next(layer)) {
    seg = layer->data;
    printf("id %d depth %d [%d,%d]\n",
        seg->id,seg->depth,seg->first_frame,seg->last_frame);
  }
}

