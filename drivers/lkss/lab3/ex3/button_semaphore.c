#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>

static struct gpio_desc *button_gpio, *red_gpio; /* TODO: Add *yellow_gpio, *green_gpio */

static irqreturn_t button_semaphore_irq_handler(int irq, void *dev_id)
{
	/*
	 * Cycle through Red, Yellow and Green leds at each button press
	 * like a semaphore.
	 */

	return IRQ_HANDLED;
}

static int gpio_button_semaphore_probe(struct platform_device *pdev)
{
	int ret, irq;

	dev_info(&pdev->dev, "gpio_button_semaphore_probe()\n");

	button_gpio = devm_gpiod_get_index(&pdev->dev, NULL, 0, GPIOD_IN);
	if (IS_ERR(button_gpio)) {
		dev_err(&pdev->dev, "Failed to get button GPIO\n");
		return PTR_ERR(button_gpio);
	}

	red_gpio = devm_gpiod_get_index(&pdev->dev, NULL, 1, GPIOD_OUT_LOW);
	if (IS_ERR(red_gpio)) {
		dev_err(&pdev->dev, "Failed to get red led GPIO\n");
		return PTR_ERR(red_gpio);
	}

	/* TODO:
	 * - Get a reference to Yellow and Green gpio leds
	 */
	irq = gpiod_to_irq(button_gpio);

	ret = devm_request_irq(&pdev->dev, irq, button_semaphore_irq_handler, IRQF_TRIGGER_FALLING,
			       "button_irq", NULL);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request_irq\n");
		return ret;
	}

	return 0;
}

static void gpio_button_semaphore_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "gpio_button_semaphore_remove()\n");
}

static const struct of_device_id gpio_button_semaphore_dt_ids[] = {
	{ .compatible = "lkss,gpio-button-semaphore", },
	{ /* sentinel */ },
};

static struct platform_driver gpio_button_semaphore_driver = {
	.probe = gpio_button_semaphore_probe,
	.remove = gpio_button_semaphore_remove,
	.driver = {
		.name = "gpio_button_semaphore",
		.of_match_table = gpio_button_semaphore_dt_ids,
	},
};

MODULE_DEVICE_TABLE(of, gpio_button_semaphore_dt_ids);

module_platform_driver(gpio_button_semaphore_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("Lab3 Ex3: Button controlled semaphore");
