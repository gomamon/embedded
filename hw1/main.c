#include "main.h"
#include "clock.h"
#include "counter.h"
#include "devices.h"
#include "textEditor.h"
#include "drawBoard.h"
struct sembuf p1 = {0, -1, SEM_UNDO }, p2 = {1, -1, SEM_UNDO}, p3 = {2, -1, SEM_UNDO};
struct sembuf v1 = {0, 1, SEM_UNDO }, v2 = {1, 1, SEM_UNDO }, v3 = {2,1,SEM_UNDO};
static int shm1, shm2, sem_id;

extern int dev_key, dev_sw;

void getseg(int **sh1, struct outbuf** sh2){ // init
	/*create shared mem*/
	if((shm1 = shmget(SHARED_KEY1, sizeof(int), 0600 | IFLAGS)) == -1){	
		perror("error shmget\n");
		exit(1);
	}
	if((shm2 = shmget(SHARED_KEY2, sizeof(struct outbuf), 0600 | IFLAGS)) == -1){ 
		perror("error shmget3\n");
		exit(1);
	}
	/* attach shared mem to process */
	if((*sh1 = (int*)shmat(shm1, 0, 0))==ERR_INT){
		perror("error shmget\n");
		exit(1);	
	}
	if((*sh2 = (struct outbuf*)shmat(shm2, 0, 0))==ERR_OUTBUF){
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

	/* remove semaphore */
	if (semctl(sem_id, 0, IPC_RMID, 0) == -1) exit(1);
}


void init_buf(int *buf_in, struct outbuf* buf_out){
	*buf_in = 0;
	memset(buf_out->text_lcd, 0, MAX_TEXT_LCD); 
}	

void proc_in(int semid, int *buf_in){
	/* input process */

	struct input_event ev[BUFF_SIZE];
    int rd, value, size = sizeof(struct input_event);
	
	unsigned char push_sw_buff[MAX_BUTTON];
	int i, size_sw;
	int flag = 0;

	size_sw = sizeof(push_sw_buff);
	while(1){
		usleep(250000);
		if((rd = read(dev_key, ev, size * BUFF_SIZE)) >= size){
			value = ev[0].value;
			if(value == KEY_PRESS){
				switch(ev[0].code){
					case KEY_VOL_UP:
					case KEY_VOL_DOWN:
					case KEY_BACK:
						*buf_in = (ev[0].code) * 10;
						flag = 1;
						break;
				}
			}
		}
		else{
			read(dev_sw, &push_sw_buff, size_sw);
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


void proc_main(int semid, int *buf_in, struct outbuf *buf_out){
	/* main process */


	int sw = SW1;
	int inflag = 0;
	*buf_in = 0;
	int mode = 1;

	struct clock_data clock_info;
	struct counter_data counter_info;
	struct text_editor_data text_editor_info;
	struct draw_board_data draw_board_info;

	/*init clock data*/
	init_clock_info(&clock_info);

	/*init counter data*/
	init_counter_info(&counter_info);

	/*init text editor data*/
	init_text_editor_info(&text_editor_info);

	/*init draw board data*/
    init_draw_board_info(&draw_board_info);

	while(1){
		inflag = 1;
		
		// printf("main! %d", *buf_in);
		switch(*buf_in){
			case (KEY_VOL_DOWN*10):
				mode = (mode>1)? mode-1 : 4;
				sw = 1<<9;
				break;
			case (KEY_VOL_UP*10):
				mode = (mode<4)? mode+1 : 1;
				sw = 1<<9;
				break;
			case (KEY_BACK*10):
				return;
				break;
			case 0 :
				inflag = 0;
				break;
            default:
                sw = *buf_in;
                printf("sw: %d\n", sw);
                break;
        }
		printf("mode : %d\n",mode);
        switch (mode) {
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
				mode_draw_board(sw,inflag,&(draw_board_info), buf_out);
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

void proc_out(int semid, struct outbuf* buf_out){
	/* output process */
	char *tmp;
	while(1){

		led(buf_out->led);
		fnd(buf_out->fnd);
		dot_matrix(buf_out->dot_matrix);
		text_lcd(buf_out->text_lcd);
		
		semop(semid, &v2, 1);
		usleep(10000);
		
	}
}


int main ()
{

	pid_t pid_in =0 , pid_out=0;
	struct outbuf *buf_out;
	int *buf_in, *buf_mode;

	sem_id = getsem(); //creat and init semaphore
	getseg (&buf_in, &buf_out);	//create and attach shared mem 
	init_buf(buf_in, buf_out);
	device_open();

	/* create child processes */
	switch(pid_in = fork()){
		case -1:
			perror("ERROR: fork()");
			break;
		case 0:
			/* input process */
			proc_in(sem_id, buf_in); 
			remobj();
			break;
		default:
			switch(pid_out = fork()){
				case -1:
					perror("ERROR: fork()");
					break;
				case 0:
					/* output process */
					proc_out(sem_id, buf_out); 
					remobj();
					break;
				default:
					/* main process */
					proc_main(sem_id, buf_in, buf_out);

					/* kill chid processes, remove semaphore and close devices  */
					kill(pid_in, SIGKILL);
					wait(NULL);
					kill(pid_out,SIGKILL);
					wait(NULL);
					remobj();
					device_close();
					break;
			}
			break;
	}

	return 0;
}


