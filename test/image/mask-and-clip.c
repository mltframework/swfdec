/* gcc `pkg-config --libs --cflags libming` mask-and-clip.c -o mask-and-clip && ./mask-and-clip
 */

#include <ming.h>

enum {
  FIRST_MOVIE_CLIP_ALL,
  SECOND_MOVIE_CLIP_ALL,
  THIRD_MOVIE_SWAP_DEPTH,
  THIRD_MOVIE_MASK,
  N_FLAGS
};
#define FLAG_SET(var, flag) ((var) & (1 << (flag)))

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
do_movie (int version, unsigned int flags)
{
  char name[100];
  SWFMovie movie;
  SWFDisplayItem item, item1, item2;

  movie = newSWFMovieWithVersion (version);
  SWFMovie_setRate (movie, 1);
  SWFMovie_setDimension (movie, 200, 150);

  item1 = item = SWFMovie_add (movie, get_rectangle (255, 0, 0));
  SWFDisplayItem_setDepth (item, 0);
  SWFDisplayItem_setName (item, "a");
  if (FLAG_SET (flags, FIRST_MOVIE_CLIP_ALL)) {
    SWFDisplayItem_setMaskLevel (item, 3);
  } else {
    SWFDisplayItem_setMaskLevel (item, 1);
  }

  item2 = item = SWFMovie_add (movie, get_rectangle (0, 255, 0));
  SWFDisplayItem_moveTo (item, 50, 25);
  SWFDisplayItem_setDepth (item, 1);
  SWFDisplayItem_setName (item, "b");
  if (FLAG_SET (flags, SECOND_MOVIE_CLIP_ALL)) {
    SWFDisplayItem_setMaskLevel (item, 3);
  } else {
    SWFDisplayItem_setMaskLevel (item, 2);
  }

  item = SWFMovie_add (movie, get_rectangle (0, 0, 255));
  SWFDisplayItem_moveTo (item, 25, 50);
  SWFDisplayItem_setDepth (item, 3);
  SWFDisplayItem_setName (item, "c");

  if (FLAG_SET (flags, THIRD_MOVIE_SWAP_DEPTH)) {
    SWFMovie_add (movie, (SWFBlock) newSWFAction ("c.swapDepths (100);"));
  }
  if (FLAG_SET (flags, THIRD_MOVIE_MASK)) {
    SWFDisplayItem mask = SWFMovie_add (movie, get_rectangle (128, 128, 128));
    SWFDisplayItem_moveTo (mask, 50, 50);
    SWFDisplayItem_setDepth (mask, 50);
    SWFDisplayItem_setName (mask, "mask");
    SWFMovie_add (movie, (SWFBlock) newSWFAction (
	  "c.setMask (mask);"
	  ));
  }
  SWFMovie_nextFrame (movie);

  sprintf (name, "mask-and-clip-%u-%d.swf", flags, version);
  SWFMovie_save (movie, name);
}

int
main (int argc, char **argv)
{
  int i, j;

  if (Ming_init ())
    return 1;

  for (i = 8; i <= 8; i++) {
    for (j = 0; j < (1 << N_FLAGS); j++) {
      do_movie (i, j);
    }
  }

  return 0;
}
