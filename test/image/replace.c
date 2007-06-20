/* gcc `pkg-config --libs --cflags libming glib-2.0` replace.c -o replace && ./replace
 */

#include <ming.h>
#include <glib.h>

typedef enum {
  SHAPE,
  MOVIE,
  BUTTON,
  N_TYPES
} Type;

char *types[] = {
  "shape",
  "movie",
  "button",
};

static SWFDisplayItem
add_rectangle (SWFMovie movie, Type type, int r, int g, int b)
{
  SWFShape shape;
  SWFFillStyle fill;
  SWFDisplayItem item;

  shape = newSWFShape ();
  fill = SWFShape_addSolidFillStyle (shape, r, g, b, 255);
  SWFShape_setRightFillStyle (shape, fill);
  SWFShape_drawLineTo (shape, 50, 0);
  SWFShape_drawLineTo (shape, 50, 50);
  SWFShape_drawLineTo (shape, 0, 50);
  SWFShape_drawLineTo (shape, 0, 0);

  switch (type) {
    case SHAPE:
      item = SWFMovie_add (movie, (SWFBlock) shape);
      break;
    case MOVIE:
      {
	SWFMovieClip clip;
	clip = newSWFMovieClip ();
	SWFMovieClip_add (clip, (SWFBlock) shape);
	SWFMovieClip_nextFrame (clip);
	item = SWFMovie_add (movie, (SWFBlock) clip);
      }
      break;
    case BUTTON:
      {
	SWFButton button;
	button = newSWFButton ();
	SWFButton_addCharacter (button, (SWFCharacter) shape, 0xF);
	item = SWFMovie_add (movie, (SWFBlock) button);
      }
      break;
    default:
      g_assert_not_reached ();
  }

  SWFDisplayItem_setDepth (item, 1);
  return item;
}

static void
modify_placement (SWFMovie movie, Type t1, Type t2)
{
  SWFDisplayItem item;

  add_rectangle (movie, t1, 255, 0, 0);
  SWFMovie_nextFrame (movie);
  item = add_rectangle (movie, t2, 0, 0, 255);
  SWFDisplayItem_setMove (item);
  SWFMovie_nextFrame (movie);
}

static void
do_movie (int version)
{
  SWFMovie movie;
  char *real_name;
  Type t1, t2;

  for (t1 = 0; t1 < N_TYPES; t1++) {
    for (t2 = 0; t2 < N_TYPES; t2++) {
      movie = newSWFMovieWithVersion (version);
      movie = newSWFMovie();
      SWFMovie_setRate (movie, 1);
      SWFMovie_setDimension (movie, 200, 150);

      modify_placement (movie, t1, t2);
      
      SWFMovie_add (movie, (SWFBlock) newSWFAction (""
	    "stop ();"
	    ));
      SWFMovie_nextFrame (movie);

      real_name = g_strdup_printf ("replace-%s-%s-%d.swf", types[t1], types[t2], version);
      SWFMovie_save (movie, real_name);
      g_free (real_name);
    }
  }
}

int
main (int argc, char **argv)
{
  int i;

  if (Ming_init ())
    return 1;

  for (i = 5; i < 8; i++)
    do_movie (i);

  return 0;
}
