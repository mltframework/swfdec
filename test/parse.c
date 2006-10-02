#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib-object.h>
#include <swfdec.h>
#include <swfdec_buffer.h>
#include <ucontext.h>
#include <sys/mman.h>

int
main (int argc, char *argv[])
{
  int ret;
  char *fn = "it.swf";
  SwfdecDecoder *s;
  SwfdecBuffer *buffer;

  swfdec_init ();

  if(argc>=2){
	fn = argv[1];
  }

  s = swfdec_decoder_new();

  buffer = swfdec_buffer_new_from_file (fn, NULL);
  if (buffer == NULL) {
    return -1;
  }
  ret = swfdec_decoder_add_buffer(s, buffer);

  while (ret != SWFDEC_EOF) {
    ret = swfdec_decoder_parse(s);
    if (ret == SWFDEC_NEEDBITS) {
      swfdec_decoder_eof(s);
    }
    if (ret == SWFDEC_ERROR) {
      g_print("error while parsing\n");
      return 1;
    }
  }

  g_object_unref (s);
  s = NULL;

  return 0;
}

