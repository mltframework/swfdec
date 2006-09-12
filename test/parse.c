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
#include <swfdec_buffer.h>
#include <ucontext.h>
#include <sys/mman.h>

#if 0
void * smash_checker (void * (func) (void *), void *priv);
void *go(void *priv);
#endif

static void buffer_free (SwfdecBuffer *buffer, void *priv)
{
  g_free (buffer->data);
}

int main (int argc, char *argv[])
{
  gsize length;
  int ret;
  char *fn = "it.swf";
  SwfdecDecoder *s;
  char *contents;
  SwfdecBuffer *buffer;

  swfdec_init ();

  if(argc>=2){
	fn = argv[1];
  }

  ret = g_file_get_contents (fn, &contents, &length, NULL);
  if (!ret) {
    exit(1);
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
      exit(1);
    }
    if (ret == SWF_IMAGE) {
      break;
    }
  }

  swfdec_decoder_free(s);
  s=NULL;

  return 0;
}

