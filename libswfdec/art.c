
#include "swfdec_internal.h"


/*
 * This file defines some libart-related functions that aren't
 * in libart.
 *
 */

int art_affine_inverted(double x[6])
{
	double det;

	det = x[0]*x[4] - x[1]*x[3];

	if(det<0)return 1;
	return 0;
}

void art_vpath_dump(FILE *out, ArtVpath *vpath)
{
	while(vpath->code != ART_END){
		fprintf(out, "%d %g %g\n",vpath->code, vpath->x, vpath->y);
		vpath++;
	}
}

void art_bpath_dump(FILE *out, ArtBpath *vpath)
{
	while(1){
		switch(vpath->code){
		case ART_END:
			fprintf(out, "end\n");
			return;
		case ART_LINETO:
			fprintf(out, "lineto %g %g\n",vpath->x3, vpath->y3);
			break;
		case ART_MOVETO:
			fprintf(out, "moveto %g %g\n",vpath->x3, vpath->y3);
			break;
		case ART_CURVETO:
			fprintf(out, "curveto %g %g\n",vpath->x3, vpath->y3);
			break;
		default:
			fprintf(out, "other\n");
		}
		vpath++;
	}
}

int art_bpath_len(ArtBpath *a)
{
	int i;
	for(i=0;a[i].code != ART_END;i++);
	return i;
}

int art_vpath_len(ArtVpath *a)
{
	int i;
	for(i=0;a[i].code != ART_END;i++);
	return i;
}

ArtBpath *art_bpath_cat(ArtBpath *a, ArtBpath *b)
{
	ArtBpath *dest;
	int len_a, len_b;

	len_a = art_bpath_len(a);
	len_b = art_bpath_len(b);
	dest = malloc((len_a + len_b + 1) * sizeof(ArtBpath));
	
	memcpy(dest,a,sizeof(ArtBpath) * len_a);
	memcpy(dest + len_a, b, sizeof(ArtBpath) * (len_b + 1));

	return dest;
}

ArtVpath *art_vpath_cat(ArtVpath *a, ArtVpath *b)
{
	ArtVpath *dest;
	int len_a, len_b;

	len_a = art_vpath_len(a);
	len_b = art_vpath_len(b);
	dest = malloc((len_a + len_b + 1) * sizeof(ArtVpath));
	
	memcpy(dest,a,sizeof(ArtVpath) * len_a);
	memcpy(dest + len_a, b, sizeof(ArtVpath) * (len_b + 1));

	return dest;
}

ArtVpath *art_vpath_reverse(ArtVpath *a)
{
	ArtVpath *dest;
	ArtVpath it;
	int len;
	int state = 0;
	int i;

	len = art_vpath_len(a);
	dest = malloc((len + 1) * sizeof(ArtVpath));

	for(i=0;i<len;i++){
		it = a[len - i - 1];
		if(state){
			it.code = ART_LINETO;
		}else{
			it.code = ART_MOVETO_OPEN;
			state = 1;
		}
		if(a[len - i - 1].code==ART_MOVETO ||
		   a[len - i - 1].code==ART_MOVETO_OPEN){
			state = 0;
		}
		dest[i] = it;
	}
	dest[len] = a[len];

	return dest;
}

ArtVpath *art_vpath_reverse_free(ArtVpath *a)
{
	ArtVpath *dest;

	dest = art_vpath_reverse(a);
	art_free(a);
	
	return dest;
}

void art_rgb_svp_alpha2 (const ArtSVP *svp, int x0, int y0,
	int x1, int y1, art_u32 rgba, art_u8 *buf, int rowstride,
	ArtAlphaGamma *alphagamma)
{
	ArtUta *uta;
	int i,x,y;

	uta = art_uta_from_svp(svp);

	for(y=0;y<uta->width;y++){
		if((uta->y0+y)*32 < y0 || (uta->y0+y+1)*32 > y1)continue;

		for(x=0;x<uta->width;x++){
			i = y*uta->width + x;
			if((uta->x0+x)*32 < x0 || (uta->x0+x+1)*32 > x1)
				continue;
			if(uta->utiles[i]){
				art_rgb_svp_alpha(svp,
					(uta->x0+x)*32,
					(uta->y0+y)*32,
					(uta->x0+x)*32 + 32,
					(uta->y0+y)*32 + 32,
					rgba,
					buf + rowstride*(uta->y0+y)*32 +
						(uta->x0+x)*32*3,
					rowstride,
					alphagamma);
			}
		}
	}

	art_uta_free(uta);
}

