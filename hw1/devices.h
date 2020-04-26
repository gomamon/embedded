#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>


//fpga led mmap
#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16 

//fpga fnd
#define MAX_DIGIT 4
#define FND_DEVICE "/dev/fpga_fnd"

/*fpga dot font*/





#define FPGA_DOT_DEVICE "/dev/fpga_dot"



/*fpga text led*/

#define MAX_BUFF 32
#define LINE_BUFF 16
#define FPGA_TEXT_LCD_DEVICE "/dev/fpga_text_lcd"



void led(unsigned char data);
void fnd(char *str_data);
void dot_matrix(int data);
void text_lcd(char *data);



