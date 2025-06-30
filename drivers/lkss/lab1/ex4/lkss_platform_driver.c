// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>

static int lkss_sample_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const char *mystr;
	u32 myint;

	dev_info(dev, "lkss sample platform driver probed\n");

	if (of_property_read_string(dev->of_node, "mystring", &mystr) == 0)
		dev_info(dev, "mystring: %s\n", mystr);
	else
		dev_warn(dev, "Failed to read 'mystring'\n");

	if (of_property_read_u32(dev->of_node, "myint", &myint) == 0)
		dev_info(dev, "myint: %u\n", myint);
	else
		dev_warn(dev, "Failed to read 'myint'\n");

	/* TODO: read `sample-name` and `sample-size` properties */
	return 0;
}

static void lkss_sample_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "lkss device removed\n");
}

static const struct of_device_id lkss_sample_of_match[] = {
	{ .compatible = "lkss,sample-device" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, lkss_sample_of_match);

static struct platform_driver lkss_sample_driver = {
	.driver = {
		.name = "lkss_sample_platform_driver",
		.of_match_table = lkss_sample_of_match,
	},
	.probe = lkss_sample_probe,
	.remove = lkss_sample_remove,
};

static int __init lkss_sample_init(void)
{
	return platform_driver_register(&lkss_sample_driver);
}

static void __exit lkss_sample_exit(void)
{
	platform_driver_unregister(&lkss_sample_driver);
}

module_init(lkss_sample_init);
module_exit(lkss_sample_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("A simple sample platform driver");
