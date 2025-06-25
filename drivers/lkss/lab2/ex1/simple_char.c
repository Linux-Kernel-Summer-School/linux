#include <linux/module.h>	/* Core header for loading LKMs into the kernel */
#include <linux/fs.h>		/* File operations structure and related functions */
#include <linux/cdev.h>		/* Character device support */
#include <linux/uaccess.h>	/* Functions for user space/kernel space access */

/* Define the device name */
#define DEVICE_NAME "simple_char"
/* Store the device's major number */
static int major;

/**
 * The device open function that is called each time the device is opened
 *
 * @param inode A pointer to an inode object (defined in linux/fs.h)
 * @param file A pointer to a file object (defined in linux/fs.h)
 */
static int my_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "simple_char: Device opened\n");
	return 0;
}

/**
 * The device release function that is called whenever the device is
 * closed/released by the userspace program
 *
 * @param inode A pointer to an inode object (defined in linux/fs.h)
 * @param file A pointer to a file object (defined in linux/fs.h)
 */
static int my_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "simple_char: Device closed\n");
	return 0;
}

/**
 * This function is called whenever device is being read from user space
 * i.e. data is being sent from the device to the user.
 * In this case it uses the copy_to_user() function to
 * send the message string to the user and captures any errors.
 *
 * @param file A pointer to a file object (defined in linux/fs.h)
 * @param buf The pointer to the buffer to which this function writes the data
 * @param len The length of the buffer
 * @param offset The offset if required
 *
 * @return ssize_t Number of bytes read
 */
static ssize_t my_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
	char msg[] = "Hello from char driver!\n";/* Message to send to user space */
	int error_count = 0;

	/* copy_to_user has the format ( * to, *from, size) and returns 0 on success */
	error_count = copy_to_user(buf, msg, sizeof(msg));
	if (error_count == 0) {/* if true then we have succeeded */
		printk(KERN_INFO "Sent %zd characters to the user\n", len);
		return len;/* return number of bytes read */
	} else {
		printk(KERN_INFO "Failed to send %d characters to the user\n", error_count);
		return len - error_count;/* failed -> still return number of bytes read, but see copy_to_user return */
	}
}

/**
 * This function is called whenever the device is being written to from user space i.e.
 * data is sent to the device from the user.
 * In this case it uses the copy_from_user() function to
 * send the buffer string from the user and captures any errors.
 *
 * @param file A pointer to a file object
 * @param buf The buffer that contains the string to write to the device
 * @param len The length of the array of data that is being passed in the const char buffer
 * @param offset The offset if required
 *
 * @return ssize_t Number of bytes written
 */
static ssize_t my_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
	char kbuf[128] = {0};/* Kernel buffer to store data from user space */
	int err;

	/* Limit the length to prevent buffer overflow */
	if (len > 127)
		len = 127;

	/*
	 * Copy data from user space to kernel buffer
	 * copy_from_user has the format ( * to, *from, size) and returns 0 on success
	 */
	err = copy_from_user(kbuf, buf, len);
	if (err)
		return err;/* Return number of bytes that couldn't be copied */

	/* Log the received message */
	printk(KERN_INFO "simple_char: Received from user: %s\n", kbuf);

	return len;/* Return number of bytes written */
}

/**
 * Devices are represented as file structure in the kernel.
 * The file_operations structure from /linux/fs.h lists the callback functions
 * that you wish to associated with your file operations.
 * char devices usually implement open, read, write and release callbacks.
 */
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_release,
	.read = my_read,
	.write = my_write
};

/**
 * Module initialization function
 */
static int __init char_init(void)
{
	/* Register the character device and get a dynamic major number */
	major = register_chrdev(0, DEVICE_NAME, &fops);
	if (major < 0) {
		/* Registration failed */
		printk(KERN_ALERT "simple_char: Registering char device failed\n");
		return major;
	}
	/* Log the assigned major number */
	printk(KERN_INFO "simple_char: Registered with major number %d\n", major);
	return 0;
}

/**
 * Module cleanup function
 */
static void __exit char_exit(void)
{
	/* Unregister the character device */
	unregister_chrdev(major, DEVICE_NAME);
	printk(KERN_INFO "simple_char: Unregistered\n");
}

/**
 * A module must use the module_init() / module_exit() macros from linux/init.h,
 * which identify the initialization function at insertion time and the cleanup
 * function (as listed above)
 */
module_init(char_init);
module_exit(char_exit);

/**
 * Module metadata
 */
MODULE_LICENSE("GPL");/* License type */
MODULE_AUTHOR("NXP Linux Kernel Summer School");/* The author - visible when you use modinfo */
MODULE_DESCRIPTION("Lab2 Ex1: Simple Char Driver");/* The description - see modinfo */
