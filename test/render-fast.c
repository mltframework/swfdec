#include <libart_lgpl/libart.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "swf.h"


swf_state_t *s;

void read_swf_file(char *fn);

int main (int argc, char *argv[])
{
  char *fn = "it.swf";

  if(argc>=2){
	fn = argv[1];
  }

  read_swf_file(fn);

  return 0;
}



unsigned char *data;
int len;

void read_swf_file(char *fn)
{
	struct stat sb;
	int fd;
	int ret;
	int i;

	s = swfdec_decoder_new();

	fd = open(fn,O_RDONLY);
	if(fd<0){
		perror(fn);
		exit(1);
	}

	ret = fstat(fd, &sb);
	if(ret<0){
		perror("stat");
		exit(1);
	}

	len = sb.st_size;
	data = malloc(len);
	ret = read(fd, data, len);
	if(ret<0){
		perror("read");
		exit(1);
	}

	ret = SWF_NEEDBITS;
	i = 0;
	while(ret != SWF_EOF){
		ret = swfdec_decoder_parse(s);
		//fprintf(stderr,"swf_parse returned %d\n",ret);
		if(ret == SWF_NEEDBITS){
			if(i==len){
				printf("needbits at eof\n");
			}
			if(i+1000 < len){
				ret = swfdec_decoder_addbits(s,data + i,1000);
				i += 1000;
			}else{
				ret = swfdec_decoder_addbits(s,data + i,len - i);
				i = len;
			}
			//fprintf(stderr,"swf_addbits returned %d\n",ret);
		}
	}

	swfdec_decoder_free(s);
	g_free(data);
}

