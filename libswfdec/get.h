
#ifndef __SWFDEC_GET_H__
#define __SWFDEC_GET_H__

void get_art_color_transform(bits_t *bits, double mult[4], double add[4]);
void get_art_matrix(bits_t *bits, double *trans);
char *get_string(bits_t *bits);
void get_color_transform(bits_t *bits);
unsigned int get_color(bits_t *bits);
unsigned int get_rgba(bits_t *bits);
void get_matrix(bits_t *bits);
unsigned int get_gradient(bits_t *bits);
unsigned int get_gradient_rgba(bits_t *bits);
void get_fill_style(bits_t *bits);
void get_line_style(bits_t *bits);
void get_rect(bits_t *bits, int *rect);

#endif

