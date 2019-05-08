/*
 * linux/regulator/sm5007-regulator.h
 *
 * Regulator driver for SILICONMITUS SM5007 power management chip.
 *
 * Copyright (C) 2012-2014 SILICONMITUS COMPANY,LTD
 *
 * Based on code
 *	Copyright (C) 2011 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __LINUX_REGULATOR_SM5007_H
#define __LINUX_REGULATOR_SM5007_H

#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>

#define sm5007_rails(_name) "SM5007_"#_name

#define LPM_LDO_ATTACH_TO_STM    1
#define LPM_LDO_FORCE_INTO_LPM   2
#define LPM_LDO_NORMAL           3
#define LPM_BUCK_AUTO            4
#define LPM_BUCK_PWM             5
#define LPM_IGNORE              0

/* SM5007 Regulator IDs */
enum regulator_id {
	SM5007_ID_BUCK1,
	SM5007_ID_BUCK1_DVS,
	SM5007_ID_BUCK2,
	SM5007_ID_BUCK3,
	SM5007_ID_BUCK4,
	SM5007_ID_LDO1,
	SM5007_ID_LDO2,
	SM5007_ID_LDO3,
	SM5007_ID_LDO4,
	SM5007_ID_LDO5,
	SM5007_ID_LDO6,
	SM5007_ID_LDO7,
	SM5007_ID_LDO8,
	SM5007_ID_LDO9,
	SM5007_ID_PS1,
	SM5007_ID_PS2,
	SM5007_ID_PS3,
	SM5007_ID_PS4,
	SM5007_ID_PS5,
};

struct sm5007_regulator_platform_data {
		struct regulator_init_data regulator;
		int init_uV;
		unsigned init_enable:1;
		unsigned init_apply:1;
		unsigned lpm;
		int sleep_uV;
		int sleep_slots;
		unsigned long ext_pwr_req;
		unsigned long flags;
};

extern	int sm5007_regulator_enable_eco_mode(struct regulator_dev *rdev);
extern	int sm5007_regulator_disable_eco_mode(struct regulator_dev *rdev);
extern	int sm5007_regulator_enable_eco_slp_mode(struct regulator_dev *rdev);
extern	int sm5007_regulator_disable_eco_slp_mode(struct regulator_dev *rdev);


#endif
