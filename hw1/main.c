#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1


#include "main.h"

int input(){

	struct input_event ev[BUFF_SIZE];
	int fd, rd, value, size = sizeof (struct input_event);
	char name[256] = "Unknown";
	int mode = 1;

	char* device = "/dev/input/event0";
	if((fd  = open(device, O_RDONLY|O_NONBLOCK)) == -1){
		printf("%s is not a vaild device \\n", device);
	}

	// ioctl (fd, EVIOCGNAME (sizeof (name)), name);
	// printf ("Reading From : %s (%s)n", device, name);

	while (1){
		if ((rd = read (fd, ev, size * BUFF_SIZE)) < size)
		{
	//		printf("read()");  
		}

		value = ev[0].value;

		if (value != ' ' && ev[1].value == 1 && ev[1].type == 1){
			printf ("code%d\n", (ev[1].code));
		}
		else	if( value == KEY_PRESS ) {
			printf ("key press code[%d]\n",ev[0].code);
			switch(ev[0].code){
				case 115: 
					printf("vol-\n");
					break;
				case 114:
					printf("vol+\n");
					break;
				case 158:
					printf("exit\n");
					return 0;
					break;
			}

		}
		//printf ("Type[%d] Value[%d] Code[%d]\n", ev[0].type, ev[0].value, (ev[0].code));
	}


}


int main (int argc, char *argv[])
{
	input();
	return 0;
}
