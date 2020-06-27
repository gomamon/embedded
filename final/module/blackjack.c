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

/* ioctl number */
#define IOCTL_SET_OPTION _IOW(MAJOR_NUM, 0, char *)
#define IOCTL_COMMAND _IO(MAJOR_NUM, 1)

#define MAJOR_NUM 242  //major number 
#define DEVICE_DRIVER_NAME "blackjack" 	//driver number
#define CARD_NUM 52

/* fpga physical address */
#define PHY_LED 0x08000016
#define PHY_FND 0x08000004
#define PHY_DOT 0x08000210
#define PHY_TEXT_LCD 0x08000090

#define DEALER 0
#define PLAYER 1
#define OFF 0
#define ON 1

#define LOSE 0
#define PAIR 1
#define WIN 2
#define BLACKJACK 3
#define START 4
#define END 5
#define FILL 6
#define CLEAR 7

unsigned char fpga_number[7][10] = {
	{0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x3e,0x00}, // L
	{0x00,0x00,0x3c,0x22,0x22,0x3c,0x20,0x20,0x20,0x00}, // P
	{0x00,0x00,0x41,0x41,0x49,0x49,0x6b,0x36,0x00,0x00}, // W
	{0x00,0x00,0x3c,0x22,0x3c,0x22,0x22,0x3c,0x00,0x00}, //B
	{0x00,0x00,0x1e,0x20,0x20,0x1c,0x02,0x02,0x3c,0x00}, // S
	{0x00,0x00,0x6e,0x20,0x20,0x6e,0x20,0x20,0x6e,0x00}, // E
	{0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f}, //FILL
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} //CLEAR
};

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

unsigned char text1[17]; 
unsigned char text2[17]; 

void fnd_write(int cnt);
void text_lcd_write(void);

void iounmap_fpga(void);
void iomap_fpga(void);

static void init_card(void);
static void start_game(void);
static void add_card(int picker);
static void start_betting(void);
static void end_betting(void);

static int onbetting = ON;


/* file operations related to blackjack module */
static struct file_operations blackjack_fops =
{
	.open = blackjack_open,
	.write = blackjack_write,
	.release = blackjack_release,
};


int cards[CARD_NUM] = {
	1,2,3,4,5,6,7,8,9,10,10,10,10,
	1,2,3,4,5,6,7,8,9,10,10,10,10,
	1,2,3,4,5,6,7,8,9,10,10,10,10,
	1,2,3,4,5,6,7,8,9,10,10,10,10
};

int cards_visit[CARD_NUM] = { 0, };
int on_game = OFF;

static struct  gamer{
	int cards[12];
	int point;
	int idx;
	int money;
} dealer, player;

static int blackjack_usage=0;		//usage counter
int interruptCount=0;				//interrupt counter


wait_queue_head_t wq_write;			//queue for wait
DECLARE_WAIT_QUEUE_HEAD(wq_write);	

static void end_game(void);


void fnd_write(int n){
	/* write miniutes and time from cnt(timer count) to fpga fnd */

	unsigned char fnd_str[4] = {0,};
	unsigned short int fnd_value;

	sprintf(fnd_str, "%04d",n);	

	/*fpga write*/
	fnd_value = ((fnd_str[0]-'0')<<12 | (fnd_str[1]-'0')<<8 | (fnd_str[2]-'0')<<4 | (fnd_str[3]-'0') );	
	outw(fnd_value, (unsigned int)fnd_addr);
	return;
}

void dot_write(int n){
	int i;
	unsigned short int dot_value;
	for(i=0; i<10; i++){
		dot_value = fpga_number[n][i] & 0x7F;
		outw(dot_value, (unsigned int)dot_addr+i*2);
	}
}

void text_lcd_write(){
	int i;
	unsigned short int text_lcd_value = 0;
	unsigned char text_lcd_data[33];

	for(i=0; i<32; i++)	text_lcd_data[i] = ' ';
	for(i=0; i< 16; i++){
		text_lcd_data[i]=text1[i];
		text_lcd_data[16+i] = text2[i];
	}

	for(i=0;i<32;i++)
    {
        text_lcd_value = (text_lcd_data[i] & 0xFF) << 8 | ( text_lcd_data[i + 1] & 0xFF);
		outw(text_lcd_value,(unsigned int)text_lcd_addr+i);
        i++;
    }

	return;
}


