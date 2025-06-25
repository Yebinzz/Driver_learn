#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define CCM_CCGR1_BASE               (0X020C406C)     
#define SW_MUX_GPIO1_IO03_BASE      (0X020E0068) 
#define SW_PAD_GPIO1_IO03_BASE      (0X020E02F4) 
#define GPIO1_DR_BASE                (0X0209C000) 
#define GPIO1_GDIR_BASE              (0X0209C004)

#define LED_ON 1
#define LED_OFF 0

#define MAJOR(dev)    ((unsigned int) ((dev) >> MINORBITS)) 
#define MINOR(dev)    ((unsigned int) ((dev) & MINORMASK)) 
#define MKDEV(ma,mi)  (((ma) << MINORBITS) | (mi))

#define NEWCHRLED_CNT 1
#define NEWCHRLED_NAME "newchrled"

static void __iomem *IMX6U_CCM_CCGR1; 
static void __iomem *SW_MUX_GPIO1_IO03; 
static void __iomem *SW_PAD_GPIO1_IO03; 
static void __iomem *GPIO1_DR; 
static void __iomem *GPIO1_GDIR;

struct newchrled_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
};

struct newchrled_dev newchrled;

void led_switch(char state)
{
    u32 val = 0;
    if (state == LED_ON)
    {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, GPIO1_DR);
    }
    else if (state == LED_OFF)
    {
        val = readl(GPIO1_DR);
        val |= (1 << 3);
        writel(val, GPIO1_DR);
    }
}

int newchrled_open (struct inode *inode, struct file *flip)
{
    flip->private_data = &newchrled;

    return 0;
}
ssize_t newchrled_read (struct file *flip, char __user *buf, size_t cnt, loff_t *oof_t)
{

    return 0;
}
ssize_t newchrled_write (struct file *flip, const char __user *buf, size_t cnt, loff_t *off_t)
{
    int retvalue = 0;
    char ledbuf[1];

    retvalue = copy_from_user(ledbuf, buf, sizeof(buf));
    if(retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -1;
    }

    if(ledbuf[0] == LED_ON)
    {
        led_switch(LED_ON);
    }
    else if(ledbuf[0] == LED_OFF)
    {
        led_switch(LED_OFF);
    }

    return 0;
}
int newchrled_release (struct inode *inode, struct file *flip)
{

    return 0;
}

struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
    .open = newchrled_open,
    .read = newchrled_read,
    .write = newchrled_write,
    .release = newchrled_release
};

static int __init newchrled_init(void)
{
    u32 val;

    IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
    SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
    GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26); /* 清除以前的设置 */
    val |= (3 << 26);  /* 设置新值 */
    writel(val, IMX6U_CCM_CCGR1);

    /* 3、设置GPIO1_IO03的复用功能，将其复用为 
 *    GPIO1_IO03，最后设置IO属性。 
 */
    writel(5, SW_MUX_GPIO1_IO03);

    /* 寄存器SW_PAD_GPIO1_IO03设置IO属性 */
    writel(0x10B0, SW_PAD_GPIO1_IO03);

    /* 4、设置GPIO1_IO03为输出功能 */
    val = readl(GPIO1_GDIR);
    val &= ~(1 << 3); /* 清除以前的设置 */
    val |= (1 << 3);  /* 设置为输出 */
    writel(val, GPIO1_GDIR);

    /* 5、默认关闭LED */
    val = readl(GPIO1_DR);
    val |= (1 << 3);
    writel(val, GPIO1_DR);

    if(newchrled.major)
    {
        newchrled.devid = MKDEV(newchrled.major,0);
        newchrled.minor = 0;
        register_chrdev_region(newchrled.devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_CNT, NEWCHRLED_NAME);
        newchrled.major = MAJOR(newchrled.devid);
        newchrled.minor = MINOR(newchrled.devid);
    }

    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&newchrled.cdev, &newchrled_fops);
    cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_CNT);

    newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    if(IS_ERR(newchrled.class))
    {
        return PTR_ERR(newchrled.class);
    }
    newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
    if(IS_ERR(newchrled.device))
    {
        return PTR_ERR(newchrled.device);
    }

    return 0;
    
}

static void __exit newchrled_exit(void)
{
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    cdev_del(&newchrled.cdev);
    unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);

    device_destroy(newchrled.class, newchrled.devid);
    class_destroy(newchrled.class);

    return ;
}

module_init(newchrled_init);
module_exit(newchrled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yebin");