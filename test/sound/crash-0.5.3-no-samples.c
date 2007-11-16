/* gcc `pkg-config --libs --cflags libming` crash-0.5.3-no-samples.c -o crash-0.5.3-no-samples && ./crash-0.5.3-no-samples
 */

#include <ming.h>

static void
do_movie ()
{
  char name[100];
  SWFMovie movie;
  FILE *file;
  SWFMovieClip movie_clip;
  SWFSound sound;
  SWFSoundInstance instance;

  movie = newSWFMovieWithVersion (8);
  movie = newSWFMovie();
  SWFMovie_setRate (movie, 10);
  SWFMovie_setDimension (movie, 200, 150);

  movie_clip = newSWFMovieClip ();

  file = fopen ("/dev/null", "r");
  sound = newSWFSound (file, SWF_SOUND_NOT_COMPRESSED | SWF_SOUND_44KHZ | SWF_SOUND_16BITS | SWF_SOUND_STEREO);
  instance = SWFMovieClip_startSound(movie_clip, sound);
  SWFMovieClip_nextFrame (movie_clip);

  SWFMovie_add (movie, (SWFBlock) movie_clip);
  SWFMovie_nextFrame (movie);

  SWFMovie_save (movie, "crash-0.5.3-no-samples.swf");

  destroySWFSound (sound);
  fclose (file);
}

int
main (int argc, char **argv)
{
  int i;

  if (Ming_init ())
    return 1;

  do_movie ();

  return 0;
}
