
#ifndef _MPGLIB_H_
#define _MPGLIB_H_

typedef struct mpglib_decoder_struct MpglibDecoder;

#define MPGLIB_ERR -1
#define MPGLIB_OK  0
#define MPGLIB_NEED_MORE 1

MpglibDecoder *mpglib_decoder_new(void);
int mpglib_decoder_decode(MpglibDecoder *mp,char *inmemory,int inmemsize,
     char *outmemory,int outmemsize,int *done);
void mpglib_decoder_free(MpglibDecoder *mp);


#endif