irqreturn_t blackjack_home_handler(int irq, void* dev_id) {
	/* home button intterupt */
	printk(KERN_ALERT "INTERRUPT - HOME! : %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	printk("on game %d\n",on_game);

	if(onbetting == OFF){
		onbetting = ON;
		start_betting();
	}
	else{
		if(on_game == OFF){
			start_game();
		}
	}

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
	int i;
	/* vol+ button intterupt - HIT */
	printk(KERN_ALERT "INTERRUPT - VOL UP! : %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

	if(on_game == ON){
		add_card(PLAYER);

		printk("cards : ");
		for(i=0; i<player.idx ; i++){
			printk("%d ",player.cards[i]);
			if(i<7){
				sprintf(&text1[i*2],"%02d",player.cards[i]);
			}
			else{
				sprintf(&text2[(i%7)*2],"%02d",player.cards[i]);
			}
		}
		text_lcd_write();
		if(player.point >= 21) end_game();
	}
	return IRQ_HANDLED;
}

irqreturn_t blackjack_voldown_handler(int irq, void* dev_id) {
	/* vol- button intterupt - STAY*/
	printk(KERN_ALERT "INTERRUPT - VOL DOWN! : %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	
	if(on_game == ON){
		end_game();
	}

	return IRQ_HANDLED;
}


static void init_card(){
	int i;
	for(i=0; i<CARD_NUM; i++){
		cards_visit[i] = 0;
	}

	for(i=0; i<12; i++){
		dealer.cards[i] = 0;
		player.cards[i] = 0;
	}

	for( i=0; i<17; i++){
		text1[i] = ' ';
		text2[i] = ' ';
	}

	dealer.point = 0;
	player.point = 0;

	dealer.idx = 0;
	player.idx = 0;

	return;
}


static void add_card(int picker){
	int card;
	printk("add_card!! ");

	while(1){
		get_random_bytes(&card, sizeof(card));
		if(card < 0) card *= (-1);
		card %= 52;
		if(cards_visit[card] == 0) break;	
	}
	cards_visit[card] = 1;

	if(picker == PLAYER){
		player.cards[player.idx] = cards[card]; 
		player.point += cards[card];
		(player.idx)++;
		printk(" PLAYER [%d] : %d \n", player.idx, player.point);
	}
	else{
		dealer.cards[dealer.idx] = cards[card]; 
		dealer.point += cards[card];
		(dealer.idx)++;
		printk(" DEALER [%d] : %d \n", dealer.idx, dealer.point);
	}

	return;
}

static void start_game(void){
	int i=0;
	printk("start game ");
	init_card();

	add_card(PLAYER);
	add_card(PLAYER);
	add_card(DEALER);
	add_card(DEALER);

	if(player.point == 21){
		player.money += 200;
		dealer.money -= 200;
		end_game();
	}
	else	dot_write(4);
	
	for(i=0; i<17 ; i++){
		text1[i] = ' ';
		text2[i] = ' ';
	}

	printk("cards : ");
	for(i=0; i<player.idx ; i++){
		if(i<7)	sprintf(&text1[i*2],"%02d",player.cards[i]);
		else	sprintf(&text2[(i%7)*2],"%02d",player.cards[i]);	
	}
	printk("%s %s \n",text1, text2);

	on_game = ON;
	text_lcd_write();
	return;
}

static int get_result(void){
	printk("result : P:%d , D:%d\n", player.point, dealer.point);

	if(player.point == 21){
		player.money += 200;
		dealer.money -= 200;
		return BLACKJACK;
	}
	else if(player.point > 21){
		player.money -= 200;
		dealer.money += 200;
		return LOSE;
	}
	else if(dealer.point > 21){
		player.money += 100;
		dealer.money -= 100;
		return WIN;
	}
	else if( player.point == dealer.point ) return PAIR;
	else if(player.point > dealer.point){
		player.money += 100;
		dealer.money -= 100;
		return WIN;
	}
	else{ 
		player.money -= 100;
		dealer.money += 100;
		return LOSE;
	}
}


static void end_game(void){
	int result;
	on_game = OFF;
	onbetting = ON;
	
	while(1){
		if(dealer.point >= 17) break;
		add_card(DEALER);
	}
	result = get_result();
	
	if(player.money <= 0){
		dot_write(END);
		init_card();
		fnd_write(0);
		onbetting = OFF;
	}
	else{
		dot_write(result);
		fnd_write(player.money);
	}
}

static void start_betting(void){
	unsigned char greeting1[17] = "Please push HOME";
	unsigned char greeting2[17] = "to start game.";
	int i;

	init_card();

	player.money = 1000;
	dealer.money = 1000;
	
	for(i=0; i<17 ; i++){
		text1[i] = greeting1[i];
		text2[i] = greeting2[i];
	}
	
	fnd_write(dealer.money);
	dot_write(FILL);
	text_lcd_write();
	onbetting = ON;
	on_game = OFF;
}

static void end_betting(void){
	init_card();
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
	ret=request_irq(irq, blackjack_voldown_handler, IRQF_TRIGGER_FALLING, "voldown", 0);
	
	
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
		start_betting();
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

	iomap_fpga();
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
