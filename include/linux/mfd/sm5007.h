/*
 * include/linux/mfd/sm5007.h
 *
 * Core driver interface to access SILICONMITUS SM5007 power management chip.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef __LINUX_MFD_SM5007_H
#define __LINUX_MFD_SM5007_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <jz_notifier.h>
#include <linux/power_supply.h>

/* Maximum number of main interrupts */
#define MAX_INTERRUPT_MASKS	6

/* Power control register */
#define SM5007_PWRONREG          0x00
#define SM5007_PWROFFREG         0x01
#define SM5007_REBOOTREG         0x02
#define SM5007_INTSOURCE         0x03
#define SM5007_INT1              0x04
#define SM5007_INT2              0x05
#define SM5007_INT3              0x06
#define SM5007_INT4              0x07
#define SM5007_INT5              0x08
#define SM5007_INT6              0x09
#define SM5007_INT1MSK           0x0A
#define SM5007_INT2MSK           0x0B
#define SM5007_INT3MSK           0x0C
#define SM5007_INT4MSK           0x0D
#define SM5007_INT5MSK           0x0E
#define SM5007_INT6MSK           0x0F
#define SM5007_STATUS            0x10
#define SM5007_CNTL1             0x11
#define SM5007_CNTL2             0x12

#define SM5007_CHGCNTL1          0x20
#define SM5007_CHGCNTL2          0x21
#define SM5007_CHGCNTL3          0x22

#define SM5007_BUCK1CNTL1        0x30
#define SM5007_BUCK1CNTL2        0x31
#define SM5007_BUCK1CNTL3        0x32
#define SM5007_BUCK2CNTL         0x33
#define SM5007_BUCK4CNTL         0x34

#define SM5007_LDO1CNTL          0x35
#define SM5007_LDO3CNTL          0x36
#define SM5007_LDO4CNTL          0x37
#define SM5007_LDO5CNTL          0x38
#define SM5007_LDO8CNTL          0x39
#define SM5007_LDO12OUT          0x3A
#define SM5007_LDO78OUT          0x3B
#define SM5007_LDO9OUT           0x3C

#define SM5007_PS1OUTCNTL        0x3D
#define SM5007_PS2OUTCNTL        0x3E
#define SM5007_PS3OUTCNTL        0x3F
#define SM5007_PS4OUTCNTL        0x40
#define SM5007_PS5OUTCNTL        0x41

#define SM5007_ONOFFCNTL         0x42

#define SM5007_LPMMODECNTL1      0x43
#define SM5007_LPMMODECNTL2      0x44
#define SM5007_LPMMODECNTL3      0x45

#define SM5007_DCDCMODECNTL      0x46

#define SM5007_ADISCNTL1         0x47
#define SM5007_ADISCNTL2         0x48
#define SM5007_ADISCNTL3         0x49

#define SM5007_CHIPID            0x4A

//CNTL1
#define SM5007_MASK_INT_MASK     0x02
#define SM5007_MASK_INT_SHIFT    0x1


/* SM5007 IRQ definitions */
enum {
	SM5007_IRQ_VBATOVP,
	SM5007_IRQ_TRITMROFF,
	SM5007_IRQ_FASTTMROFF,
	SM5007_IRQ_THEMREG,
	SM5007_IRQ_THEMSHDN,
	SM5007_IRQ_1STTHEMWARN,
	SM5007_IRQ_2NDTHEMWARN ,
	SM5007_IRQ_THEMSAFE,

	SM5007_IRQ_VBUSPOK,
	SM5007_IRQ_VBUSUVLO,
	SM5007_IRQ_VBUSOVP,
	SM5007_IRQ_CHGON,
	SM5007_IRQ_TOPOFF,
	SM5007_IRQ_DONE,
	SM5007_IRQ_CHGRSTF,

	SM5007_IRQ_SHORTKEY,
	SM5007_IRQ_LNGKEY_nONKEY,
	SM5007_IRQ_MANUALRST,
	SM5007_IRQ_VBATNG,

	SM5007_IRQ_BUCK2POK,
	SM5007_IRQ_BUCK3POK,
	SM5007_IRQ_BUCK4POK,
	SM5007_IRQ_LDO1POK,
	SM5007_IRQ_LDO2POK,
	SM5007_IRQ_LDO3POK,
	SM5007_IRQ_LDO4POK,

	SM5007_IRQ_LDO5POK,
	SM5007_IRQ_LDO6POK,
	SM5007_IRQ_LDO7POK,
	SM5007_IRQ_LDO8POK,
	SM5007_IRQ_LDO9POK,
	SM5007_IRQ_PS1POK,
	SM5007_IRQ_PS2POK,
	SM5007_IRQ_PS3POK,

	SM5007_IRQ_PS4POK,
	SM5007_IRQ_PS5POK,
	SM5007_IRQ_CPPOK,
	SM5007_IRQ_PWRSTMF,
	SM5007_IRQ_PWRSTMR,

	/* Should be last entry */
	SM5007_NR_IRQS,
};

#define SM5007_IRQ_VBATOVP_NAME			"VBATOVP"
#define SM5007_IRQ_TRITMROFF_NAME		"TRITMROFF"
#define SM5007_IRQ_FASTTMROFF_NAME		"FASTTMROFF"
#define SM5007_IRQ_THEMREG_NAME			"THEMREG"
#define SM5007_IRQ_THEMSHDN_NAME		"THEMSHDN"
#define SM5007_IRQ_1STTHEMWARN_NAME		"1STTHEMWARN"
#define SM5007_IRQ_2NDTHEMWARN_NAME 	"2NDTHEMWARN"
#define SM5007_IRQ_THEMSAFE_NAME		"THEMSAFE"

