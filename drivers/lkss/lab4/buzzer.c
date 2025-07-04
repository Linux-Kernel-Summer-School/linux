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
	unsigned long next_jiffies;
	struct buzzer_data *data;
	struct buzzer_note *note;

	data = container_of(timer, struct buzzer_data, timer);

	/* TODO 5: disable the PWM */

	/* song is over, signal write() to proceed */
	if (data->crt_note == data->song.note_count) {
		complete(&data->completion);
		return;
	}

	note = &data->song.notes[data->crt_note];

	/* TODO 5: play the current note */
	/* TODO 5: increment the index of the currently played note */

	/* compute the time at which the timer needs to be re-triggered */
	next_jiffies = get_next_jiffies(note->beats, data->song.bpm);

	/* TODO 6: re-arm the timer */
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

	/* TODO 3: parse the song provided by the user
	 *
	 * The data provided by the user has the following
	 * format:
	 *
	 * 0x0: BPM (4 bytes)
	 * 0x4: NOTE_COUNT (4 bytes)
	 * (0x8 + i * 0x8): FREQUENCY (4 bytes)
	 * (0x8 + i * 0x8 + 0x4): BEATS (4 bytes)
	 */

	/* TODO 4: reset the currently played note to 0 */
	/* TODO 4: trigger the timer immediately */

	/* wait for song completion */
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

	/* TODO 1: request the PWM channel */

	data->dev = &pdev->dev;
	dev_set_drvdata(dev, data);

	ret = register_chrdev_region(MKDEV(BUZZER_MAJOR, 0), 1, "buzzer");
	if (ret) {
		dev_err(dev, "failed to register chrdev region\n");
		goto err_pwm_put;
	}

	cdev_init(&data->cdev, &buzzer_fops);
	cdev_add(&data->cdev, MKDEV(BUZZER_MAJOR, 0), 1);

	/* TODO 2: initialize the timer */

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

	/* TODO 7: disable the PWM channel */
	/* TODO 1: release the PWM channel */

	kfree(data);
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
