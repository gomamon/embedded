#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/uaccess.h>

#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <asm/irq.h>
#include <asm/gpio.h>
#include <linux/random.h>

#include <mach/gpio.h>

#define MAJOR_NUM 242  //major number 
#define DEVICE_DRIVER_NAME "blackjack" 	//driver number

/* fpga physical address */
#define PHY_LED 0x08000016
#define PHY_FND 0x08000004
#define PHY_DOT 0x08000210
#define PHY_TEXT_LCD 0x08000090

#define STATUS_UNSET 0
#define STATUS_SET 1

/* device */
static int blackjack_major=MAJOR_NUM, blackjack_minor=0;
static int result;	// result of register device successfuly
static dev_t blackjack_dev;	
static struct cdev blackjack_cdev;

/* device deriver function */
static int blackjack_open(struct inode *, struct file *);
static int blackjack_release(struct inode *, struct file *);
static int blackjack_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

/* interrupt handler */
irqreturn_t blackjack_home_handler(int irq, void* dev_id);
irqreturn_t blackjack_back_handler(int irq, void* dev_id);
irqreturn_t blackjack_volup_handler(int irq, void* dev_id);
irqreturn_t blackjack_voldown_handler(int irq, void* dev_id);

/* virtual address returned by ioremap() */
unsigned char *fnd_addr;  
unsigned char *led_addr;
unsigned char *dot_addr;
unsigned char *text_lcd_addr;

void fnd_write(int cnt);

/* file operations related to blackjack module */
static struct file_operations blackjack_fops =
{
	.open = blackjack_open,
	.write = blackjack_write,
	.release = blackjack_release,
};



static int blackjack_usage=0;		//usage counter
int interruptCount=0;				//interrupt counter


wait_queue_head_t wq_write;			//queue for wait
DECLARE_WAIT_QUEUE_HEAD(wq_write);	


void fnd_write(int n){
	/* write miniutes and time from cnt(timer count) to fpga fnd */

	unsigned char fnd_str[4] = {0,};
	unsigned short int fnd_value;

	sprintf(fnd_str, "%02d%02d",n);	

	/*fpga write*/
	fnd_value = ((fnd_str[0]-'0')<<12 | (fnd_str[1]-'0')<<8 | (fnd_str[2]-'0')<<4 | (fnd_str[3]-'0') );	
	outw(fnd_value, (unsigned int)fnd_addr);

	return;
}


irqreturn_t blackjack_home_handler(int irq, void* dev_id) {
	/* home button intterupt */
	printk(KERN_ALERT "INTERRUPT - HOME! : %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));

	return IRQ_HANDLED;
}

irqreturn_t blackjack_back_handler(int irq, void* dev_id) {
	/* back button intterupt */

	printk(KERN_ALERT "INTERRUPT - BACK! : %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	
	__wake_up(&wq_write, 1, 1, NULL);
	printk("wake up\n");
	
	return IRQ_HANDLED;
}

irqreturn_t blackjack_volup_handler(int irq, void* dev_id) {
	/* vol+ button intterupt */

	printk(KERN_ALERT "INTERRUPT - VOL UP! : %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

	return IRQ_HANDLED;
}

irqreturn_t blackjack_voldown_handler(int irq, void* dev_id) {
	/* vol- button intterupt */

	printk(KERN_ALERT "INTERRUPT - VOL DOWN! : %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	
	return IRQ_HANDLED;
}




static int blackjack_open(struct inode *minode, struct file *mfile){
	/* blackjack module open*/

	int ret;
	int irq;

	printk(KERN_ALERT "OPEN : blackjack module\n");

	/*check usage counter*/
	if(blackjack_usage != 0)
		return -EBUSY;

	/* set interrupt request */
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	ret = request_irq(irq, blackjack_home_handler, IRQF_TRIGGER_FALLING, "home", 0);

	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	ret=request_irq(irq, blackjack_back_handler, IRQF_TRIGGER_FALLING , "back", 0);

	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	ret=request_irq(irq, blackjack_volup_handler, IRQF_TRIGGER_FALLING, "volup", 0);

	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	ret=request_irq(irq, blackjack_voldown_handler, IRQF_TRIGGER_FALLING| IRQF_TRIGGER_RISING, "voldown", 0);
	
	
	blackjack_usage = 1;
	return 0;
}

static int blackjack_release(struct inode *minode, struct file *mfile){
	/* release blackjack */

	/* free interupt request */
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	blackjack_usage = 0;

	printk(KERN_ALERT "RELEASE : blackjack module\n");
	return 0;
}

static int blackjack_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
	/*write blackjack*/

	printk("WRITE\n");

	if(interruptCount==0){
		printk("sleep\n");
		interruptible_sleep_on(&wq_write);
	}
	return 0;
}


static int blackjack_register_cdev(void)
{
	int error;

	/* register blackjack device */
	if(blackjack_major) {
		blackjack_dev = MKDEV(MAJOR_NUM, 0);		//make device
		error = register_chrdev_region(blackjack_dev, 1,"blackjack");	//register a range of device numbers
	}
	else{
		error = alloc_chrdev_region(&blackjack_dev,blackjack_minor,1,"blackjack"); //register a range of char device numbers
		blackjack_major = MAJOR(blackjack_dev);	
	}
	if(error<0) {
		printk(KERN_WARNING "blackjack: can't get major %d\n", blackjack_major);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", blackjack_major);

	/*initialize device*/

	cdev_init(&blackjack_cdev, &blackjack_fops);	//initialize device with fop
	blackjack_cdev.owner = THIS_MODULE;				//set owner
	blackjack_cdev.ops = &blackjack_fops;			//set fop
	error = cdev_add(&blackjack_cdev, blackjack_dev, 1);	//add character device(blackjack device)
	
	if(error){	
		printk(KERN_NOTICE "inter Register Error %d\n", error);
		return -1;
	}
	return 0;
}

void iomap_fpga(){
	/*save kernel virtual address mapped with the physical address space of fpga*/
	led_addr = ioremap(PHY_LED, 0x1);
	fnd_addr = ioremap(PHY_FND, 0x4);
	dot_addr = ioremap(PHY_DOT, 0x10);
	text_lcd_addr = ioremap(PHY_TEXT_LCD, 0x32);		
}

void iounmap_fpga(){
	iounmap(led_addr);
	iounmap(fnd_addr);
	iounmap(dot_addr);
	iounmap(text_lcd_addr);
}


static int __init blackjack_init(void) {
	/* initialize module */
	int result;
	if((result = blackjack_register_cdev()) < 0 )	//register blackjack device
		return result;

	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/%s, Major Num : %d \n",DEVICE_DRIVER_NAME ,MAJOR_NUM );
	return 0;
}


static void __exit blackjack_exit(void) {
	/*exit module*/
	
	blackjack_usage=0;

	iounmap_fpga();		//free the mapped address space by iomap
	cdev_del(&blackjack_cdev);		//delete character device
	unregister_chrdev_region(blackjack_dev, 1);		//unregister device

	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(blackjack_init);
module_exit(blackjack_exit);
MODULE_LICENSE("GPL");
