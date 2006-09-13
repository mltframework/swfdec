
#ifndef _SWFDEC_DECODER_INTERNAL_H_
#define _SWFDEC_DECODER_INTERNAL_H_


gboolean swfdec_decoder_has_mouse (SwfdecDecoder * s, SwfdecSpriteSegment *seg,
    SwfdecObject *obj);
void swfdec_decoder_grab_mouse (SwfdecDecoder * s, SwfdecObject *obj);

#endif

