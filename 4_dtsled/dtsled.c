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
#include <linux/of.h>
#include <linux/of_address.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

#define LED_ON 1
#define LED_OFF 0

#define MAJOR(dev) ((unsigned int)((dev) >> MINORBITS))
#define MINOR(dev) ((unsigned int)((dev)&MINORMASK))
#define MKDEV(ma, mi) (((ma) << MINORBITS) | (mi))

#define DTSLED_CNT 1
#define DTSLED_NAME "dtsled"

struct dtsled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
};

struct dtsled_dev dtsled;

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

int dtsled_open(struct inode *inode, struct file *flip)
{
    flip->private_data = &dtsled;

    return 0;
}
ssize_t dtsled_read(struct file *flip, char __user *buf, size_t cnt, loff_t *oof_t)
{

    return 0;
}
ssize_t dtsled_write(struct file *flip, const char __user *buf, size_t cnt, loff_t *off_t)
{
    int retvalue = 0;
    char ledbuf[1];

    retvalue = copy_from_user(ledbuf, buf, sizeof(buf));
    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -1;
    }

    if (ledbuf[0] == LED_ON)
    {
        led_switch(LED_ON);
    }
    else if (ledbuf[0] == LED_OFF)
    {
        led_switch(LED_OFF);
    }

    return 0;
}
int dtsled_release(struct inode *inode, struct file *flip)
{

    return 0;
}

struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = dtsled_open,
    .read = dtsled_read,
    .write = dtsled_write,
    .release = dtsled_release};

static int __init dtsled_init(void)
{
    u32 val = 0;
    struct property *proper;
    int ret;
    const char *str;
    u32 regdata[14];
    dtsled.nd = of_find_node_by_path("/alphaled");
    if (dtsled.nd == NULL)
    {
        printk("alphaled node can not be found\r\n");
        return -EINVAL;
    }
    else
    {
        printk("alphaled node has been found!\r\n");
    }

    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if (proper == NULL)
    {
        printk("compatible property find failed!\r\n");
    }
    else
    {
        printk("compatible = %s\r\n", (char *)proper->value);
    }

    ret = of_property_read_string(dtsled.nd, "status", &str);
    if (ret < 0)
    {
        printk("status read failed!\r\n");
    }
    else
    {
        printk("status = %s\r\n", str);
    }

    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
    if (ret < 0)
    {
        printk("regdata read failed!\r\n");
    }
    else
    {
        u8 i = 0;
        printk("reg data:\r\n");
        for (i = 0; i < 10; i++)
        {
            printk("%#X ", regdata[i]);
        }
        printk("\r\n");
    }
#if 0
    IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
    SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
    SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
    GPIO1_DR = ioremap(regdata[6], regdata[7]);
    GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
#else
    IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
    SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
    SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
    GPIO1_DR = of_iomap(dtsled.nd, 3);
    GPIO1_GDIR = of_iomap(dtsled.nd, 4);
#endif

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

    if (dtsled.major)
    {
        dtsled.devid = MKDEV(dtsled.major, 0);
        dtsled.minor = 0;
        register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }

    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);
    cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);

    dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
    if (IS_ERR(dtsled.class))
    {
        return PTR_ERR(dtsled.class);
    }

    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
    if (IS_ERR(dtsled.device))
    {
        return PTR_ERR(dtsled.device);
    }

    return 0;
}

static void __exit dtsled_exit(void)
{
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    cdev_del(&dtsled.cdev);
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);
}

module_init(dtsled_init);
module_exit(dtsled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yebin");