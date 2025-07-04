// SPDX-License-Identifier: GPL-2.0

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#define RGB_LED_MAJOR 52

#define RGB_RED_SET_DUTY_CYCLE		_IOW('l', 0x0, unsigned int)
#define RGB_BLUE_SET_DUTY_CYCLE		_IOW('l', 0x1, unsigned int)
#define RGB_GREEN_SET_DUTY_CYCLE	_IOW('l', 0x2, unsigned int)

struct rgb_led_data {
	struct cdev cdev;
	struct device *dev;
	struct pwm_device *red;
	struct pwm_device *green;
	struct pwm_device *blue;
};

static int rgb_led_open(struct inode *inode, struct file *file)
{
	struct rgb_led_data *data =
		container_of(inode->i_cdev, struct rgb_led_data, cdev);

	file->private_data = data;

	return 0;
}

static int rgb_led_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long rgb_led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct rgb_led_data *data = file->private_data;
	struct pwm_state pwm_state;
	struct pwm_device *pwm_dev;
	unsigned int duty_cycle;
	int ret;

	switch (cmd) {
	case RGB_RED_SET_DUTY_CYCLE:
		pwm_dev = data->red;
		break;
	case RGB_BLUE_SET_DUTY_CYCLE:
		pwm_dev = data->blue;
		break;
	case RGB_GREEN_SET_DUTY_CYCLE:
		pwm_dev = data->green;
		break;
	default:
		dev_err(data->dev, "invalid command: 0x%x\n", cmd);
		return -EINVAL;
	}

	duty_cycle = arg;

	if (duty_cycle > 100) {
		dev_err(data->dev, "invalid duty cycle: %u\n", duty_cycle);
		return -EINVAL;
	}

	pwm_init_state(pwm_dev, &pwm_state);

	ret = pwm_set_relative_duty_cycle(&pwm_state, duty_cycle, 100);
	if (ret) {
		dev_err(data->dev, "failed to set relative duty cycle: %d\n", ret);
		return ret;
	}

	ret = pwm_apply_might_sleep(pwm_dev, &pwm_state);
	if (ret) {
		dev_err(data->dev, "failed to apply PWM state: %d\n", ret);
		return ret;
	}

	ret = pwm_enable(pwm_dev);
	if (ret) {
		dev_err(data->dev, "failed to enable PWM: %d\n", ret);
		return ret;
	}

	return 0;
}

static struct file_operations rgb_led_fops = {
	.owner = THIS_MODULE,
	.open = rgb_led_open,
	.release = rgb_led_release,
	.unlocked_ioctl = rgb_led_ioctl,
};

static int rgb_led_probe(struct platform_device *pdev)
{
	struct rgb_led_data *data;
	struct device *dev;
	int ret;

	dev = &pdev->dev;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "failed to allocate RGB LED data\n");
		return -ENOMEM;
	}

	data->red = pwm_get(&pdev->dev, "red");
	if (IS_ERR(data->red)) {
		ret = PTR_ERR(data->red);
		dev_err(dev, "failed to get RED PWM handle\n");
		goto err_data_free;
	}

	pwm_apply_args(data->red);

	data->blue = pwm_get(&pdev->dev, "blue");
	if (IS_ERR(data->blue)) {
		ret = PTR_ERR(data->blue);
		dev_err(dev, "failed to get BLUE PWM handle\n");
		goto err_red_put;
	}

	data->green = pwm_get(&pdev->dev, "green");
	if (IS_ERR(data->green)) {
		ret = PTR_ERR(data->green);
		dev_err(dev, "failed to get GREEN PWM handle\n");
		goto err_blue_put;
	}

	data->dev = &pdev->dev;
	dev_set_drvdata(dev, data);

	ret = register_chrdev_region(MKDEV(RGB_LED_MAJOR, 0), 1, "rgb-led");
	if (ret) {
		dev_err(dev, "failed to register chrdev region\n");
		goto err_green_put;
	}

	cdev_init(&data->cdev, &rgb_led_fops);
	cdev_add(&data->cdev, MKDEV(RGB_LED_MAJOR, 0), 1);

	dev_info(&pdev->dev, "RGB LED probed successfully!\n");

	return 0;

err_green_put:
	pwm_put(data->green);
err_blue_put:
	pwm_put(data->blue);
err_red_put:
	pwm_put(data->red);
err_data_free:
	kfree(data);

	return ret;
}

static void rgb_led_remove(struct platform_device *pdev)
{
	struct rgb_led_data *data = dev_get_drvdata(&pdev->dev);

	cdev_del(&data->cdev);
	unregister_chrdev_region(MKDEV(RGB_LED_MAJOR, 0), 1);

	pwm_disable(data->red);
	pwm_disable(data->blue);
	pwm_disable(data->green);

	pwm_put(data->red);
	pwm_put(data->blue);
	pwm_put(data->green);

	kfree(data);
}

static const struct of_device_id rgb_led_of_ids[] = {
	{ .compatible = "lkss,pwm-rgb-led", },
	{ }
};
MODULE_DEVICE_TABLE(of, rgb_led_of_ids);

static struct platform_driver rgb_led_of_driver = {
	.probe = rgb_led_probe,
	.remove = rgb_led_remove,
	.driver = {
		.name = "pwm-rgb-led",
		.of_match_table = rgb_led_of_ids,
	},
};
module_platform_driver(rgb_led_of_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("RGB LED driver");
