/* gcc `pkg-config --libs --cflags libming` negative-color-transform.c -o negative-color-transform && ./negative-color-transform
 */

#include <ming.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  SWFMovie movie;
  SWFDisplayItem item;
  SWFJpegBitmap image;
  SWFShape shape;

  if (Ming_init ())
    return 1;
  Ming_useSWFVersion (7);

  movie = newSWFMovie();
  SWFMovie_setRate (movie, 1);
  SWFMovie_setDimension (movie, 10, 10);

  image = newSWFJpegBitmap (fopen ("bw.jpg", "r+"));
  shape = newSWFShapeFromBitmap ((SWFBitmap) image, SWFFILL_BITMAP);

  item = SWFMovie_add (movie, (SWFBlock) shape);
  SWFDisplayItem_setCXform (item, newSWFCXform (100, 0, 0, 0, -90./256, 1.0, 1.0, 1.0));
  SWFMovie_nextFrame (movie);

  SWFMovie_save (movie, "negative-color-transform.swf");
  return 0;
}
