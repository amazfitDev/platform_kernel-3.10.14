#ifndef __FRIZZ_H__
#define __FRIZZ_H__

struct frizz_platform_data {
	unsigned int irq_gpio;
	unsigned int reset_gpio;
	unsigned int wakeup_frizz_gpio;
	unsigned int g_chip_orientation;
};

#endif
