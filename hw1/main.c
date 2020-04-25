#include <unistd.h> // for fork

#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
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
#define SEM_KEY (key_t) 0x20 //semaphore key
#define IFLAGS (IPC_CREAT)
#define ERR ((struct databuf *)-1)
#define SIZE 2048


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

static int shm_id1, shm_id2, sem_id;

void getseg(struct databuf **p1, struct databuf **p2){ // init
	/*create shared mem*/
	if((shm_id1 = shmget(SHARED_KEY1, sizeof(struct databuf), 0600 | IFLAGS)) == -1){	
		perror("error shmget\n");
		exit(1);
	}

	if((shm_id2 = shmget(SHARED_KEY2, sizeof(struct databuf), 0600 | IFLAGS)) == -1){	
		perror("error shmget\n");
		exit(1);
	}
	// if((shm_id3 = shmget(SHARED_KEY3, sizeof(struct databuf), 0600 | IFLAGS)) == -1){ 
	// 	perror("error shmget\n");
	// 	exit(1);
	// }

	/* attach shared mem to process */
	if((*p1 = (struct databuf*)shmat(shm_id1, 0, 0))==ERR){
		perror("error shmget\n");
		exit(1);	
	}
	if((*p2 = (struct databuf*)shmat(shm_id2, 0, 0))==ERR){
		perror("error shmget\n");
		exit(1);
	}

	// if((*p3 = (struct databuf*)shmat(shm_id3, 0, 0))==ERR){
	// 	perror("error shmget\n");
	// 	exit(1);
	// }

	puts("getseg!!");
}

int getsem(){
	semun x;
	x.val = 0;
	int id = -1;

	puts("start getsem!!");
	if ((id = semget (SEM_KEY, 3, IPC_CREAT)) == -1){
		puts("getsem1!!");
		exit(1);	//?
	} 

	if (semctl (id, 0, SETVAL, x) == -1){					// set semaphore
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
	if (shmctl(shm_id1, IPC_RMID, 0) == -1) exit(1);
	if (shmctl(shm_id2, IPC_RMID, 0) == -1) exit(1);

	/* remove semaphore */
	if (semctl(sem_id, 0, IPC_RMID, 0) == -1) exit(1);
}


void proc_in(int semid, struct databuf *buf1){
	/* input process */
	while(1){
		printf("read!\n");
		buf1 -> d_nread = read(0, buf1 -> d_buf, SIZE); 
		semop(semid, &v1, 1);
		semop(semid, &p3, 1);

		if (buf1 -> d_nread <= 0) return;

	}
}


void proc_main(int semid, struct databuf *buf1, struct databuf *buf2){
	/* main process */

	while(1){
		semop(semid, &p1, 1);
		semop(semid, &v2,1);
		if(buf1 -> d_nread <= 0)
			return;
		printf("main!");
		write(1, buf1->d_buf, buf1->d_nread);
		printf("\n");
		char tmp[4] = "ddo\0";
		strcpy(buf2->d_buf, tmp);
		buf2->d_nread = strlen(tmp);
	}
}

void proc_out(int semid, struct databuf *buf2){
	/* output process */

	while(1){
		semop(semid, &p2, 1);
		semop(semid, &v3,1);
		write(1, buf2->d_buf, buf2->d_nread);
	}
}

int main (int argc, char *argv[])
{

	pid_t pid_in =0 , pid_out=0;
	struct databuf *buf1, *buf2;

	puts("START!");

	sem_id = getsem(); //creat and init semaphore
	getseg (&buf1, &buf2);	//create and attach shared mem 
	
	/* create child processes */
	switch(pid_in = fork()){
		case -1:
			perror("ERROR: fork()");
			break;
		case 0:
			printf("input process\n");
			proc_in(sem_id, buf1); 	//in -> main
			remobj();
			break;
		default:
			switch(pid_out = fork()){
				case -1:
					perror("ERROR: fork()");
					break;
				case 0:
					printf("output process\n");
					proc_out(sem_id, buf2); // main -> out
					remobj();
					break;
				default:
					proc_main(sem_id, buf1, buf2); // in -> main -> out
					break;
			}
			break;
	}


	return 0;
}


