#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void){
	
	int fd, val;
	char buf[2] = {0,};

	fd = open("/dev/stopwatch", O_RDWR);	// open stopwatch device

	if(fd < 0) {
		perror("ERROR : open /dev/stopwatch");
		exit(-1);
	}
	
	val = write(fd, buf, 2);	// write to device file

	close(fd);					// release
	return 0;
}


