/*
 * board-power-619.c
 *
 * Copyright (C) 2012-2014, Ricoh Company,Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 */
#include <linux/i2c.h>
#include <linux/pda_power.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/ricoh619.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/ricoh619-regulator.h>
#include <linux/power/ricoh619_battery.h>
#include <linux/rtc.h>
#include "board_base.h"

#define PMIC_IRQ	0

/* If don't use the GPIO function, Set this macro to -1 */
#define PLATFORM_RICOH_GPIO_BASE	(-1)
#define INT_PMIC_BASE	IRQ_RESERVED_BASE

#define PMC_CTRL		0x0
#define PMC_CTRL_INTR_LOW	(1 << 17)

/* RICOH619 IRQs */
#define RICOH619_IRQ_BASE	INT_PMIC_BASE
#define RICOH619_GPIO_BASE  	PLATFORM_RICOH_GPIO_BASE
#define RICOH619_GPIO_IRQ   	(RICOH619_GPIO_BASE + 8)

static struct regulator_consumer_supply ricoh619_dc2_supply_0[] = {
	REGULATOR_SUPPLY(DC2_NAME, NULL),
};

static struct regulator_consumer_supply ricoh619_dc4_supply_0[] = {
	REGULATOR_SUPPLY(DC4_NAME, NULL),
};

static struct regulator_consumer_supply ricoh619_ldo1_supply_0[] = {
	REGULATOR_SUPPLY(LDO1_NAME, NULL),
};

static struct regulator_consumer_supply ricoh619_ldo2_supply_0[] = {
	REGULATOR_SUPPLY(LDO2_NAME, NULL),
};

static struct regulator_consumer_supply ricoh619_ldo3_supply_0[] = {
	REGULATOR_SUPPLY(LDO3_NAME, NULL),
};

static struct regulator_consumer_supply ricoh619_ldo4_supply_0[] = {
	REGULATOR_SUPPLY(LDO4_NAME, NULL),
};

static struct regulator_consumer_supply ricoh619_ldo5_supply_0[] = {
	REGULATOR_SUPPLY(LDO5_NAME, NULL),
};

