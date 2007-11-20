/* gcc `pkg-config --libs --cflags libming` clip-change.c -o clip-change && ./clip-change
 */

#include <ming.h>

static SWFBlock
get_rectangle (int r, int g, int b)
{
  SWFShape shape;
  SWFFillStyle fill;

  shape = newSWFShape ();
  fill = SWFShape_addSolidFillStyle (shape, r, g, b, 255);
  SWFShape_setRightFillStyle (shape, fill);
  SWFShape_drawLineTo (shape, 100, 0);
  SWFShape_drawLineTo (shape, 100, 100);
  SWFShape_drawLineTo (shape, 0, 100);
  SWFShape_drawLineTo (shape, 0, 0);

  return (SWFBlock) shape;
}

static void
do_movie (int version, int reverse)
{
  const char *suffixes[4] = { "forward", "backward", "remove", "set" };
  char name[100];
  SWFMovie movie;
  SWFDisplayItem item, clip;

  movie = newSWFMovieWithVersion (version);
  SWFMovie_setRate (movie, 1);
  SWFMovie_setDimension (movie, 200, 150);

  clip = SWFMovie_add (movie, get_rectangle (255, 0, 0));
  SWFDisplayItem_setDepth (clip, 0);
  if (reverse != 3)
    SWFDisplayItem_setMaskLevel (clip, (reverse & 1) ? 3 : 2);

  item = SWFMovie_add (movie, get_rectangle (0, 255, 0));
  SWFDisplayItem_moveTo (item, 0, 50);
  SWFDisplayItem_setDepth (item, 2);

  item = SWFMovie_add (movie, get_rectangle (0, 0, 255));
  SWFDisplayItem_moveTo (item, 50, 0);
  SWFDisplayItem_setDepth (item, 3);

  SWFMovie_nextFrame (movie);
  if (reverse != 2)
    SWFDisplayItem_setMaskLevel (clip, (reverse & 1) ? 2 : 3);
  else
    SWFDisplayItem_setMaskLevel (clip, 0);
  SWFMovie_nextFrame (movie);

  sprintf (name, "clip-change-%s-%d.swf", suffixes[reverse], version);
  SWFMovie_save (movie, name);
}

int
main (int argc, char **argv)
{
  int i;

  if (Ming_init ())
    return 1;

  for (i = 8; i >= 5; i--) {
    do_movie (i, 0);
    do_movie (i, 1);
    do_movie (i, 2);
    do_movie (i, 3);
  }

  return 0;
}
