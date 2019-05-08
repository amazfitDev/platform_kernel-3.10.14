/* include/linux/timed_regulator.h
 *
 * Copyright (C) 2013 Ingenic, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#ifndef _LINUX_TIMED_REGULATOR_H
#define _LINUX_TIMED_REGULATOR_H

#define TIMED_REGULATOR_NAME "timed-regulator"

struct timed_regulator {
	const char *name;
	const char *reg_name;
	int  max_timeout;
};

struct timed_regulator_platform_data {
	int num_regulators;
	struct timed_regulator *regs;
};

#endif
