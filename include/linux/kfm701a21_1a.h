/*
 * LCD driver data for KFM701A21_1A
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _KFM701A21_1A_H
#define _KFM701A21_1A_H

/**
 * @gpio_lcd_im0: interface select pin
 * @gpio_lcd_im1: interface select pin
 * @gpio_lcd_im2: interface select pin
 * @gpio_lcd_reset: global reset pin, active low to enter reset state
 * @gpio_lcd_vsync: vertical synchronize signal input
 * @gpio_lcd_hsync: horizontal synchronize signal input
 * @gpio_lcd_dotclk: dot clock signal
 * @gpio_lcd_enable: synchronized with valid graphic data input
 * @gpio_lcd_sdo: serial data output
 * @gpio_lcd_sdi: serial data input
 * @gpio_lcd_rd: read execution control pin
 * @gpio_lcd_wr_scl: write execution control pin
 * @gpio_lcd_rs: data/command write select pin
 * @gpio_lcd_cs: chip select input pin
 */
struct platform_kfm701a21_1a_data {
	unsigned int gpio_lcd_im0;
	unsigned int gpio_lcd_im1;
	unsigned int gpio_lcd_im2;
	unsigned int gpio_lcd_reset;
	unsigned int gpio_lcd_vsync;
	unsigned int gpio_lcd_hsync;
	unsigned int gpio_lcd_dotclk;
	unsigned int gpio_lcd_enable;
	unsigned int gpio_lcd_sdo;
	unsigned int gpio_lcd_sdi;
	unsigned int gpio_lcd_rd;
	unsigned int gpio_lcd_wr_scl;
	unsigned int gpio_lcd_rs;
	unsigned int gpio_lcd_cs;
};

#endif /* _KFM701A21_1A_H */
