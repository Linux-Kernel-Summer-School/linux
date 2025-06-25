#include <linux/module.h>	/* Core header for loading LKMs into the kernel */
#include <linux/fs.h>		/* File operations structure and related functions */
#include <linux/cdev.h>		/* Character device support */
#include <linux/uaccess.h>	/* Functions for user space/kernel space access */

/* Define device name and class name */
#define DEVICE_NAME "reverse_char"
#define CLASS_NAME "rev_class"

static int major;/* Major number assigned to the driver */
static char message[256] = {0};/* Buffer to store user data */

static struct class* rev_class = NULL;/* Device class pointer */
static struct device* rev_device = NULL;/* Device structure pointer */

/**
 * Called when the device is opened
 */
static int dev_open(struct inode *inodep, struct file *filep) {
	printk(KERN_INFO "reverse_char: Device opened\n");
	return 0;
}

/**
 * Called when the device is closed
 */
static int dev_release(struct inode *inodep, struct file *filep) {
	printk(KERN_INFO "reverse_char: Device closed\n");
	return 0;
}

/**
 * Called when user reads from the device
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
	/* TODO 2: Copy data from kernel buffer to user buffer */
	return 0;
}

/**
 * Helper function to reverse a string in place
 */
static void reverse_string(char *str) {
	int i, j;
	char tmp;
	/* TODO 2: Reverse a string in place */
}

/**
 * Called when user writes to the device
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
	int err;

	/* Limit message length to prevent overflow */
	if (len > 255)
		len = 255;

	/* TODO 2: Copy data from user space to kernel buffer */

	/* TODO 2:  Null-terminate the string */

	/* TODO 2: Reverse the message in place */

	printk(KERN_INFO "reverse_char: Received and reversed: %s\n", message);

	return len;/* Return number of bytes written */
}

/**
 * File operations structure for this driver
 */
static struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release
};

/**
 * Initialization function for the module
 */
static int __init reverse_init(void) {
	/* TODO 3: Register character device and get dynamically assigned major number */

	/* TODO 3: Create a device class in sysfs (/sys/class/rev_class) */

	/* TODO 3: Create the device file (/dev/reverse_char) */

	printk(KERN_INFO "reverse_char: Device initialized\n");
	return 0;
}

/**
 * Cleanup function for the module
 */
static void __exit reverse_exit(void) {
	/* TODO 4: Remove /dev entry, use device_destroy() */
	/* TODO 4: Unregister class */
	/* TODO 4: Destroy class */
	/* TODO 4: Unregister char device */
	printk(KERN_INFO "reverse_char: Goodbye!\n");
}

module_init(reverse_init);
module_exit(reverse_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("Lab2 Ex2: Char driver that reverses input");
