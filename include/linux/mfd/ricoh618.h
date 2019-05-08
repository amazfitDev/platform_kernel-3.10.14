/*
 * include/linux/mfd/ricoh618.h
 *
 * Core driver interface to access RICOH RN5T618 power management chip.
 *
 * Copyright (C) 2012-2013 RICOH COMPANY,LTD
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

#ifndef __LINUX_MFD_RICOH618_H
#define __LINUX_MFD_RICOH618_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/notifier.h>
#include <irq.h>

/* Maximum number of main interrupts */
#define MAX_INTERRUPT_MASKS	11
#define MAX_MAIN_INTERRUPT	8
#define MAX_GPEDGE_REG		1

/* Power control register */
#define RICOH618_PWR_ON_HIS			0x09
#define RICOH618_PWR_OFF_HIS		0x0A
#define RICOH618_PWR_WD			0x0B
#define RICOH618_PWR_WD_COUNT		0x0C
#define RICOH618_PWR_FUNC		0x0D
#define RICOH618_PWR_SLP_CNT		0x0E
#define RICOH618_PWR_REP_CNT		0x0F
#define RICOH618_PWR_ON_TIMSET		0x10
#define RICOH618_PWR_NOE_TIMSET		0x11
#define RICOH618_PWR_IRSEL		0x15

/* Interrupt enable register */
#define RICOH618_INT_EN_SYS		0x12
#define RICOH618_INT_EN_DCDC		0x40
#define RICOH618_INT_EN_ADC1		0x88
#define RICOH618_INT_EN_ADC2		0x89
#define RICOH618_INT_EN_ADC3		0x8A
#define RICOH618_INT_EN_GPIO		0x94
#define RICOH618_INT_EN_GPIO2		0x94
#define RICOH618_INT_MSK_CHGCTR		0xBE
#define RICOH618_INT_MSK_CHGSTS1	0xBF
#define RICOH618_INT_MSK_CHGSTS2	0xC0
#define RICOH618_INT_MSK_CHGERR		0xC1

/* Interrupt select register */
#define RICOH618_PWR_IRSEL			0x15
#define RICOH618_CHG_CTRL_DETMOD1	0xCA
#define RICOH618_CHG_CTRL_DETMOD2	0xCB
#define RICOH618_CHG_STAT_DETMOD1	0xCC
#define RICOH618_CHG_STAT_DETMOD2	0xCD
#define RICOH618_CHG_STAT_DETMOD3	0xCE

/* interrupt status registers (monitor regs)*/
#define RICOH618_INTC_INTPOL		0x9C
#define RICOH618_INTC_INTEN		0x9D
#define RICOH618_INTC_INTMON		0x9E

#define RICOH618_INT_MON_SYS		0x14
#define RICOH618_INT_MON_DCDC		0x42

#define RICOH618_INT_MON_CHGCTR		0xC6
#define RICOH618_INT_MON_CHGSTS1	0xC7
#define RICOH618_INT_MON_CHGSTS2	0xC8
#define RICOH618_INT_MON_CHGERR		0xC9

/* interrupt clearing registers */
#define RICOH618_INT_IR_SYS		0x13
#define RICOH618_INT_IR_DCDC		0x41
#define RICOH618_INT_IR_ADCL		0x8C
#define RICOH618_INT_IR_ADCH		0x8D
#define RICOH618_INT_IR_ADCEND		0x8E
#define RICOH618_INT_IR_GPIOR		0x95
#define RICOH618_INT_IR_GPIOF		0x96
#define RICOH618_INT_IR_CHGCTR		0xC2
#define RICOH618_INT_IR_CHGSTS1		0xC3
#define RICOH618_INT_IR_CHGSTS2		0xC4
#define RICOH618_INT_IR_CHGERR		0xC5

/* GPIO register base address */
#define RICOH618_GPIO_IOSEL		0x90
#define RICOH618_GPIO_IOOUT		0x91
#define RICOH618_GPIO_GPEDGE1		0x92
#define RICOH618_GPIO_GPEDGE2		0x93
//#define RICOH618_GPIO_EN_GPIR		0x94
//#define RICOH618_GPIO_IR_GPR		0x95
//#define RICOH618_GPIO_IR_GPF		0x96
#define RICOH618_GPIO_MON_IOIN		0x97
#define RICOH618_GPIO_LED_FUNC		0x98

#define RICOH618_REG_BANKSEL		0xFF

/* Charger Control register */
#define RICOH618_CHG_CTL1		0xB3

