/*
 * board-power-sm5007.c
 *
 * Copyright (C) 2012-2014, SILICONMITUS Company,Ltd.
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
#include <linux/mfd/sm5007.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/sm5007-regulator.h>
#include <linux/power/sm5007-fuelgauge.h>
#include <linux/power/sm5007_charger.h>
#include <linux/rtc.h>
#include "board_base.h"

#define PMIC_IRQ	0

/* If don't use the GPIO function, Set this macro to -1 */
#define INT_PMIC_BASE	IRQ_RESERVED_BASE

/* SM5007 IRQs */
#define SM5007_IRQ_BASE	INT_PMIC_BASE

static struct regulator_consumer_supply sm5007_buck1_supply_0[] = {
	REGULATOR_SUPPLY(BUCK1_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_buck1_dvs_supply_0[] = {
	REGULATOR_SUPPLY(BUCK1_DVS_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_buck2_supply_0[] = {
	REGULATOR_SUPPLY(BUCK2_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_buck3_supply_0[] = {
	REGULATOR_SUPPLY(BUCK3_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_buck4_supply_0[] = {
	REGULATOR_SUPPLY(BUCK4_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo1_supply_0[] = {
	REGULATOR_SUPPLY(LDO1_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo2_supply_0[] = {
	REGULATOR_SUPPLY(LDO2_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo3_supply_0[] = {
	REGULATOR_SUPPLY(LDO3_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo4_supply_0[] = {
	REGULATOR_SUPPLY(LDO4_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo5_supply_0[] = {
	REGULATOR_SUPPLY(LDO5_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo6_supply_0[] = {
	REGULATOR_SUPPLY(LDO6_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo7_supply_0[] = {
	REGULATOR_SUPPLY(LDO7_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo8_supply_0[] = {
	REGULATOR_SUPPLY(LDO8_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ldo9_supply_0[] = {
	REGULATOR_SUPPLY(LDO9_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ps1_supply_0[] = {
	REGULATOR_SUPPLY(PS1_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ps2_supply_0[] = {
	REGULATOR_SUPPLY(PS2_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ps3_supply_0[] = {
	REGULATOR_SUPPLY(PS3_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ps4_supply_0[] = {
	REGULATOR_SUPPLY(PS4_NAME, NULL),
};
static struct regulator_consumer_supply sm5007_ps5_supply_0[] = {
	REGULATOR_SUPPLY(PS5_NAME, NULL),
};

#define SM_PDATA_INIT(_name, _sname, _minmv, _maxmv, _supply_reg, _always_on, \
		_boot_on, _apply_uv, _init_uV, _init_enable, _init_apply, _flags, \
		_ext_contol, _ds_slots, _lpm) \
static struct sm5007_regulator_platform_data pdata_##_name##_##_sname = \
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
		ARRAY_SIZE(sm5007_##_name##_supply_##_sname),	\
		.consumer_supplies = sm5007_##_name##_supply_##_sname, \
		.supply_regulator = _supply_reg,		\
	},							\
	.init_uV =  _init_uV * 1000,				\
	.init_enable = _init_enable,				\
	.init_apply = _init_apply,				\
	.lpm = _lpm,                    \
	.sleep_slots = _ds_slots,				\
	.flags = _flags,					\
	.ext_pwr_req = _ext_contol,				\
}

/* min_uV/max_uV : Please set the appropriate value for the devices that
   the power supplied within a range from min to max voltage according
   to SM5007 specification. */
/*_name,_sname,_minmv,_maxmv,_supply_reg,_always_on,_boot_on,_apply_uv,
  _init_uV,_init_enable,_init_apply,_flags,_ext_contol,_ds_slots) */
SM_PDATA_INIT(buck1, 0,	700,   1300, 0, BUCK1_ALWAYS_ON, BUCK1_BOOT_ON, 1,
		BUCK1_INIT_UV, BUCK1_INIT_ENABLE, 1, 0, 0, 0, BUCK1_LPM);

SM_PDATA_INIT(buck1_dvs, 0,	600,   3500, 0, BUCK1_ALWAYS_ON, BUCK1_BOOT_ON, 1,
		BUCK1_INIT_UV, BUCK1_INIT_ENABLE, 1, 0, 0, 0, BUCK1_LPM);

SM_PDATA_INIT(buck2, 0,	1200,   1200, 0, BUCK2_ALWAYS_ON, BUCK2_BOOT_ON, 1,
		BUCK2_INIT_UV, 1, BUCK2_INIT_ENABLE, 0, 0, 0, BUCK2_LPM);

SM_PDATA_INIT(buck3, 0,	3300,   3300, 0, BUCK3_ALWAYS_ON, BUCK3_BOOT_ON, 1,
		BUCK3_INIT_UV, BUCK3_INIT_ENABLE, 1, 0, 0, 0, BUCK3_LPM);

SM_PDATA_INIT(buck4, 0,	1800,   3300, 0, BUCK4_ALWAYS_ON, BUCK4_BOOT_ON, 1,
		BUCK4_INIT_UV, BUCK4_INIT_ENABLE, 1, 0, 0, 0, BUCK4_LPM);

SM_PDATA_INIT(ldo1, 0,	800,   3300, 0, LDO1_ALWAYS_ON, LDO1_BOOT_ON, 1,
		LDO1_INIT_UV, LDO1_INIT_ENABLE, 1, 0, 0, 0, LDO1_LPM);

SM_PDATA_INIT(ldo2, 0,	800,   3300, 0, LDO2_ALWAYS_ON, LDO2_BOOT_ON, 1,
		LDO2_INIT_UV, LDO2_INIT_ENABLE, 1, 0, 0, 0, LDO2_LPM);

SM_PDATA_INIT(ldo3, 0,	1800,   1800, 0, LDO3_ALWAYS_ON, LDO3_BOOT_ON, 1,
		LDO3_INIT_UV, LDO3_INIT_ENABLE, 1, 0, 0, 0, LDO3_LPM);

SM_PDATA_INIT(ldo4, 0,	2800,   2800, 0, LDO4_ALWAYS_ON, LDO4_BOOT_ON, 1,
		LDO4_INIT_UV, LDO4_INIT_ENABLE, 1, 0, 0, 0, LDO4_LPM);

SM_PDATA_INIT(ldo5, 0,	1800,   1800, 0, LDO5_ALWAYS_ON, LDO5_BOOT_ON, 1,
		LDO5_INIT_UV, LDO5_INIT_ENABLE, 1, 0, 0, 0, LDO5_LPM);

SM_PDATA_INIT(ldo6, 0,	1800,   1800, 0, LDO6_ALWAYS_ON, LDO6_BOOT_ON, 1,
		LDO6_INIT_UV, LDO6_INIT_ENABLE, 1, 0, 0, 0, LDO6_LPM);

SM_PDATA_INIT(ldo7, 0,	800,   3300, 0, LDO7_ALWAYS_ON, LDO7_BOOT_ON, 1,
		LDO7_INIT_UV, LDO7_INIT_ENABLE, 1, 0, 0, 0, LDO7_LPM);

SM_PDATA_INIT(ldo8, 0,	800,   3300, 0, LDO8_ALWAYS_ON, LDO8_BOOT_ON, 1,
		LDO8_INIT_UV, LDO8_INIT_ENABLE, 1, 0, 0, 0, LDO8_LPM);

SM_PDATA_INIT(ldo9, 0,	800,   3300, 0, LDO9_ALWAYS_ON, LDO9_BOOT_ON, 1,
		LDO9_INIT_UV, LDO9_INIT_ENABLE, 1, 0, 0, 0, LDO9_LPM);

SM_PDATA_INIT(ps1, 0,	700,   1300, 0, PS1_ALWAYS_ON, PS1_BOOT_ON, 1,
		PS1_INIT_UV, PS1_INIT_ENABLE, 1, 0, 0, 0, PS1_LPM);
SM_PDATA_INIT(ps2, 0,	1200,   1200, 0, PS2_ALWAYS_ON, PS2_BOOT_ON, 1,
		PS2_INIT_UV, PS2_INIT_ENABLE, 1, 0, 0, 0, PS2_LPM);
SM_PDATA_INIT(ps3, 0,   3300,   3300, 0, PS3_ALWAYS_ON, PS3_BOOT_ON, 1,
		PS1_INIT_UV, PS1_INIT_ENABLE, 1, 0, 0, 0, PS3_LPM);
SM_PDATA_INIT(ps4, 0,	1800,   1800, 0, PS4_ALWAYS_ON, PS4_BOOT_ON, 1,
		PS4_INIT_UV, PS4_INIT_ENABLE, 1, 0, 0, 0, PS4_LPM);
SM_PDATA_INIT(ps5, 0,	1800,   1800, 0, PS5_ALWAYS_ON, PS5_BOOT_ON, 1,
		PS5_INIT_UV, PS5_INIT_ENABLE, 1, 0, 0, 0, PS5_LPM);

#define SM_REG(_id, _name, _sname)				\
{								\
	.id		= SM5007_ID_##_id,			\
	.name		= "sm5007-regulator",			\
	.platform_data	= &pdata_##_name##_##_sname,		\
}

#define SM5007_DECLARE_IRQ(irq) { \
	irq, irq, \
	irq##_NAME, IORESOURCE_IRQ }

const static struct resource sm5007_charger_res[] = {
	SM5007_DECLARE_IRQ(SM5007_IRQ_VBATOVP),
	SM5007_DECLARE_IRQ(SM5007_IRQ_TRITMROFF),
	SM5007_DECLARE_IRQ(SM5007_IRQ_FASTTMROFF),
	SM5007_DECLARE_IRQ(SM5007_IRQ_THEMREG),
	SM5007_DECLARE_IRQ(SM5007_IRQ_THEMSHDN),
	SM5007_DECLARE_IRQ(SM5007_IRQ_1STTHEMWARN),
	SM5007_DECLARE_IRQ(SM5007_IRQ_2NDTHEMWARN ),
	SM5007_DECLARE_IRQ(SM5007_IRQ_THEMSAFE),

	SM5007_DECLARE_IRQ(SM5007_IRQ_VBUSPOK),
	SM5007_DECLARE_IRQ(SM5007_IRQ_VBUSUVLO),
	SM5007_DECLARE_IRQ(SM5007_IRQ_VBUSOVP),
	SM5007_DECLARE_IRQ(SM5007_IRQ_CHGON),
	SM5007_DECLARE_IRQ(SM5007_IRQ_TOPOFF),
	SM5007_DECLARE_IRQ(SM5007_IRQ_DONE),
	SM5007_DECLARE_IRQ(SM5007_IRQ_CHGRSTF),
};

static struct sm5007_fuelgauge_platform_data sm5007_fuelgauge_data = {
	.irq 		= IRQ_RESERVED_BASE,
	.alarm_vol_mv 	= 0, /* the voltage(mV) to report soc 0%, for battery safe, 0mV means diable */

	/* some parameter is depend of battery type */
	/*the battery for LARGE_CAPACITY_BATTERY*/
	.type[0] = {
		.id = 0,
		.battery_type = 4350,		/* 4.200V 4.350V 4.400V */
		//.battery_table = battery_table_para,
		.temp_std = 25,
		.temp_offset = 10,
		.temp_offset_cal = 0x01,
		.charge_offset_cal = 0x00,
		.rce_value[0] = 0x0749,		/*rs mix_factor max min*/
		.rce_value[1] = 0x0580,
		.rce_value[2] = 0x371,
		.dtcd_value = 0x1,
		.rs_value[0] = 0x1ae,		/*rs mix_factor max min*/
		.rs_value[1] = 0x47a,
		.rs_value[2] = 0x3800,
		.rs_value[3] = 0x00a4,
		.vit_period = 0x3506,
		.mix_value[0] = 0x0a03,		/*mix_rate init_blank*/
		.mix_value[1] = 0x0004,
		.topoff_soc[0] = 0x0,		/*enable soc*/
		.topoff_soc[1] = 0x000,
		.volt_cal = 0x8000,
		.curr_cal = 0x8000,
	},
	/*the battery for 320mAh*/
	.type[1] = {
		.id = 0,
		.battery_type = 4200,                       /* 4.150V */
		//.battery_table = battery_data_table0,
		.temp_std = 25,
		.temp_offset = 10,
		.temp_offset_cal = 0x01,
		.charge_offset_cal = 0x00,
		.rce_value[0] = 0x0749,
		.rce_value[1] = 0x0580,
		.rce_value[2] = 0x371,
		.dtcd_value = 0x1,
		.rs_value[0] = 0x1ae,		/*rs mix_factor max min*/
		.rs_value[1] = 0x47a,
		.rs_value[2] = 0x3800,
		.rs_value[3] = 0x00a4,
		.vit_period = 0x3506,
		.mix_value[0] = 0x0a03,		/*mix_rate init_blank*/
		.mix_value[1] = 0x0004,
		.topoff_soc[0] = 0x0,		/*enable topoff_soc  1: enable, 0 disable*/
		.topoff_soc[1] = 0x000,		/*topoff_soc threadhold*/
		.volt_cal = 0x8000,
		.curr_cal = 0x8000,
	},
};

#define SM5007_BATTERY_REG    				\
{								\
	.id			= -1,    				\
	.name		= "sm5007-charger",     		\
	.num_resources  = ARRAY_SIZE(sm5007_charger_res),    				\
	.resources      = sm5007_charger_res,           \
	.platform_data	= &sm5007_charger_data,    		\
}

//==========================================
//SM5007 Power_Key device data
//==========================================
static struct sm5007_pwrkey_platform_data sm5007_pwrkey_data = {
	.irq 		= SM5007_IRQ_BASE,
	.delay_ms 	= 20,
};
#define SM5007_PWRKEY_REG 					\
{ 								\
	.id 		= -1, 					\
	.name 		= "sm5007-pwrkey", 			\
	.platform_data 	= &sm5007_pwrkey_data, 		\
}


static struct sm5007_charger_platform_data sm5007_charger_data = {
	.irq 		= SM5007_IRQ_BASE,
	/* some parameter is depend of charger type */
	.type = {
#if defined(CONFIG_320MAH_BATTERY)
		.chg_batreg 	= 4200, /* mV */
#else
		.chg_batreg     = 4350,
#endif
		.chg_fastchg    = 200,   /* mA */
		.chg_autostop   = SM5007_AUTOSTOP_ENABLED,  /* autostop enable */
		.chg_rechg      = SM5007_RECHG_M100,  /* -100mV */
		.chg_topoff     = 10,   /* mA */
	},
};

#define SM5007_DEV_REG 		\
	SM_REG(BUCK1, buck1, 0),		\
SM_REG(BUCK1_DVS, buck1_dvs, 0),	\
SM_REG(BUCK2, buck2, 0),		\
SM_REG(BUCK3, buck3, 0),		\
SM_REG(BUCK4, buck4, 0),		\
SM_REG(LDO1, ldo1, 0),	\
SM_REG(LDO2, ldo2, 0),	\
SM_REG(LDO3, ldo3, 0),	\
SM_REG(LDO4, ldo4, 0),	\
SM_REG(LDO5, ldo5, 0),	\
SM_REG(LDO6, ldo6, 0),	\
SM_REG(LDO7, ldo7, 0),	\
SM_REG(LDO8, ldo8, 0),	\
SM_REG(LDO9, ldo9, 0),	\
SM_REG(PS1, ps1, 0),	\
SM_REG(PS2, ps2, 0),	\
SM_REG(PS3, ps3, 0),	\
SM_REG(PS4, ps4, 0),    \
SM_REG(PS5, ps5, 0)

static struct sm5007_subdev_info sm5007_devs_dcdc[] = {
	SM5007_DEV_REG,
	SM5007_BATTERY_REG,
	SM5007_PWRKEY_REG,
};

static struct sm5007_platform_data sm5007_platform = {
	.num_subdevs		= ARRAY_SIZE(sm5007_devs_dcdc),
	.subdevs		= sm5007_devs_dcdc,
	.irq_base		= SM5007_IRQ_BASE,
	.enable_shutdown_pin 	= true,
};

struct i2c_board_info __initdata sm5007_regulator = {
	I2C_BOARD_INFO("sm5007", 0x47),
	.irq		= PMIC_IRQ,
	.platform_data	= &sm5007_platform,
};

struct i2c_board_info __initdata sm5007_fuelgauge_i2c_info = {
	I2C_BOARD_INFO("sm5007-fuelgauge", 0x71),
	.irq		= 0,
	//	.platform_data	= &sm5007_fg_platform,
	.platform_data = &sm5007_fuelgauge_data,
};

//FIXED_REGULATOR_DEF(usi_vbat, "USI_VDD", 3900*1000 ,GPIO_WLAN_PW_EN, 1, 0, 0, NULL,"usi_vbat", NULL);
static struct platform_device *fixed_regulator_devices[] __initdata = {
	//&usi_vbat_regulator_device,
};

static int __init pmu_dev_init(void)
{
	struct i2c_adapter *adap, *adap_fg;
	struct i2c_client *client;
#ifdef CONFIG_SM5007_FUELGUAGE
	struct i2c_client *client_fg;
#endif
	int busnum = PMU_I2C_BUSNUM5007;
	int i;

	if (gpio_request_one(PMU_IRQ_N5007,
				GPIOF_DIR_IN, "sm5007_irq")) {
		pr_err("The GPIO %d is requested by other driver,"
				" not available for sm5007\n", PMU_IRQ_N5007);
		sm5007_regulator.irq = -1;
	} else {
		sm5007_regulator.irq = gpio_to_irq(PMU_IRQ_N5007);
	}

	adap = i2c_get_adapter(busnum);
	if (!adap) {
		pr_err("failed to get adapter i2c%d\n", busnum);
		return -1;
	}

	client = i2c_new_device(adap, &sm5007_regulator);
	if (!client) {
		pr_err("failed to register pmu to i2c%d\n", busnum);
		return -1;
	}

	i2c_put_adapter(adap);

	adap_fg = i2c_get_adapter(busnum);
	if (!adap_fg) {
		pr_err("failed to get adapter i2c%d\n", busnum);
		return -1;
	}

#ifdef CONFIG_SM5007_FUELGUAGE
	client_fg = i2c_new_device(adap_fg, &sm5007_fuelgauge_i2c_info);
	if (!client_fg) {
		pr_err("failed to register pmu to i2c%d\n", busnum);
		return -1;
	}
#endif

	i2c_put_adapter(adap_fg);

	for (i = 0; i < ARRAY_SIZE(fixed_regulator_devices); i++)
		fixed_regulator_devices[i]->id = i;

	return platform_add_devices(fixed_regulator_devices,
			ARRAY_SIZE(fixed_regulator_devices));
}
subsys_initcall_sync(pmu_dev_init);
