#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "app.h"

int main(int argc, char **argv)
{
	int fd;
	int interval;
	int cnt;
	int init;
	char c;
	char command[8] ;

	if(argc != 4) { 
		perror("Usage : TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]\n");
		return -1;
	}

	fd = open("/dev/dev_driver", O_RDWR);
	if (fd<0){
		perror("ERROR : Open Failured!\n");
		return -1;
	}

	interval = atoi(argv[1]);
	if(!(1<=interval && interval <=100)){
		perror("TIMER_INTERVAL[1-100] \n");
		return -1;
	}

	cnt = atoi(argv[2]);
	if(!(1<=interval && interval <=100)){
		perror("TIMER_CNT[1-100]\n");
		return -1;
	}

	init = atoi(argv[3]);
	if(!(1<=interval && interval <=100)){
		perror("TIMER_CNT[1-100]\n");
		return -1;
	}

	sprintf(command, "%03d%04d", cnt, init);

	memcpy(interval, argv[1], sizeof(interval));
	memcpy(cnt, argv[2], sizeof(cnt));
	memcpy(init, argv[1], sizeof(cnt));
	

	c = ioctl(fd, IOCTL_SET_OPTION, &interval);
	if(c < 0){
		perror("IOCTL_SET_OPTION failed!");
	}
	c = ioctl(fd, IOCTL_COMMAND, command);
	if(c < 0){
		perror("IOCTL_COMMAND failed!");
	}

	close(fd);

	return 0;
}