#define SM5007_IRQ_VBUSPOK_NAME			"VBUSPOK"
#define SM5007_IRQ_VBUSUVLO_NAME		"VBUSUVLO"
#define SM5007_IRQ_VBUSOVP_NAME			"VBUSOVP"
#define SM5007_IRQ_CHGON_NAME			"CHGON"
#define SM5007_IRQ_TOPOFF_NAME			"TOPOFF"
#define SM5007_IRQ_DONE_NAME			"DONE"
#define SM5007_IRQ_CHGRSTF_NAME			"CHGRSTF"

#define SM5007_IRQ_SHORTKEY_NAME		"SHORTKEY"
#define SM5007_IRQ_LNGKEY_nONKEY_NAME	"LNGKEY_nONKEY"
#define SM5007_IRQ_MANUALRST_NAME		"MANUALRST"
#define SM5007_IRQ_VBATNG_NAME			"VBATNG"

#define SM5007_IRQ_BUCK2POK_NAME		"BUCK2POK"
#define SM5007_IRQ_BUCK3POK_NAME		"BUCK3POK"
#define SM5007_IRQ_BUCK4POK_NAME		"BUCK4POK"
#define SM5007_IRQ_LDO1POK_NAME			"LDO1POK"
#define SM5007_IRQ_LDO2POK_NAME			"LDO2POK"
#define SM5007_IRQ_LDO3POK_NAME			"LDO3POK"
#define SM5007_IRQ_LDO4POK_NAME			"LDO4POK"

#define SM5007_IRQ_LDO5POK_NAME			"LDO5POK"
#define SM5007_IRQ_LDO6POK_NAME			"LDO6POK"
#define SM5007_IRQ_LDO7POK_NAME			"DO7POK"
#define SM5007_IRQ_LDO8POK_NAME			"LDO8POK"
#define SM5007_IRQ_LDO9POK_NAME			"LDO9POK"
#define SM5007_IRQ_PS1POK_NAME			"PS1POK"
#define SM5007_IRQ_PS2POK_NAME			"PS2POK"
#define SM5007_IRQ_PS3POK_NAME			"PS3POK"

#define SM5007_IRQ_PS4POK_NAME			"PS4POK"
#define SM5007_IRQ_PS5POK_NAME			"PS5POK"
#define SM5007_IRQ_CPPOK_NAME			"CPPOK"
#define SM5007_IRQ_PWRSTMF_NAME			"PWRSTMF"
#define SM5007_IRQ_PWRSTMR_NAME			"PWRSTMR"


struct sm5007_subdev_info {
	int			id;
	const char	*name;
	void		*platform_data;
	int			num_resources;
	const struct resource	*resources;
};

struct sm5007 {
	struct device		*dev;
	struct i2c_client	*client;
	struct mutex		io_lock;
    struct jz_notifier      sm5007_notifier;
	int			irq_base;
	int			chip_irq;
	struct mutex		irq_lock;

    u8 irq_masks_cache[6];

    u8 irq_status_cache[6];
};

struct sm5007_platform_data {
	int		num_subdevs;
	struct	sm5007_subdev_info *subdevs;
	int (*init_port)(int irq_num); /* Init GPIO for IRQ pin */
	int		irq_base;
	bool enable_shutdown_pin;
};

static inline struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

#define psy_do_property(name, function, property, value) \
{	\
	struct power_supply *psy;	\
	int ret;	\
	psy = get_power_supply_by_name((name));	\
	if (!psy) {	\
		printk("%s: Fail to "#function" psy (%s)\n",	\
			__func__, (name));	\
		value.intval = 0;	\
	} else {	\
		if (psy->function##_property != NULL) { \
			ret = psy->function##_property(psy, (property), &(value)); \
			if (ret < 0) {	\
				printk("%s: Fail to %s "#function" (%d=>%d)\n", \
						__func__, name, (property), ret);	\
				value.intval = 0;	\
			}	\
		}	\
	}	\
}

const char *sm5007_get_irq_name_by_index(int index);

/* ==================================== */
/* SM5007 Power_Key device data	*/
/* ==================================== */
struct sm5007_pwrkey_platform_data {
	int irq;
	unsigned long delay_ms;
};
extern int pwrkey_wakeup;
/* ==================================== */
/* SM5007 battery device data		*/
/* ==================================== */
extern int sm5007_read(struct device *dev, uint8_t reg, uint8_t *val);
extern int sm5007_read_bank1(struct device *dev, uint8_t reg, uint8_t *val);
extern int sm5007_bulk_reads(struct device *dev, u8 reg, u8 count,
								uint8_t *val);
extern int sm5007_bulk_reads_bank1(struct device *dev, u8 reg, u8 count,
								uint8_t *val);
extern int sm5007_write(struct device *dev, u8 reg, uint8_t val);
extern int sm5007_write_bank1(struct device *dev, u8 reg, uint8_t val);
extern int sm5007_bulk_writes(struct device *dev, u8 reg, u8 count,
								uint8_t *val);
extern int sm5007_bulk_writes_bank1(struct device *dev, u8 reg, u8 count,
								uint8_t *val);
extern int sm5007_set_bits(struct device *dev, u8 reg, uint8_t bit_mask);
extern int sm5007_clr_bits(struct device *dev, u8 reg, uint8_t bit_mask);
extern int sm5007_update(struct device *dev, u8 reg, uint8_t val,
								uint8_t mask);
extern int sm5007_update_bank1(struct device *dev, u8 reg, uint8_t val,
								uint8_t mask);
extern int sm5007_power_off(void);
extern int sm5007_irq_init(struct sm5007 *sm5007, int irq, int irq_base);
extern int sm5007_irq_exit(struct sm5007 *sm5007);

#endif
