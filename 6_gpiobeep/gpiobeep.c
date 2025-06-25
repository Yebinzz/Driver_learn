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
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define BEEP_ON 1
#define BEEP_OFF 0

#define GPIOBEEP_CNT 1
#define GPIOBEEP_NAME "gpiobeep"

struct gpiobeep_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int beep_gpio;
};

struct gpiobeep_dev gpiobeep;

int gpiobeep_open(struct inode *inode, struct file *flip)
{
    flip->private_data = &gpiobeep;

    return 0;
}
ssize_t gpiobeep_read(struct file *flip, char __user *buf, size_t cnt, loff_t *oof_t)
{

    return 0;
}
ssize_t gpiobeep_write(struct file *flip, const char __user *buf, size_t cnt, loff_t *off_t)
{
    struct gpiobeep_dev *dev = flip->private_data;
    int retvalue = 0;
    char beepbuf[1];

    retvalue = copy_from_user(beepbuf, buf, sizeof(buf));
    if (retvalue < 0)
    {
        printk("kernel write faibeep!\r\n");
        return -1;
    }

    if (beepbuf[0] == BEEP_ON)
    {
        gpio_set_value(dev->beep_gpio, 0);
    }
    else if (beepbuf[0] == BEEP_OFF)
    {
        gpio_set_value(dev->beep_gpio, 1);
    }

    return 0;
}
int gpiobeep_release(struct inode *inode, struct file *flip)
{

    return 0;
}

struct file_operations gpiobeep_fops = {
    .owner = THIS_MODULE,
    .open = gpiobeep_open,
    .read = gpiobeep_read,
    .write = gpiobeep_write,
    .release = gpiobeep_release};

static int __init gpiobeep_init(void)
{
    int ret;
    gpiobeep.nd = of_find_node_by_path("/gpiobeep");
    if (gpiobeep.nd == NULL)
    {
        printk("gpiobeep node can not be found\r\n");
        return -EINVAL;
    }
    else
    {
        printk("gpiobeep node has been found!\r\n");
    }

    gpiobeep.beep_gpio = of_get_named_gpio(gpiobeep.nd, "beep-gpio", 0);
    if (gpiobeep.beep_gpio < 0)
    {
        printk("can't get beep-gpio!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("beep-gpio = %d\r\n", gpiobeep.beep_gpio);
    }

    ret = gpio_direction_output(gpiobeep.beep_gpio, 1);
    if (ret < 0)
    {
        printk("can't set gpio!\r\n");
    }


    if (gpiobeep.major)
    {
        gpiobeep.devid = MKDEV(gpiobeep.major, 0);
        gpiobeep.minor = 0;
        register_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT, GPIOBEEP_NAME);
    }
    else
    {
        alloc_chrdev_region(&gpiobeep.devid, 0, GPIOBEEP_CNT, GPIOBEEP_NAME);
        gpiobeep.major = MAJOR(gpiobeep.devid);
        gpiobeep.minor = MINOR(gpiobeep.devid);
    }

    gpiobeep.cdev.owner = THIS_MODULE;
    cdev_init(&gpiobeep.cdev, &gpiobeep_fops);
    cdev_add(&gpiobeep.cdev, gpiobeep.devid, GPIOBEEP_CNT);

    gpiobeep.class = class_create(THIS_MODULE, GPIOBEEP_NAME);
    if (IS_ERR(gpiobeep.class))
    {
        return PTR_ERR(gpiobeep.class);
    }

    gpiobeep.device = device_create(gpiobeep.class, NULL, gpiobeep.devid, NULL, GPIOBEEP_NAME);
    if (IS_ERR(gpiobeep.device))
    {
        return PTR_ERR(gpiobeep.device);
    }

    return 0;
}

static void __exit gpiobeep_exit(void)
{
    cdev_del(&gpiobeep.cdev);
    unregister_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT);

    device_destroy(gpiobeep.class, gpiobeep.devid);
    class_destroy(gpiobeep.class);
}

module_init(gpiobeep_init);
module_exit(gpiobeep_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yebin");