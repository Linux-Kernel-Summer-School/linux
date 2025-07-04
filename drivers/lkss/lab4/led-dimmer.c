// SPDX-License-Identifier: GPL-2.0

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#define LED_DIMMER_MAJOR 51

#define LED_SET_DUTY_CYCLE	_IOW('l', 0x3, unsigned int)

struct led_dimmer_data {
	struct cdev cdev;
	struct pwm_device *pwm_dev;
	struct device *dev;
};

static int led_dimmer_open(struct inode *inode, struct file *file)
{
	struct led_dimmer_data *data =
		container_of(inode->i_cdev, struct led_dimmer_data, cdev);

	file->private_data = data;

	return 0;
}

static int led_dimmer_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long led_dimmer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct led_dimmer_data *data = file->private_data;
	struct pwm_state pwm_state;
	unsigned int duty_cycle;
	int ret;

	if (cmd != LED_SET_DUTY_CYCLE) {
		dev_err(data->dev, "invalid command: 0x%x\n", cmd);
		return -EINVAL;
	}

	if (duty_cycle > 100) {
		dev_err(data->dev, "invalid duty cycle: %u\n", duty_cycle);
		return -EINVAL;
	}

	pwm_init_state(data->pwm_dev, &pwm_state);

	ret = pwm_set_relative_duty_cycle(&pwm_state, duty_cycle, 100);
	if (ret) {
		dev_err(data->dev, "failed to set relative duty cycle: %d\n", ret);
		return ret;
	}

	ret = pwm_apply_might_sleep(data->pwm_dev, &pwm_state);
	if (ret) {
		dev_err(data->dev, "failed to apply PWM state: %d\n", ret);
		return ret;
	}

	ret = pwm_enable(data->pwm_dev);
	if (ret) {
		dev_err(data->dev, "failed to enable PWM: %d\n", ret);
		return ret;
	}

	return 0;
}

static struct file_operations led_dimmer_fops = {
	.owner = THIS_MODULE,
	.open = led_dimmer_open,
	.release = led_dimmer_release,
	.unlocked_ioctl = led_dimmer_ioctl,
};

static int led_dimmer_probe(struct platform_device *pdev)
{
	struct led_dimmer_data *data;
	struct device *dev;
	int ret;

	dev = &pdev->dev;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "failed to allocate LED dimmer data\n");
		return -ENOMEM;
	}

	data->pwm_dev = pwm_get(dev, NULL);
	if (IS_ERR(data->pwm_dev)) {
		ret = PTR_ERR(data->pwm_dev);
		dev_err(dev, "failed to get PWM handle\n");
		goto err_data_free;
	}

	data->dev = dev;
	dev_set_drvdata(dev, data);

	ret = register_chrdev_region(MKDEV(LED_DIMMER_MAJOR, 0), 1, "led-dimmer");
	if (ret) {
		dev_err(dev, "failed to register chrdev region\n");
		goto err_pwm_put;
	}

	cdev_init(&data->cdev, &led_dimmer_fops);
	cdev_add(&data->cdev, MKDEV(LED_DIMMER_MAJOR, 0), 1);

	dev_info(dev, "LED dimmer probed successfully!\n");

	return 0;

err_pwm_put:
	pwm_put(data->pwm_dev);
err_data_free:
	kfree(data);

	return ret;
}

static void led_dimmer_remove(struct platform_device *pdev)
{
	struct led_dimmer_data *data = dev_get_drvdata(&pdev->dev);

	cdev_del(&data->cdev);
	unregister_chrdev_region(MKDEV(LED_DIMMER_MAJOR, 0), 1);

	pwm_disable(data->pwm_dev);
	pwm_put(data->pwm_dev);

	kfree(data);
}

static const struct of_device_id led_dimmer_of_ids[] = {
	{ .compatible = "lkss,led-dimmer", },
	{ }
};
MODULE_DEVICE_TABLE(of, led_dimmer_of_ids);

static struct platform_driver led_dimmer_of_driver = {
	.probe = led_dimmer_probe,
	.remove = led_dimmer_remove,
	.driver = {
		.name = "led-dimmer",
		.of_match_table = led_dimmer_of_ids,
	},
};
module_platform_driver(led_dimmer_of_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("LED dimmer driver");
