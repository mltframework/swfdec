
#ifndef _SWF_PROTO_H_
#define _SWF_PROTO_H_

#include "swfdec_types.h"
#include "swf.h"

/* render.c */

void swf_invalidate_irect(SwfdecDecoder *s, ArtIRect *rect);
unsigned int transform_color(unsigned int in, double mult[4], double add[4]);
SwfdecLayer *swf_layer_get(SwfdecDecoder *s, int depth);
void swf_layer_del(SwfdecDecoder *s, SwfdecLayer *layer);
int art_place_object_2(SwfdecDecoder *s);
int art_remove_object(SwfdecDecoder *s);
int art_remove_object_2(SwfdecDecoder *s);
int art_show_frame(SwfdecDecoder *s);
SwfdecObject *swfdec_object_get(SwfdecDecoder *s, int id);
void swf_render_frame(SwfdecDecoder *s);
void swf_layer_prerender(SwfdecDecoder *s, SwfdecLayer *layer);

/* swf.c */

SwfdecDecoder *swf_init(void);
int swf_addbits(SwfdecDecoder *s, unsigned char *bits, int len);
int swf_parse(SwfdecDecoder *s);
int swf_parse_header(SwfdecDecoder *s);
int swf_parse_tag(SwfdecDecoder *s);
int tag_func_zero(SwfdecDecoder *s);
int tag_func_ignore_quiet(SwfdecDecoder *s);
int tag_func_ignore(SwfdecDecoder *s);
int tag_func_dumpbits(SwfdecDecoder *s);
void get_actions(SwfdecDecoder *s, bits_t *bits);
int tag_func_set_background_color(SwfdecDecoder *s);
int tag_func_frame_label(SwfdecDecoder *s);
int tag_func_do_action(SwfdecDecoder *s);
int tag_func_place_object_2(SwfdecDecoder *s);
int tag_func_remove_object(SwfdecDecoder *s);
int tag_func_remove_object_2(SwfdecDecoder *s);

#endif

