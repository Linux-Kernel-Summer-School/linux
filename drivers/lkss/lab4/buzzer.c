// SPDX-License-Identifier: GPL-2.0

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#include "buzzer.h"

static void timer_fn(struct timer_list *timer)
{
	struct buzzer_data *data;
	struct buzzer_note *note;
	struct pwm_state pwm_state;
	unsigned long next_jiffies;
	int ret;

	data = container_of(timer, struct buzzer_data, timer);

	pwm_disable(data->pwm_dev);

	/* song is over, signal write() to proceed */
	if (data->crt_note == data->song.note_count) {
		complete(&data->completion);
		return;
	}

	note = &data->song.notes[data->crt_note];

	if (note->freq) {
		pwm_init_state(data->pwm_dev, &pwm_state);

		/* convert note's freq (hertz) into PWM period (ns) */
		pwm_state.period = FREQ_TO_PERIOD_NS(note->freq);
		pwm_state.duty_cycle = pwm_state.period / 2;

		ret = pwm_apply_might_sleep(data->pwm_dev, &pwm_state);
		if (ret) {
			dev_err(data->dev, "failed to apply PWM state: %d\n", ret);
			complete(&data->completion);
			return;
		}

		pwm_enable(data->pwm_dev);
	}

	data->crt_note++;

	/* don't forget to re-arm timer */
	next_jiffies = get_next_jiffies(note->beats, data->song.bpm);

	ret = mod_timer(&data->timer, next_jiffies);
	if (ret) {
		dev_err(data->dev, "failed to re-arm timer: %d\n", ret);
		complete(&data->completion);
		return;
	}
}

static int buzzer_open(struct inode *inode, struct file *file)
{
	struct buzzer_data *data =
		container_of(inode->i_cdev, struct buzzer_data, cdev);

	file->private_data = data;

	return 0;
}

static int buzzer_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t buzzer_write(struct file *file, const char __user *buf,
			    size_t len, loff_t *offset)
{
	struct buzzer_data *data = file->private_data;
	int i;

	if (len > BUFSZ) {
		dev_err(data->dev, "write length (%ld) exceeds buffer size (%d)\n",
			len, BUFSZ);
		return -EINVAL;
	}

	memset(data->buffer, 0, BUFSZ);

	if (copy_from_user(data->buffer, buf, len)) {
		dev_err(data->dev, "failed to transfer data from userspace\n");
		return -EFAULT;
	}

	/* parse data from userspace */
	data->song.bpm = *(u32 *)data->buffer;
	data->song.note_count = *(u32 *)(data->buffer + 4);

	if (data->song.note_count > MAX_NUM_NOTES) {
		dev_err(data->dev, "exceeded maximum note count\n");
		return -EINVAL;
	}

	for (i = 0; i < data->song.note_count; i++) {
		data->song.notes[i].freq =
			*(u32 *)(data->buffer + 8 + 8 * i);
		data->song.notes[i].beats =
			*(u32 *)(data->buffer + 8 + 8 * i + 4);
	}

	/* reset the node count before re-arming the timer */
	data->crt_note = 0;

	/* start the song immediately */
	mod_timer(&data->timer, jiffies);

	/* wait for its completion */
	wait_for_completion(&data->completion);

	/* was the song interrupted? */
	if (data->crt_note != data->song.note_count) {
		dev_err(data->dev, "could not play song\n");
		return -EFAULT;
	}

	*offset += len;

	return len;
}

static struct file_operations buzzer_fops = {
	.owner = THIS_MODULE,
	.open = buzzer_open,
	.release = buzzer_release,
	.write = buzzer_write,
};

static int buzzer_probe(struct platform_device *pdev)
{
	struct buzzer_data *data;
	struct device *dev;
	int ret;

	dev = &pdev->dev;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "failed to allocate BUZZER data\n");
		return -ENOMEM;
	}

	data->pwm_dev = pwm_get(&pdev->dev, NULL);
	if (IS_ERR(data->pwm_dev)) {
		ret = PTR_ERR(data->pwm_dev);
		dev_err(dev, "failed to get PWM handle\n");
		goto err_data_free;
	}

	data->dev = &pdev->dev;
	dev_set_drvdata(dev, data);

	ret = register_chrdev_region(MKDEV(BUZZER_MAJOR, 0), 1, "buzzer");
	if (ret) {
		dev_err(dev, "failed to register chrdev region\n");
		goto err_pwm_put;
	}

	cdev_init(&data->cdev, &buzzer_fops);
	cdev_add(&data->cdev, MKDEV(BUZZER_MAJOR, 0), 1);

	timer_setup(&data->timer, timer_fn, 0);
	init_completion(&data->completion);

	dev_info(&pdev->dev, "BUZZER probed successfully!\n");

	return 0;

err_pwm_put:
	pwm_put(data->pwm_dev);
err_data_free:
	kfree(data);

	return ret;
}

static void buzzer_remove(struct platform_device *pdev)
{
	struct buzzer_data *data = dev_get_drvdata(&pdev->dev);

	timer_delete_sync(&data->timer);

	cdev_del(&data->cdev);
	unregister_chrdev_region(MKDEV(BUZZER_MAJOR, 0), 1);

	pwm_disable(data->pwm_dev);
	pwm_put(data->pwm_dev);
}

static const struct of_device_id buzzer_of_ids[] = {
	{ .compatible = "lkss,buzzer" },
	{ }
};
MODULE_DEVICE_TABLE(of, buzzer_of_ids);

static struct platform_driver buzzer_of_driver = {
	.probe = buzzer_probe,
	.remove = buzzer_remove,
	.driver = {
		.name = "buzzer",
		.of_match_table = buzzer_of_ids,
	},
};
module_platform_driver(buzzer_of_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NXP Linux Kernel Summer School");
MODULE_DESCRIPTION("Buzzer driver");
