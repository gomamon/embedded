#include "main.h"
#include "clock.h"
#include "counter.h"
#include "devices.h"
#include "textEditor.h"

struct sembuf p1 = {0, -1, SEM_UNDO }, p2 = {1, -1, SEM_UNDO}, p3 = {2, -1, SEM_UNDO};
struct sembuf v1 = {0, 1, SEM_UNDO }, v2 = {1, 1, SEM_UNDO }, v3 = {2,1,SEM_UNDO};
static int shm1, shm2, shm3, sem_id;

void getseg(int **sh1, int **sh2, struct outbuf** sh3){ // init
	/*create shared mem*/
	if((shm1 = shmget(SHARED_KEY1, sizeof(int), 0600 | IFLAGS)) == -1){	
		perror("error shmget\n");
		exit(1);
	}

	if((shm2 = shmget(SHARED_KEY2, sizeof(int), 0600 | IFLAGS)) == -1){	
		perror("error shmget\n");
		exit(1);
	}
	if((shm3 = shmget(SHARED_KEY3, sizeof(struct outbuf), 0600 | IFLAGS)) == -1){ 
		perror("error shmget3\n");
		exit(1);
	}
	/* attach shared mem to process */
	if((*sh1 = (int*)shmat(shm1, 0, 0))==ERR_INT){
		perror("error shmget\n");
		exit(1);	
	}
	if((*sh2 = (int*)shmat(shm2, 0, 0))==ERR_INT){
		perror("error shmget\n");
		exit(1);
	}

	if((*sh3 = (struct outbuf*)shmat(shm3, 0, 0))==ERR_OUTBUF){
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
	if (shmctl(shm1, IPC_RMID, 0) == -1) exit(1);
	if (shmctl(shm2, IPC_RMID, 0) == -1) exit(1);
	if (shmctl(shm3, IPC_RMID, 0) == -1) exit(1);
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
	int flag = 0;
	size_sw = sizeof(push_sw_buff);
	while(1){
		//printf("read %d\n", *buf_in);
		usleep(350000);
		if((rd = read(fd_key, ev, size * BUFF_SIZE)) >= size){
			value = ev[0].value;
			if(value == KEY_PRESS){
				switch(ev[0].code){
					case KEY_VOL_UP:
					case KEY_VOL_DOWN:
					case KEY_BACK:
					//	printf("READ code: %d\n",ev[0].code);
						*buf_in = (ev[0].code) * 10;
						flag = 1;
						break;
				}
			}
		}
		else{
			read(fd_sw, &push_sw_buff, size_sw);
			int btn=0;
			for( i=0; i<MAX_BUTTON;i++){
				btn <<= 1;
				if(push_sw_buff[i] == 1){
					btn += 1;
					flag= 1;
				} 
			}
			*buf_in = btn;
		}
		if(flag == 1){
			printf("wait %d\n", *buf_in);
			semop(semid, &p1, 1);
			printf("release! %d\n", *buf_in);
			flag = 0;
		}
	}
}


void proc_main(int semid, int *buf_in, int *buf_mode, struct outbuf *buf_out){
	/* main process */
	int sw = SW1;
	int inflag = 0;
	*buf_in = 0;

	/*init clock data*/
	struct clock_data clock_info;
	clock_info.clock_h =0;
	clock_info.clock_m =0;
	clock_info.clock_mode = 0;

	/*init counter data*/
	struct counter_data counter_info;
	counter_info.number = 0;
	counter_info.system = 10;

	/*init text editor data*/
	struct text_editor_data text_editor_info;
	text_editor_info.cnt = 0;
	text_editor_info.mode = TEXT_ENG_MODE;
	memset(text_editor_info.text , 0, 9);
	text_editor_info.idx = 0;

	while(1){
		inflag = 1;
		
		// printf("main! %d", *buf_in);
		switch(*buf_in){
			case (KEY_VOL_DOWN*10):
				*buf_mode = (*buf_mode>1)? *buf_mode-1 : 4;
				sw = 1<<9;
				break;
			case (KEY_VOL_UP*10):
				*buf_mode = (*buf_mode<4)? *buf_mode+1 : 1;
				sw = 1<<9;
				break;
			case (KEY_BACK*10):
				*buf_mode = 0;
				sw = 1<<9;
				break;
			case 0 :
				inflag = 0;
				break;
            default:
                sw = *buf_in;
                printf("sw: %d\n", sw);
                break;
        }
		printf("mode : %d\n",*buf_mode);
        switch (*buf_mode) {
            case 1:
                mode_clock(sw, inflag, &(clock_info), buf_out);
                break;
            case 2:
				mode_counter(sw, inflag, &(counter_info), buf_out);
				break;
			case 3:
				mode_text_editor(sw,inflag, &(text_editor_info), buf_out);
				break;
            case 4:
                break;
        }

		char tmp[4] = "out\0";

		*buf_in = 0;
		semop(semid, &p2,1);
		if(inflag == 1){
			semop(semid, &v1, 1);
		}

	}	
}

void proc_out(int semid, int *buf_mode, struct outbuf* buf_out){
	/* output process */
	char *tmp;
	while(1){
		char nullstr[16];
		memset(nullstr, 0, 16);
		
		switch(*buf_mode){
			case 1:
				printf("CLOCK OUT: %d %s\n",buf_out->led, buf_out->fnd);
				led(buf_out->led);
				fnd(buf_out->fnd);
				dot_matrix(buf_out->dot_matrix);
				text_lcd(buf_out->text_lcd);
				break;

			case 2:
				printf("CLOCK OUT: %d %s\n",buf_out->led, buf_out->fnd);
				led(buf_out->led);
				fnd(buf_out->fnd);
				dot_matrix(buf_out->dot_matrix);
				text_lcd(buf_out->text_lcd);
				break;
			case 3:
				led(buf_out->led);
				fnd(buf_out->fnd);
				dot_matrix(buf_out->dot_matrix);
				text_lcd(buf_out->text_lcd);
				break;
			default:
				break;
		}
		
		semop(semid, &v2, 1);
		sleep(1);
	}
}

init_buf(int *buf_in, int *buf_mode, struct outbuf* buf_out){
	*buf_mode = 1;
	*buf_in = 0;
	memset(buf_out->text_lcd, 0, MAX_TEXT_LCD); 
}	

int main (int argc, char *argv[])
{

	pid_t pid_in =0 , pid_out=0;
	struct outbuf *buf_out;
	int *buf_in, *buf_mode;
	puts("START!");

	sem_id = getsem(); //creat and init semaphore
	getseg (&buf_in, &buf_mode, &buf_out);	//create and attach shared mem 
	init_buf(buf_in, buf_mode, buf_out);
	device_open();
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
					proc_out(sem_id, buf_mode, buf_out); // main -> out
					remobj();
					break;
				default:
					proc_main(sem_id, buf_in, buf_mode, buf_out); // in -> main -> out
					break;
			}
			break;
	}


	return 0;
}


