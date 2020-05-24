#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <asm/ioctl.h>
#include <asm/io.h>
#include "fpga_dot_font.h"

/* device drvier information */
#define DEVICE_DRIVER_NAME "dev_driver"
#define MAJOR_NUM 242

/* fpga physical address */
#define PHY_LED 0x08000016
#define PHY_FND 0x08000004
#define PHY_DOT 0x08000210
#define PHY_TEXT_LCD 0x08000090

/* ioctl number */
#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 0, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 1)

/* ioctl set_option parameter size */
#define PARAM_SIZE 12

/* device drivaer usage counter */
static int dev_driver_usage = 0;

/* device driver function */
int dev_driver_open(struct inode *,struct file *);
int dev_driver_release(struct inode *, struct file *);
long dev_driver_ioctl(struct file *, unsigned int, unsigned long);

/* timer fuction */
void timer_setup(void);
void timer_handler(unsigned long);

/* handling fpga function */
void iomap_fpga(void);
void iounmap_fpga(void);
void fpga_clear(void);


int count = 0;

unsigned char text1[17] = "20161622";
unsigned char text2[17] = "YEEUN LEE";

/* file operations related to dev_driver module */
static struct file_operations fops = {
	.open = dev_driver_open,
	.release = dev_driver_release,
	.unlocked_ioctl = dev_driver_ioctl
};

static struct timer_data{
	int interval;	//timer interval
	int init;		//init pattern
	int init_pos;	//init pattern position
	int count;		//timer count
	int text1_pos;	//position in a first-line text_lcd
	int text2_pos;	//position in a second-line text_lcd
	int text1_dir;	//text moving direction in a first-line text_lcd
	int text2_dir;	//text moving direction in a second-line text_lcd
	struct timer_list timer;	//timer list struct
} mydata;

/* save kernal virtual address returned by ioremap() */
static struct fpga_virtual_address{
	unsigned char *led;	
	unsigned char *fnd;
	unsigned char *dot;
	unsigned char *text_lcd;
} fpga_addr;


void iomap_fpga(){
	/*save kernel virtual address mapped with the physical address space of fpga*/
	fpga_addr.led = ioremap(PHY_LED, 0x1);
	fpga_addr.fnd = ioremap(PHY_FND, 0x4);
	fpga_addr.dot = ioremap(PHY_DOT,0x10);
	fpga_addr.text_lcd = ioremap(PHY_TEXT_LCD,0x32);
}

void iounmap_fpga(){
	/*free the mapped address space by iomap */
	iounmap(fpga_addr.led);
	iounmap(fpga_addr.fnd);
	iounmap(fpga_addr.dot);
	iounmap(fpga_addr.text_lcd);
}


