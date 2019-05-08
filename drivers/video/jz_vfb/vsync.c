/*
 * Copyright (C) 2005-2014, Ingenic Semiconductor Inc.
 * http://www.ingenic.cn/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/wakelock.h>
#include <linux/kthread.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include "vsync.h"

static int vsync_skip_set(int vsync_skip)
{
	unsigned int map_wide10 = 0;
	int rate, i, p, n;
	int fake_float_1k;

	if (vsync_skip < 0 || vsync_skip > 9)
		return -EINVAL;

	rate = vsync_skip + 1;
	fake_float_1k = 10000 / rate;	/* 10.0 / rate */

	p = 1;
	n = (fake_float_1k * p + 500) / 1000;	/* +0.5 to int */

	for (i = 1; i <= 10; i++) {
		map_wide10 = map_wide10 << 1;
		if (i == n) {
			map_wide10++;
			p++;
			n = (fake_float_1k * p + 500) / 1000;
		}
	}
	vsync_data.vsync_skip_map = map_wide10;
	vsync_data.vsync_skip_ratio = rate - 1;	/* 0 ~ 9 */
	return 0;
}

int jzfb_vsync_thread(void *data)
{
	int delay = *(int *)data;
	while (!kthread_should_stop()) {
		msleep(delay);
		vsync_data.vsync_skip_map = (vsync_data.vsync_skip_map >> 1 |
				vsync_data.vsync_skip_map << 9) & 0x3ff;
		if (likely(vsync_data.vsync_skip_map & 0x1)) {
			vsync_data.timestamp.value[vsync_data.timestamp.wp] =
				ktime_to_ns(ktime_get());
			vsync_data.timestamp.wp = (vsync_data.timestamp.wp + 1) % TIMESTAMP_CAP;
			wake_up_interruptible(&vsync_data.vsync_wq);
		}
	}
	return 0;
}

int vsync_soft_set(int data)
{
	vsync_skip_set(CONFIG_FB_VSYNC_SKIP);
	init_waitqueue_head(&vsync_data.vsync_wq);
	vsync_data.timestamp.rp = 0;
	vsync_data.timestamp.wp = 0;
	vsync_data.vsync_thread = kthread_run(jzfb_vsync_thread,&data,"jzfb-vsync");
	if(vsync_data.vsync_thread == ERR_PTR(-ENOMEM)){
		return -1;
	}
	return 0;
}

void vsync_soft_stop()
{
	kthread_stop(vsync_data.vsync_thread);
}

int vsync_wait(struct fb_info *info,unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret;
	if (likely(vsync_data.timestamp.wp == vsync_data.timestamp.rp)) {
				unlock_fb_info(info);
				interruptible_sleep_on(&vsync_data.vsync_wq);
				lock_fb_info(info);
			} else {
				printk("Send vsync\n");
			}

			ret = copy_to_user(argp, vsync_data.timestamp.value + vsync_data.timestamp.rp,
					sizeof(u64));
			vsync_data.timestamp.rp = (vsync_data.timestamp.rp + 1) % TIMESTAMP_CAP;
	return ret;
}

