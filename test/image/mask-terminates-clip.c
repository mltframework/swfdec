/* gcc `pkg-config --libs --cflags libming` mask-terminates-clip.c -o mask-terminates-clip && ./mask-terminates-clip
 */

#include <ming.h>

static SWFBlock
get_rectangle (int r, int g, int b)
{
  SWFMovieClip clip;
  SWFShape shape;
  SWFFillStyle fill;

  clip = newSWFMovieClip ();
  shape = newSWFShape ();
  fill = SWFShape_addSolidFillStyle (shape, r, g, b, 255);
  SWFShape_setRightFillStyle (shape, fill);
  SWFShape_drawLineTo (shape, 100, 0);
  SWFShape_drawLineTo (shape, 100, 100);
  SWFShape_drawLineTo (shape, 0, 100);
  SWFShape_drawLineTo (shape, 0, 0);

  SWFMovieClip_add (clip, (SWFBlock) shape);
  SWFMovieClip_nextFrame (clip);
  return (SWFBlock) clip;
}

static void
do_movie (int version, int reverse)
{
  char name[100];
  SWFMovie movie;
  SWFDisplayItem item;

  movie = newSWFMovieWithVersion (version);
  SWFMovie_setRate (movie, 1);
  SWFMovie_setDimension (movie, 200, 150);

  item = SWFMovie_add (movie, get_rectangle (255, 0, 0));
  SWFDisplayItem_setDepth (item, 0);
  SWFDisplayItem_setName (item, "a");
  SWFDisplayItem_setMaskLevel (item, 2);

  item = SWFMovie_add (movie, get_rectangle (0, 255, 0));
  SWFDisplayItem_moveTo (item, 0, 50);
  SWFDisplayItem_setDepth (item, 1);
  SWFDisplayItem_setName (item, "b");

  item = SWFMovie_add (movie, get_rectangle (0, 0, 255));
  SWFDisplayItem_moveTo (item, 50, 0);
  SWFDisplayItem_setDepth (item, 3);
  SWFDisplayItem_setName (item, "c");

  SWFMovie_add (movie, (SWFBlock) newSWFAction (
	reverse ? "a.setMask (c); a.setMask (null);" : "c.setMask (a); c.setMask (null);"
	));
  SWFMovie_nextFrame (movie);

  sprintf (name, "mask-terminates-clip-%s-%d.swf", reverse ? "mask" : "maskee", version);
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
  }

  return 0;
}
