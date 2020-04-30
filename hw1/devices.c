#include "main.h"
#include "devices.h"

int dev_fnd;
int dev_text_lcd;
int dev_dot;
int dev_led;
extern int dev_key, dev_sw;

void device_open(){
	/* open device and get discription about device*/

	/* input key */
    if ((dev_key = open(DEV_KEY, O_RDONLY | O_NONBLOCK)) == -1) {
        perror("/dev/input/event0 is not a vaild device \n");
		exit(1);
    }

	/* switch */
	if ((dev_sw = open(DEV_SW, O_RDWR ))== -1) {
		perror("/dev/fpga_push_switch is not a vaild device \n");
		exit(1);
	}

	/* led mmap */
	dev_led = open("/dev/mem", O_RDWR | O_SYNC);
	if (dev_led < 0) {
		perror("/dev/mem open error");
		exit(1);
	}

	/* fnd */
	dev_fnd = open(FND_DEVICE, O_RDWR);
	if(dev_fnd<0){
		perror("Devices open error : fnd\n");
	}

	/* dot_matrix */
	dev_dot = open(FPGA_DOT_DEVICE, O_WRONLY);
	if (dev_dot<0) {
		perror("Device open error : dot device\n");
		exit(1);
	}

	/*text lcd*/
	dev_text_lcd = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);
	if ( dev_text_lcd<0) {
		perror("Device open error : text lcd device\n");
		exit(1);
	}
}

void device_close(){
	int i;
	unsigned char empty_fnd_data[MAX_FND];
	unsigned char empty_dot_matrix_data[MAX_DOT_MATRIX];
	unsigned char emtpy_text_lcd_data[MAX_TEXT_LCD];
	
	/*create empty string by device*/
	sprintf(empty_fnd_data, "0000");
	for(i=0; i<MAX_DOT_MATRIX; i++)	empty_dot_matrix_data[i] = 0x00;
	sprintf(emtpy_text_lcd_data, "");

	/* reset device */
	led(0);
	fnd(empty_fnd_data);
	dot_matrix(empty_dot_matrix_data);
	text_lcd(emtpy_text_lcd_data);


	/*close all devices*/
	close(dev_key);
	close(dev_sw);
	close(dev_led);
	close(dev_fnd);
	close(dev_dot);
	close(dev_text_lcd);
}

/* fpga led mmap*/
void led(unsigned char data)
{
	int i;
	unsigned long *fpga_addr = 0;
	unsigned char *led_addr =0;

	//get fpga address 
	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, dev_led, FPGA_BASE_ADDRESS);
	if (fpga_addr == MAP_FAILED)
	{
		perror("mmap error!\n");
		close(dev_led);
		exit(1);
	}
	
	//get led address
	led_addr=(unsigned char*)((void*)fpga_addr+LED_ADDR);
	
	//intput data to led address to turn on the led
	*led_addr=data; 
	data=0;
	data=*led_addr; 

	munmap(led_addr, 4096);
	return;
}

/* fpga fnd*/
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

	//show data using fnd device driver
    retval=write(dev_fnd,&data,4);	
    if(retval<0) {
        printf("Write Error!\n");
        exit(1);
    }
	memset(data,0,sizeof(data));

	return;
}

/* fpga dot matrix*/
void dot_matrix(unsigned char* data)
{
	int i;
	//show data using dot matrix device driver
	write(dev_dot,data,10);
	
	return;
}



/* fpga text lcd*/
void text_lcd(unsigned char *str_data)
{
	int i;
	int str_size;
	int chk_size;
	unsigned char string[32];
	int tok_size = 32;


	for(i=0; i<32; i++){
		if(str_data[i] == '\0' || str_data[i] == 0 ){
			tok_size = i;
			break;
		}
		string[i] = str_data[i];
	}

	// make remain text empty
	if(tok_size>=0){
		memset(string+tok_size, ' ', 32-tok_size);
	}

	// show text using text lcd driver
	write( dev_text_lcd,string,MAX_BUFF);		
	return;
}
