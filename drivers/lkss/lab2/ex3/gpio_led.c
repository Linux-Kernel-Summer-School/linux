#include <linux/of.h>			/* For Open Firmware (Device Tree) support */
#include <linux/platform_device.h>	/* For platform device/driver APIs */
#include <linux/cdev.h>			/* For character device support */
#include <linux/gpio/consumer.h>	/* For GPIO descriptor (gpiod_*) API */

/* Name of the character device node */
#define DEVICE_NAME "gpio_led"

/* Global variables for device components */
static struct gpio_desc *led_gpio;	/* Pointer to GPIO descriptor for the LED */
static dev_t dev_num;			/* Device number for character device */
static struct cdev my_cdev;		/* Character device structure */
static struct class *led_class;		/* Device class for sysfs entry */

/**
 * Write function: control LED (on/off)
 */
static ssize_t gpio_led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char kbuf[2];/* Kernel buffer to store one char and null terminator */

	if (copy_from_user(kbuf, buf, 1))/* Copy 1 byte from user space */
		return -EFAULT;

	kbuf[1] = '\0';/* Null-terminate the string */

	if (kbuf[0] == '1')/* If user wrote '1', turn LED on */
		gpiod_set_value(led_gpio, 1);
	else/* Otherwise, turn LED off */
		gpiod_set_value(led_gpio, 0);

	return count;/* Return number of bytes written */
}

/**
 *  Read function: get LED state ("on\n" or "off\n")
 */
static ssize_t gpio_led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	const char *state_str;/* String to return LED state */
	size_t len;/* Length of the string */
	int state;/* Current GPIO state */

	if (*ppos > 0)/* Return 0 on subsequent reads (EOF) */
		return 0;

	state = gpiod_get_value(led_gpio);/* Read actual GPIO value (0 or 1) */

	if (state)/* Convert value to string */
		state_str = "on\n";
	else
		state_str = "off\n";

	len = strlen(state_str) + 1;/* Length including null terminator */

	if (copy_to_user(buf, state_str, len))/* Copy state string to user space */
		return -EFAULT;

	*ppos = len;/* Update file position to mark EOF */
	return len;/* Return number of bytes read */
}

/**
 * File operations structure for the character device
 */
static struct file_operations fops = {
	.owner = THIS_MODULE,	/* Points to the current module */
	.write = gpio_led_write,/* Assign write handler */
	.read = gpio_led_read,	/* Assign read handler */
};

/**
 * Probe function: called when the driver is matched with a device
 */
static int gpio_led_probe(struct platform_device *pdev)
{
	int ret;

	/* Request GPIO associated with device node (default: output low) */
	led_gpio = gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
	if (IS_ERR(led_gpio)) {
		dev_err(&pdev->dev, "Failed to get GPIO\n");
		return PTR_ERR(led_gpio);
	}

	/* Allocate device number dynamically */
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if (ret)
		goto err_free_gpio;

	/* Initialize character device with file operations */
	cdev_init(&my_cdev, &fops);
	my_cdev.owner = THIS_MODULE;

	/* Add character device to the system */
	ret = cdev_add(&my_cdev, dev_num, 1);
	if (ret)
		goto err_unregister_chrdev;

	/* Create device class in sysfs (/sys/class) */
	led_class = class_create("led_class");
	if (IS_ERR(led_class)) {
		ret = PTR_ERR(led_class);
		goto err_del_cdev;
	}

	/* Create device node in /dev (e.g., /dev/gpio_led) */
	device_create(led_class, NULL, dev_num, NULL, DEVICE_NAME);
	dev_info(&pdev->dev, "LED driver loaded\n");

	return 0;

	/* Error handling (reverse order of allocation) */
err_del_cdev:
	cdev_del(&my_cdev);
err_unregister_chrdev:
	unregister_chrdev_region(dev_num, 1);
err_free_gpio:
	gpiod_put(led_gpio);
	return ret;
}

/**
 * Remove function: cleanup when device is removed/unregistered
 */
static void gpio_led_remove(struct platform_device *pdev)
{
	device_destroy(led_class, dev_num);	/* Remove /dev/gpio_led */
	class_destroy(led_class);		/* Remove class from /sys/class */
	cdev_del(&my_cdev);			/* Remove character device */
	unregister_chrdev_region(dev_num, 1);	/* Free device number */
	gpiod_put(led_gpio);			/* Release GPIO */
}

/**
 * Device tree match table
 */
static const struct of_device_id gpio_led_dt_ids[] = {
	{ .compatible = "lkss,gpio-led", }, /* Matches with Device Tree entry */
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, gpio_led_dt_ids);/* Expose match table to userspace */

/**
 * Platform driver structure
 */
static struct platform_driver gpio_led_driver = {
	.probe = gpio_led_probe,/* Called when device is found */
	.remove = gpio_led_remove,/* Called when device is removed */
	.driver = {
		.name = "gpio-led",/* Name of the driver */
		.of_match_table = gpio_led_dt_ids,/* Device Tree matching */
	},
};

module_platform_driver(gpio_led_driver);/* Register platform driver */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("Lab2 Ex3: Character device LED driver using gpiod");
