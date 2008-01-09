//gcc -Wall -Werror `pkg-config --libs --cflags libming glib-2.0` compiler.c -o compiler

#include <glib.h>
#include <ming.h>
#include <string.h>

/* This is what is used to compile the Actionscript parts of the source to
 * Swfdec test scripts that can later be executed.
 * Note that this is pretty much a hack until someone writes a proper
 * Actionscript compiler for Swfdec.
 * Also note that the creation of the include-scripts should probably not be 
 * autorun, as we don't want to depend on external bugs in Ming, only on internal ones.
 */

/* header to put in front of script */
#define HEADER "Swfdec Test Script\0\1"

int
main (int argc, char **argv)
{
  SWFAction action;
  char *contents;
  GError *error = NULL;
  int len;
  byte *data;

  if (argc != 3) {
    g_print ("usage: %s INFILE OUTFILE\n\n", argv[0]);
    return 1;
  }

  Ming_init ();

  if (!g_file_get_contents (argv[1], &contents, NULL, &error)) {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
    error = NULL;
    return 1;
  }
  action = newSWFAction (contents);
  if (SWFAction_compile (action, 8, &len) != 0) {
    g_printerr ("compilation failed\n");
    return 1;
  }
  data = SWFAction_getByteCode (action, NULL);
  contents = g_malloc (len + sizeof (HEADER));
  memcpy (contents, HEADER, sizeof (HEADER));
  memcpy (contents + sizeof (HEADER), data, len);
  if (!g_file_set_contents (argv[2], contents, len + sizeof (HEADER), &error)) {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
    error = NULL;
    return 1;
  }
  g_free (contents);
  return 0;
}
