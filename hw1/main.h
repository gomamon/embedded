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
#include <time.h>
#include <unistd.h> // for fork

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>	//message queue

#include <sys/shm.h>	//shared mem
#include <sys/sem.h>	//semaphore

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1


#define SHARED_KEY1 (key_t) 0x10
#define SHARED_KEY2 (key_t) 0x11
#define SEM_KEY (key_t) 0x20 //semaphore key
#define IFLAGS (IPC_CREAT)
#define ERR_OUTBUF ((struct outbuf *)-1)
#define ERR_INT ((int *)-1)
#define SIZE 2048

#define KEY_VOL_DOWN 114
#define KEY_VOL_UP 115
#define KEY_BACK 158

#define SW1 256
#define SW2 128
#define SW3 64 
#define SW4 32
#define SW5 16
#define SW6 8
#define SW7 4
#define SW8 2
#define SW9 1
#define KEY_VOL 1<<9

#define MAX_BUTTON 9
#define MODE_NUM 4

#define MAX_FND 5
#define MAX_DOT_MATRIX 10
#define MAX_TEXT_LCD 33

typedef union {
	int val;
	struct semid_ds *buf;
	unsigned short * array;
} semun;

struct outbuf{
	int led;
	unsigned char fnd[MAX_FND];
	unsigned char dot_matrix[MAX_DOT_MATRIX];
	unsigned char text_lcd[MAX_TEXT_LCD];
};
