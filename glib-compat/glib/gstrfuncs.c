
#include "config.h"
#include "glib.h"

#include <string.h>

gchar*	              g_strdup	       (const gchar *str)
{
  char *s;
  int len;

  len = strlen(str);
  s = g_malloc(len+1);
  memcpy (s,str,len+1);

  return s;
}

gchar*	              g_memdup	       (const gchar *mem, int len)
{
  char *s;

  s = g_malloc(len);
  memcpy (s,mem,len);

  return s;
}



