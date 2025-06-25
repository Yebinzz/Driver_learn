#include <linux/types.h> 
#include <linux/kernel.h> 
#include <linux/delay.h> 
#include <linux/ide.h> 
#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/errno.h> 
#include <linux/gpio.h> 
#include <asm/mach/map.h> 
#include <asm/uaccess.h> 
#include <asm/io.h>

#define LED_MAJOR 200
#define LED_NAME "led"

#define LED_ON 1
#define LED_OFF 0

/* 寄存器物理地址 */ 
#define CCM_CCGR1_BASE               (0X020C406C)     
#define SW_MUX_GPIO1_IO03_BASE      (0X020E0068) 
#define SW_PAD_GPIO1_IO03_BASE      (0X020E02F4) 
#define GPIO1_DR_BASE                (0X0209C000) 
#define GPIO1_GDIR_BASE              (0X0209C004)

static void __iomem *IMX6U_CCM_CCGR1; 
static void __iomem *SW_MUX_GPIO1_IO03; 
static void __iomem *SW_PAD_GPIO1_IO03; 
static void __iomem *GPIO1_DR; 
static void __iomem *GPIO1_GDIR;

void led_switch(int state)
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

int led_open (struct inode *inode, struct file *flip)
{

    return 0;
}
ssize_t led_read (struct file *flip, char __user *buf, size_t cnt, loff_t *off_t)
{

    return 0;
}
ssize_t led_write (struct file *flip, const char __user *buf, size_t cnt, loff_t *off_t)
{
    int retvalue;
    char ledbuf[1];

    retvalue = copy_from_user(ledbuf, buf, cnt);
    if(retvalue < 0)
    {
        printk("kernel write failed!\r\n");
    }

    if(ledbuf[0] == LED_ON)
    {
        led_switch(LED_ON);
    }
    else if(ledbuf[0] == LED_OFF)
    {
        led_switch(LED_OFF);
    }
}
int led_release (struct inode *inode, struct file *flip)
{

}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release
};

static int __init led_init(void)
{
    int retvalue = 0;
    u32 val = 0;

    /* 初始化LED */
    /* 1、寄存器地址映射 */
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

    retvalue = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
    if (retvalue < 0)
    {
        printk("led driver register failed!\r\n");
        return -1;
    }

    return 0;
}

static void __exit led_exit(void)
{
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    unregister_chrdev(LED_MAJOR, LED_NAME);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yebin");