#include "main.h"
#include "devices.h"



unsigned char fpga_number[10][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};

unsigned char fpga_set_full[10] = {
	// memset(array,0x7e,sizeof(array));
	0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f
};

unsigned char fpga_set_blank[10] = {
	// memset(array,0x00,sizeof(array));
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};


void led(unsigned char data)
{
	int fd,i;

	unsigned long *fpga_addr = 0;
	unsigned char *led_addr =0;


	if( (data<0) || (data>255) ){
		printf("Invalid range!\n");
		exit(1);
	}

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("/dev/mem open error");
		exit(1);
	}

	fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPGA_BASE_ADDRESS);
	if (fpga_addr == MAP_FAILED)
	{
		printf("mmap error!\n");
		close(fd);
		exit(1);
	}
	
	led_addr=(unsigned char*)((void*)fpga_addr+LED_ADDR);
	
	*led_addr=data; //write led
	
	//for read
	data=0;
	data=*led_addr; //read led

	munmap(led_addr, 4096);
	close(fd);
	return;
}


void fnd(char *str_data)
{
	int dev;
	unsigned char retval;
	unsigned char data[4];
	int i;
	int str_size;

	//printf("fnd:%s\n",str_data);

    for(i=0;i<4;i++)
    {
        data[i]=str_data[i]-0x30;
		// printf("%d\n",data[i]);
	}

    dev = open(FND_DEVICE, O_RDWR);
    if (dev<0) {
        printf("Device open error : %s\n",FND_DEVICE);
        exit(1);
    }


    retval=write(dev,&data,4);	
    if(retval<0) {
        printf("Write Error!\n");
        exit(1);
    }
	memset(data,0,sizeof(data));

	retval=read(dev,&data,4);
	if(retval<0) {
		printf("Read Error!\n");
		return;
	}

	close(dev);

	return;
}





//#include "./fpga_dot_font.h"
//#define FPGA_DOT_DEVICE "/dev/fpga_dot"

void dot_matrix(int data)
{
	int i;
	int dev;
	int str_size;
	int set_num;
	
	printf("dot: %d\n",data);
	// if(argc!=2) {
	// 	printf("please input the parameter! \n");
	// 	printf("ex)./fpga_dot_test 7\n");
	// 	return -1;
	// }

	// set_num = atoi(argv[1]);
	// if(set_num<0||set_num>9) {
	// 	printf("Invalid Numner (0~9) !\n");
	// 	return -1;
	// }

	dev = open(FPGA_DOT_DEVICE, O_WRONLY);
	if (dev<0) {
		printf("Device open error : %s\n",FPGA_DOT_DEVICE);
		exit(1);
	}
	
	str_size=sizeof(fpga_number[data]);

    /*
	write(dev,fpga_set_full,sizeof(fpga_set_full));
	sleep(1);

	write(dev,fpga_set_blank,sizeof(fpga_set_blank));
	sleep(1);
    */

	write(dev,fpga_number[data],str_size);	


	close(dev);
	
	return;
}



/*text lcd*/

void text_lcd(char *str_data)
{
	int i;
	int dev;
	int str_size;
	int chk_size;
	// char *data[2] = {"",""};
	unsigned char string[32];
	int tok_size = strlen(str_data);

	printf("text_lcd: %s\n",str_data);
	memset(string,0,sizeof(string));	
		
	for(i=0; i<sizeof(str_data); i++){
		if(str_data[i] == '\n');
			tok_size = i;
	}

	dev = open(FPGA_TEXT_LCD_DEVICE, O_WRONLY);
	if (dev<0) {
		printf("Device open error : %s\n",FPGA_TEXT_LCD_DEVICE);
		exit(1);
	}

	// str_size=strlen(data[0]);
	// if(str_size>0) {
	// 	strncat(string,data[0],str_size);
	// 	memset(string+str_size,' ',LINE_BUFF-str_size);
	// }
	if(tok_size>0){
		strncat(string, str_data, tok_size);
		memset(string+tok_size, ' ', LINE_BUFF-tok_size);
	}
	
	if(tok_size<strlen(str_data)){
		int tok2_idx = tok_size+2;
		int tok2_size = strlen(str_data+tok2_idx);		
		strncat(string,str_data+tok2_idx, tok2_size);
		memset(string+LINE_BUFF+tok2_size, ' ', LINE_BUFF-tok2_size);
	}
	printf("string: %s\n",string);
	// str_size=strlen(data[1]);
	// if(str_size>0) {	
	// 	strncat(string,data[1],str_size);
	// 	memset(string+LINE_BUFF+str_size,' ',LINE_BUFF-str_size);
	// }

	write(dev,string,MAX_BUFF);		
	close(dev);
	
	return;
}
