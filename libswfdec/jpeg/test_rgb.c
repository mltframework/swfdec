
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#include "jpeg_rgb_decoder.h"

/* getfile */

void *getfile (char *path, int *n_bytes);
static void dump_pnm (unsigned char *ptr, int rowstride, int width, int height);

int
main2 (int argc, char *argv[])
{
  unsigned char *data;
  int len;
  JpegRGBDecoder *dec;
  char *fn = "biglebowski.jpg";
  unsigned char *ptr;
  int rowstride;
  int width;
  int height;

  dec = jpeg_rgb_decoder_new ();

  if (argc > 1)
    fn = argv[1];
  data = getfile (fn, &len);

  jpeg_rgb_decoder_addbits (dec, data, len);
  jpeg_rgb_decoder_parse (dec);

  jpeg_rgb_decoder_get_image (dec, &ptr, &rowstride, &width, &height);

  dump_pnm (ptr, rowstride, width, height);

  g_free (ptr);

  jpeg_rgb_decoder_free (dec);
  g_free (data);

  return 0;
}

int
main (int argc, char *argv[])
{
  return main2 (argc, argv);
}





/* getfile */

void *
getfile (char *path, int *n_bytes)
{
  int fd;
  struct stat st;
  void *ptr = NULL;
  int ret;

  fd = open (path, O_RDONLY);
  if (!fd)
    return NULL;

  ret = fstat (fd, &st);
  if (ret < 0) {
    close (fd);
    return NULL;
  }

  ptr = g_malloc (st.st_size);
  if (!ptr) {
    close (fd);
    return NULL;
  }

  ret = read (fd, ptr, st.st_size);
  if (ret != st.st_size) {
    g_free (ptr);
    close (fd);
    return NULL;
  }

  if (n_bytes)
    *n_bytes = st.st_size;

  close (fd);
  return ptr;
}

static void
dump_pnm (unsigned char *ptr, int rowstride, int width, int height)
{
  int x, y;

  printf ("P3\n");
  printf ("%d %d\n", width, height);
  printf ("255\n");

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      printf ("%d ", ptr[x * 4 + 0]);
      printf ("%d ", ptr[x * 4 + 1]);
      printf ("%d ", ptr[x * 4 + 2]);
      if ((x & 15) == 15) {
        printf ("\n");
      }
    }
    printf ("\n");
    ptr += rowstride;
  }
}
