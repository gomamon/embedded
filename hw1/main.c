#include <unistd.h> // for fork

#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
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
#define SHARED_KEY2 (key_t) 0x11
#define SHARED_KEY3 (key_t) 0x12
#define SEM_KEY (key_t) 0x20 //semaphore key
#define IFLAGS (IPC_CREAT)
#define ERR ((struct databuf *)-1)
#define ERR_INT ((int *)-1)
#define SIZE 2048

#define KEY_VOL_DOWN 114
#define KEY_VOL_UP 115
#define KEY_BACK 158

#define MAX_BUTTON 9

#define MODE_NUM 4

struct sembuf p1 = {0, -1, SEM_UNDO }, p2 = {1, -1, SEM_UNDO}, p3 = {2, -1, SEM_UNDO};
struct sembuf v1 = {0, 1, SEM_UNDO }, v2 = {1, 1, SEM_UNDO }, v3 = {2,1,SEM_UNDO};

typedef union {
	int val;
	struct semid_ds *buf;
	unsigned short * array;
} semun;


struct databuf {
	int d_nread;
	char d_buf[SIZE];
};


static int shm_key, shm_mode, shm_status, sem_id;

void getseg(int **p1, int **p2, struct databuf **p3){ // init
	/*create shared mem*/
	if((shm_key = shmget(SHARED_KEY1, sizeof(int), 0600 | IFLAGS)) == -1){	
		perror("error shmget\n");
		exit(1);
	}

	if((shm_mode = shmget(SHARED_KEY2, sizeof(int), 0600 | IFLAGS)) == -1){	
		perror("error shmget\n");
		exit(1);
	}
	if((shm_status = shmget(SHARED_KEY3, sizeof(struct databuf), 0600 | IFLAGS)) == -1){ 
		perror("error shmget\n");
		exit(1);
	}

	/* attach shared mem to process */
	if((*p1 = (int*)shmat(shm_key, 0, 0))==ERR_INT){
		perror("error shmget\n");
		exit(1);	
	}
	if((*p2 = (int*)shmat(shm_mode, 0, 0))==ERR_INT){
		perror("error shmget\n");
		exit(1);
	}

	if((*p3 = (struct databuf*)shmat(shm_status, 0, 0))==ERR){
		perror("error shmget\n");
		exit(1);
	}

	puts("getseg!!");
}

int getsem(){
	semun x;
	x.val = 0;
	int id = -1;

	puts("start getsem!!");
	if ((id = semget (SEM_KEY, 3, IPC_CREAT)) == -1){
		puts("getsem1!!");
		exit(1);	
	} 

	if (semctl (id, 0, SETVAL, x) == -1){	
		puts("getsem2!!");
		exit(1);
	}

	if (semctl (id, 1, SETVAL, x) == -1){			
		puts("getsem3!!");
		exit(1);	
	}

	if (semctl (id, 2, SETVAL, x) == -1){				
		puts("getsem4!!");
		exit(1);
	}

	puts("end getsem!!");
	return (id);
}

void remobj(){

	/* remove shm */
	if (shmctl(shm_key, IPC_RMID, 0) == -1) exit(1);
	if (shmctl(shm_mode, IPC_RMID, 0) == -1) exit(1);
	if (shmctl(shm_status, IPC_RMID, 0) == -1) exit(1);

	/* remove semaphore */
	if (semctl(sem_id, 0, IPC_RMID, 0) == -1) exit(1);
}


void proc_in(int semid, int *buf_in){
	/* input process */

	struct input_event ev[BUFF_SIZE];
    int fd_key, rd, value, size = sizeof(struct input_event);
	
	unsigned char push_sw_buff[MAX_BUTTON];
	int i, fd_sw, size_sw;

	char *dev_key = "/dev/input/event0";
	char *dev_sw = "/dev/fpga_push_switch";
    if ((fd_key = open(dev_key, O_RDONLY | O_NONBLOCK)) == -1) {
        printf("%s is not a vaild device \\n", dev_key);
    }
	if ((fd_sw = open(dev_sw, O_RDWR ))== -1) {
		printf("%s is not a vaild device \n", dev_sw);
		close(fd_sw);
	}

	size_sw = sizeof(push_sw_buff);
	while(1){
		int flag= 0;
		usleep(350000);
		if((rd = read(fd_key, ev, size * BUFF_SIZE)) >= size){
			value = ev[0].value;
			if(value == KEY_PRESS){
				switch(ev[0].code){
					case KEY_VOL_UP:
					case KEY_VOL_DOWN:
					case KEY_BACK:
						flag  = 1;
					//	printf("READ code: %d\n",ev[0].code);
						*buf_in = ev[0].code;
						break;
				}
			}
		}
		else{
			read(fd_sw, &push_sw_buff, size_sw);
			for( i=0; i<MAX_BUTTON;i++){
				if(push_sw_buff[i] == 1){
				//	printf("READ switch: %d\n",push_sw_buff[i]);
					*buf_in = i;
					flag = 1;
					break;
				} 
			}
		}
		if(flag == 1){
			semop(semid, &v1, 1);
			semop(semid, &p3, 1);
			flag = 0;
		}


	}
}


void proc_main(int semid, int *buf_in, int *buf_mode, struct databuf *buf_status){
	/* main process */

	while(1){
		semop(semid, &p1, 1);

		printf("main! %d", *buf_in);
		
		switch(*buf_in){
			case KEY_VOL_DOWN:
				*buf_mode = (*buf_mode>1)? *buf_mode-1 : 4;
				break;
			case KEY_VOL_UP:
				*buf_mode = (*buf_mode<4)? *buf_mode+1 : 1;
				break;
			case KEY_BACK:
				*buf_mode = 0;
			default:
				printf("main switch: %d\n",*buf_in);
		}

		char tmp[4] = "out\0";
		strcpy(buf_status->d_buf, tmp);
		buf_status->d_nread = strlen(tmp);

		semop(semid, &v2,1);
	}	
}

void proc_out(int semid, int *buf_mode, struct databuf *buf_status){
	/* output process */

	while(1){
		semop(semid, &p2, 1);
	
		printf("Out %d", *buf_mode);
		write(1, buf_status->d_buf, buf_status->d_nread);
		printf("\n");
		semop(semid, &v3,1);
	}
}

int main (int argc, char *argv[])
{

	pid_t pid_in =0 , pid_out=0;
	struct databuf *buf_status;
	int *buf_in, *buf_mode;
	puts("START!");

	sem_id = getsem(); //creat and init semaphore
	getseg (&buf_in, &buf_mode, &buf_status);	//create and attach shared mem 
	
	/* create child processes */
	switch(pid_in = fork()){
		case -1:
			perror("ERROR: fork()");
			break;
		case 0:
			printf("input process\n");
			proc_in(sem_id, buf_in); 	//in -> main
			remobj();
			break;
		default:
			switch(pid_out = fork()){
				case -1:
					perror("ERROR: fork()");
					break;
				case 0:
					printf("output process\n");
					proc_out(sem_id, buf_mode, buf_status); // main -> out
					remobj();
					break;
				default:
					proc_main(sem_id, buf_in, buf_mode, buf_status); // in -> main -> out
					break;
			}
			break;
	}


	return 0;
}


