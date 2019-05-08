#ifndef _JZ_NOTIFIER_H_
#define _JZ_NOTIFIER_H_

#include <linux/notifier.h>

/*
 * Hibernation and suspend events
 */
enum jz_notif_cmd {
	JZ_CMD_START = 0,
	JZ_CLK_PRECHANGE,
	JZ_CLK_CHANGING,
	JZ_CLK_CHANGED,
	JZ_CLKGATE_CHANGE,
	JZ_POST_HIBERNATION, /* Hibernation finished */
	MMU_CONTEXT_EXIT_MMAP,
	JZ_CMD_END
};
enum {
	NOTEFY_PROI_START=1,
	NOTEFY_PROI_HIGH,
	NOTEFY_PROI_NORMAL,
	NOTEFY_PROI_LOW,
	NOTEFY_PROI_END
};

struct clk_notify_data
{
	unsigned long current_rate;
	unsigned long target_rate;
};

struct jz_notifier {
	struct notifier_block nb;
	int (*jz_notify)(struct jz_notifier *notify,void *d);
	int level;
	enum jz_notif_cmd msg;
};

int jz_notifier_register(struct jz_notifier *notify, unsigned int priority);
int jz_notifier_unregister(struct jz_notifier *notify, unsigned int priority);
int jz_notifier_call(unsigned int priority, unsigned long val, void *v);

#endif /* _JZ_NOTIFIER_H_ */
