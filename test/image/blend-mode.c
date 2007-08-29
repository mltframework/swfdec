/* gcc -Wall `pkg-config --libs --cflags libming glib-2.0` blend-mode.c -o blend-mode && ./blend-mode
 */

#include <ming.h>
#include <glib.h>

#define SIZE 240

const char *modes[] = {
  "0",
  "normal",
  "layer",
  "multiply",
  "screen",
  "lighten",
  "darken",
  "difference",
  "add",
  "subtract",
  "inverse",
  "alpha",
  "erase",
  "overlay",
  "hardlight",
  "15"
};

SWFBitmap
create_yellow_image (guint w, guint h)
{
  guint32 *data;
  guint x, y;
  SWFBitmap ret;

  data = g_malloc (w * h * 4);
  for (y = 0; y < h; y++) {
    /* get 15 steps of 10 pixel high lines fading in yellow */
    guint32 pixel = 255 / 15 * (y * 16 / h);
    pixel *= 0x01010100;
    for (x = 0; x < w; x++) {
      data[y * w + x] = pixel;
    }
  }
  ret = newSWFBitmap_fromData ((guint8 *) data, w, h, TRUE);
  g_free (data);
  return ret;
}

SWFBitmap
create_teal_image (guint w, guint h)
{
  guint32 *data;
  guint x, y;
  SWFBitmap ret;

  data = g_malloc (w * h * 4);
  for (y = 0; y < h; y++) {
    /* get 15 steps of 10 pixel high lines fading in yellow */
    for (x = 0; x < w; x++) {
      guint32 pixel = 255 / 15 * (x * 16 / w);
      pixel *= 0x01000101;
      data[y * w + x] = pixel;
    }
  }
  ret = newSWFBitmap_fromData ((guint8 *) data, w, h, TRUE);
  g_free (data);
  return ret;
}

static SWFDisplayItem
add_images (SWFMovie movie, int blend_mode)
{
  SWFBitmap bitmap;
  SWFShape shape;
  SWFMovieClip clip;
  SWFDisplayItem item;

  clip = newSWFMovieClip ();

  bitmap = create_yellow_image (SIZE, SIZE);
  shape = newSWFShapeFromBitmap (bitmap, SWFFILL_CLIPPED_BITMAP);
  item = SWFMovieClip_add (clip, (SWFBlock) shape);
  SWFDisplayItem_setDepth (item, 1);

  bitmap = create_teal_image (SIZE, SIZE);
  shape = newSWFShapeFromBitmap (bitmap, SWFFILL_CLIPPED_BITMAP);
  item = SWFMovieClip_add (clip, (SWFBlock) shape);
  SWFDisplayItem_setDepth (item, 2);
  SWFDisplayItem_setBlendMode (item, blend_mode);

  SWFMovieClip_nextFrame (clip);

  item = SWFMovie_add (movie, (SWFBlock) clip);
  SWFDisplayItem_setBlendMode (item, SWFBLEND_MODE_LAYER);

  return item;
}

static void
do_movie (int version)
{
  SWFMovie movie;
  char *real_name;
  int mode;

  for (mode = 0; mode < G_N_ELEMENTS (modes); mode++) {
    movie = newSWFMovieWithVersion (version);
    movie = newSWFMovie();
    SWFMovie_setRate (movie, 1);
    SWFMovie_setDimension (movie, SIZE, SIZE);
    SWFMovie_setBackground (movie, 0, 0, 0);

    add_images (movie, mode);
    SWFMovie_nextFrame (movie);

    real_name = g_strdup_printf ("blend-mode-%s-%d.swf", modes[mode], version);
    SWFMovie_save (movie, real_name);
    g_free (real_name);
  }
}

int
main (int argc, char **argv)
{
  int i;

  if (Ming_init ())
    return 1;

  for (i = 7; i < 9; i++)
    do_movie (i);

  return 0;
}
