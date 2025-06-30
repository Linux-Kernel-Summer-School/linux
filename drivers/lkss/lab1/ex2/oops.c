// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int __init lkss_oops_init(void)
{
	int *p = NULL;

	*p = 0x42;

	return 0;
}

static void __exit lkss_oops_exit(void)
{
	pr_info("Goodbye from kernel space!\n");
}

module_init(lkss_oops_init);
module_exit(lkss_oops_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("A simple module to demonstrate an OOPS");
