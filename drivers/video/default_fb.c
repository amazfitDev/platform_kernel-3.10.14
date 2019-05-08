/*
 *  Copyright (C) 2015 Wu Jiao <jwu@ingenic.cn wujiaoosos@qq.com>
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

#include <linux/default_fb.h>
#include <linux/mutex.h>

static struct fb_ctrl *default_fb = NULL;
static DEFINE_MUTEX(m_lock);

void register_default_fb(struct fb_ctrl *fb) {
	mutex_lock(&m_lock);
	BUG_ON(default_fb);
	BUG_ON(!fb);
	default_fb = fb;
	mutex_unlock(&m_lock);
}

void unregister_default_fb(struct fb_ctrl *fb) {
	mutex_lock(&m_lock);
	BUG_ON(!default_fb);
	BUG_ON(default_fb != fb);
	default_fb = NULL;
	mutex_unlock(&m_lock);
}

static void do_power_off_fb(struct fb_ctrl *fb) {
	if (fb && fb->power_off)
		fb->power_off(fb);
}

static void do_power_on_fb(struct fb_ctrl *fb) {
	if (fb && fb->power_on)
		fb->power_on(fb);
}

void power_off_default_fb(void) {
	mutex_lock(&m_lock);
	do_power_off_fb(default_fb);
	mutex_unlock(&m_lock);
}

void power_on_default_fb(void) {
	mutex_lock(&m_lock);
	do_power_on_fb(default_fb);
	mutex_unlock(&m_lock);
}
