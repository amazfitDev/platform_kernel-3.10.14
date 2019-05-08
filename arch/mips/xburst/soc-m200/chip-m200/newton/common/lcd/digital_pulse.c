/*
 *  Copyright (C) 2015 Wu Jiao <jiao.wu@ingenic.com wujiaososo@qq.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>

#include <linux/gpio.h>

static DEFINE_SPINLOCK(lock);
static DEFINE_MUTEX(mutex);

struct digital_pulse {
	/*
	 * digital pulse timeing，unit(us) ,details see datasheet !!!
	 */
	unsigned int t_init;
	unsigned int t_ss;
	unsigned int t_off;
	unsigned int t_high;
	unsigned int t_low;
	unsigned int t_store;
	unsigned int t_set;

	/* gpio, default high enable */
	unsigned int gpio;
	unsigned int low_enable;
};

static inline void set_high(struct digital_pulse *dp) {
	gpio_direction_output(dp->gpio, dp->low_enable ? 0 : 1);
}

static inline void set_low(struct digital_pulse *dp) {
	gpio_direction_output(dp->gpio, dp->low_enable ? 1 : 0);
}

void digital_pulse_power_on(struct digital_pulse *dp, unsigned int level) {
	unsigned long flags;
	unsigned int i;

	spin_lock_irqsave(&lock, flags);
	/* shut down it first */
	set_low(dp);
	udelay(dp->t_off);

	/* t_init and t_ss */
	set_high(dp);
	udelay(dp->t_init);
	udelay(dp->t_ss);

	/* send the pulses */
	for (i = 0; i < level; ++i) {
		set_low(dp);
		udelay(dp->t_low);
		set_high(dp);
		udelay(dp->t_high);
	}

	/* unlock irq */
	spin_unlock_irqrestore(&lock, flags);

	/* store */
	if (dp->t_store > dp->t_high)
	udelay(dp->t_store - dp->t_high);

	/* wait it work */
	udelay(dp->t_set);

}

void __digital_pulse(struct digital_pulse *dp, unsigned int level) {
	unsigned int i;
	/* send the pulses */
	for (i = 0; i < level; ++i) {
		set_low(dp);
		udelay(dp->t_low);
		set_high(dp);
		udelay(dp->t_high);
	}
 }

void digital_pulse_power_off(struct digital_pulse *dp) {
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	/* shut down it first */
	set_low(dp);
	udelay(dp->t_off);
	spin_unlock_irqrestore(&lock, flags);
}

struct digital_pulse tps65137_digital_pulse = {
	.t_init = 350,
	.t_ss = 1000,
	.t_off = 70,
	.t_high = 15,
	.t_low = 15,
	.t_store = 70,
	.t_set = 20 * 1000,
};

void tps65137_digital_pulse_power_on(int gpio, int low_enable, unsigned int level) {
	mutex_lock(&mutex);
	tps65137_digital_pulse.gpio = gpio;
	tps65137_digital_pulse.low_enable = low_enable;

	digital_pulse_power_on(&tps65137_digital_pulse, level);
	mutex_unlock(&mutex);
}

void __tps65137_digital_pulse(int gpio, int low_enable, unsigned int level) {
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);

	tps65137_digital_pulse.gpio = gpio;
	tps65137_digital_pulse.low_enable = low_enable;

	__digital_pulse(&tps65137_digital_pulse, level);

	spin_unlock_irqrestore(&lock, flags);
}

void tps65137_digital_pulse_power_off(int gpio, int low_enable) {
	mutex_lock(&mutex);
	tps65137_digital_pulse.gpio = gpio;
	tps65137_digital_pulse.low_enable = low_enable;

	digital_pulse_power_off(&tps65137_digital_pulse);
	mutex_unlock(&mutex);
}