void art_rgb_fill_run(unsigned char *buf, unsigned char r,
	unsigned char g, unsigned char b, int n)
{
	int i;

	for(i=0;i<n;i++){
		*buf++ = r;
		*buf++ = g;
		*buf++ = b;
	}
}

void art_rgb_run_alpha(unsigned char *buf, unsigned char r,
	unsigned char g, unsigned char b, int alpha, int n)
{
	int i;
	int add_r,add_g,add_b,unalpha;

	if(alpha==0)return;
	if(alpha>=0xff){
		for(i=0;i<n;i++){
			*buf++ = r;
			*buf++ = g;
			*buf++ = b;
		}
		return;
	}
#define APPLY_ALPHA(x,y,a) (x) = (((y)*(alpha)+(x)*(255-alpha))>>8)
	unalpha = 255-alpha;
	add_r = r*alpha + 0x80;
	add_g = g*alpha + 0x80;
	add_b = b*alpha + 0x80;
	for(i=0;i<n;i++){
		*buf = (add_r + unalpha*(*buf))>>8;
		buf++;
		*buf = (add_g + unalpha*(*buf))>>8;
		buf++;
		*buf = (add_b + unalpha*(*buf))>>8;
		buf++;
	}
}

void art_rgb565_run_alpha(unsigned char *buf, unsigned char r,
	unsigned char g, unsigned char b, int alpha, int n)
{
	int i;
	unsigned short *x = (void *)buf;
	unsigned short c;
	int rr,gg,bb;
	int ar,ag,ab;
	int unalpha;

#define RGB565_COMBINE(r,g,b) (((r)&0xf8)<<8)|(((g)&0xfc)<<3)|(((b)&0xf8)>>3);
#define RGB565_R(color) (((color)&0xf800)>>8)
#define RGB565_G(color) (((color)&0x07e0)>>3)
#define RGB565_B(color) (((color)&0x001f)<<3)

	if(alpha==0)return;
	if(alpha>=0xff){
		c = RGB565_COMBINE(r,g,b);
		for(i=0;i<n;i++){
			*x++ = c;
		}
		return;
	}
	unalpha = 255-alpha;
	ar = r * alpha + 0x80;
	ag = g * alpha + 0x80;
	ab = b * alpha + 0x80;
	for(i=0;i<n;i++){
		rr = (RGB565_R(*x)*unalpha + ar)>>8;
		gg = (RGB565_G(*x)*unalpha + ag)>>8;
		bb = (RGB565_B(*x)*unalpha + ab)>>8;
		*x = RGB565_COMBINE(rr,gg,bb);
		x++;
	}
}

void art_rgb565_fill_run(unsigned char *buf, unsigned char r,
	unsigned char g, unsigned char b, int n)
{
	int i;
	unsigned short *x = (void *)buf;
	unsigned short c = RGB565_COMBINE(r,g,b);

	for(i=0;i<n;i++){
		*x++ = c;
	}
}

void art_rgb565_fillrect(char *buffer, int stride, unsigned int color, ArtIRect *rect)
{
	int i;

	buffer += rect->x0 * 2;
	for(i=rect->y0;i<rect->y1;i++){
		art_rgb565_run_alpha(buffer + i*stride,
			SWF_COLOR_R(color),
			SWF_COLOR_G(color),
			SWF_COLOR_B(color),
			SWF_COLOR_A(color),
			rect->x1 - rect->x0);
	}
}

void art_rgb_fillrect(char *buffer, int stride, unsigned int color, ArtIRect *rect)
{
	int i;

	buffer += rect->x0 * 3;
	for(i=rect->y0;i<rect->y1;i++){
		art_rgb_run_alpha(buffer + i*stride,
			SWF_COLOR_R(color),
			SWF_COLOR_G(color),
			SWF_COLOR_B(color),
			SWF_COLOR_A(color),
			rect->x1 - rect->x0);
	}
}

void art_rgb_render_callback(void *data, int y, int start,
	ArtSVPRenderAAStep *steps, int n_steps)
{


}


/* taken from:
 * Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
 */

