#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <asm/ioctl.h>
#include <asm/io.h>
//#include "device_driver.h"

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

static struct file_operations fops = {
	.open = dev_driver_open,
	.release = dev_driver_release,
	.unlocked_ioctl = dev_driver_ioctl
};

static struct timer_data{
	int interval;
	int init;
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

static void iounmap_fpag(){
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
					mydata.init *= 10;
					mydata.init += tmp;
				}
			}
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
}

static void timer_handler(unsigned long timeout){
	struct timer_data *t_data = (struct timer_data*)timeout;
	printk("timer handler %d\n", t_data->count);

	t_data->count++;
	if(t_data -> count > count ){
		return ;
	}

	mydata.timer.expires = get_jiffies_64() + (mydata.interval * HZ);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;

	add_timer(&mydata.timer);
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
	iounmap_fpag();
	unregister_chrdev(MAJOR_NUM, DEVICE_DRIVER_NAME);
}

module_init( dev_driver_init );
module_exit( dev_driver_exit );
MODULE_LICENSE("Dual BSD/GPL");
