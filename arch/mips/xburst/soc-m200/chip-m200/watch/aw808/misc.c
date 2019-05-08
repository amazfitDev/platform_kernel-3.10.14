/*
 * Copyright (c) 2014 Ingenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * M200 acrab board.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/fixed.h>
#include <soc/base.h>
#include "board.h"

#define FIXED_REGULATOR_DEF(NAME, SNAME, MV, GPIO, EH, EB, DELAY, SREG, SUPPLY, DEV_NAME)\
static struct regulator_consumer_supply NAME##_regulator_consumer =			\
	REGULATOR_SUPPLY(SUPPLY, DEV_NAME);						\
											\
static struct regulator_init_data NAME##_regulator_init_data = {			\
	.supply_regulator = SREG,							\
	.constraints = {								\
		.name = SNAME,								\
		.min_uV = MV,								\
		.max_uV = MV,								\
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,				\
	},										\
	.num_consumer_supplies = 1,							\
	.consumer_supplies = &NAME##_regulator_consumer,				\
};											\
											\
static struct fixed_voltage_config NAME##_regulator_data = {				\
	.supply_name = SNAME,								\
	.microvolts = MV,								\
	.gpio = GPIO,									\
	.enable_high = EH,								\
	.enabled_at_boot = EB,								\
	.startup_delay = DELAY,								\
	.init_data = &NAME##_regulator_init_data,					\
};											\
											\
static struct platform_device NAME##_regulator_device = {				\
	.name = "reg-fixed-voltage",							\
	.id = -1,									\
	.dev = {									\
		.platform_data = &NAME##_regulator_data,				\
	}										\
}

#if defined(CONFIG_TOUCHSCREEN_FT6X0X) || defined(CONFIG_TOUCHSCREEN_ITE7258)
FIXED_REGULATOR_DEF(tp_vio, "tp_vio", 1800 * 1000, GPIO_TP_EN, 1, 0, 10, NULL,
		"tp_vio", NULL);
#endif

#ifdef CONFIG_LCD_X163
FIXED_REGULATOR_DEF(lcd_blk, "lcd_blk_vcc", 4000 * 1000, GPIO_LCD_BLK_EN, 1, 1, 0, NULL,
		"lcd_blk_vcc", NULL);
#endif

static struct platform_device *fixed_regulator_devices[] __initdata = {
#if defined(CONFIG_TOUCHSCREEN_FT6X0X) || defined(CONFIG_TOUCHSCREEN_ITE7258)
	&tp_vio_regulator_device,
#endif
#ifdef CONFIG_LCD_X163
	&lcd_blk_regulator_device,
#endif
};

static int pmu_richoh619_init(void)
{
	int i;
	int ret = 0;
	for (i = 0; i < ARRAY_SIZE(fixed_regulator_devices); i++)
		fixed_regulator_devices[i]->id = i;

	 ret = platform_add_devices(fixed_regulator_devices,
				    ARRAY_SIZE(fixed_regulator_devices));
	 return 0;
}

static int uart1_function_init(void)
{
#define GPIO_PORT_D_BASE     GPIO_IOBASE+0x300
#define GPIO_PORT_D_SIZE     0x100
#define BLUETOOTH_UART_FUNC_CTS		(0x1<<27)
    void __iomem *base;

    base = ioremap(GPIO_PORT_D_BASE, GPIO_PORT_D_SIZE);
    if(base == NULL) {
        printk("uart1_function_init ioremap failed\n");
        return -1;
    }
    pr_debug("uart1_function_init set gpio_pd_28 function for uart1 cts\n");
    writel(BLUETOOTH_UART_FUNC_CTS, base + 0x18);
    writel(BLUETOOTH_UART_FUNC_CTS, base + 0x28);
    writel(BLUETOOTH_UART_FUNC_CTS, base + 0x34);
    writel(BLUETOOTH_UART_FUNC_CTS, base + 0x48);

    iounmap(base);

    return 0;
}

static int __init board_init(void)
{
	pmu_richoh619_init();
	uart1_function_init();
	return 0;
}

arch_initcall(board_init);
