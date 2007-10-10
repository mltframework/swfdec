/* gcc -Wall `pkg-config --libs --cflags libming glib-2.0` morph-gradient.c -o morph-gradient && ./morph-gradient
 */

#include <ming.h>
#include <glib.h>
#include <stdio.h>

extern void
SWFMatrix_set (SWFMatrix m, float a, float b, float c, float d, int x, int y);


static void do_shape (SWFShape shape, int i)
{
  SWFGradient gradient;
  SWFFillStyle style;

  gradient = newSWFGradient();
  SWFGradient_addEntry (gradient, 0.0, 0, 0, 0, 255);
  SWFGradient_addEntry (gradient, 0.25, i ? 255 : 0, 0, 0, 255);
  SWFGradient_addEntry (gradient, 0.5, 255, i ? 255 : 0, 0, 255);
  SWFGradient_addEntry (gradient, 0.75, 255, 255, i ? 255 : 0, 255);
  SWFGradient_addEntry (gradient, 1.0, 255, 255, 255, 255);

  style = SWFShape_addGradientFillStyle (shape, gradient, 0);
  if (i)
    SWFMatrix_set (SWFFillStyle_getMatrix (style), 4000. / 32768, 0, 0, 4000. / 32768, 2000, 2000);
  else
    SWFMatrix_set (SWFFillStyle_getMatrix (style), 0, 4000. / 32768, 4000. / 32768, 0, 2000, 2000);

  SWFShape_setLeftFillStyle (shape, style);
  SWFShape_movePenTo (shape, -16384 / 10, -16384 / 10);
  SWFShape_drawLineTo (shape, 16384 / 10, -16384 / 10);
  SWFShape_drawLineTo (shape, 16384 / 10, 16384 / 10);
  SWFShape_drawLineTo (shape, -16384 / 10, 16384 / 10);
  SWFShape_drawLineTo (shape, -16384 / 10, -16384 / 10);

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
  SWFDisplayItem_setRatio (item, 0.5);
  SWFMovie_nextFrame (movie);
  SWFDisplayItem_setRatio (item, 0.5);
  SWFMovie_nextFrame (movie);

  s = g_strdup_printf ("morph-gradient-%d.swf", version);
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
