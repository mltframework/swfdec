
#ifndef _GMESSAGES_H
#define _GMESSAGES_H

#include <stdlib.h>
#include <stdio.h>

#define g_return_if_fail(expr) do{ \
  if (expr) { } else { \
    fprintf(stderr, "*** file %s: line %d (%s): assertion '%s' failed\n", \
        __FILE__, __LINE__, "", #expr); \
    return; \
  } \
}while(0)

#define g_return_val_if_fail(expr,val) do{ \
  if (expr) { } else { \
    fprintf(stderr, "*** file %s: line %d (%s): assertion '%s' failed\n", \
        __FILE__, __LINE__, "", #expr); \
    return (val); \
  } \
}while(0)

#define g_warning(x) printf(x)
#define g_assert(expr) do { \
  if (expr) { } else { \
    fprintf(stderr, "*** file %s: line %d (%s): assertion '%s' failed\n", \
        __FILE__, __LINE__, "", #expr); \
    exit (1); \
  } \
}while(0)

#endif

