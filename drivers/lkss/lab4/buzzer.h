#ifndef _BUZZER_H_
#define _BUZZER_H_

#include <linux/jiffies.h>

#define BUZZER_MAJOR 50
#define FREQ_TO_PERIOD_NS(freq) (NSEC_PER_SEC / freq)

struct buzzer_note {
	u32 freq;
	u32 beats;
};

struct buzzer_song {
	u32 bpm;
	u32 note_count;
#define MAX_NUM_NOTES 200
	struct buzzer_note notes[MAX_NUM_NOTES];
};

struct buzzer_data {
	struct cdev cdev;
	struct timer_list timer;
	struct completion completion;
	struct device *dev;
	struct pwm_device *pwm_dev;
#define BUFSZ 4096
	char buffer[BUFSZ];
	bool open;
	struct buzzer_song song;
	u32 crt_note;
};

/* compute the amount of jiffies until the next timer re-trigger
 *
 * beats - number of beats a note lasts for
 * bpm - song bpm
 */
static inline unsigned long get_next_jiffies(u32 beats, u32 bpm)
{
#define DURATION_SCALE_FACTOR 100
	return jiffies + (HZ * beats * 60) / (bpm * DURATION_SCALE_FACTOR);
}

#endif /* _SIMPLE_BUZZER_H_ */