/* ADC Control register */
#define RICOH618_ADC_CNT1		0x64
#define RICOH618_ADC_CNT2		0x65
#define RICOH618_ADC_CNT3		0x66
#define RICOH618_ADC_VADP_THL		0x7C
#define RICOH618_ADC_VSYS_THL		0x80

#define	RICOH618_FG_CTRL		0xE0
#define	RICOH618_PSWR			0x07

/* RICOH618 IRQ definitions */
enum {
	RICOH618_IRQ_POWER_ON,
	RICOH618_IRQ_EXTIN,
	RICOH618_IRQ_PRE_VINDT,
	RICOH618_IRQ_PREOT,
	RICOH618_IRQ_POWER_OFF,
	RICOH618_IRQ_NOE_OFF,
	RICOH618_IRQ_WD,

	RICOH618_IRQ_DC1LIM,
	RICOH618_IRQ_DC2LIM,
	RICOH618_IRQ_DC3LIM,

	RICOH618_IRQ_ILIMLIR,
	RICOH618_IRQ_VBATLIR,
	RICOH618_IRQ_VADPLIR,
	RICOH618_IRQ_VUSBLIR,
	RICOH618_IRQ_VSYSLIR,
	RICOH618_IRQ_VTHMLIR,
	RICOH618_IRQ_AIN1LIR,
	RICOH618_IRQ_AIN0LIR,

	RICOH618_IRQ_ILIMHIR,
	RICOH618_IRQ_VBATHIR,
	RICOH618_IRQ_VADPHIR,
	RICOH618_IRQ_VUSBHIR,
	RICOH618_IRQ_VSYSHIR,
	RICOH618_IRQ_VTHMHIR,
	RICOH618_IRQ_AIN1HIR,
	RICOH618_IRQ_AIN0HIR,

	RICOH618_IRQ_ADC_ENDIR,

	RICOH618_IRQ_GPIO0,
	RICOH618_IRQ_GPIO1,
	RICOH618_IRQ_GPIO2,
	RICOH618_IRQ_GPIO3,

	RICOH618_IRQ_FVADPDETSINT,
	RICOH618_IRQ_FVUSBDETSINT,
	RICOH618_IRQ_FVADPLVSINT,
	RICOH618_IRQ_FVUSBLVSINT,
	RICOH618_IRQ_FWVADPSINT,
	RICOH618_IRQ_FWVUSBSINT,

	RICOH618_IRQ_FONCHGINT,
	RICOH618_IRQ_FCHGCMPINT,
	RICOH618_IRQ_FBATOPENINT,
	RICOH618_IRQ_FSLPMODEINT,
	RICOH618_IRQ_FBTEMPJTA1INT,
	RICOH618_IRQ_FBTEMPJTA2INT,
	RICOH618_IRQ_FBTEMPJTA3INT,
	RICOH618_IRQ_FBTEMPJTA4INT,

	RICOH618_IRQ_FCURTERMINT,
	RICOH618_IRQ_FVOLTERMINT,
	RICOH618_IRQ_FICRVSINT,
	RICOH618_IRQ_FPOOR_CHGCURINT,
	RICOH618_IRQ_FOSCFDETINT1,
	RICOH618_IRQ_FOSCFDETINT2,
	RICOH618_IRQ_FOSCFDETINT3,
	RICOH618_IRQ_FOSCMDETINT,

	RICOH618_IRQ_FDIEOFFINT,
	RICOH618_IRQ_FDIEERRINT,
	RICOH618_IRQ_FBTEMPERRINT,
	RICOH618_IRQ_FVBATOVINT,
	RICOH618_IRQ_FTTIMOVINT,
	RICOH618_IRQ_FRTIMOVINT,
	RICOH618_IRQ_FVADPOVSINT,
	RICOH618_IRQ_FVUSBOVSINT,

	/* Should be last entry */
	RICOH618_NR_IRQS,
};

/* Ricoh618 gpio definitions */
enum {
	RICOH618_GPIO0,
	RICOH618_GPIO1,
	RICOH618_GPIO2,
	RICOH618_GPIO3,

	RICOH618_NR_GPIO,
};

