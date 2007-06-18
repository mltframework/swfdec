/* gcc `pkg-config --libs --cflags libming glib-2.0` duplicate-depth.c -o duplicate-depth && ./duplicate-depth
 */

#include <ming.h>
#include <glib.h>

static void
add_rectangle (SWFMovieClip clip, int r, int g, int b)
{
  SWFShape shape;
  SWFFillStyle fill;

  shape = newSWFShape ();
  fill = SWFShape_addSolidFillStyle (shape, r, g, b, 255);
  SWFShape_setRightFillStyle (shape, fill);
  SWFShape_drawLineTo (shape, 50, 0);
  SWFShape_drawLineTo (shape, 50, 50);
  SWFShape_drawLineTo (shape, 0, 50);
  SWFShape_drawLineTo (shape, 0, 0);

  SWFMovieClip_add (clip, (SWFBlock) shape);
}

static void
modify_placement (SWFMovie movie, guint mod)
{
  SWFDisplayItem item;
  SWFBlock clip, clip2;

  clip = (SWFBlock) newSWFMovieClip ();
  add_rectangle ((SWFMovieClip) clip, 255, 0, 0);
  SWFMovieClip_nextFrame ((SWFMovieClip) clip);
  clip2 = (SWFBlock) newSWFMovieClip ();
  add_rectangle ((SWFMovieClip) clip2, 0, 0, 255);
  SWFMovieClip_nextFrame ((SWFMovieClip) clip2);
  
  item = SWFMovie_add (movie, clip);
  SWFDisplayItem_setDepth (item, 1);
  SWFDisplayItem_setName (item, "a");
  SWFMovie_nextFrame (movie);

  item = SWFMovie_add (movie, clip2);
  SWFDisplayItem_setDepth (item, 1);
  SWFDisplayItem_moveTo (item, 20, 20);
  SWFDisplayItem_setName (item, "b");
}

static void
do_movie (int version)
{
  SWFMovie movie;
  char *real_name;
  guint i;

  movie = newSWFMovieWithVersion (version);
  movie = newSWFMovie();
  SWFMovie_setRate (movie, 1);
  SWFMovie_setDimension (movie, 200, 150);

  modify_placement (movie, i);
  SWFMovie_nextFrame (movie);
  
  SWFMovie_add (movie, (SWFBlock) newSWFAction (""
#if 0
	"loadMovie (\"FSCommand:quit\", \"\");"
#else
	"stop ();"
#endif
	));
  SWFMovie_nextFrame (movie);

  real_name = g_strdup_printf ("duplicate-depth-%d.swf", version);
  SWFMovie_save (movie, real_name);
  g_free (real_name);
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