void
art_rgb565_svp_alpha_callback (void *callback_data, int y,
			    int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  struct swf_svp_render_struct *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  art_u32 running_sum = start;
  int x0, x1;
  int k;
  art_u8 r, g, b, a;
  int alpha;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  r = SWF_COLOR_R(data->color);
  g = SWF_COLOR_G(data->color);
  b = SWF_COLOR_B(data->color);
  a = SWF_COLOR_A(data->color);

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	{
	  alpha = (a * running_sum>>8) >> 16;
	  if (alpha)
	    art_rgb565_run_alpha (linebuf,
			       r, g, b, alpha,
			       run_x1 - x0);
	}

      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = run_x1;
	  run_x1 = steps[k + 1].x;
	  if (run_x1 > run_x0)
	    {
	      alpha = (a * (running_sum>>8)) >> 16;
	      if (alpha)
		art_rgb565_run_alpha (linebuf + (run_x0 - x0) * 2,
				   r, g, b, alpha,
				   run_x1 - run_x0);
	    }
	}
      running_sum += steps[k].delta;
      if (x1 > run_x1)
	{
	  alpha = (a * (running_sum>>8)) >> 16;
	  if (alpha)
	    art_rgb565_run_alpha (linebuf + (run_x1 - x0) * 2,
			       r, g, b, alpha,
			       x1 - run_x1);
	}
    }
  else
    {
      alpha = (a * (running_sum>>8)) >> 16;
      if (alpha)
	art_rgb565_run_alpha (linebuf,
			   r, g, b, alpha,
			   x1 - x0);
    }

  data->buf += data->rowstride;
}

void
art_rgb_svp_alpha_callback (void *callback_data, int y,
			    int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  struct swf_svp_render_struct *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  art_u32 running_sum = start;
  int x0, x1;
  int k;
  art_u8 r, g, b, a;
  int alpha;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  r = SWF_COLOR_R(data->color);
  g = SWF_COLOR_G(data->color);
  b = SWF_COLOR_B(data->color);
  a = SWF_COLOR_A(data->color);

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	{
	  alpha = (a * (running_sum>>8)) >> 16;
	  if (alpha)
	    art_rgb_run_alpha (linebuf,
			       r, g, b, alpha,
			       run_x1 - x0);
	}

      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = run_x1;
	  run_x1 = steps[k + 1].x;
	  if (run_x1 > run_x0)
	    {
	      alpha = (a * (running_sum>>8)) >> 16;
	      if (alpha)
		art_rgb_run_alpha (linebuf + (run_x0 - x0) * 3,
				   r, g, b, alpha,
				   run_x1 - run_x0);
	    }
	}
      running_sum += steps[k].delta;
      if (x1 > run_x1)
	{
	  alpha = (a * (running_sum>>8)) >> 16;
	  if (alpha)
	    art_rgb_run_alpha (linebuf + (run_x1 - x0) * 3,
			       r, g, b, alpha,
			       x1 - run_x1);
	}
    }
  else
    {
      alpha = (a * (running_sum>>8)) >> 16;
      if (alpha)
	art_rgb_run_alpha (linebuf,
			   r, g, b, alpha,
			   x1 - x0);
    }

  data->buf += data->rowstride;
}


inline void art_grey_run_alpha(unsigned char *buf, int alpha, int n)
{
	if(alpha>0xff)alpha=0xff;
	memset(buf, alpha, n);
}

void
art_grey_svp_alpha_callback (void *callback_data, int y,
			    int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  struct swf_svp_render_struct *data = callback_data;
  art_u8 *linebuf;
  int run_x0, run_x1;
  art_u32 running_sum = start;
  int x0, x1;
  int k;
  int alpha;

  linebuf = data->buf;
  x0 = data->x0;
  x1 = data->x1;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0)
	{
	  alpha = running_sum >> 16;
	  art_grey_run_alpha (linebuf, alpha, run_x1 - x0);
	}

      for (k = 0; k < n_steps - 1; k++)
	{
	  running_sum += steps[k].delta;
	  run_x0 = run_x1;
	  run_x1 = steps[k + 1].x;
	  if (run_x1 > run_x0)
	    {
	      alpha = running_sum >> 16;
		art_grey_run_alpha (linebuf + (run_x0 - x0),
				   alpha, run_x1 - run_x0);
	    }
	}
      running_sum += steps[k].delta;
      if (x1 > run_x1)
	{
	  alpha = running_sum >> 16;
	  if (alpha)
	    art_grey_run_alpha (linebuf + (run_x1 - x0),
			       alpha, x1 - run_x1);
	}
    }
  else
    {
      alpha = running_sum >> 16;
      art_grey_run_alpha (linebuf, alpha, x1 - x0);
    }

  data->buf += data->rowstride;
}

