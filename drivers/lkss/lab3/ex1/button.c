#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>

static struct gpio_desc *button_gpio;

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
	/*
	 * TODO: Count how many time the button was pressed and print the info
	 * each time
	 */

	return IRQ_HANDLED;
}

static int gpio_button_probe(struct platform_device *pdev)
{
	int ret, irq;

	dev_info(&pdev->dev, "gpio_button_probe()\n");

	button_gpio = devm_gpiod_get(&pdev->dev, NULL, GPIOD_IN);
	if (IS_ERR(button_gpio)) {
		dev_err(&pdev->dev, "Failed to get GPIO\n");
		return PTR_ERR(button_gpio);
	}

	irq = gpiod_to_irq(button_gpio);

	ret = devm_request_irq(&pdev->dev, irq, button_irq_handler, IRQF_TRIGGER_FALLING,
			       "button_irq", NULL);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request_irq\n");
		return ret;
	}

	return 0;
}

static void gpio_button_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "gpio_button_remove()\n");
}

static const struct of_device_id gpio_button_dt_ids[] = {
	{ .compatible = "lkss,gpio-button", },
	{ /* sentinel */ },
};

static struct platform_driver gpio_button_driver = {
	.probe = gpio_button_probe,
	.remove = gpio_button_remove,
	.driver = {
		.name = "gpio_button",
		.of_match_table = gpio_button_dt_ids,
	},
};

MODULE_DEVICE_TABLE(of, gpio_button_dt_ids);

module_platform_driver(gpio_button_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("Lab3 Ex1: Simple button driver");
