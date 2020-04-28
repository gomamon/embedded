#include "main.h"
#include "devices.h"

int dev_fnd;
int dev_text_lcd;
int dev_dot;
int fd_led;
extern int fd_key, fd_sw;

void device_open(){

    if ((fd_key = open(DEV_KEY, O_RDONLY | O_NONBLOCK)) == -1) {
        printf("/dev/input/event0 is not a vaild device \n");
		exit(1);
    }
	if ((fd_sw = open(DEV_SW, O_RDWR ))== -1) {
		printf("/dev/fpga_push_switch is not a vaild device \n");
		close(fd_sw);
		exit(1);
	}

	fd_led = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd_led < 0) {
		perror("/dev/mem open error");
		exit(1);
	}

	dev_fnd = open(FND_DEVICE, O_RDWR);
	if(dev_fnd<0){
		printf("Devices open error : %s\n",FND_DEVICE);
	}

	/*text_dot_matrix*/
	dev_dot = open(FPGA_DOT_DEVICE, O_WRONLY);
	if (dev_dot<0) {
		printf("Device open error : %s\n",FPGA_DOT_DEVICE);
		exit(1);
	}

	/*text_lcd_open*/
	dev_text_lcd = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);
	if ( dev_text_lcd<0) {
		printf("Device open error : %s\n",FPGA_TEXT_LCD_DEVICE);
		exit(1);
	}
}


void led(unsigned char data)
{
	int i;

	unsigned long *fpga_addr = 0;
	unsigned char *led_addr =0;


	if( (data<0) || (data>255) ){
		printf("Invalid range!\n");
		exit(1);
	}



	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd_led, FPGA_BASE_ADDRESS);
	if (fpga_addr == MAP_FAILED)
	{
		printf("mmap error!\n");
		close(fd_led);
		exit(1);
	}
	
	led_addr=(unsigned char*)((void*)fpga_addr+LED_ADDR);
	
	*led_addr=data; //write led
	
	//for read
	data=0;
	data=*led_addr; //read led

	munmap(led_addr, 4096);
	return;
}


void fnd(char *str_data)
{
	unsigned char retval;
	unsigned char data[4];
	int i;
	int str_size;

    for(i=0;i<4;i++)
    {
        data[i]=str_data[i]-0x30;
	}


    retval=write(dev_fnd,&data,4);	
    if(retval<0) {
        printf("Write Error!\n");
        exit(1);
    }
	memset(data,0,sizeof(data));

	return;
}


void dot_matrix(unsigned char* data)
{
	int i;

	printf("dot: %s\n",data);

	write(dev_dot,data,10);
	
	return;
}



/*text lcd*/

void text_lcd(unsigned char *str_data)
{
	int i;
	int str_size;
	int chk_size;
	unsigned char string[32];
	int tok_size = 32;

//	printf("text_lcd: %s\n",str_data);
	for(i=0; i<32; i++){
		if(str_data[i] == '\0' || str_data[i] == 0 ){
			tok_size = i;
			break;
		}
		string[i] = str_data[i];
	}
//	printf("tok_size %d\n",tok_size);
	if(tok_size>=0){
		//strncat(string, str_data, tok_size);
		memset(string+tok_size, ' ', 32-tok_size);
	}

//	printf("STRING:%s\n",string);
	write( dev_text_lcd,string,MAX_BUFF);		
	
	return;
}
