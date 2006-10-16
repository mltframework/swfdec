#include <libswfdec/swfdec.h>

int
main (int argc, char *argv[])
{
  char *fn = "it.swf";
  SwfdecPlayer *player;
  GError *error = NULL;

  swfdec_init ();

  if(argc>=2){
	fn = argv[1];
  }

  player = swfdec_player_new_from_file (argv[1], &error);
  if (player == NULL) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], error->message);
    g_error_free (error);
    return 1;
  }
  if (swfdec_player_get_rate (player) == 0) {
    g_printerr ("Error parsing file \"%s\"\n", argv[1]);
    g_object_unref (player);
    player = NULL;
    return 1;
  }

  g_object_unref (player);
  player = NULL;

  return 0;
}

