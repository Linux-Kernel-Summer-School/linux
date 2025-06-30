// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define TIMER_INTERVAL_MS 1000  /* 1000 milliseconds = 1 second */

static struct timer_list my_timer;

/* Timer callback */
static void my_timer_callback(struct timer_list *t)
{
	pr_info("Timer callback executed at jiffies=%lu\n", jiffies);
	/* TODO schedule the timer again, using `mod_timer` function */

}

static int __init lkss_timer_init(void)
{
	pr_info("Initializing the timer module\n");

	/* Initialize the timer */
	timer_setup(&my_timer, my_timer_callback, 0);

	/* schedule the timer for the first time */
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS));
	return 0;
}

static void __exit lkss_timer_exit(void)
{
	/* Delete the timer if still active */
	timer_delete_sync(&my_timer);

	pr_info("Timer module exited\n");
}

module_init(lkss_timer_init);
module_exit(lkss_timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("A simple Linux kernel module using a timer.");
