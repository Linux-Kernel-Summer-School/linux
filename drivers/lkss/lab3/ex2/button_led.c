// SPDX-License-Identifier: GPL-2.0

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>

static struct gpio_desc *button_gpio, *led_gpio;

static irqreturn_t button_led_irq_handler(int irq, void *dev_id)
{
	/*TODO: toggle led each time button is pressed */

	return IRQ_HANDLED;
}

static int gpio_button_led_probe(struct platform_device *pdev)
{
	int ret, irq;

	dev_info(&pdev->dev, "gpio_button_led_probe()\n");

	button_gpio = devm_gpiod_get(&pdev->dev, "button", GPIOD_IN);
	if (IS_ERR(button_gpio)) {
		dev_err(&pdev->dev, "Failed to get button GPIO\n");
		return PTR_ERR(button_gpio);
	}

	led_gpio = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
	if (IS_ERR(led_gpio)) {
		dev_err(&pdev->dev, "Failed to get led GPIO\n");
		return PTR_ERR(led_gpio);
	}


	irq = gpiod_to_irq(button_gpio);

	ret = devm_request_irq(&pdev->dev, irq, button_led_irq_handler, IRQF_TRIGGER_FALLING,
			       "button_led_irq", NULL);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request_irq\n");
		return ret;
	}

	return 0;
}

static void gpio_button_led_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "gpio_button_led_remove()\n");
}

static const struct of_device_id gpio_button_led_dt_ids[] = {
	{ .compatible = "lkss,gpio-button-led", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, gpio_button_led_dt_ids);

static struct platform_driver gpio_button_led_driver = {
	.probe = gpio_button_led_probe,
	.remove = gpio_button_led_remove,
	.driver = {
		.name = "gpio_button_led",
		.of_match_table = gpio_button_led_dt_ids,
	},
};
module_platform_driver(gpio_button_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("Lab3 Ex1: Simple button_led driver");
