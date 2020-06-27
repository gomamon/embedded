#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

struct  gamer{
	int cards[12];
	int point;
	int idx;
	int money;
} dealer;

int main(void){
	
	int fd, val;
	char buff[100] = {0,};

	fd = open("/dev/blackjack", O_RDWR);	// open stopwatch device

	if(fd < 0) {
		perror("ERROR : open /dev/bp");
		exit(-1);
	}
	while(1){
		val = write(fd, buff, 2);	// write to device file
		read(fd, &(dealer), sizeof(struct gamer));
		if(dealer.point<0) break;
		printf("------<dealer>----- \n point: %d\n money : %d\n", dealer.point, dealer.money);
		printf("--------------------\n");
		if(dealer.point<0) break;
	}
	close(fd);					// release
	return 0;
}


