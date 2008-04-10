/* gcc `pkg-config --libs --cflags libming glib-2.0 -lz` movie21.c -o movie21 && ./movie21
 */

#include <string.h>
#include <zlib.h>
#include <glib.h>

static const guint32 palette[5] = { 0xFF0000FF, 0xCF00CF00, 0x9F9F0000, 0x6F006F6F, 0x3F3F003F };

static const guint8 indexes[9 * 11] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 1, 1, 1, 1, 1, 1, 1, 0, 
  0, 1, 2, 2, 2, 2, 2, 1, 0, 
  0, 1, 2, 3, 3, 3, 2, 1, 0, 
  0, 1, 2, 3, 4, 3, 2, 1, 0, 
  0, 1, 2, 3, 4, 3, 2, 1, 0, 
  0, 1, 2, 3, 4, 3, 2, 1, 0, 
  0, 1, 2, 3, 3, 3, 2, 1, 0, 
  0, 1, 2, 2, 2, 2, 2, 1, 0, 
  0, 1, 1, 1, 1, 1, 1, 1, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

#define SWFDEC_COLOR_COMBINE(r,g,b,a)	(((a)<<24) | ((r)<<16) | ((g)<<8) | (b))
#define SWFDEC_COLOR_A(x)		(((x)>>24)&0xff)
#define SWFDEC_COLOR_R(x)		(((x)>>16)&0xff)
#define SWFDEC_COLOR_G(x)		(((x)>>8)&0xff)
#define SWFDEC_COLOR_B(x)		((x)&0xff)

gsize
create_palletized (guint8 *data, gboolean alpha)
{
  guint i;
  guint8 *start = data;

  if (!alpha)
    *data++ = G_N_ELEMENTS (palette) - 1;
  for (i = 0; i < G_N_ELEMENTS (palette); i++) {
    *data++ = SWFDEC_COLOR_R (palette[i]);
    *data++ = SWFDEC_COLOR_G (palette[i]);
    *data++ = SWFDEC_COLOR_B (palette[i]);
    if (alpha)
      *data++ = SWFDEC_COLOR_A (palette[i]);
  }
  for (i = 0; i < 11; i++) {
    memcpy (data, &indexes[9 * i], 9);
    data += 12;
  }
  return data - start;
}

static void
run (int format, gboolean alpha)
{
  guint8 data[2000] = { 0, };
  guint8 compressed[2000] = { 0, };
  gsize len; 
  gulong clen;
  int ret;
  char *s;

  switch (format) {
    case 3:
      len = create_palletized (data, alpha);
      g_print ("%u\n", len);
      break;
    default:
      g_assert_not_reached ();
      return;
  }
  
  if (alpha) {
    clen = sizeof (compressed);
    ret = compress2 (compressed, &clen, data, len, 9);
  } else {
    clen = sizeof (compressed) - 1;
    ret = compress2 (compressed + 1, &clen, data + 1, len - 1, 9);
    compressed[0] = data[0];
  }
  g_assert (ret == Z_OK);

  s = g_base64_encode (compressed, clen + 1);
  g_print ("%s\n\n", s);
  g_free (s);
}

int
main (int argc, char **argv)
{
  run (3, FALSE);
  run (3, TRUE);

  return 0;
}
