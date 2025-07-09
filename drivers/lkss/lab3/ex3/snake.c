// SPDX-License-Identifier: GPL-2.0

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "snake.h"

#define SNAKE_MAJOR 53

static bool button_was_pressed(struct snake_data *data, int irq, int btn)
{
	struct gpio_desc *gpio;

	switch (btn) {
	case UP:
		gpio = data->up;
		break;
	case DOWN:
		gpio = data->down;
		break;
	case LEFT:
		gpio = data->left;
		break;
	case RIGHT:
		gpio = data->right;
		break;
	default:
		return false;
	}

	return (irq == gpiod_to_irq(gpio));
}

irqreturn_t snake_isr(int irq, void *dev_id)
{
	struct snake_data *data = dev_id;

	if (button_was_pressed(data, irq, UP)) {
		queue_button(&data->btn_q, UP);
	} else if (button_was_pressed(data, irq, DOWN)) {
		queue_button(&data->btn_q, DOWN);
	} else if (button_was_pressed(data, irq, LEFT)) {
		queue_button(&data->btn_q, LEFT);
	} else if (button_was_pressed(data, irq, RIGHT)) {
		queue_button(&data->btn_q, RIGHT);
	} else {
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static int snake_open(struct inode *inode, struct file *file)
{
	unsigned long flags;
	struct snake_data *data =
		container_of(inode->i_cdev, struct snake_data, cdev);

	file->private_data = data;

     	spin_lock_irqsave(&data->btn_q.lock, flags);

	data->btn_q.head = 0;
	data->btn_q.tail = 0;
	data->btn_q.size = 0;

	spin_unlock_irqrestore(&data->btn_q.lock, flags);
	
	return 0;
}

static int snake_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t snake_read(struct file *file, char __user *buf,
			  size_t size, loff_t *offset)
{
	struct snake_data *data = file->private_data;
	int btn;

	if (button_queue_is_empty(&data->btn_q))
		btn = NONE;
	else
		btn = dequeue_button(&data->btn_q);

	if (copy_to_user(buf, &btn, sizeof(int)))
		return -EFAULT;

	*offset += 4;

	return 4;
}

static struct file_operations snake_fops = {
	.owner = THIS_MODULE,
	.open = snake_open,
	.release = snake_release,
	.read = snake_read,
};

static int snake_probe(struct platform_device *pdev)
{
	struct snake_data *data;
	struct device *dev;
	int ret;

	dev = &pdev->dev;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "failed to allocate SNAKE data\n");
		return -ENOMEM;
	}

	data->dev = &pdev->dev;
	dev_set_drvdata(dev, data);
	button_queue_init(&data->btn_q);

	data->up = gpiod_get(&pdev->dev, "up", GPIOD_IN);
	if (IS_ERR(data->up)) {
		ret = PTR_ERR(data->up);
		dev_err(dev, "failed to get UP GPIO\n");
		goto err_data_free;
	}
	data->down = gpiod_get(&pdev->dev, "down", GPIOD_IN);
	if (IS_ERR(data->down)) {
		ret = PTR_ERR(data->down);
		dev_err(dev, "failed to get DOWN GPIO\n");
		goto err_up_put;
	}

	data->left = gpiod_get(&pdev->dev, "left", GPIOD_IN);
	if (IS_ERR(data->left)) {
		ret = PTR_ERR(data->left);
		dev_err(dev, "failed to get LEFT GPIO\n");
		goto err_down_put;
	}

	data->right = gpiod_get(&pdev->dev, "right", GPIOD_IN);
	if (IS_ERR(data->right)) {
		ret = PTR_ERR(data->right);
		dev_err(dev, "failed to get RIGHT GPIO\n");
		goto err_left_put;
	}

	ret = snake_request_gpio_irq(data, data->up, "snake_up_irq");
	if (ret) {
		dev_err(dev, "failed to request UP IRQ\n");
		goto err_right_put;
	}

	ret = snake_request_gpio_irq(data, data->down, "snake_down_irq");
	if (ret) {
		dev_err(dev, "failed to request DOWN IRQ\n");
		goto err_up_irq_release;
	}

	ret = snake_request_gpio_irq(data, data->left, "snake_left_irq");
	if (ret) {
		dev_err(dev, "failed to request LEFT IRQ\n");
		goto err_down_irq_release;
	}

	ret = snake_request_gpio_irq(data, data->right, "snake_right_irq");
	if (ret) {
		dev_err(dev, "failed to request RIGHT IRQ\n");
		goto err_left_irq_release;
	}

	ret = register_chrdev_region(MKDEV(SNAKE_MAJOR, 0), 1, "snake");
	if (ret) {
		dev_err(dev, "failed to register chrdev region\n");
		goto err_right_irq_release;
	}

	cdev_init(&data->cdev, &snake_fops);
	cdev_add(&data->cdev, MKDEV(SNAKE_MAJOR, 0), 1);

	dev_info(&pdev->dev, "SNAKE probed successfully with MAJOR 53 and minor 0!\n");

	return 0;

err_right_irq_release:
	snake_free_gpio_irq(data, data->right);
err_left_irq_release:
	snake_free_gpio_irq(data, data->left);
err_down_irq_release:
	snake_free_gpio_irq(data, data->down);
err_up_irq_release:
	snake_free_gpio_irq(data, data->up);
err_right_put:
	gpiod_put(data->right);
err_left_put:
	gpiod_put(data->left);
err_down_put:
	gpiod_put(data->down);
err_up_put:
	gpiod_put(data->up);
err_data_free:
	kfree(data);

	return ret;
}

static void snake_remove(struct platform_device *pdev)
{
	struct snake_data *data = dev_get_drvdata(&pdev->dev);

	snake_free_gpio_irq(data, data->up);
	snake_free_gpio_irq(data, data->down);
	snake_free_gpio_irq(data, data->left);
	snake_free_gpio_irq(data, data->right);

	cdev_del(&data->cdev);
	unregister_chrdev_region(MKDEV(SNAKE_MAJOR, 0), 1);
}

static const struct of_device_id snake_of_ids[] = {
	{ .compatible = "lkss,snake" },
	{ }
};
MODULE_DEVICE_TABLE(of, snake_of_ids);

static struct platform_driver snake_of_driver = {
	.probe = snake_probe,
	.remove = snake_remove,
	.driver = {
		.name = "snake",
		.of_match_table = snake_of_ids,
	},
};
module_platform_driver(snake_of_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("Snake driver");
