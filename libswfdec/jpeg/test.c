
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#include "jpeg.h"

/* getfile */

void *getfile(char *path, int *n_bytes);


int main(int argc, char *argv[])
{
	unsigned char *data;
	int len;
	JpegDecoder *dec;
	char *fn = "biglebowski.jpg";

	dec = jpeg_decoder_new();

	if(argc>1)fn = argv[1];
	data = getfile(fn,&len);

	jpeg_decoder_addbits(dec, data, len);

	jpeg_decoder_parse(dec);

	return 0;
}





/* getfile */

void *getfile(char *path, int *n_bytes)
{
	int fd;
	struct stat st;
	void *ptr = NULL;
	int ret;

	fd = open(path, O_RDONLY);
	if(!fd)return NULL;

	ret = fstat(fd, &st);
	if(ret<0){
		close(fd);
		return NULL;
	}
	
	ptr = malloc(st.st_size);
	if(!ptr){
		close(fd);
		return NULL;
	}

	ret = read(fd, ptr, st.st_size);
	if(ret!=st.st_size){
		free(ptr);
		close(fd);
		return NULL;
	}

	if(n_bytes)*n_bytes = st.st_size;

	close(fd);
	return ptr;
}

