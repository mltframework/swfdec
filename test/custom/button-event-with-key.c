/* gcc `pkg-config --libs --cflags libming` button-event-with-key.c -o button-event-with-key && ./button-event-with-key
 */

#include <ming.h>

static SWFCharacter
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

  return (SWFCharacter) shape;
}

static void
do_movie (int version)
{
  SWFMovie movie;
  SWFButton button;
  SWFDisplayItem item;
  char name[100];

  movie = newSWFMovieWithVersion (version);
  movie = newSWFMovie();
  SWFMovie_setRate (movie, 1);
  SWFMovie_setDimension (movie, 200, 150);

  button = newSWFButton ();
  SWFButton_addCharacter (button, get_rectangle (255, 0, 0), SWFBUTTON_HIT | SWFBUTTON_UP | SWFBUTTON_DOWN | SWFBUTTON_OVER);
  SWFButton_addAction (button, newSWFAction ("trace (this + \" click\");"), SWFBUTTON_KEYPRESS (13) | SWFBUTTON_MOUSEDOWN);
  SWFButton_addAction (button, newSWFAction ("trace (this + \" unclick\");"), SWFBUTTON_KEYPRESS (32) | SWFBUTTON_MOUSEUP);

  item = SWFMovie_add (movie, (SWFBlock) button);
  SWFMovie_nextFrame (movie);

  sprintf (name, "button-event-with-key-%d.swf", version);
  SWFMovie_save (movie, name);
}

int
main (int argc, char **argv)
{
  int i;

  if (Ming_init ())
    return 1;

  for (i = 5; i < 9; i++)
    do_movie (i);

  return 0;
}
