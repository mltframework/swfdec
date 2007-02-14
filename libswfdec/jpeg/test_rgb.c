
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <liboil/liboil.h>
//#include <liboil/liboilcolorspace.h>

#define oil_argb_R(color) (((color)>>16)&0xff)
#define oil_argb_G(color) (((color)>>8)&0xff)
#define oil_argb_B(color) (((color)>>0)&0xff)



#include "jpeg.h"

/* getfile */

void *getfile (char *path, int *n_bytes);
static void dump_pnm (uint32_t *ptr, int rowstride, int width, int height);

int
main (int argc, char *argv[])
{
  unsigned char *data;
  int len;
  JpegDecoder *dec;
  char *fn;
  uint32_t *image;
  int width;
  int height;

  dec = jpeg_decoder_new ();

  if (argc < 2) {
    printf("jpeg_rgb_test <file.jpg>\n");
    exit(1);
  }
  fn = argv[1];
  data = getfile (fn, &len);

  if (data == NULL) {
    printf("cannot read file %s\n", fn);
    exit(1);
  }

  jpeg_decoder_addbits (dec, data, len);
  jpeg_decoder_parse (dec);

  jpeg_decoder_get_image_size (dec, &width, &height);

  image = (uint32_t *)jpeg_decoder_get_argb_image (dec);

  dump_pnm (image, width*4, width, height);

  free (image);

  jpeg_decoder_free (dec);
  free (data);

  return 0;
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

  ptr = malloc (st.st_size);
  if (!ptr) {
    close (fd);
    return NULL;
  }

  ret = read (fd, ptr, st.st_size);
  if (ret != st.st_size) {
    free (ptr);
    close (fd);
    return NULL;
  }

  if (n_bytes)
    *n_bytes = st.st_size;

  close (fd);
  return ptr;
}

static void
dump_pnm (uint32_t *ptr, int rowstride, int width, int height)
{
  int x, y;

  printf ("P3\n");
  printf ("%d %d\n", width, height);
  printf ("255\n");

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      printf ("%d ", oil_argb_R(ptr[x]));
      printf ("%d ", oil_argb_G(ptr[x]));
      printf ("%d ", oil_argb_B(ptr[x]));
      if ((x & 15) == 15) {
        printf ("\n");
      }
    }
    printf ("\n");
    ptr += rowstride/4;
  }
}
