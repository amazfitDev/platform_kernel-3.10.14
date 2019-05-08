/*
 * Copyright (c) 2012 Engenic Semiconductor Co., Ltd.
 *              http://www.ingenic.com/
 *
 * JZ4780 orion board lcd setup routines.
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
#include <linux/digital_pulse_backlight.h>

#include <mach/jzfb.h>
#include <mach/fb_hdmi_modes.h>

#include "board.h"

#ifdef CONFIG_LCD_BYD_BM8766U
#include <linux/byd_bm8766u.h>
static struct platform_byd_bm8766u_data byd_bm8766u_pdata= {
	.gpio_lcd_disp = GPIO_PB(30),
	.gpio_lcd_de   = 0,		//GPIO_PC(9),	/* chose sync mode */
	.gpio_lcd_vsync = 0,		//GPIO_PC(19),
	.gpio_lcd_hsync = 0,		//GPIO_PC(18),
};

/* LCD device */
struct platform_device byd_bm8766u_device = {
	.name		= "byd_bm8766u-lcd",
	.dev		= {
		.platform_data = &byd_bm8766u_pdata,
	},
};
#endif

#ifdef CONFIG_LCD_KD50G2_40NM_A2
#include <linux/kd50g2_40nm_a2.h>
static struct platform_kd50g2_40nm_a2_data kd50g2_40nm_a2_pdata= {
	.gpio_lcd_disp = GPIO_PB(30),
	.gpio_lcd_de   = 0,		//GPIO_PC(9),	/* chose sync mode */
	.gpio_lcd_vsync = 0,		//GPIO_PC(19),
	.gpio_lcd_hsync = 0,		//GPIO_PC(18),
};

/* LCD device */
struct platform_device kd50g2_40nm_a2_device = {
	.name		= "kd50g2_40nm_a2-lcd",
	.dev		= {
		.platform_data = &kd50g2_40nm_a2_pdata,	
	},
};
#endif


/**************************************************************************************************/

struct fb_videomode jzfb_videomode = {
#ifdef CONFIG_LCD_BYD_BM8766U
	.name = "800x480",
	.refresh = 55,
	.xres = 800,
	.yres = 480,
	.pixclock = KHZ2PICOS(33260),
	.left_margin = 88,
	.right_margin = 40,
	.upper_margin = 8,
	.lower_margin = 35,
	.hsync_len = 128,
	.vsync_len = 2,
	.sync = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
#endif

#ifdef CONFIG_LCD_AT070TN93
	.name = "800x480",
	.refresh = 55,
	.xres = 800,
	.yres = 480,
	.pixclock = KHZ2PICOS(25863),
	.left_margin = 16,
	.right_margin = 48,
	.upper_margin = 7,
	.lower_margin = 23,
	.hsync_len = 30,
	.vsync_len = 16,
	.sync = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
#endif

#ifdef CONFIG_LCD_KD50G2_40NM_A2 // 60Hz@vpll=888MHz
	.name = "800x480",
	.refresh = 55,
	.xres = 800,
	.yres = 480,
	.pixclock = KHZ2PICOS(33260),
	.left_margin = 88,
	.right_margin = 40,
	.upper_margin = 8,
	.lower_margin = 35,
	.hsync_len = 128,
	.vsync_len = 2,
	.sync = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
#endif
#ifdef CONFIG_LCD_AUO_A043FL01V2
	.name = "480x272",
	.refresh = 60,
	.xres = 480,
	.yres = 272,
	.pixclock = KHZ2PICOS(9200),
	.left_margin = 4,
	.right_margin = 8,
	.upper_margin = 2,
	.lower_margin = 4,
	.hsync_len = 41,
	.vsync_len = 10,
	.sync = ~FB_SYNC_HOR_HIGH_ACT & ~FB_SYNC_VERT_HIGH_ACT,
	.vmode = FB_VMODE_NONINTERLACED,
	.flag = 0,
#endif
};


struct jzfb_platform_data jzfb_pdata = {
#ifdef CONFIG_LCD_BYD_BM8766U
	.num_modes = 1,
	.modes = &jzfb_videomode,

	.lcd_type = LCD_TYPE_GENERIC_24_BIT,
	.bpp = 24,
	.width = 108,
	.height = 65,

	.pixclk_falling_edge = 0,
	.date_enable_active_low = 0,

	.alloc_vidmem = 1,
#endif

#ifdef CONFIG_LCD_AT070TN93
	.num_modes = 1,
	.modes = &jzfb1_videomode,

	.lcd_type = LCD_TYPE_GENERIC_24_BIT,
	.bpp = 24,
	.width = 154,
	.height = 86,

	.pixclk_falling_edge = 0,
	.date_enable_active_low = 0,

	.alloc_vidmem = 1,
#endif

#ifdef CONFIG_LCD_KD50G2_40NM_A2
	.num_modes = 1,
	.modes = &jzfb_videomode,

	.lcd_type = LCD_TYPE_GENERIC_24_BIT,
	.bpp = 24,
	.width = 108,
	.height = 65,

	.pixclk_falling_edge = 0,
	.date_enable_active_low = 0,

	.alloc_vidmem = 1,
#endif
#ifdef CONFIG_LCD_AUO_A043FL01V2
	.num_modes = 1,
	.modes = &jzfb1_videomode,

	.lcd_type = LCD_TYPE_GENERIC_24_BIT,
	.bpp = 24,
	.width = 95,
	.height = 54,

	.pixclk_falling_edge = 0,
	.date_enable_active_low = 0,

	.alloc_vidmem = 1,
#endif
};

/**************************************************************************************************/

#ifdef CONFIG_BACKLIGHT_PWM
static int backlight_init(struct device *dev)
{
	int ret;
	ret = gpio_request(GPIO_LCD_PWM, "Backlight");
	if (ret) {
		printk(KERN_ERR "failed to request GPF for PWM-OUT1\n");
		return ret;
	}

	/* Configure GPIO pin with S5P6450_GPF15_PWM_TOUT1 */
	//gpio_direction_output(GPIO_LCD_PWM, 1);

	return 0;
}

static void backlight_exit(struct device *dev)
{
	gpio_free(GPIO_LCD_PWM);
}

static struct platform_pwm_backlight_data backlight_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 120,
	.pwm_period_ns	= 30000,
	.init		= backlight_init,
	.exit		= backlight_exit,
};

struct platform_device backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.platform_data	= &backlight_data,
	},
};

#endif

/***********************************************************************************************/
#ifdef CONFIG_BACKLIGHT_DIGITAL_PULSE
static int init_backlight(struct device *dev)
{
	return 0;
}
static void exit_backlight(struct device *dev)
{

}

struct platform_digital_pulse_backlight_data bl_data = {
	.digital_pulse_gpio = GPIO_PE(1),
	.max_brightness = 255,
	.dft_brightness = 120,
	.max_brightness_step = 16,
	.high_level_delay_us = 5,
	.low_level_delay_us = 5,
	.init = init_backlight,
	.exit = exit_backlight,
};

struct platform_device digital_pulse_backlight_device = {
	.name		= "digital-pulse-backlight",
	.dev		= {
		.platform_data	= &bl_data,
	},
};
#endif
