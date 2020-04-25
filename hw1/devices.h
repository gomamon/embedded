#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16 


#define MAX_DIGIT 4
#define FND_DEVICE "/dev/fpga_fnd"


void led(unsigned char data);
void fnd(char *str_data);