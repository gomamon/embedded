#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

int main(void){
	
	int fd, val;
	char buf[2] = {0,};

	fd = open("/dev/blackjack", O_RDWR);	// open stopwatch device

	if(fd < 0) {
		perror("ERROR : open /dev/blackjack");
		exit(-1);
	}
	
	val = write(fd, buf, 2);	// write to device file

	close(fd);					// release
	return 0;
}


