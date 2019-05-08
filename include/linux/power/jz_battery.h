/* include/linux/power/jz_battery.h
 * Copyright (C) 2012 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 *	Sun Jiwei<jwsun@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __JZ_BATTERY_H
#define __JZ_BATTERY_H

#include <linux/power_supply.h>

struct jz_battery_info {
	int max_vol;
	int min_vol;
	int usb_max_vol;
	int usb_min_vol;
	int ac_max_vol;
	int ac_min_vol;

	int battery_max_cpt;
	int ac_chg_current;
	int usb_chg_current;

	unsigned int	sleep_current;

	char slop_r;
	char cut_r;
};

struct jz_battery_platform_data {
	struct jz_battery_info info;
};

enum {
	USB,
	AC,
	STATUS,
};

struct private {
	unsigned long timecount;
	unsigned int voltage;
};

struct jz_battery {
	struct jz_battery_platform_data *pdata;
	struct platform_device *pdev;

	struct resource *mem;
	void __iomem *base;

	int irq;

	bool power_on_flag;

	const struct mfd_cell *cell;

	int status_charge;
	int status;/*modified by PMU driver, return the current status of charging or discharging*/
	int status_tmp;

	unsigned int voltage;

	struct completion read_completion;
	struct completion get_status_completion;

	struct power_supply battery;
	struct delayed_work work;
	struct delayed_work init_work;
	struct delayed_work resume_work;

	struct mutex lock;

	struct private private;

	unsigned int next_scan_time;
	unsigned int time;
	unsigned int usb;
	unsigned int ac;

	/* Online charger modified by PMU driver */
	unsigned int charger;


	unsigned int ac_charge_time;
	unsigned int usb_charge_time;

	__kernel_time_t resume_time;
	__kernel_time_t suspend_time;

	int capacity;
	int capacity_calculate;
	unsigned long gate_voltage;

	long	slop;
	long	cut;

	struct wake_lock *work_wake_lock;

	void *pmu_interface;
	int (*get_pmu_status)(void *pmu_interface, int status);
	void (*pmu_work_enable)(void *pmu_interface);
};

#define get_charger_online(bat, n)	((bat->charger & (1 << n)) ? 1 : 0)
#define set_charger_online(bat, n)	(bat->charger |= (1 << n))
#define set_charger_offline(bat, n)	(bat->charger &= ~(1 << n))

#endif
