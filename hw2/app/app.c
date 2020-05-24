#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/ioctl.h>

/* dev_driver major number */
#define MAJOR_NUM 242

/* ioctl number */
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

	/* charck count of argument  */
	if(argc != 4) { 
		perror("Usage : TIMER_INTERVAL[1-100] TIMER_CNT[1-100] TIMER_INIT[0001-8000]\n");
		return -1;
	}

	/* dev_driver device file open and check error*/
	fd = open("/dev/dev_driver", O_RDWR);
	if (fd<0){
		perror("ERROR : Open Failured!\n");
		return -1;
	}

	/* check argument for interval */
	interval = atoi(argv[1]);
	if(!(1<=interval && interval <=100)){
		perror("TIMER_INTERVAL[1-100] \n");
		return -1;
	}

	/* check argument for count */
	cnt = atoi(argv[2]);
	if(!(1<=interval && interval <=100)){
		perror("TIMER_CNT[1-100]\n");
		return -1;
	}

	/* check argument for init */
	init = atoi(argv[3]);
	if(!(1<=interval && interval <=8000)){
		perror("TIMER_INIT[0001-8000]\n");
		return -1;
	}

	/* make arguments of interval, count, init to string*/
	sprintf(params, "%03d%03d%04d", interval, cnt, init);

	/* pass to argument to kernel using ioctl(IOCTL_SET_OPTION) */
	c = ioctl(fd, IOCTL_SET_OPTION, params);
	if(c < 0){
		perror("IOCTL_SET_OPTION failed!");
	}

	/* start count using ioctl(IOCTL_COMMAND) */
	c = ioctl(fd, IOCTL_COMMAND);
	if(c < 0){
		perror("IOCTL_COMMAND failed!");
	}

	/* close device file */
	close(fd);

	return 0;
}

