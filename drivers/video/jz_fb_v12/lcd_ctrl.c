#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/fb.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/lcd_ctrl.h>

#include <mach/jzfb.h>
#include "jz_fb.h"
#include "regs.h"

static LIST_HEAD(jzfb_list);
static DEFINE_SPINLOCK(jzfb_lock);

void jzfb_ctrl_register(struct jzfb *jzfb) {
	unsigned long flags;

	spin_lock_irqsave(&jzfb_lock, flags);
	list_add_tail(&jzfb->link, &jzfb_list);
	spin_unlock_irqrestore(&jzfb_lock, flags);
}

void jzfb_ctrl_unregister(struct jzfb *jzfb) {
	unsigned long flags;

	spin_lock_irqsave(&jzfb_lock, flags);
	list_del(&jzfb->link);
	spin_unlock_irqrestore(&jzfb_lock, flags);
}

static inline struct jzfb *get_jzfb_by_id_internal(int id) {
	struct list_head *pos;
	struct jzfb *fb;

	list_for_each(pos, &jzfb_list) {
		fb = list_entry(pos, struct jzfb, link);
		return fb;
	}
	return NULL;
}

struct jzfb *get_jzfb_by_id(int id) {
	unsigned long flags;
	struct jzfb *fb;

	spin_lock_irqsave(&jzfb_lock, flags);
	fb = get_jzfb_by_id_internal(id);
	spin_unlock_irqrestore(&jzfb_lock, flags);

	return fb;
}

const char *jzfb_get_lcd_name(int id) {
	const char *name;
	unsigned long flags;
	struct jzfb *fb;

	spin_lock_irqsave(&jzfb_lock, flags);
	fb = get_jzfb_by_id_internal(id);
	name = (fb && fb->pdata) ? fb->pdata->name : NULL;
	spin_unlock_irqrestore(&jzfb_lock, flags);

	return name;
}

struct jzfb *get_jzfb_by_lcd_name(const char *name) {
	struct list_head *pos;
	struct jzfb *jzfb = NULL;
	unsigned long flags;

	spin_lock_irqsave(&jzfb_lock, flags);
	list_for_each(pos, &jzfb_list) {
		struct jzfb *fb;
		fb = list_entry(pos, struct jzfb, link);
		if (fb->pdata && fb->pdata->name && !strcmp(name, fb->pdata->name)) {
			jzfb = fb;
			break;
		}
	}
	spin_unlock_irqrestore(&jzfb_lock, flags);

	return jzfb;
}

#ifdef CONFIG_SLPT
const char *slpt_get_default_lcd(int *idp) {
	struct list_head *pos;

	pr_info("%s %d\n", __FUNCTION__, __LINE__);

	list_for_each(pos, &jzfb_list) {
		struct jzfb *fb;
		fb = list_entry(pos, struct jzfb, link);
		if (fb->pdata && fb->pdata->name) {
			if (idp) {
				*idp = 0;
				/* *idp = fb->id; */
			}
			pr_info("slpt: detected lcd name: %s\n", fb->pdata->name ? fb->pdata->name : "null");
			return fb->pdata->name;
		}
	}

	return NULL;
}
EXPORT_SYMBOL(slpt_get_default_lcd);

unsigned long slpt_get_jzfb_param(int id, const char *param) {
	struct jzfb *fb;

	fb = get_jzfb_by_id_internal(id);
	if (!fb || !param) {
		pr_err("jzfb: %s failed\n", __FUNCTION__);
		return 0;
	}

	if (!strcmp(param, "fb-mem"))
		return (unsigned long)fb->vidmem;
	else if (!strcmp(param, "fb-num"))
		return NUM_FRAME_BUFFERS;
	else if (!strcmp(param, "frmsize"))
		return (unsigned long) fb->frm_size;
	else if (!strcmp(param, "lcd-name"))
		return (unsigned long) (fb->pdata ? fb->pdata->name : 0);
	return 0;
}
EXPORT_SYMBOL(slpt_get_jzfb_param);
#endif
