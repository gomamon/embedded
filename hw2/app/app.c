#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/ioctl.h>

#define MAJOR_NUM 242

#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 0, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 1)


int main(int argc, char **argv)
{
	int fd;
	int interval;
	int cnt;
	int init;
	char c;
	char params[12] ;

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
	if(!(1<=interval && interval <=8000)){
		perror("TIMER_INIT[0001-8000]\n");
		return -1;
	}

	sprintf(params, "%03d%03d%04d", interval, cnt, init);
	printf("PARAMETER in USER : %s",params);

	c = ioctl(fd, IOCTL_SET_OPTION, params);
	if(c < 0){
		perror("IOCTL_SET_OPTION failed!");
	}

	c = ioctl(fd, IOCTL_COMMAND);
	if(c < 0){
		perror("IOCTL_COMMAND failed!");
	}

	close(fd);

	return 0;
}

