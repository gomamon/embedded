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

static int stopwatch_major=MAJOR_NUM, stopwatch_minor=0;
static int result;
static dev_t stopwatch_dev;
static struct cdev stopwatch_cdev;
unsigned char *fnd_addr;

/* device deriver function */
static int stopwatch_open(struct inode *, struct file *);
static int stopwatch_release(struct inode *, struct file *);
static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

/* interrupt handler*/
irqreturn_t stopwatch_handler1(int irq, void* dev_id);
irqreturn_t stopwatch_handler2(int irq, void* dev_id);
irqreturn_t stopwatch_handler3(int irq, void* dev_id);
irqreturn_t stopwatch_handler4(int irq, void* dev_id);


void timer_handler(unsigned long timeout);
void fnd_write(int cnt);
void timer_setup(int jiffies_gap);


void exit_handler(unsigned long);


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


/* write miniutes and time from cnt(timer count) to fpga fnd */
void fnd_write(int cnt){
	unsigned char fnd_str[4] = {0,};
	unsigned short int fnd_value;
	int s=(int)(cnt)%60;
	int m=(int)(cnt/60)%60;

	sprintf(fnd_str, "%02d%02d",m,s);
	printk("fnd_str: %s\n", fnd_str);
	fnd_value = ((fnd_str[0]-'0')<<12 | (fnd_str[1]-'0')<<8 | (fnd_str[2]-'0')<<4 | (fnd_str[3]-'0') );
	outw(fnd_value, (unsigned int)fnd_addr);

	return;
}

void timer_handler(unsigned long timeout){
	struct timer_data *t_data = (struct timer_data*)timeout;
	(t_data->cnt)++;
	(t_data->cnt)%=(60*60);
	fnd_write(t_data->cnt);
	t_data->jiffies = get_jiffies_64();
	paused_jiffies = 0;

	t_data->timer.expires = get_jiffies_64() + HZ;
	t_data->timer.data = (unsigned long)&mydata;
	t_data->timer.function = timer_handler;
	add_timer(&(t_data->timer));	

}


void timer_setup(int jiffies_gap){
	mydata.jiffies = get_jiffies_64();

	mydata.timer.expires = get_jiffies_64() + (HZ-jiffies_gap);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;
	
	add_timer(&mydata.timer);
	return;
}

void exit_handler(unsigned long timeout){
	del_timer(&exit_timer);
	printk("exit handler!\n");
	exit_status = STATUS_UNSET;
	if(vol_down_pressed == 1){
		__wake_up(&wq_write, 1, 1, NULL);
		printk("wake up\n");
	}
	return;
}

void exit_timer_setup(void){
	if(exit_status == STATUS_SET){
		del_timer(&exit_timer);
	}
	
	printk("exit setup!\n");
	exit_timer.expires = get_jiffies_64()+ 3*HZ;
	exit_timer.function = exit_handler;

	exit_status = STATUS_SET;
	add_timer(&exit_timer);
	return;
}

void data_clear(void){
	if(mydata.status == STATUS_SET)
		del_timer(&mydata.timer);
	mydata.status = STATUS_UNSET;
	mydata.jiffies = 0;
	mydata.cnt = 0;
	paused_jiffies = 0 ;

	fnd_write(mydata.cnt);
	return;
}

irqreturn_t stopwatch_handler1(int irq, void* dev_id) {
	printk(KERN_ALERT "HOME! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	if(mydata.status == STATUS_UNSET){
		if(paused_jiffies == 0 || mydata.jiffies ==0 )
			timer_setup(0);
		else{
			timer_setup( paused_jiffies - mydata.jiffies);
		}
	}
	paused_jiffies = 0;
	mydata.status = STATUS_SET;
	return IRQ_HANDLED;
}

irqreturn_t stopwatch_handler2(int irq, void* dev_id) {
	printk(KERN_ALERT "BACK! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	
	if(paused_jiffies == 0 || mydata.jiffies!=0)
		paused_jiffies = get_jiffies_64();
	if(mydata.status == STATUS_SET)
		del_timer(&mydata.timer);
	mydata.status = STATUS_UNSET;
	
	return IRQ_HANDLED;
}

irqreturn_t stopwatch_handler3(int irq, void* dev_id) {
	printk(KERN_ALERT "VOL UP! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

	data_clear();
	return IRQ_HANDLED;
}

irqreturn_t stopwatch_handler4(int irq, void* dev_id) {
	printk(KERN_ALERT "VOL DOWN! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	
	if(vol_down_pressed == 0){
		exit_timer_setup();
		vol_down_pressed = 1;
	}else{
		vol_down_pressed = 0;
	}
	return IRQ_HANDLED;
}

static int stopwatch_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	printk(KERN_ALERT "Open Module\n");
	if(stopwatch_usage != 0)
		return -EBUSY;

	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret = request_irq(irq, stopwatch_handler1, IRQF_TRIGGER_FALLING, "home", 0);

	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, stopwatch_handler2, IRQF_TRIGGER_FALLING , "back", 0);

	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, stopwatch_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, stopwatch_handler4, IRQF_TRIGGER_FALLING| IRQF_TRIGGER_RISING, "voldown", 0);
	
	exit_status= STATUS_UNSET;
	stopwatch_usage = 1;

	return 0;
}

static int stopwatch_release(struct inode *minode, struct file *mfile){

	data_clear();

	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	stopwatch_usage = 0;

	printk(KERN_ALERT "Release Module\n");
	return 0;
}

static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
	
	if(interruptCount==0){
		printk("sleep on\n");
		interruptible_sleep_on(&wq_write);
	}

	printk("write\n");
	return 0;
}


static int stopwatch_register_cdev(void)
{
	int error;

	if(stopwatch_major) {
		stopwatch_dev = MKDEV(MAJOR_NUM, 0);
		error = register_chrdev_region(stopwatch_dev, 1,"stopwatch");
	}
	else{
		error = alloc_chrdev_region(&stopwatch_dev,stopwatch_minor,1,"stopwatch");
		stopwatch_major = MAJOR(stopwatch_dev);
	}
	if(error<0) {
		printk(KERN_WARNING "stopwatch: can't get major %d\n", stopwatch_major);
		return result;
	}
	
	printk(KERN_ALERT "major number = %d\n", stopwatch_major);
	cdev_init(&stopwatch_cdev, &stopwatch_fops);
	stopwatch_cdev.owner = THIS_MODULE;
	stopwatch_cdev.ops = &stopwatch_fops;
	error = cdev_add(&stopwatch_cdev, stopwatch_dev, 1);
	
	if(error){
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

	init_timer(&(mydata.timer));			//
	init_timer(&exit_timer);				//
	fnd_addr = ioremap(PHY_FND, 0x4);		//

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
	iounmap(fnd_addr);		//iounmap fnd address
	cdev_del(&stopwatch_cdev);		//delete character device
	unregister_chrdev_region(stopwatch_dev, 1);		//unregister device

	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);
MODULE_LICENSE("GPL");
