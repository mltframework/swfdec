/* gcc `pkg-config --libs --cflags libming` button-events.c -o button-events && ./button-events
 */

#include <ming.h>

static const char *events[] = { "Press", "Release", "ReleaseOutside", "RollOver", "RollOut", "DragOver", "DragOut" };
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
get_clip_events_movie (const char *name)
{
  SWFMovieClip clip;
  SWFDisplayItem item;

  clip = newSWFMovieClip ();
  item = SWFMovieClip_add (clip, (SWFBlock) newSWFMovieClip ());
  SWFDisplayItem_addAction (item, newSWFAction ("trace (\"load \" + this);"), SWFACTION_ONLOAD);
  SWFDisplayItem_addAction (item, newSWFAction ("trace (\"unload \" + this);"), SWFACTION_UNLOAD);
  SWFDisplayItem_setName (item, name);
  SWFMovieClip_nextFrame (clip);
  return (SWFCharacter) clip;
}

static void
add_item_events (SWFDisplayItem item)
{
  char script[100];
  unsigned int i;

  for (i = 0; i < sizeof (events) / sizeof (events[0]); i++) {
    sprintf (script, "trace (\"place %s: \" + this);", events[i]);
    SWFDisplayItem_addAction (item, newSWFAction (script), 1 << (10 + i));
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
do_movie (int version, int menu)
{
  char name[100];
  SWFMovie movie;
  SWFDisplayItem item;
  SWFButton button;
  SWFButtonRecord rec;

  movie = newSWFMovieWithVersion (version);
  SWFMovie_setRate (movie, 10);
  SWFMovie_setDimension (movie, 200, 150);

  SWFMovie_add (movie, newSWFInitAction (newSWFAction (
	  "button.onPress = function () { trace (\"onPress: \" + this); };"
	  "button.onRelease = function () { trace (\"onRelease: \" + this); };"
	  "button.onReleaseOutside = function () { trace (\"onReleaseOutside: \" + this); };"
	  "button.onRollOver = function () { trace (\"onRollOver: \" + this); };"
	  "button.onRollOut = function () { trace (\"onRollOut: \" + this); };"
	  "button.onDragOver = function () { trace (\"onDragOver: \" + this); };"
	  "button.onDragOut = function () { trace (\"onDragOut: \" + this); };"
	  )));
  button = newSWFButton ();
  SWFButton_setMenu (button, menu);
  rec = SWFButton_addCharacter (button, get_rectangle (255, 0, 0), SWFBUTTON_UP | SWFBUTTON_HIT);
  SWFButtonRecord_setDepth (rec, 1);
  rec = SWFButton_addCharacter (button, get_rectangle (0, 255, 0), SWFBUTTON_OVER);
  SWFButtonRecord_setDepth (rec, 1);
  rec = SWFButton_addCharacter (button, get_rectangle (0, 0, 255), SWFBUTTON_DOWN);
  SWFButtonRecord_setDepth (rec, 1);

  rec = SWFButton_addCharacter (button, get_clip_events_movie ("up2"), SWFBUTTON_UP);
  SWFButtonRecord_setDepth (rec, 2);
  rec = SWFButton_addCharacter (button, get_clip_events_movie ("up3"), SWFBUTTON_UP);
  SWFButtonRecord_setDepth (rec, 3);

  rec = SWFButton_addCharacter (button, get_clip_events_movie ("over4"), SWFBUTTON_OVER);
  SWFButtonRecord_setDepth (rec, 4);
  rec = SWFButton_addCharacter (button, get_clip_events_movie ("over2"), SWFBUTTON_OVER);
  SWFButtonRecord_setDepth (rec, 2);

  rec = SWFButton_addCharacter (button, get_clip_events_movie ("down2"), SWFBUTTON_DOWN);
  SWFButtonRecord_setDepth (rec, 2);
  rec = SWFButton_addCharacter (button, get_clip_events_movie ("down5"), SWFBUTTON_DOWN);
  SWFButtonRecord_setDepth (rec, 5);

  rec = SWFButton_addCharacter (button, get_clip_events_movie ("up_down"), SWFBUTTON_UP | SWFBUTTON_DOWN);
  SWFButtonRecord_setDepth (rec, 6);
  rec = SWFButton_addCharacter (button, get_clip_events_movie ("up_over"), SWFBUTTON_UP | SWFBUTTON_OVER);
  SWFButtonRecord_setDepth (rec, 7);
  rec = SWFButton_addCharacter (button, get_clip_events_movie ("over_down"), SWFBUTTON_DOWN | SWFBUTTON_OVER);
  SWFButtonRecord_setDepth (rec, 8);

  add_button_events (button);
  item = SWFMovie_add (movie, button);
  add_item_events (item);
  SWFDisplayItem_setDepth (item, 0);
  SWFDisplayItem_setName (item, "button");

  SWFMovie_nextFrame (movie);

  sprintf (name, "button-events-%s-%d.swf", menu ? "menu" : "button", version);
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
