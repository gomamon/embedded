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

#define MAJOR_NUM 242  
#define DEVICE_DRIVER_NAME "stopwatch"
#define PHY_FND 0x08000004

static int stopwatch_major=MAJOR_NUM, stopwatch_minor=0;
static int result;
static dev_t stopwatch_dev;
static struct cdev stopwatch_cdev;
unsigned char *fnd_addr;

static int stopwatch_open(struct inode *, struct file *);
static int stopwatch_release(struct inode *, struct file *);
static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t stopwatch_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t stopwatch_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t stopwatch_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t stopwatch_handler4(int irq, void* dev_id, struct pt_regs* reg);

#define STATUS_UNSET 0
#define STATUS_SET 1
#define STATUS_PUASE 2
#define STATUS_RESET 3
#define DECIMAL_SEC 10

static struct timer_data{
	int cnt;
	int status;
	struct timer_list timer;	//timer list struct
} mydata;

static int stopwatch_usage=0;
int interruptCount=0;

int vol_down_status = 0;
static u64 vol_down_jiffies = 0;

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

static struct file_operations stopwatch_fops =
{
	.open = stopwatch_open,
	.write = stopwatch_write,
	.release = stopwatch_release,
};

/* timer */
void timer_handler(unsigned long timeout);
void fnd_write(int cnt);
void timer_setup(void);


void fnd_write(int cnt){
	unsigned char fnd_str[4] = {0,};
	unsigned short int fnd_value;
	int s=(int)(cnt/DECIMAL_SEC)%60;
	int m=((int)(cnt/DECIMAL_SEC)/60)%60;

	sprintf(fnd_str, "%02d%02d",m,s);
	printk("fnd_str: %s\n", fnd_str);
	fnd_value = ((fnd_str[0]-'0')<<12 | (fnd_str[1]-'0')<<8 | (fnd_str[2]-'0')<<4 | (fnd_str[3]-'0') );
	outw(fnd_value, (unsigned int)fnd_addr);

	return;
}

void timer_handler(unsigned long timeout){
	struct timer_data *t_data = (struct timer_data*)timeout;
	printk("time %d \n", t_data->cnt);
	(t_data->cnt)++;

	fnd_write(t_data->cnt);

	mydata.timer.expires = get_jiffies_64() + HZ/DECIMAL_SEC;
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;
	add_timer(&mydata.timer);	

}


void timer_setup(void){
	
	mydata.timer.expires = get_jiffies_64() + HZ/DECIMAL_SEC;
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;

	add_timer(&mydata.timer);
	return;
}



irqreturn_t stopwatch_handler1(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "HOME! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	if(mydata.status == STATUS_UNSET){
		timer_setup();
	}
	mydata.status = STATUS_SET;
	return IRQ_HANDLED;
}

irqreturn_t stopwatch_handler2(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "BACK! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	
	if(mydata.status == STATUS_SET)
		del_timer(&mydata.timer);
	mydata.status = STATUS_UNSET;

	return IRQ_HANDLED;
}

irqreturn_t stopwatch_handler3(int irq, void* dev_id,struct pt_regs* reg) {
	printk(KERN_ALERT "VOL UP! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
	
	if(mydata.status == STATUS_SET)
		del_timer(&mydata.timer);
	mydata.status = STATUS_UNSET;

	mydata.cnt = 0;
	fnd_write(mydata.cnt);
	
	return IRQ_HANDLED;
}

irqreturn_t stopwatch_handler4(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "VOL DOWN! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	
	if(vol_down_status == 0){
		vol_down_jiffies = get_jiffies_64();
		vol_down_status = 1;
	}else{
		
		if(vol_down_jiffies - get_jiffies_64() >= 3000){
			__wake_up(&wq_write, 1, 1, NULL);
			printk("wake up\n");
		}
	 	vol_down_status = 0;
		vol_down_jiffies = 0;
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
	ret=request_irq(irq, stopwatch_handler1, IRQF_TRIGGER_FALLING, "home", 0);

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
	
	stopwatch_usage = 1;

	return 0;
}

static int stopwatch_release(struct inode *minode, struct file *mfile){

	if(mydata.status == STATUS_SET)
		del_timer(&mydata.timer);
		mydata.status = STATUS_UNSET;

	mydata.cnt =0;
	fnd_write(mydata.cnt);

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
		error = register_chrdev_region(stopwatch_dev, 1,"inter");
	}
	else{
		error = alloc_chrdev_region(&stopwatch_dev,stopwatch_minor,1,"inter");
		stopwatch_major = MAJOR(stopwatch_dev);
	}
	if(error<0) {
		printk(KERN_WARNING "inter: can't get major %d\n", stopwatch_major);
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
	int result;
	if((result = stopwatch_register_cdev()) < 0 )
		return result;

	init_timer(&(mydata.timer));
	fnd_addr = ioremap(PHY_FND, 0x4);

	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/%s, Major Num : %d \n",DEVICE_DRIVER_NAME ,MAJOR_NUM );
	return 0;
}


static void __exit stopwatch_exit(void) {
	stopwatch_usage=0;

	del_timer_sync(&mydata.timer);
	iounmap(fnd_addr);

	cdev_del(&stopwatch_cdev);
	unregister_chrdev_region(stopwatch_dev, 1);

	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);
MODULE_LICENSE("GPL");
