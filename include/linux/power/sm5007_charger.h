/*
 * include/linux/power/sm5007_charger.h
 *
 * SILICONMITUS SM5007 Charger Driver
 *
 * Copyright (C) 2012-2014 SILICONMITUS COMPANY,LTD
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
#ifndef __LINUX_POWER_SM5007_H_
#define __LINUX_POWER_SM5007_H_

/* #include <linux/power_supply.h> */
/* #include <linux/types.h> */

/* Defined charger information */
#define	ADC_VDD_MV	2800
#define	MIN_VOLTAGE	3100
#define	MAX_VOLTAGE	4200
#define	B_VALUE		3435


/* SM5007 Register information */
/* CHGCNTL1 */
#define SM5007_TOPOFFTIMER_20MIN    0x0
#define SM5007_TOPOFFTIMER_30MIN    0x1
#define SM5007_TOPOFFTIMER_45MIN    0x2
#define SM5007_TOPOFFTIMER_DISABLED 0x3

#define SM5007_FASTTIMMER_3P5HOURS  0x0
#define SM5007_FASTTIMMER_4P5HOURS  0x1
#define SM5007_FASTTIMMER_5P5HOURS  0x2
#define SM5007_FASTTIMMER_DISABLED  0x3

#define SM5007_RECHG_M100           0x0
#define SM5007_RECHG_M50            0x1
#define SM5007_RECHG_MASK           0x04
#define SM5007_RECHG_SHIFT          0x2

#define SM5007_AUTOSTOP_DISABLED    0x0
#define SM5007_AUTOSTOP_ENABLED     0x1
#define SM5007_AUTOSTOP_MASK        0x02
#define SM5007_AUTOSTOP_SHIFT       0x1

#define SM5007_CHGEN_DISABLED     0x0
#define SM5007_CHGEN_ENABLED      0x1
#define SM5007_CHGEN_MASK         0x01
#define SM5007_CHGEN_SHIFT        0x0
/* CHGCNTL3 */
#define SM5007_TOPOFF_10MA          0x0
#define SM5007_TOPOFF_20MA          0x1
#define SM5007_TOPOFF_30MA          0x2
#define SM5007_TOPOFF_40MA          0x3
#define SM5007_TOPOFF_50MA          0x4
#define SM5007_TOPOFF_60MA          0x5
#define SM5007_TOPOFF_70MA          0x6
#define SM5007_TOPOFF_80MA          0x7

#define SM5007_TOPOFF_MASK         0xE0
#define SM5007_TOPOFF_SHIFT        0x5

#define SM5007_FASTCHG_MASK         0x1F
#define SM5007_FASTCHG_SHIFT        0x0

/**************************/

/* detailed status in STATUS (0x10) */
#define SM5007_STATE_VBATOVP    0x01
#define SM5007_STATE_VBUSPOK    0x02
#define SM5007_STATE_VBUSUVLO   0x04
#define SM5007_STATE_VBUSOVP    0x08
#define SM5007_STATE_CHGON      0x10
#define SM5007_STATE_TOPOFF     0x20
#define SM5007_STATE_DONE       0x40
#define SM5007_STATE_THEMSAFE   0x80

struct sm5007_charger_type_data {
	int	chg_batreg;
	int	chg_fastchg;
	int	chg_autostop;
	int	chg_rechg;
	int	chg_topoff;
};

struct sm5007_charger_platform_data {
	int	irq;
	struct sm5007_charger_type_data type;
};
#endif
