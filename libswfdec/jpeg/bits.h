
#ifndef __BITS_H__
#define __BITS_H__

typedef struct bits_struct bits_t;
struct bits_struct {
	unsigned char *ptr;
	int idx;
	unsigned char *end;
};

static inline int bits_needbits(bits_t *b, int n_bytes)
{
	if(b->ptr==NULL)return 1;
	if(b->ptr + n_bytes > b->end)return 1;

	return 0;
}

static inline int getbit(bits_t *b)
{
	int r;

	r = ((*b->ptr)>>(7-b->idx))&1;

	b->idx++;
	if(b->idx>=8){
		b->ptr++;
		b->idx = 0;
	}

	return r;
}

static inline unsigned int getbits(bits_t *b, int n)
{
	unsigned long r = 0;
	int i;

	for(i=0;i<n;i++){
		r <<=1;
		r |= getbit(b);
	}

	return r;
}

static inline unsigned int peekbits(bits_t *b, int n)
{
	bits_t tmp = *b;

	return getbits(&tmp, n);
}

static inline int getsbits(bits_t *b, int n)
{
	unsigned long r = 0;
	int i;

	if(n==0)return 0;
	r = -getbit(b);
	for(i=1;i<n;i++){
		r <<=1;
		r |= getbit(b);
	}

	return r;
}

static inline unsigned int peek_u8(bits_t *b)
{
	return *b->ptr;
}

static inline unsigned int get_u8(bits_t *b)
{
	return *b->ptr++;
}

static inline unsigned int get_u16(bits_t *b)
{
	unsigned int r;

	r = b->ptr[0] | (b->ptr[1]<<8);
	b->ptr+=2;

	return r;
}

static inline unsigned int get_be_u16(bits_t *b)
{
	unsigned int r;

	r = (b->ptr[0]<<8) | b->ptr[1];
	b->ptr+=2;

	return r;
}

static inline unsigned int get_u32(bits_t *b)
{
	unsigned int r;

	r = b->ptr[0] | (b->ptr[1]<<8) | (b->ptr[2]<<16) | (b->ptr[3]<<24);
	b->ptr+=4;

	return r;
}

static inline void syncbits(bits_t *b)
{
	if(b->idx){
		b->ptr++;
		b->idx=0;
	}

}

#endif

