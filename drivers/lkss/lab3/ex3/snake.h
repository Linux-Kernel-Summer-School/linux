#ifndef _SNAKE_H_
#define _SNAKE_H_

#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>

#define UP	0x0
#define DOWN	0x1
#define LEFT	0x2
#define RIGHT	0x3
#define NONE	0x4

struct snake_button_queue {
#define MAX_NUM_COMMANDS 100
	int mem[MAX_NUM_COMMANDS];
	int head;
	int tail;
	int size;
	spinlock_t lock;
};

struct snake_data {
	struct cdev cdev;
	struct device *dev;
	struct snake_button_queue btn_q;
	struct gpio_desc *up;
	struct gpio_desc *down;
	struct gpio_desc *left;
	struct gpio_desc *right;
};

irqreturn_t snake_isr(int irq, void *dev_id);

static inline int queue_button(struct snake_button_queue *q, int btn)
{
	unsigned long flags;

	spin_lock_irqsave(&q->lock, flags);

	if (q->size == MAX_NUM_COMMANDS) {
		spin_unlock_irqrestore(&q->lock, flags);
		return -EINVAL;
	}

	q->mem[q->tail] = btn;
	q->tail = (q->tail + 1) % MAX_NUM_COMMANDS;
	q->size++;

	spin_unlock_irqrestore(&q->lock, flags);

	return 0;
}

static inline int dequeue_button(struct snake_button_queue *q)
{
	unsigned long flags;
	int btn;

	spin_lock_irqsave(&q->lock, flags);

	if (!q->size) {
		spin_unlock_irqrestore(&q->lock, flags);
		return -EINVAL;
	}

	btn = q->mem[q->head];
	q->head = (q->head + 1) % MAX_NUM_COMMANDS;
	q->size--;

	spin_unlock_irqrestore(&q->lock, flags);

	return btn;
}

static inline void button_queue_init(struct snake_button_queue *q)
{
	spin_lock_init(&q->lock);
	q->head = 0;
	q->tail = 0;
	q->size = 0;
}

static inline bool button_queue_is_empty(struct snake_button_queue *q)
{
	unsigned long flags;
	bool is_empty;

	spin_lock_irqsave(&q->lock, flags);
	is_empty = (q->size == 0);
	spin_unlock_irqrestore(&q->lock, flags);

	return is_empty;
}

static inline int snake_request_gpio_irq(struct snake_data *data,
					 struct gpio_desc *gpio,
					 const char *name)
{
	int ret;

	ret = request_irq(gpiod_to_irq(gpio), snake_isr,
			  IRQF_TRIGGER_FALLING,
			  name, data);

	return ret;
}

static inline void snake_free_gpio_irq(struct snake_data *data,
				       struct gpio_desc *gpio)
{
	free_irq(gpiod_to_irq(gpio), data);
}

#endif /* _SNAKE_H_ */
