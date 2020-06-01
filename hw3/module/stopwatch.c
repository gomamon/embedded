#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <linux/ioctl.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

#define MAJOR_NUM 242  //major number 
#define DEVICE_DRIVER_NAME "stopwatch" 	//driver number
#define PHY_FND 0x08000004	

#define STATUS_UNSET 0
#define STATUS_SET 1

/* device */
static int stopwatch_major=MAJOR_NUM, stopwatch_minor=0;
static int result;	// result of register device successfuly
static dev_t stopwatch_dev;	
static struct cdev stopwatch_cdev;

/* device deriver function */
static int stopwatch_open(struct inode *, struct file *);
static int stopwatch_release(struct inode *, struct file *);
static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

/* interrupt handler */
irqreturn_t stopwatch_home_handler(int irq, void* dev_id);
irqreturn_t stopwatch_back_handler(int irq, void* dev_id);
irqreturn_t stopwatch_volup_handler(int irq, void* dev_id);
irqreturn_t stopwatch_voldown_handler(int irq, void* dev_id);

/* fnd */
unsigned char *fnd_addr; //kernal virtual address returned by ioremap()
void fnd_write(int cnt);

/* timer */
void timer_handler(unsigned long timeout);
void timer_setup(int jiffies_gap);

/* exit timer */
void exit_handler(unsigned long);
void exit_timer_setup(void);

/* file operations related to stopwatch module */
static struct file_operations stopwatch_fops =
{
	.open = stopwatch_open,
	.write = stopwatch_write,
	.release = stopwatch_release,
};


/* timer data for stopwatch */
static struct timer_data{
	int cnt;					//timer count
	int jiffies;				//last jiffies when timer function called
	int status;					//status about timer set or unset
	struct timer_list timer;	//timer list struct
} mydata;

int paused_jiffies =0;	//save jiffies when paused (back)

/* exit */
struct timer_list exit_timer;		// timer for exit(vol donw button). 
int exit_status = STATUS_UNSET;		// exit_status is about that exit timer is set or not
int vol_down_pressed = 0; 			// If vol down button pressed, this value is 1

static int stopwatch_usage=0;		//usage counter
int interruptCount=0;				//interrupt counter


wait_queue_head_t wq_write;			//queue for wait
DECLARE_WAIT_QUEUE_HEAD(wq_write);	


void fnd_write(int cnt){
	/* write miniutes and time from cnt(timer count) to fpga fnd */

	unsigned char fnd_str[4] = {0,};
	unsigned short int fnd_value;
	int s=(int)(cnt)%60;	//cnt to second
	int m=(int)(cnt/60)%60;	//cnt to minutes

	sprintf(fnd_str, "%02d%02d",m,s);	//get string formatted from 'mmss'
	printk("stopwatch : %s\n", fnd_str);	

	/*fpga write*/
	fnd_value = ((fnd_str[0]-'0')<<12 | (fnd_str[1]-'0')<<8 | (fnd_str[2]-'0')<<4 | (fnd_str[3]-'0') );	
	outw(fnd_value, (unsigned int)fnd_addr);

	return;
}

void timer_handler(unsigned long timeout){
/* timer callback function to handle timer for stopwatch*/


	struct timer_data *t_data = (struct timer_data*)timeout;

	/* increase timer and write to device*/
	(t_data->cnt)++;
	(t_data->cnt)%=(60*60);
	fnd_write(t_data->cnt);

	/* save to current jiffies */
	t_data->jiffies = get_jiffies_64();
	paused_jiffies = 0;


	/*add timer*/
	t_data->timer.expires = get_jiffies_64() + HZ;
	t_data->timer.data = (unsigned long)&mydata;
	t_data->timer.function = timer_handler;
	add_timer(&(t_data->timer));	

}


void timer_setup(int jiffies_gap){
	/*setup timer paramter and add timer*/
	
	mydata.jiffies = get_jiffies_64();	//save current jiffies

	mydata.timer.expires = get_jiffies_64() + (HZ-jiffies_gap);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;
	
	add_timer(&mydata.timer);
	return;
}

void exit_handler(unsigned long timeout){
	/*	exit timer callback function 
	*	If vol down button is pressed wake up process in wait queue 
	*/

	/* delete timer */
	del_timer(&exit_timer); //delete timer
	exit_status = STATUS_UNSET;

	/* check vol down button is not released */
	if(vol_down_pressed == 1){
		/*wake up precess in wait queue*/
		__wake_up(&wq_write, 1, 1, NULL);
		printk("wake up\n");
	}
	return;
}

void exit_timer_setup(void){
	/*setup exit timer paramter and add timer*/

	/*delete exit timer if registered*/
	if(exit_status == STATUS_SET){
		del_timer(&exit_timer);
	}
	
	/* add exit timer*/
	exit_timer.expires = get_jiffies_64()+ 3*HZ;	//expired after 3 seconds
	exit_timer.function = exit_handler;
	exit_status = STATUS_SET;
	add_timer(&exit_timer);

	return;
}

void data_clear(void){
	/* clear fnd, data and timer */

	if(mydata.status == STATUS_SET)
		del_timer(&mydata.timer);
	mydata.status = STATUS_UNSET;
	mydata.jiffies = 0;
	mydata.cnt = 0;
	paused_jiffies = 0 ;

	fnd_write(mydata.cnt);
	return;
}

