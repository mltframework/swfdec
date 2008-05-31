
#ifndef _COG_COMPAT_H_
#define _COG_COMPAT_H_

#include <swfdec_debug.h>
#include <glib.h>

#define COG_LOG(...) SWFDEC_LOG(__VA_ARGS__)
#define COG_DEBUG(...) SWFDEC_DEBUG(__VA_ARGS__)
#define COG_INFO(...) SWFDEC_INFO(__VA_ARGS__)
#define COG_WARNING(...) SWFDEC_WARNING(__VA_ARGS__)
#define COG_ERROR(...) SWFDEC_ERROR(__VA_ARGS__)

#define malloc g_malloc
#define free g_free

#endif

