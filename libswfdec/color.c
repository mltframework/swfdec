
#include <math.h>

#include "swfdec_internal.h"

unsigned int transform_color(unsigned int in, double mult[4], double add[4])
{
	int r, g, b, a;

	r = SWF_COLOR_R(in);
	g = SWF_COLOR_G(in);
	b = SWF_COLOR_B(in);
	a = SWF_COLOR_A(in);

	//SWF_DEBUG(0,"in rgba %d,%d,%d,%d\n",r,g,b,a);

	r = rint((r*mult[0] + add[0]));
	g = rint((g*mult[1] + add[1]));
	b = rint((b*mult[2] + add[2]));
	a = rint((a*mult[3] + add[3]));

	r = CLAMP(r,0,255);
	g = CLAMP(g,0,255);
	b = CLAMP(b,0,255);
	a = CLAMP(a,0,255);

	//SWF_DEBUG(0,"out rgba %d,%d,%d,%d\n",r,g,b,a);

	return SWF_COLOR_COMBINE(r,g,b,a);
}

