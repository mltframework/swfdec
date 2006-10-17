
#ifndef _JPEG_DEBUG_H_
#define _JPEG_DEBUG_H_

#include <libswfdec/swfdec_debug.h>

//#define JPEG_DEBUG_ON

#define JPEG_ERROR(...) \
    JPEG_DEBUG_LEVEL(SWFDEC_LEVEL_ERROR, __VA_ARGS__)
#define JPEG_WARNING(...) \
    JPEG_DEBUG_LEVEL(SWFDEC_LEVEL_WARNING, __VA_ARGS__)
#define JPEG_INFO(...) \
    JPEG_DEBUG_LEVEL(SWFDEC_LEVEL_INFO, __VA_ARGS__)
#define JPEG_DEBUG(...) \
    JPEG_DEBUG_LEVEL(SWFDEC_LEVEL_DEBUG, __VA_ARGS__)
#define JPEG_LOG(...) \
    JPEG_DEBUG_LEVEL(SWFDEC_LEVEL_LOG, __VA_ARGS__)

#ifdef JPEG_DEBUG_ON
#define JPEG_DEBUG_LEVEL(level,...) \
    swfdec_debug_log ((level), __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define JPEG_DEBUG_LEVEL(level,...)
#endif


#endif
