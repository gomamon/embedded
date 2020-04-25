#include "main.h"
#include "devices.h"

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
	
	sleep(1);
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


	memset(data,0,sizeof(data));

    for(i=0;i<str_size;i++)
    {
        data[i]=str_data[i]-0x30;
    }


    dev = open(FND_DEVICE, O_RDWR);
    if (dev<0) {
        printf("Device open error : %s\n",FND_DEVICE);
        exit(1);
    }


    retval=write(dev,&data,4);	
    if(retval<0) {
        printf("Write Error!\n");
        return;
    }

	memset(data,0,sizeof(data));

	sleep(1);

	retval=read(dev,&data,4);
	if(retval<0) {
		printf("Read Error!\n");
		return;
	}

	close(dev);

	return;
}