enum ricoh618_sleep_control_id {
	RICOH618_DS_DC1,
	RICOH618_DS_DC2,
	RICOH618_DS_DC3,
	RICOH618_DS_LDO1,
	RICOH618_DS_LDO2,
	RICOH618_DS_LDO3,
	RICOH618_DS_LDO4,
	RICOH618_DS_LDO5,
	RICOH618_DS_LDORTC1,
	RICOH618_DS_LDORTC2,
	RICOH618_DS_PSO0,
	RICOH618_DS_PSO1,
	RICOH618_DS_PSO2,
	RICOH618_DS_PSO3,
	RICOH618_DS_PSO4,
	RICOH618_DS_PSO5,
	RICOH618_DS_PSO6,
	RICOH618_DS_PSO7,
};


struct ricoh618_subdev_info {
	int			id;
	const char	*name;
	void		*platform_data;
};

/*
struct ricoh618_rtc_platform_data {
	int irq;
	struct rtc_time time;
};
*/

struct ricoh618_gpio_init_data {
	unsigned output_mode_en:1; 	/* Enable output mode during init */
	unsigned output_val:1;  	/* Output value if it is in output mode */
	unsigned init_apply:1;  	/* Apply init data on configuring gpios*/
	unsigned led_mode:1;  		/* Select LED mode during init */
	unsigned led_func:1;  		/* Set LED function if LED mode is 1 */
};

struct ricoh618 {
	struct device		*dev;
	struct i2c_client	*client;
	struct mutex		io_lock;
	struct notifier_block ricoh618_notifier;
	int			gpio_base;
	struct gpio_chip	gpio_chip;
	int			irq_base;
//	struct irq_chip		irq_chip;
	int			chip_irq;
	struct mutex		irq_lock;
	unsigned long		group_irq_en[MAX_MAIN_INTERRUPT];

	/* For main interrupt bits in INTC */
	u8			intc_inten_cache;
	u8			intc_inten_reg;

	/* For group interrupt bits and address */
	u8			irq_en_cache[MAX_INTERRUPT_MASKS];
	u8			irq_en_reg[MAX_INTERRUPT_MASKS];
	int			*irq_en_add;

	/* Interrupt monitor and clear register */
	int			*irq_mon_add;
	int			*irq_clr_add;
	int			*main_int_type;

	/* For gpio edge */
	u8			gpedge_cache[MAX_GPEDGE_REG];
	u8			gpedge_reg[MAX_GPEDGE_REG];
	u8			gpedge_add[MAX_GPEDGE_REG];

	int			bank_num;
};

struct ricoh618_platform_data {
	int		num_subdevs;
	struct	ricoh618_subdev_info *subdevs;
	int (*init_port)(int irq_num); /* Init GPIO for IRQ pin */
	int		gpio_base;
	int		irq_base;
	struct ricoh618_gpio_init_data *gpio_init_data;
	int num_gpioinit_data;
	bool enable_shutdown_pin;
};

/* ==================================== */
/* RICOH618 Power_Key device data	*/
/* ==================================== */
struct ricoh618_pwrkey_platform_data {
	int irq;
	unsigned long delay_ms;
};
extern int pwrkey_wakeup;
/* ==================================== */
/* RICOH618 battery device data		*/
/* ==================================== */
extern int g_soc;
extern int g_fg_on_mode;
extern int ricoh618_read(struct device *dev, uint8_t reg, uint8_t *val);
extern int ricoh618_read_bank1(struct device *dev, uint8_t reg, uint8_t *val);
extern int ricoh618_bulk_reads(struct device *dev, u8 reg, u8 count, uint8_t *val);

extern int ricoh618_bulk_reads_bank1(struct device *dev, u8 reg, u8 count, uint8_t *val);

extern int ricoh618_write(struct device *dev, u8 reg, uint8_t val);
extern int ricoh618_write_bank1(struct device *dev, u8 reg, uint8_t val);
extern int ricoh618_bulk_writes(struct device *dev, u8 reg, u8 count, uint8_t *val);

extern int ricoh618_bulk_writes_bank1(struct device *dev, u8 reg, u8 count,	uint8_t *val);

extern int ricoh618_set_bits(struct device *dev, u8 reg, uint8_t bit_mask);
extern int ricoh618_clr_bits(struct device *dev, u8 reg, uint8_t bit_mask);
extern int ricoh618_update(struct device *dev, u8 reg, uint8_t val,	uint8_t mask);

extern int ricoh618_update_bank1(struct device *dev, u8 reg, uint8_t val, uint8_t mask);
extern int ricoh618_irq_init(struct ricoh618 *ricoh618, int irq,int irq_base);
extern int ricoh618_irq_exit(struct ricoh618 *ricoh618);

#endif
