#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <asm/ioctl.h>
//#include "device_driver.h"

#define DEVICE_DRIVER_NAME "dev_driver"
#define MAJOR_NUM 242
#define BUF_LEN 50

#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 0, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 1)

#define COMMAND_SIZE 12

static dev_driver_usage = 0;

long number = 0;
int major_number;
char *numchar;

int dev_driver_open(struct inode *,struct file *);
int dev_driver_release(struct inode *, struct file *);
long dev_driver_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations fops = {
	.open = dev_driver_open,
	.release = dev_driver_release,
	.unlocked_ioctl = dev_driver_ioctl
};

static struct timer_data{
	int interval;
	int init;
	int count;
	
	struct timer_list my_timer;
};

struct timer_data mydata;


long dev_driver_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){
	int i;
	char *temp;

	char buff1[4] = {'\0',};
	char buff2[4] = { '\0', };
	char buff3[5] = { '\0' ,};

	printk("device_driver_ioctl\n");

	switch(ioctl_num){
		case IOCTL_SET_OPTION:
			temp = (char *)ioctl_param;

			for(i=0; i<COMMAND_SIZE; i++){
				if(i<3)
					get_user(buff1[i], temp+i);
				else if(i<6)
					get_user(buff2[i-3],temp+i);
				else
					get_user(buff3[i-6], temp+i);
			}
			printk("param : %s %s %s\n",buff1, buff2, buff3);
			
			break;
		case IOCTL_COMMAND:
			printk("IOCTL COMMAND");
			break;
		default:
			printk("IOCTL ERROR\n");
			return -1;
			break;
	}

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


// int timer_setup(int interval){
	
// //	mydata.interval =  interval;

// }


int __init dev_driver_init(void){
	printk("device_driver_init\n");

	major_number = register_chrdev(MAJOR_NUM, DEVICE_DRIVER_NAME, &fops);
	if(major_number < 0){
		printk( "error %d\n", major_number);
		return major_number;
	}
	
	major_number = MAJOR_NUM;
	printk("dev_file: /dev/%s , major: %d\n", DEVICE_DRIVER_NAME, major_number);
	
	//init_timer(&(mydata.timer));

	printk("init module\n");
	return 0;
}

void __exit dev_driver_exit(void){
	printk("device_driver_exit\n");
	dev_driver_usage = 0;
	unregister_chrdev(MAJOR_NUM, DEVICE_DRIVER_NAME);
}

module_init( dev_driver_init );
module_exit( dev_driver_exit );
MODULE_LICENSE("Dual BSD/GPL");