#define RICOH_PDATA_INIT(_name, _sname, _minmv, _maxmv, _supply_reg, _always_on, \
	_boot_on, _apply_uv, _init_uV, _sleep_uV, _init_enable, _init_apply, _flags, \
	_ext_contol, _ds_slots) \
	static struct ricoh619_regulator_platform_data pdata_##_name##_##_sname = \
	{								\
		.regulator = {						\
			.constraints = {				\
				.min_uV = (_minmv)*1000,		\
				.max_uV = (_maxmv)*1000,		\
				.valid_modes_mask = (REGULATOR_MODE_NORMAL |  \
						     REGULATOR_MODE_STANDBY), \
				.valid_ops_mask = (REGULATOR_CHANGE_MODE |    \
						   REGULATOR_CHANGE_STATUS |  \
						   REGULATOR_CHANGE_VOLTAGE), \
				.always_on = _always_on,		\
				.boot_on = _boot_on,			\
				.apply_uV = _apply_uv,			\
			},						\
			.num_consumer_supplies =			\
				ARRAY_SIZE(ricoh619_##_name##_supply_##_sname),	\
			.consumer_supplies = ricoh619_##_name##_supply_##_sname, \
			.supply_regulator = _supply_reg,		\
		},							\
		.init_uV =  _init_uV * 1000,				\
		.sleep_uV = _sleep_uV * 1000,				\
		.init_enable = _init_enable,				\
		.init_apply = _init_apply,				\
		.sleep_slots = _ds_slots,				\
		.flags = _flags,					\
		.ext_pwr_req = _ext_contol,				\
	}

/* min_uV/max_uV : Please set the appropriate value for the devices that
  the power supplied within a range from min to max voltage according
  to RC5T619 specification. */
/*_name,_sname,_minmv,_maxmv,_supply_reg,_always_on,_boot_on,_apply_uv,_init_uV,_init_enable,_init_apply,
 * _flags,_ext_contol,_ds_slots) */
RICOH_PDATA_INIT(dc2, 0,	600,   3300, 0, DC2_ALWAYS_ON, DC2_BOOT_ON, 1,
		 DC2_INIT_UV, DC2_INIT_SLP_UV, DC2_INIT_ENABLE, 1, 0, 0, 0);
RICOH_PDATA_INIT(dc4, 0,	600,   3500, 0, DC4_ALWAYS_ON, DC4_BOOT_ON, 1,
		 DC4_INIT_UV, DC4_INIT_SLP_UV, DC4_INIT_ENABLE, 1, 0, 0, 0);
RICOH_PDATA_INIT(ldo1, 0,	900,   2500, 0, LDO1_ALWAYS_ON, LDO1_BOOT_ON, 1,
		 LDO1_INIT_UV, LDO1_INIT_SLP_UV, LDO1_INIT_ENABLE, 1, 0, 0, 0);
RICOH_PDATA_INIT(ldo2, 0,	900,   3500, 0, LDO2_ALWAYS_ON, LDO2_BOOT_ON, 1,
		 LDO2_INIT_UV, LDO2_INIT_SLP_UV, LDO2_INIT_ENABLE, 1, 0, 0, 0);
RICOH_PDATA_INIT(ldo3, 0,	600,   3500, 0, LDO3_ALWAYS_ON, LDO3_BOOT_ON, 1,
		 LDO3_INIT_UV, LDO3_INIT_SLP_UV, LDO3_INIT_ENABLE, 1, 0, 0, 0);
RICOH_PDATA_INIT(ldo4, 0,	900,   3500, 0, LDO4_ALWAYS_ON, LDO4_BOOT_ON, 1,
		 LDO4_INIT_UV, LDO4_INIT_SLP_UV, LDO4_INIT_ENABLE, 1, 0, 0, 0);
RICOH_PDATA_INIT(ldo5, 0,	900,   3500, 0, LDO5_ALWAYS_ON, LDO5_BOOT_ON, 1,
		 LDO5_INIT_UV, LDO5_INIT_SLP_UV, LDO5_INIT_ENABLE, 1, 0, 0, 0);

/*-------- if Ricoh RTC exists -----------*/
#ifdef CONFIG_RTC_DRV_R5T619
static struct ricoh_rtc_platform_data rtc_data = {
	.irq = RICOH619_IRQ_BASE,
	.time = {
		.tm_year = 1970,
		.tm_mon = 0,
		.tm_mday = 1,
		.tm_hour = 0,
		.tm_min = 0,
		.tm_sec = 0,
	},
};

#define RICOH_RTC_REG						\
{								\
	.id		= 0,					\
	.name		= "rtc_ricoh619",			\
	.platform_data	= &rtc_data,				\
}
#endif
/*-------- if Ricoh RTC exists -----------*/

#define RICOH_REG(_id, _name, _sname)				\
{								\
	.id		= RICOH619_ID_##_id,			\
	.name		= "ricoh61x-regulator",			\
	.platform_data	= &pdata_##_name##_##_sname,		\
}

//==========================================
//RICOH619 Power_Key device data
//==========================================
static struct ricoh619_pwrkey_platform_data ricoh619_pwrkey_data = {
	.irq 		= RICOH619_IRQ_BASE,
	.delay_ms 	= 20,
};
#define RICOH619_PWRKEY_REG 					\
{ 								\
	.id 		= -1, 					\
	.name 		= "ricoh619-pwrkey", 			\
	.platform_data 	= &ricoh619_pwrkey_data, 		\
}

/*  JEITA Parameter
*
*          VCHG
*            |
* jt_vfchg_h~+~~~~~~~~~~~~~~~~~~~+
*            |                   |
* jt_vfchg_l-| - - - - - - - - - +~~~~~~~~~~+
*            |    Charge area    +          |
*  -------0--+-------------------+----------+--- Temp
*            !                   +
*          ICHG
*            |                   +
*  jt_ichg_h-+ - -+~~~~~~~~~~~~~~+~~~~~~~~~~+
*            +    |              +          |
*  jt_ichg_l-+~~~~+   Charge area           |
*            |    +              +          |
*         0--+----+--------------+----------+--- Temp
*            0   jt_temp_l      jt_temp_h   55
*/

#define RICOH619_DEV_REG 		\
	RICOH_REG(DC2, dc2, 0),		\
	RICOH_REG(DC4, dc4, 0),		\
	RICOH_REG(LDO1, ldo1, 0),	\
	RICOH_REG(LDO2, ldo2, 0),	\
	RICOH_REG(LDO3, ldo3, 0),	\
	RICOH_REG(LDO4, ldo4, 0),	\
	RICOH_REG(LDO5, ldo5, 0)

static struct ricoh619_subdev_info ricoh_devs_dcdc[] = {
	RICOH619_DEV_REG,
	RICOH619_PWRKEY_REG,
#ifdef CONFIG_RTC_DRV_R5T619
	RICOH_RTC_REG,
#endif
};

#define RICOH_GPIO_INIT(_init_apply, _output_mode, _output_val, _led_mode, _led_func) \
	{									\
		.output_mode_en = _output_mode,		\
		.output_val		= _output_val,	\
		.init_apply		= _init_apply,	\
		.led_mode		= _led_mode,	\
		.led_func		= _led_func,	\
	}
struct ricoh619_gpio_init_data ricoh_gpio_data[] = {
	RICOH_GPIO_INIT(false, false, 0, 0, 0),
	RICOH_GPIO_INIT(false, false, 0, 0, 0),
	RICOH_GPIO_INIT(false, false, 0, 0, 0),
	RICOH_GPIO_INIT(false, false, 0, 0, 0),
};

static struct ricoh619_platform_data ricoh_platform = {
	.num_subdevs		= ARRAY_SIZE(ricoh_devs_dcdc),
	.subdevs		= ricoh_devs_dcdc,
	.irq_base		= RICOH619_IRQ_BASE,
	.gpio_base		= RICOH619_GPIO_BASE,
	.gpio_init_data		= ricoh_gpio_data,
	.num_gpioinit_data	= ARRAY_SIZE(ricoh_gpio_data),
	.enable_shutdown_pin 	= true,
};

struct i2c_board_info __initdata ricoh619_regulator = {
	I2C_BOARD_INFO("ricoh619", 0x32),
	.irq		= PMIC_IRQ,
	.platform_data	= &ricoh_platform,
};

//FIXED_REGULATOR_DEF(usi_vbat, "USI_VDD", 3900*1000 ,GPIO_WLAN_PW_EN, 1, 0, 0, NULL,"usi_vbat", NULL);
static struct platform_device *fixed_regulator_devices[] __initdata = {
	//&usi_vbat_regulator_device,
};

static int __init pmu_dev_init(void)
{
	struct i2c_adapter *adap;
	struct i2c_client *client;
	int busnum = PMU_I2C_BUSNUM;
	int i;

	/*output high for pmu slp pin*/
	if (gpio_request_one(PMU_SLP_N,
				GPIOF_DIR_OUT, "pmu_slp_rst")) {
		pr_err("The GPIO %d is requested by other driver,"
				" not available for RICOH619\n",PMU_SLP_N);
	}
	gpio_direction_output(PMU_SLP_N, SLP_PIN_DISABLE_VALUE);

	if (gpio_request_one(PMU_IRQ_N,
				GPIOF_DIR_IN, "ricoh619_irq")) {
		pr_err("The GPIO %d is requested by other driver,"
				" not available for ricoh619\n", PMU_IRQ_N);
		ricoh619_regulator.irq = -1;
	} else {
		ricoh619_regulator.irq = gpio_to_irq(PMU_IRQ_N);
	}

	adap = i2c_get_adapter(busnum);
	if (!adap) {
		pr_err("failed to get adapter i2c%d\n", busnum);
		return -1;
	}

	client = i2c_new_device(adap, &ricoh619_regulator);
	if (!client) {
		pr_err("failed to register pmu to i2c%d\n", busnum);
		return -1;
	}

	i2c_put_adapter(adap);

	for (i = 0; i < ARRAY_SIZE(fixed_regulator_devices); i++)
		fixed_regulator_devices[i]->id = i;

	return platform_add_devices(fixed_regulator_devices,
				    ARRAY_SIZE(fixed_regulator_devices));
}
subsys_initcall_sync(pmu_dev_init);
