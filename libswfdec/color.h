
#ifndef __SWFDEC_COLOR_H__
#define __SWFDEC_COLOR_H__

typedef unsigned int swf_color;

#define SWF_COLOR_COMBINE(r,g,b,a)	(((r)<<24) | ((g)<<16) | ((b)<<8) | (a))
#define SWF_COLOR_R(x)		(((x)>>24)&0xff)
#define SWF_COLOR_G(x)		(((x)>>16)&0xff)
#define SWF_COLOR_B(x)		(((x)>>8)&0xff)
#define SWF_COLOR_A(x)		((x)&0xff)

unsigned int transform_color(unsigned int in, double mult[4], double add[4]);

#endif