long dev_driver_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){
	/*device driver ioctl*/

	int i;
	char *temp;
	int tmp;
	char buff[12] = {'\0',};

	switch(ioctl_num){
		case IOCTL_SET_OPTION:
			/* Take the parameters and store it in struct mydata for counter*/
			printk("dev_driver ioctl set_option\n");
			temp = (char *)ioctl_param;
			
			/*init struct mydata */
			mydata.interval = 0;
			mydata.count = 0;
			count =0;
			mydata.init = 0;
			mydata.text1_pos = 0;	mydata.text2_pos = 0;
			mydata.text1_dir = 0;	mydata.text2_dir = 0;

			/*read ioctl parameters(interval,count,init) and save to mydata*/
			for(i=0; i<PARAM_SIZE; i++){
				get_user(buff[i], temp+i);
				if(buff[i]<'0'||buff[i]>'9') break;
				tmp = (int)buff[i]-'0';
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

			break;
		case IOCTL_COMMAND:
			/* ioctl command
			* set timer and start count  	
			*/
			printk("dev_driver ioctl command\n");
			init_timer(&(mydata.timer)); //initailize timer
			timer_setup();	//setup timer parameter and start count
			break;
		default:
			printk("ERROR : invalid ioctl_num \n");
			return -1;
			break;
	}
	return 0;
}

void timer_setup(){
	/*setup timer parameter and add timer*/

	del_timer_sync(&mydata.timer);	//delete before timer
	
	/*set timer inforamtion*/
	mydata.timer.expires = jiffies + (mydata.interval *HZ);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;

	add_timer(&mydata.timer);	//add timer
	return;
}

int fpga_handler(struct timer_data *t_data){
	/* write processed data in t_data to fpga */
	int i;
	
	unsigned short led_value = (unsigned short) (1<<(8-t_data->init));

	unsigned char fnd_str[4] = {0,};
	unsigned short int fnd_value;

	unsigned short int dot_value; 

	unsigned short int text_lcd_value = 0;
	unsigned char text_lcd_data[33];

	/* led write*/
	outw(led_value, (unsigned int)fpga_addr.led);

	/* fnd write */
	fnd_str[t_data->init_pos] = t_data->init;
	fnd_value = fnd_str[0]<<12 | fnd_str[1]<<8 | fnd_str[2]<<4 | fnd_str[3];
	outw(fnd_value, (unsigned int)fpga_addr.fnd);


	/* dot write */
	for(i=0; i<10 ; i++){
		dot_value = fpga_number[t_data->init][i] & 0x7F;
		outw(dot_value, (unsigned int)fpga_addr.dot+i*2);
	}

	/* text lcd write*/
	for(i=0; i<32; i++)	text_lcd_data[i] = ' ';
	for(i=0; i< 8; i++){
		text_lcd_data[i+t_data->text1_pos]=text1[i];
		text_lcd_data[16+(i+t_data->text2_pos)] = text2[i];
	}
	text_lcd_data[16+(8+t_data->text2_pos)]=text2[8];

	for(i=0;i<32;i++)
    {
        text_lcd_value = (text_lcd_data[i] & 0xFF) << 8 | ( text_lcd_data[i + 1] & 0xFF);
		outw(text_lcd_value,(unsigned int)fpga_addr.text_lcd+i);
        i++;
    }

	return 0;
}

void fpga_clear(){
	/* write clear in fpga */
	int i;
	unsigned char c = ' ';
	unsigned short int text_lcd_value = 0;

	/* led clear */
	outw(0, (unsigned int)fpga_addr.led);

	/* fnd clear */
	outw(0, (unsigned int)fpga_addr.fnd);
	
	/* dot clear */
	for(i=0; i<10 ; i++){
		outw(fpga_set_blank[i], (unsigned int)fpga_addr.dot+i*2);
	}

	/* text lcd clear*/
	for(i=0;i<32;i++)
    {
		text_lcd_value = (c & 0xFF) << 8 | (c & 0xFF) ;
		outw(text_lcd_value,(unsigned int)fpga_addr.text_lcd+i);
        i++;
    }

	return;

}

void timer_handler(unsigned long timeout){
	/* timer callback function to handle timer 
	* Write processed timer data to fpga and increase count and process other timer datas
	* If timer count is over count(set by ioctl set_options), clear fpga and return.
	* If not, processing timer data and add timer
	*/
	struct timer_data *t_data = (struct timer_data*)timeout;
	printk("timer count : %d\n", t_data->count);

	if(fpga_handler(t_data) != 0)
		return;

	/* increase count */
	t_data->count++;
	
	/* set init pattern and init pattern position*/
	t_data->init++;
	if(t_data->init > 8) t_data->init = 1;
	
	if((t_data->count)%8 == 0)
		t_data->init_pos = 1 + t_data->init_pos;
	if(t_data->init_pos == 4) t_data->init_pos = 0;

	/* set text position for text lcd*/
	if(t_data->text1_dir == 0) t_data->text1_pos++;
	else if(t_data->text1_dir == 1) t_data->text1_pos--;
	if(t_data->text1_pos+8 >= 16){
		t_data->text1_dir = 1;
	}
	else if(t_data->text1_pos <= 0 ){
		t_data->text1_dir = 0;
	}

	if(t_data->text2_dir == 0) t_data->text2_pos++;
	else if(t_data->text2_dir == 1) t_data->text2_pos--;
	if(t_data->text2_pos+9 >= 16){
		t_data->text2_dir = 1;
	}
	else if(t_data->text2_pos <= 0 ){
		t_data->text2_dir = 0;
	}

	/* if this timer count over count(set by ioctl set_options) fpga clear and return*/
	if(t_data -> count > count ){
		fpga_clear();
		return ;
	}

	/* set timer information */
	mydata.timer.expires = get_jiffies_64() + (mydata.interval * HZ);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;

	/* add timer */
	add_timer(&mydata.timer);

	return;
}

int dev_driver_open(struct inode *inode, struct file *file) {
	/* dev_driver open */
	printk("dev_driver open\n");

	if (dev_driver_usage != 0) { //if usage counter set, return -EBUSY 
		return -EBUSY;
	}

	dev_driver_usage = 1; //set usage counter

	return 0;
}

int dev_driver_release(struct inode *inode, struct file *file) {
	/* dev_driver release */
	printk("dev_driver release\n");
	dev_driver_usage = 0;	//unset usage counter
	return 0;
}


int __init dev_driver_init(void){
	/* initialize module */
	int major_number;
	printk("dev_driver_init\n");

	major_number = register_chrdev(MAJOR_NUM, DEVICE_DRIVER_NAME, &fops);
	if(major_number < 0){
		printk( "error %d\n", major_number);
		return major_number;
	}

	iomap_fpga(); // iomap fpga devices
	printk("dev_file: /dev/%s , major: %d\n", DEVICE_DRIVER_NAME, MAJOR_NUM);
	printk("init module\n");
	return 0;
}

void __exit dev_driver_exit(void){
	/* exit module */
	printk("dev_driver_exit\n");
	dev_driver_usage = 0;	//unset usage counter
	del_timer_sync(&mydata.timer);	//delete timer
	iounmap_fpga();	//iounmap fpga address
	unregister_chrdev(MAJOR_NUM, DEVICE_DRIVER_NAME);
}

module_init( dev_driver_init );
module_exit( dev_driver_exit );
MODULE_LICENSE("GPL");
MODULE_AUTHOR("YEEUN");