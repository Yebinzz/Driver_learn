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

#define LED_ON 1
#define LED_OFF 0

#define GPIOLED_CNT 1
#define GPIOLED_NAME "gpioled"

struct gpioled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int led_gpio;
    atomic_t lock;
};

struct gpioled_dev gpioled;

int gpioled_open(struct inode *inode, struct file *flip)
{
    if(!atomic_dec_and_test(&gpioled.lock))
    {
        atomic_inc(&gpioled.lock);
        return -EBUSY;
    }
    flip->private_data = &gpioled;

    return 0;
}
ssize_t gpioled_read(struct file *flip, char __user *buf, size_t cnt, loff_t *oof_t)
{

    return 0;
}
ssize_t gpioled_write(struct file *flip, const char __user *buf, size_t cnt, loff_t *off_t)
{
    struct gpioled_dev *dev = flip->private_data;
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
        gpio_set_value(dev->led_gpio, 0);
    }
    else if (ledbuf[0] == LED_OFF)
    {
        gpio_set_value(dev->led_gpio, 1);
    }

    return 0;
}
int gpioled_release(struct inode *inode, struct file *flip)
{
    struct gpioled_dev *dev = flip->private_data;

    atomic_inc(&dev->lock);
    return 0;
}

struct file_operations gpioled_fops = {
    .owner = THIS_MODULE,
    .open = gpioled_open,
    .read = gpioled_read,
    .write = gpioled_write,
    .release = gpioled_release};

static int __init gpioled_init(void)
{
    int ret;
    atomic_set(&gpioled.lock, 1);
    gpioled.nd = of_find_node_by_path("/gpioled");
    if (gpioled.nd == NULL)
    {
        printk("gpioled node can not be found\r\n");
        return -EINVAL;
    }
    else
    {
        printk("gpioled node has been found!\r\n");
    }

    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpio", 0);
    if (gpioled.led_gpio < 0)
    {
        printk("can't get led-gpio!\r\n");
        return -EINVAL;
    }
    else
    {
        printk("led-gpio = %d\r\n", gpioled.led_gpio);
    }

    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if (ret < 0)
    {
        printk("can't set gpio!\r\n");
    }


    if (gpioled.major)
    {
        gpioled.devid = MKDEV(gpioled.major, 0);
        gpioled.minor = 0;
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }

    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &gpioled_fops);
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if (IS_ERR(gpioled.class))
    {
        return PTR_ERR(gpioled.class);
    }

    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if (IS_ERR(gpioled.device))
    {
        return PTR_ERR(gpioled.device);
    }

    return 0;
}

static void __exit gpioled_exit(void)
{
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);
}

module_init(gpioled_init);
module_exit(gpioled_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yebin");