
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

	if(r<0)r=0; if(r>255)r=255;
	if(g<0)g=0; if(g>255)g=255;
	if(b<0)b=0; if(b>255)b=255;
	if(a<0)a=0; if(a>255)a=255;

	//SWF_DEBUG(0,"out rgba %d,%d,%d,%d\n",r,g,b,a);

	return SWF_COLOR_COMBINE(r,g,b,a);
}

