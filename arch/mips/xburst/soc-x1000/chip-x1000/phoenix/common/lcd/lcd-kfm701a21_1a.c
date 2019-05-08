/*
 * Copyright (c) 2015 Engenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * JZ_x1000 fpga board lcd setup routines.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/lcd.h>
#include <linux/regulator/consumer.h>
#include <mach/jzfb.h>
#include "../board_base.h"
#include "board.h"
#include <linux/kfm701a21_1a.h>

static struct platform_kfm701a21_1a_data kfm701a21_1a_pdata = {
	 .gpio_lcd_cs = GPIO_LCD_CS,
	 .gpio_lcd_reset = GPIO_LCD_RST,
};

/* LCD Panel Device */
struct platform_device kfm701a21_1a_device = {
	.name = "kfm701a21_1a_slcd",
	.dev = {
		.platform_data = &kfm701a21_1a_pdata,
		},
};

static struct smart_lcd_data_table kfm701a21_1a_data_table[] = {
	/* soft reset */
	{SMART_CONFIG_CMD,	0x0600},{SMART_CONFIG_DATA,	0x0001},{SMART_CONFIG_UDELAY,10000},
	/* soft reset */
	{SMART_CONFIG_CMD,	0x0600},{SMART_CONFIG_DATA,	0x0000},{SMART_CONFIG_UDELAY,10000},
	{SMART_CONFIG_CMD,	0x0606},{SMART_CONFIG_DATA,	0x0000},{SMART_CONFIG_UDELAY,10000},
	{SMART_CONFIG_CMD,	0x0007},{SMART_CONFIG_DATA,	0x0001},{SMART_CONFIG_UDELAY,10000},
	{SMART_CONFIG_CMD,	0x0110},{SMART_CONFIG_DATA,	0x0001},{SMART_CONFIG_UDELAY,10000},
	{SMART_CONFIG_CMD,	0x0100},{SMART_CONFIG_DATA,	0x17b0},
	{SMART_CONFIG_CMD,	0x0101},{SMART_CONFIG_DATA,	0x0147},
	{SMART_CONFIG_CMD,	0x0102},{SMART_CONFIG_DATA,	0x019d},
	{SMART_CONFIG_CMD,	0x0103},{SMART_CONFIG_DATA,	0x8600},
	{SMART_CONFIG_CMD,	0x0281},{SMART_CONFIG_DATA,	0x0010},{SMART_CONFIG_UDELAY,10000},
	{SMART_CONFIG_CMD,	0x0102},{SMART_CONFIG_DATA,	0x01bd},{SMART_CONFIG_UDELAY,10000},
	/* initial */
	{SMART_CONFIG_CMD,	0x0000},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0001},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0002},{SMART_CONFIG_DATA,	0x0400},
	/* up:0x1288 down:0x12B8 left:0x1290 right:0x12A0 */
	{SMART_CONFIG_CMD,	0x0003},	/* BGR */
	{SMART_CONFIG_DATA,	0x12b8},
	/*{0x0003, 0x02b8, 0, 0}, *//* RGB */
	{SMART_CONFIG_CMD,	0x0006},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0008},{SMART_CONFIG_DATA,	0x0503},
	{SMART_CONFIG_CMD,	0x0009},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x000b},{SMART_CONFIG_DATA,	0x0010},
	{SMART_CONFIG_CMD,	0x000c},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x000f},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0007},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x0010},{SMART_CONFIG_DATA,	0x0010},
	{SMART_CONFIG_CMD,	0x0011},{SMART_CONFIG_DATA,	0x0202},
	{SMART_CONFIG_CMD,	0x0012},{SMART_CONFIG_DATA,	0x0300},
	{SMART_CONFIG_CMD,	0x0020},{SMART_CONFIG_DATA,	0x021e},
	{SMART_CONFIG_CMD,	0x0021},{SMART_CONFIG_DATA,	0x0202},
	{SMART_CONFIG_CMD,	0x0022},{SMART_CONFIG_DATA,	0x0100},
	{SMART_CONFIG_CMD,	0x0090},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0092},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0100},{SMART_CONFIG_DATA,	0x16b0},
	{SMART_CONFIG_CMD,	0x0101},{SMART_CONFIG_DATA,	0x0147},
	{SMART_CONFIG_CMD,	0x0102},{SMART_CONFIG_DATA,	0x01bd},
	{SMART_CONFIG_CMD,	0x0103},{SMART_CONFIG_DATA,	0x2c00},
	{SMART_CONFIG_CMD,	0x0107},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0110},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x0210},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0211},{SMART_CONFIG_DATA,	0x00ef},
	{SMART_CONFIG_CMD,	0x0212},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0213},{SMART_CONFIG_DATA,	0x018f},
	{SMART_CONFIG_CMD,	0x0280},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0281},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x0282},{SMART_CONFIG_DATA,	0x0000},
	/* gamma corrected value table */
	{SMART_CONFIG_CMD,	0x0300},{SMART_CONFIG_DATA,	0x0101},
	{SMART_CONFIG_CMD,	0x0301},{SMART_CONFIG_DATA,	0x0b27},
	{SMART_CONFIG_CMD,	0x0302},{SMART_CONFIG_DATA,	0x132a},
	{SMART_CONFIG_CMD,	0x0303},{SMART_CONFIG_DATA,	0x2a13},
	{SMART_CONFIG_CMD,	0x0304},{SMART_CONFIG_DATA,	0x270b},
	{SMART_CONFIG_CMD,	0x0305},{SMART_CONFIG_DATA,	0x0101},
	{SMART_CONFIG_CMD,	0x0306},{SMART_CONFIG_DATA,	0x1205},
	{SMART_CONFIG_CMD,	0x0307},{SMART_CONFIG_DATA,	0x0512},
	{SMART_CONFIG_CMD,	0x0308},{SMART_CONFIG_DATA,	0x0005},
	{SMART_CONFIG_CMD,	0x0309},{SMART_CONFIG_DATA,	0x0003},
	{SMART_CONFIG_CMD,	0x030a},{SMART_CONFIG_DATA,	0x0f04},
	{SMART_CONFIG_CMD,	0x030b},{SMART_CONFIG_DATA,	0x0f00},
	{SMART_CONFIG_CMD,	0x030c},{SMART_CONFIG_DATA,	0x000f},
	{SMART_CONFIG_CMD,	0x030d},{SMART_CONFIG_DATA,	0x040f},
	{SMART_CONFIG_CMD,	0x030e},{SMART_CONFIG_DATA,	0x0300},
	{SMART_CONFIG_CMD,	0x030f},{SMART_CONFIG_DATA,	0x0500},
	/* secorrect gamma2 */
	{SMART_CONFIG_CMD,	0x0400},{SMART_CONFIG_DATA,	0x3500},
	{SMART_CONFIG_CMD,	0x0401},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x0404},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0500},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0501},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0502},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0503},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0504},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0505},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0600},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x0606},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x06f0},{SMART_CONFIG_DATA,	0x0000},
	{SMART_CONFIG_CMD,	0x07f0},{SMART_CONFIG_DATA,	0x5420},
	{SMART_CONFIG_CMD,	0x07f3},{SMART_CONFIG_DATA,	0x288a},
	{SMART_CONFIG_CMD,	0x07f4},{SMART_CONFIG_DATA,	0x0022},
	{SMART_CONFIG_CMD,	0x07f5},{SMART_CONFIG_DATA,	0x0001},
	{SMART_CONFIG_CMD,	0x07f0},{SMART_CONFIG_DATA,	0x0000},
	/* end of gamma corrected value table */
	{SMART_CONFIG_CMD,	0x0007},{SMART_CONFIG_DATA,	0x0173},
	/* Write Data to GRAM */
	{SMART_CONFIG_CMD,	0x0202},{SMART_CONFIG_UDELAY, 10000},
	/* Set the start address of screen, for example (0, 0) */
	{SMART_CONFIG_CMD,	0x200},{SMART_CONFIG_DATA,	0},{SMART_CONFIG_UDELAY, 1},
	{SMART_CONFIG_CMD,	0x201},{SMART_CONFIG_DATA,	0},{SMART_CONFIG_UDELAY, 1},
	{SMART_CONFIG_CMD,	0x202},{SMART_CONFIG_UDELAY, 100},
};

unsigned long kfm701a21_1a_cmd_buf[] = {
	0x02020202,
};

struct fb_videomode jzfb_videomode = {
	.name = "400x240",
	.refresh = 60,
	.xres = 400,
	.yres = 240,
	.pixclock = KHZ2PICOS(5760),
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.hsync_len = 0,
	.vsync_len = 0,
	.sync = FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
};

struct jzfb_platform_data jzfb_pdata = {

	.num_modes = 1,
	.modes = &jzfb_videomode,
	.lcd_type = LCD_TYPE_SLCD,
	.bpp = 18,
	.width = 39,
	.height = 65,
	.pinmd = 0,

	.smart_config.rsply_cmd_high = 0,
	.smart_config.csply_active_high = 0,
	.smart_config.newcfg_fmt_conv = 1,


	.smart_config.write_gram_cmd = kfm701a21_1a_cmd_buf,
	.smart_config.length_cmd = ARRAY_SIZE(kfm701a21_1a_cmd_buf),
	.smart_config.bus_width = 16,
	.smart_config.data_table = kfm701a21_1a_data_table,
	.smart_config.length_data_table = ARRAY_SIZE(kfm701a21_1a_data_table),
	.dither_enable = 0,
};
