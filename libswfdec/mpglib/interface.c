
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#include "mpg123.h"
#include "mpglib.h"

/* Global mp .. it's a hack */
struct mpstr *gmp;

static void remove_buf(struct mpstr *mp);

gboolean InitMP3(struct mpstr *mp) 
{
	static int init = 0;

	memset(mp,0,sizeof(struct mpstr));

	mp->framesize = 0;
	mp->fsizeold = -1;
	mp->bsize = 0;
	mp->buffers = NULL;
	mp->fr.single = -1;
	mp->bsnum = 0;
	mp->synth_bo = 1;

	mp->bsspace[0] = malloc(MAXFRAMESIZE+512);
	mp->bsspace[1] = malloc(MAXFRAMESIZE+512);

	if(!init) {
		init = 1;
		make_decode_tables(32767);
		init_layer2();
		init_layer3(SBLIMIT);
	}

	return TRUE;
}

void ExitMP3(struct mpstr *mp)
{
	free(mp->bsspace);
	while(mp->buffers){
		remove_buf(mp);
	}
}

static struct buf *addbuf(struct mpstr *mp,char *buf,int size)
{
	struct buf *nbuf;

	nbuf = malloc( sizeof(struct buf) );
	if(!nbuf) return NULL;
	memset(nbuf, 0, sizeof(struct buf));

	nbuf->pnt = malloc(size);
	if(!nbuf->pnt) {
		free(nbuf);
		return NULL;
	}
	nbuf->size = size;
	memcpy(nbuf->pnt,buf,size);
	nbuf->pos = 0;
	mp->bsize += size;

	mp->buffers = g_list_append(mp->buffers, nbuf);

	return nbuf;
}

static void remove_buf(struct mpstr *mp)
{
	struct buf *buf;
  
	buf = mp->buffers->data;

	mp->buffers = g_list_remove(mp->buffers, buf);
  
	free(buf->pnt);
	free(buf);
}

static int read_buf_byte(struct mpstr *mp)
{
	unsigned int b;
	struct buf *buf;
	int pos;

	buf = mp->buffers->data;
	pos = buf->pos;
	while(pos >= buf->size) {
		remove_buf(mp);
		if(!mp->buffers) {
			fprintf(stderr,"Fatal error!\n");
			exit(1);
		}
		buf = mp->buffers->data;
		pos = buf->pos;
	}

	b = buf->pnt[pos];
	mp->bsize--;
	buf->pos++;

	return b;
}

static void read_head(struct mpstr *mp)
{
	unsigned long head;

	head = read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);
	head <<= 8;
	head |= read_buf_byte(mp);

	mp->header = head;
}

int decodeMP3(struct mpstr *mp,char *in,int isize,char *out,
		int osize,int *done)
{
	int len;

	gmp = mp;

	if(osize < 4608) {
		fprintf(stderr,"Too small out space\n");
		return MP3_ERR;
	}

	if(in) {
		if(isize<1){
			fprintf(stderr,"decodeMP3() called with isize<1\n");
			abort();
			return MP3_ERR;
		}
		if(addbuf(mp,in,isize) == NULL) {
			return MP3_ERR;
		}
	}

	/* First decode header */
	if(mp->framesize == 0) {
		if(mp->bsize < 4) {
			return MP3_NEED_MORE;
		}
		read_head(mp);
		decode_header(&mp->fr,mp->header);
		mp->framesize = mp->fr.framesize;
	}

	if(mp->fr.framesize > mp->bsize)
		return MP3_NEED_MORE;

	wordpointer = mp->bsspace[mp->bsnum] + 512;
	mp->bsnum = (mp->bsnum + 1) & 0x1;
	bitindex = 0;

	len = 0;
	while(len < mp->framesize) {
		int nlen;
		struct buf *buf;
		int blen;
		
		buf = mp->buffers->data;
		blen = buf->size - buf->pos;

		if( (mp->framesize - len) <= blen) {
                  nlen = mp->framesize-len;
		}
		else {
                  nlen = blen;
                }
		memcpy(wordpointer+len,buf->pnt+buf->pos,nlen);
                len += nlen;
                buf->pos += nlen;
		mp->bsize -= nlen;
                if(buf->pos == buf->size) {
                   remove_buf(mp);
                }
	}

	*done = 0;
	if(mp->fr.error_protection)
           getbits(16);
        switch(mp->fr.lay) {
          case 1:
	    do_layer1(&mp->fr,(unsigned char *) out,done);
            break;
          case 2:
	    do_layer2(&mp->fr,(unsigned char *) out,done);
            break;
          case 3:
	    do_layer3(&mp->fr,(unsigned char *) out,done);
            break;
        }

	mp->fsizeold = mp->framesize;
	mp->framesize = 0;

	return MP3_OK;
}

int set_pointer(long backstep)
{
  unsigned char *bsbufold;
  if(gmp->fsizeold < 0 && backstep > 0) {
    fprintf(stderr,"Can't step back %ld!\n",backstep);
    return MP3_ERR;
  }
  bsbufold = gmp->bsspace[gmp->bsnum] + 512;
  wordpointer -= backstep;
  if (backstep)
    memcpy(wordpointer,bsbufold+gmp->fsizeold-backstep,backstep);
  bitindex = 0;
  return MP3_OK;
}