irqreturn_t stopwatch_home_handler(int irq, void* dev_id) {
	/* home button intterupt */
	printk(KERN_ALERT "INTERRUPT - HOME! : %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));

	if(mydata.status == STATUS_UNSET){	
		if(paused_jiffies == 0 || mydata.jiffies ==0 )	
			timer_setup(0);
		else{
			//when resuming timer after pause
			timer_setup( paused_jiffies - mydata.jiffies);
		}
	}
	paused_jiffies = 0;	
	mydata.status = STATUS_SET;
	return IRQ_HANDLED;
}

irqreturn_t stopwatch_back_handler(int irq, void* dev_id) {
	/* back button intterupt */

	printk(KERN_ALERT "INTERRUPT - BACK! : %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	
	if(paused_jiffies == 0 || mydata.jiffies!=0)	//check renter back button
		paused_jiffies = get_jiffies_64();	//save jiffies when be paused 

	/*if timer is set, delete timer*/
	if(mydata.status == STATUS_SET)	
		del_timer(&mydata.timer);
	mydata.status = STATUS_UNSET;
	
	return IRQ_HANDLED;
}

irqreturn_t stopwatch_volup_handler(int irq, void* dev_id) {
	/* vol+ button intterupt */

	printk(KERN_ALERT "INTERRUPT - VOL UP! : %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

	//clear data and fpga
	data_clear();
	return IRQ_HANDLED;
}

irqreturn_t stopwatch_voldown_handler(int irq, void* dev_id) {
	/* vol- button intterupt */

	printk(KERN_ALERT "INTERRUPT - VOL DOWN! : %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	
	if(vol_down_pressed == 0){	//when press button
		exit_timer_setup();		//timer setup for exit
		vol_down_pressed = 1;
	}else{						//when release button
		vol_down_pressed = 0;
	}
	return IRQ_HANDLED;
}

static int stopwatch_open(struct inode *minode, struct file *mfile){
	/* stopwatch module open*/

	int ret;
	int irq;

	printk(KERN_ALERT "OPEN : stopwatch module\n");

	/*check usage counter*/
	if(stopwatch_usage != 0)
		return -EBUSY;

	/* set interrupt request */
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	ret = request_irq(irq, stopwatch_home_handler, IRQF_TRIGGER_FALLING, "home", 0);

	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	ret=request_irq(irq, stopwatch_back_handler, IRQF_TRIGGER_FALLING , "back", 0);

	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	ret=request_irq(irq, stopwatch_volup_handler, IRQF_TRIGGER_FALLING, "volup", 0);

	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	ret=request_irq(irq, stopwatch_voldown_handler, IRQF_TRIGGER_FALLING| IRQF_TRIGGER_RISING, "voldown", 0);
	

	exit_status= STATUS_UNSET;
	stopwatch_usage = 1;

	return 0;
}

static int stopwatch_release(struct inode *minode, struct file *mfile){
	/* release stopwatch */

	/*set data, fnd and timer clear*/
	data_clear();

	/* free interupt request */
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	stopwatch_usage = 0;

	printk(KERN_ALERT "RELEASE : stopwatch module\n");
	return 0;
}

static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
	/*write stopwatch*/
	
	printk("WRITE\n");

	if(interruptCount==0){
		printk("sleep\n");
		interruptible_sleep_on(&wq_write);
	}
	return 0;
}


static int stopwatch_register_cdev(void)
{
	int error;

	/* register stopwatch device */
	if(stopwatch_major) {
		stopwatch_dev = MKDEV(MAJOR_NUM, 0);		//make device
		error = register_chrdev_region(stopwatch_dev, 1,"stopwatch");	//register a range of device numbers
	}
	else{
		error = alloc_chrdev_region(&stopwatch_dev,stopwatch_minor,1,"stopwatch"); //register a range of char device numbers
		stopwatch_major = MAJOR(stopwatch_dev);	
	}
	if(error<0) {
		printk(KERN_WARNING "stopwatch: can't get major %d\n", stopwatch_major);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", stopwatch_major);

	/*initialize device*/

	cdev_init(&stopwatch_cdev, &stopwatch_fops);	//initialize device with fop
	stopwatch_cdev.owner = THIS_MODULE;				//set owner
	stopwatch_cdev.ops = &stopwatch_fops;			//set fop
	error = cdev_add(&stopwatch_cdev, stopwatch_dev, 1);	//add character device(stopwatch device)
	
	if(error){	//if de
		printk(KERN_NOTICE "inter Register Error %d\n", error);
		return -1;
	}
	return 0;
}

static int __init stopwatch_init(void) {
	/* initialize module */
	int result;
	if((result = stopwatch_register_cdev()) < 0 )	//register stopwatch device
		return result;

	init_timer(&(mydata.timer));			//initialize timer 
	init_timer(&exit_timer);				//initialize exit timer 
	fnd_addr = ioremap(PHY_FND, 0x4);		//get kernel virtual address mapped with the physical addr space of fpga

	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/%s, Major Num : %d \n",DEVICE_DRIVER_NAME ,MAJOR_NUM );
	return 0;
}


static void __exit stopwatch_exit(void) {
	/*exit module*/
	
	stopwatch_usage=0;

	if(exit_status == STATUS_SET){		//delete exit timer
		del_timer(&exit_timer);
	}
	if(mydata.status == STATUS_SET){	//delete timer for stopwatch
		del_timer(&mydata.timer);
	}
	iounmap(fnd_addr);		//free the mapped address space by iomap
	cdev_del(&stopwatch_cdev);		//delete character device
	unregister_chrdev_region(stopwatch_dev, 1);		//unregister device

	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);
MODULE_LICENSE("GPL");
