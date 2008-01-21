/* gcc `pkg-config --libs --cflags libming` button-hittest.c -o button-hittest && ./button-hittest
 */

#include <ming.h>

static const char *button_events[] = { "Idle => OverUp", "OverUp => Idle", "OverUp => OverDown", "OverDown => OverUp", "OverDown => OutDown", "OutDown => OverDown", "OutDown => Idle", "Idle => OutDown", "OverDown => Idle" };

static void
add_button_events (SWFButton button)
{
  char script[100];
  unsigned int i;

  for (i = 0; i < sizeof (button_events) / sizeof (button_events[0]); i++) {
    sprintf (script, "trace (\"button %s: \" + this);", button_events[i]);
    SWFButton_addAction (button, newSWFAction (script), (1 << i));
  }
}

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
  char name[100];
  SWFMovie movie;
  SWFDisplayItem item;
  SWFButton button, button2;
  SWFButtonRecord rec;

  movie = newSWFMovieWithVersion (version);
  SWFMovie_setRate (movie, 10);
  SWFMovie_setDimension (movie, 200, 150);

  button = newSWFButton ();
  add_button_events (button);
  rec = SWFButton_addCharacter (button, get_rectangle (255, 0, 0), SWFBUTTON_HIT);
  SWFButtonRecord_setDepth (rec, 42);
  SWFButtonRecord_moveTo (rec, 40, 40);
  SWFButtonRecord_scaleTo (rec, 0.5, 0.5);

  button2 = newSWFButton ();
  SWFButton_setMenu (button2, 1);
  add_button_events (button2);
  rec = SWFButton_addCharacter (button2, get_rectangle (0, 255, 0), SWFBUTTON_HIT);
  SWFButtonRecord_scaleTo (rec, 2, 1.5);
  rec = SWFButton_addCharacter (button2, (SWFCharacter) button, SWFBUTTON_UP | SWFBUTTON_OVER | SWFBUTTON_DOWN);

  item = SWFMovie_add (movie, button2);

  SWFMovie_nextFrame (movie);

  sprintf (name, "button-hittest-%d.swf", version);
  SWFMovie_save (movie, name);
}

int
main (int argc, char **argv)
{
  int i;

  if (Ming_init ())
    return 1;

  for (i = 8; i >= 5; i--) {
    do_movie (i);
  }

  return 0;
}
