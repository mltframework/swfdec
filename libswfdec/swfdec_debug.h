
#ifndef __SWFDEC_DEBUG_H__
#define __SWFDEC_DEBUG_H__

enum
{
  SWFDEC_LEVEL_NONE = 0,
  SWFDEC_LEVEL_ERROR,
  SWFDEC_LEVEL_WARNING,
  SWFDEC_LEVEL_INFO,
  SWFDEC_LEVEL_DEBUG,
  SWFDEC_LEVEL_LOG
};

#define SWFDEC_ERROR(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_ERROR, __VA_ARGS__)
#define SWFDEC_WARNING(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_WARNING, __VA_ARGS__)
#define SWFDEC_INFO(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_INFO, __VA_ARGS__)
#define SWFDEC_DEBUG(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_DEBUG, __VA_ARGS__)
#define SWFDEC_LOG(...) \
  SWFDEC_DEBUG_LEVEL(SWFDEC_LEVEL_LOG, __VA_ARGS__)

#define SWFDEC_DEBUG_LEVEL(level,...) \
  swfdec_debug_log ((level), __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

void swfdec_debug_log (int level, const char *file, const char *function,
    int line, const char *format, ...);
void swfdec_debug_set_level (int level);
int swfdec_debug_get_level (void);

#endif
