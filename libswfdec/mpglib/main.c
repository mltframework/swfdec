
#include <unistd.h>
#include <stdio.h>
#include <mpglib.h>
#include <stdlib.h>

char buf[16384];
MpglibDecoder *mp;

int main(int argc,char **argv)
{
	int size;
	char out[8192];
	int len,ret;
	
	mp = mpglib_decoder_new();

	while(1) {
		len = read(0,buf,16384);
		if(len <= 0)
			break;
		ret = mpglib_decoder_decode(mp,buf,len,out,8192,&size);
		while(ret == MPGLIB_OK) {
			write(1,out,size);
			ret = mpglib_decoder_decode(mp,NULL,0,out,8192,&size);
		}
		if(ret == MPGLIB_ERR){
			fprintf(stderr,"stream error\n");
			exit(1);
		}
	}

  return 0;

}

