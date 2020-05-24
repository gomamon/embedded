#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <asm/ioctl.h>
#include <asm/io.h>
//#include "device_driver.h"
#include "fpga_dot_font.h"

#define DEVICE_DRIVER_NAME "dev_driver"
#define MAJOR_NUM 242
#define BUF_LEN 50

#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 0, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 1)

#define PHY_LED 0x08000016
#define PHY_FND 0x08000004
#define PHY_DOT 0x08000210
#define PHY_TEXT_LCD 0x08000090


#define COMMAND_SIZE 12

static dev_driver_usage = 0;

long number = 0;
int major_number;
char *numchar;

int timer_flag = 0;
int count = 0;

int dev_driver_open(struct inode *,struct file *);
int dev_driver_release(struct inode *, struct file *);
long dev_driver_ioctl(struct file *, unsigned int, unsigned long);
static void timer_setup();
static void timer_handler(unsigned long);

unsigned char text1[17] = "20161622";
unsigned char text2[17] = "YEEUN LEE";
int text1_pos = 0, text2_pos = 0;

static struct file_operations fops = {
	.open = dev_driver_open,
	.release = dev_driver_release,
	.unlocked_ioctl = dev_driver_ioctl
};

static struct timer_data{
	int interval;
	int init;
	int init_pos;
	int count;
	
	struct timer_list timer;
};

static struct fpga_virtual_address{
	unsigned char *led;
	unsigned char *fnd;
	unsigned char *dot;
	unsigned char *text_lcd;
} fpga_addr;

struct timer_data mydata;


static void iomap_fpga(){
	fpga_addr.led = ioremap(PHY_LED, 0x1);
	fpga_addr.fnd = ioremap(PHY_FND, 0x4);
	fpga_addr.dot = ioremap(PHY_DOT,0x10);
	fpga_addr.text_lcd = ioremap(PHY_TEXT_LCD,0x32);
}

static void iounmap_fpga(){
	iounmap(fpga_addr.led);
	iounmap(fpga_addr.fnd);
	iounmap(fpga_addr.dot);
	iounmap(fpga_addr.text_lcd);
}


long dev_driver_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){
	int i;
	char *temp;

	char buff[12] = {'\0',};
	printk("device_driver_ioctl\n");

	switch(ioctl_num){
		case IOCTL_SET_OPTION:
			temp = (char *)ioctl_param;
			
			mydata.interval = 0;
			mydata.count = 0;
			count =0;
			mydata.init = 0;

			for(i=0; i<COMMAND_SIZE; i++){
				get_user(buff[i], temp+i);
				if(buff[i]<'0'||buff[i]>'9') break;
				int tmp  = buff[i]-'0';
				if(i<3){
					mydata.interval *=10;
					mydata.interval += tmp;
				}
				else if(i<6){
					count *= 10;
					count += tmp;
				}
				else{
					if(tmp != 0){
						mydata.init_pos = i-6;
					 	mydata.init = tmp;
					}
				}
			}
			text1_pos=0;
			text2_pos=0;
			//printk("param int : %d %d %d\n",mydata.interval, mydata.count, mydata.init);
			break;
		case IOCTL_COMMAND:
			printk("IOCTL COMMAND");
			init_timer(&(mydata.timer));
			timer_setup();
			printk("TIMER ON\n");
			break;
		default:
			printk("IOCTL ERROR\n");
			return -1;
			break;
	}

}

static void timer_setup(){

	del_timer_sync(&mydata.timer);
	
	mydata.timer.expires = jiffies + (mydata.interval *HZ);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;

	add_timer(&mydata.timer);

	return;
}

int fpga_handler(struct timer_data *t_data){
	int i;
	
	unsigned short led_value = (unsigned short) (1<<(8-t_data->init));

	unsigned char fnd_str[4] = {0,};
	unsigned short int fnd_value;

	unsigned short int dot_value; 

	unsigned short int text_lcd_value = 0;
	unsigned char text_lcd_data[33] = {' ', };

	/* led write*/
	outw(led_value, (unsigned int)fpga_addr.led);

	/* fnd write */
	fnd_str[t_data->init_pos] = t_data->init;
	printk("fnd: %d %d %d %d",fnd_str[0],fnd_str[1],fnd_str[2],fnd_str[3]);
	fnd_value = fnd_str[0]<<12 | fnd_str[1]<<8 | fnd_str[2]<<4 | fnd_str[3];
	outw(fnd_value, (unsigned int)fpga_addr.fnd);


	/* dot write */
	for(i=0; i<10 ; i++){
		dot_value = fpga_number[t_data->init][i] & 0x7F;
		outw(dot_value, (unsigned int)fpga_addr.dot+i*2);
	}

	/* text lcd write*/
	for(i=0; i<32; i++)
	{
		if(i<16){
			if(i>=text1_pos){
				text_lcd_data[i] = text1[text1_pos+i];
			}
			else if(text1_pos+8 >16){
				text_lcd_data[i] 
			}
			else
				text_lcd_data[i] = ' ';
		}
		else{
			if(0<=(i-16)-text2_pos && (i-16)-text2_pos < 9)
				text_lcd_data[i] = text2[text2_pos+(i-16)];
			else
			
				text_lcd_data[i] = ' ';
		}
	}
	for(i=0;i<32;i++)
    {
        text_lcd_value = (text_lcd_data[i] & 0xFF) << 8 | text_lcd_data[i + 1] & 0xFF;
		outw(text_lcd_value,(unsigned int)fpga_addr.text_lcd+i);
        i++;
    }


	return 0;
}


static void timer_handler(unsigned long timeout){
	struct timer_data *t_data = (struct timer_data*)timeout;
	printk("timer handler %d\n", t_data->count);

	if(fpga_handler(t_data) == -1)
		return;

	t_data->count++;
	t_data->init++;
	if(t_data->init > 8) t_data->init = 1;
	
	if((t_data->count)%8 == 0)
		t_data->init_pos = 1 + t_data->init_pos;
	if(t_data->init_pos == 4) t_data->init_pos = 0;

	if(t_data -> count > count ){
		return ;
	}
	
	mydata.timer.expires = get_jiffies_64() + (mydata.interval * HZ);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;

	add_timer(&mydata.timer);

	return;
}

int dev_driver_open(struct inode *inode, struct file *file) {
	printk("device_driver_open\n");
	if (dev_driver_usage != 0) {
		return -EBUSY;
	}
	dev_driver_usage = 1;
	return 0;
}

int dev_driver_release(struct inode *inode, struct file *file) {
	printk("device_driver_release\n");
	dev_driver_usage = 0;
	return 0;
}


int __init dev_driver_init(void){
	printk("device_driver_init\n");

	major_number = register_chrdev(MAJOR_NUM, DEVICE_DRIVER_NAME, &fops);
	if(major_number < 0){
		printk( "error %d\n", major_number);
		return major_number;
	}
	
	major_number = MAJOR_NUM;
	iomap_fpga();
	printk("dev_file: /dev/%s , major: %d\n", DEVICE_DRIVER_NAME, major_number);

	printk("init module\n");
	return 0;
}

void __exit dev_driver_exit(void){
	printk("device_driver_exit\n");
	dev_driver_usage = 0;
	del_timer_sync(&mydata.timer);
	iounmap_fpga();
	unregister_chrdev(MAJOR_NUM, DEVICE_DRIVER_NAME);
}

module_init( dev_driver_init );
module_exit( dev_driver_exit );
MODULE_LICENSE("Dual BSD/GPL");
