#include <unistd.h> // for fork

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

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>	//message queue

#include <sys/shm.h>	//shared mem
#include <sys/sem.h>	//semaphore

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1


#include "main.h"

#define SHARED_KEY1 (key_t) 0x10
#define SHARED_KEY2 (key_t) 0x15
#define SHARED_KEY3 (key_t) 0x20
#define SEM_KEY (key_t) 0x25 //semaphore key
#define IFLAGS (IPC_CREAT)
#define ERR ((struct databuf *)-1)
#define SIZE 2048


struct sembuf p1 = {0, -1, SEM_UNDO }, p2 = {1, -1, SEM_UNDO}, p3 = {2, -1, SEM_UNDO};
struct sembuf v1 = {0, 1, SEM_UNDO }, v2 = {1, 1, SEM_UNDO }, v3 = {2,1,SEM_UNDO};

struct databuf {
	int d_nread;
	char d_buf[SIZE];
};

static int shm_id1, shm_id2, shm_id3, sem_id;
/*
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
}
 */

void getseg(struct databuf **p1, struct databuff **p2){
	if((shm_id1 = shmget(SHARED_KEY1, sizeof(struct databuf), 0600 | IFLAGS)) == -1){
		perror("error shmget\n");
		exit(1);
	}

	if((shm_id2 = shmget(SHARED_KEY2, sizeof(struct databuf), 0600 | IFLAGS)) == -1){
		perror("error shmget\n");
		exit(1);
	}
	if((shm_id3 = shmget(SHARED_KEY3, sizeof(struct databuf), 0600 | IFLAGS)) == -1){
		perror("error shmget\n");
		exit(1);
	}

	if((*p1 = (struct databuf*)shmat(shm_id1, 0, 0))==ERR){
		perror("error shmget\n");
		exit(1);	
	}
	if((*p2 = (struct databuf*)shmat(shm_id2, 0, 0))==ERR){
		perror("error shmget\n");
		exit(1);
	}
	if((*p3 = (struct databuf*)shmat(shm_id3, 0, 0))==ERR){
		perror("error shmget\n");
		exit(1);
	}
}

int getsem(){
	semun x;
	x.val = 0;
	int id = -1;

	if ((id = semget (SEM_KEY, 2, 0600 | IFLAGS ) == -1)) exit(1);
	if (semctl (id, 0, SETVAL, x) == -1)				exit(1);
	if (semctl (id, 1, SETVAL, x) == -1)				exit(1);
	if (semctl (id, 2, SETVAL, x) == -1)				exit(1);

	return (id);
}

void remobj(){
	if (shmctl(shm_id1, IPC_RMID, 0) == -1) exit(1);
	if (shmctl(shm_id2, IPC_RMID, 0) == -1) exit(1);
	if (shmctl(shm_id3, IPC_RMID, 0) == -1) exit(1);	 //delete shared mem
	if (semctl(sem_id, 0, IPC_RMID, 0) == -1) exit(1);
}


int glob = 6;


void proc_in(){


}
void proc_out(){

}

void proc_main(){

}

int main (int argc, char *argv[])
{

	pid_t pid_in =0 , pid_out=0;
	struct databuf *buf1, *buf2, *buf3;

	sem_id = getsem(); //creat and init semaphore
	getseg (&buf1, &buf2);	//create and attach shared mem 
	
	/* create child processes */
	switch(pid_in = fork()){
		case -1:
			perror("ERROR: fork()");
			break;
		case 0:
			printf("input process\n");
			proc_in();
			break;
		default:
			switch(pid_out = fork()){
				case -1:
					perror("ERROR: fork()");
					break;
				case 0:
					printf("output process\n")
					proc_out();
					break;
				default:
					proc_main();
					break;
			}
			break;
	}


	return 0;
}


