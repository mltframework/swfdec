/* gcc -Wall `pkg-config --libs --cflags libming glib-2.0` morph-end.c -o morph-end && ./morph-end
 */

#include <ming.h>
#include <glib.h>
#include <stdio.h>

extern void
SWFMatrix_set (SWFMatrix m, float a, float b, float c, float d, int x, int y);


static void do_shape (SWFShape shape, int i)
{
  SWFFillStyle style;

  style = SWFShape_addSolidFillStyle (shape, 255, i ? 255 : 0, 0, 255);

  i = i ? 10 : 50;
  SWFShape_setLeftFillStyle (shape, style);
  SWFShape_movePenTo (shape, i, i);
  SWFShape_drawLineTo (shape, i, 2 * i);
  SWFShape_drawLineTo (shape, 2 * i, 2 * i);
  SWFShape_drawLineTo (shape, 2 * i, i);
  SWFShape_drawLineTo (shape, i, i);

}

static void
do_movie (int version)
{
  SWFMovie movie;
  SWFMorph morph;
  SWFDisplayItem item;
  char *s;

  movie = newSWFMovieWithVersion (version);
  SWFMovie_setRate (movie, 1);
  SWFMovie_setDimension (movie, 200, 150);

  morph = newSWFMorphShape ();

  do_shape (SWFMorph_getShape1 (morph), 0);
  do_shape (SWFMorph_getShape2 (morph), 1);
  item = SWFMovie_add (movie, morph);
  SWFDisplayItem_setRatio (item, 1.0);
  SWFMovie_nextFrame (movie);

  s = g_strdup_printf ("morph-end-%d.swf", version);
  SWFMovie_save (movie, s);
  g_free (s);
}

int
main (int argc, char **argv)
{
  int i;

  if (Ming_init ())
    return 1;

  for (i = 4; i < 9; i++)
    do_movie (i);

  return 0;
}
