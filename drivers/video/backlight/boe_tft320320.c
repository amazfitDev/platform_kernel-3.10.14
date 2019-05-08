/*
 * Copyright (C) 2015 Ingenic Electronics
 *
 * BOE 1.54 320*320 TFT LCD Driver
 *
 * Model : BV015Z2M-N00-2B00
 *
 * Author: MaoLei.Wang <maolei.wang@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/backlight.h>

#include <video/mipi_display.h>
#include <mach/jz_dsim.h>

#define POWER_IS_ON(pwr)	((pwr) == FB_BLANK_UNBLANK)
#define POWER_IS_OFF(pwr)	((pwr) == FB_BLANK_POWERDOWN)
#define POWER_IS_NRM(pwr)	((pwr) == FB_BLANK_NORMAL)

#define lcd_to_master(a)	(a->dsim_dev->master)
#define lcd_to_master_ops(a)	((lcd_to_master(a))->master_ops)

struct boe_tft320320_dev {
	struct device *dev;
	unsigned int id;
	unsigned int power;

	struct lcd_device *ld;
	struct mipi_dsim_lcd_device *dsim_dev;
	struct lcd_platform_data    *ddi_pd;
	struct mutex lock;
};

#ifdef CONFIG_PM
static void boe_tft320320_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power);
#endif

static void boe_tft320320_sleep_in(struct boe_tft320320_dev *lcd)
{
	unsigned char data_to_send[] = {0x05, 0x10, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void boe_tft320320_sleep_out(struct boe_tft320320_dev *lcd)
{
	unsigned char data_to_send[] = {0x05, 0x11, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void boe_tft320320_display_on(struct boe_tft320320_dev *lcd)
{
	unsigned char data_to_send[] = {0x05, 0x29, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void boe_tft320320_display_off(struct boe_tft320320_dev *lcd)
{
	unsigned char data_to_send[] = {0x05, 0x28, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void boe_tft320320_panel_condition_setting(struct boe_tft320320_dev *lcd)
{
	int array_size;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);

	unsigned char boe_tft320320_cmd_list[] = {
		0x39, 0x02, 0x00, 0xF0, 0xC3,
		0x39, 0x02, 0x00, 0xF0, 0x96,
		0x39, 0x04, 0x00, 0xb6, 0x8A, 0x07, 0x3b,
		0x39, 0x05, 0x00, 0xb5, 0x20, 0x20, 0x00, 0x20,
		0x39, 0x03, 0x00, 0xb1, 0x80, 0x10,
		0x39, 0x02, 0x00, 0xb4, 0x02,

		0x39, 0x02, 0x00, 0xc5, 0x28,
		0x39, 0x02, 0x00, 0xc1, 0x04,
#ifdef CONFIG_LCD_BOE_TFT320320_ROTATION_180
		0x39, 0x02, 0x00, 0x36, 0x88,
		0x39, 0x05, 0x00, 0x2a, 0x00,     0x00,       319 >> 8, 319 & 0xff,
		0x39, 0x05, 0x00, 0x2b, 160 >> 8, 160 & 0xff, 479 >> 8, 479 & 0xff,
		0x39, 0x03, 0x00, 0x44, 320 >> 8, 320 & 0xff,
#else // 0 angle rotation
		0x39, 0x02, 0x00, 0x36, 0x48,
		0x39, 0x05, 0x00, 0x2a, 0x00, 0x00, 319 >> 8, 319 & 0xff,
		0x39, 0x05, 0x00, 0x2b, 0x00, 0x00, 319 >> 8, 319 & 0xff,
		0x39, 0x03, 0x00, 0x44, 0x00, 0x00,
#endif
		0x05, 0x21, 0x00,
		0x39, 0x09, 0x00, 0xE8, 0x40, 0x84, 0x1b, 0x1b, 0x10, 0x03, 0xb8, 0x33,

		0x39, 0x0f, 0x00, 0xe0, 0x00, 0x03, 0x07, 0x07, 0x07, 0x23, 0x2b, 0x33, 0x46, 0x1a, 0x19, 0x19, 0x27, 0x2f,
		0x39, 0x0f, 0x00, 0xe1, 0x00, 0x03, 0x06, 0x07, 0x04, 0x22, 0x2f, 0x54, 0x49, 0x1b, 0x17, 0x15, 0x25, 0x2d,
		0x05, 0x12, 0x00,
		0x39, 0x05, 0x00, 0x30, 0x00, 0x00, 0x01, 0x3f,
		0x05, 0x35, 0x00,
		0x39, 0x02, 0x00, 0xe4, 0x31,
		0x39, 0x02, 0x00, 0xb9, 0x02,
		0x39, 0x02, 0x00, 0x3a, 0x77,
		0x39, 0x02, 0x00, 0xe4, 0x31,
		0x39, 0x02, 0x00, 0xF0, 0x3c,
		0x39, 0x02, 0x00, 0xF0, 0x69,
		/* {0x05, 0x29, 0x00}, */
	};

	array_size = ARRAY_SIZE(boe_tft320320_cmd_list);

	boe_tft320320_sleep_out(lcd);
//	udelay(10);
	ops->cmd_write(dsi, boe_tft320320_cmd_list, array_size);

	return;
}

static void boe_tft320320_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct boe_tft320320_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd->ddi_pd->lcd_enabled) {
		mutex_lock(&lcd->lock);
		boe_tft320320_panel_condition_setting(lcd);
		lcd->power = FB_BLANK_UNBLANK;
		mutex_unlock(&lcd->lock);
	}

	return;
}

static int boe_tft320320_ioctl(struct mipi_dsim_lcd_device *dsim_dev, int cmd)
{
	struct boe_tft320320_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd) {
		pr_err(" boe_tft320320_ioctl get drv failed\n");
		return -EFAULT;
	}

	mutex_lock(&lcd->lock);
	switch (cmd) {
	case CMD_MIPI_DISPLAY_ON:
		boe_tft320320_display_on(lcd);
#ifdef CONFIG_PM
		boe_tft320320_power_on(dsim_dev, POWER_ON_BL);
#endif
		break;
	default:
		break;
	}
	mutex_unlock(&lcd->lock);

	return 0;
}

static int boe_tft320320_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct boe_tft320320_dev *lcd = NULL;

	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct boe_tft320320_dev), GFP_KERNEL);
	if (!lcd) {
		dev_err(&dsim_dev->dev, "failed to allocate boe_tft320320_dev structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->ddi_pd = (struct lcd_platform_data *)dsim_dev->platform_data;
	lcd->dev = &dsim_dev->dev;

	mutex_init(&lcd->lock);

	lcd->ld = lcd_device_register("boe_tft320320_dev", lcd->dev, lcd, NULL);
	if (IS_ERR(lcd->ld)) {
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	dev_set_drvdata(&dsim_dev->dev, lcd);
	dev_dbg(lcd->dev, "probed boe_tft320320_dev panel driver.\n");

	return 0;
}

#ifdef CONFIG_PM
static void boe_tft320320_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct boe_tft320320_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	printk("####### %s %d\n", __func__, __LINE__);
	if (!lcd->ddi_pd->lcd_enabled) {
	printk("####### %s %d\n", __func__, __LINE__);
		/* lcd power on */
		if (lcd->ddi_pd->power_on) {
			printk("\n\n\nenable backlight power_on pin\n");
			lcd->ddi_pd->power_on(lcd->ld, power);
		}

		if (power == POWER_ON_LCD) {
			/* lcd reset */
			if (lcd->ddi_pd->reset) {
				lcd->ddi_pd->reset(lcd->ld);
			}
		}
	}

	return;
}

static int boe_tft320320_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct boe_tft320320_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	lcd->ddi_pd->lcd_enabled = 0;

	mutex_lock(&lcd->lock);
	boe_tft320320_power_on(dsim_dev, !POWER_ON_LCD);
	mutex_unlock(&lcd->lock);

	return 0;
}
#else
#define boe_tft320320_suspend		NULL
#define boe_tft320320_resume		NULL
#define boe_tft320320_power_on		NULL
#endif


static struct mipi_dsim_lcd_driver boe_tft320320_dsim_ddi_driver = {
	.name = "boe_tft320320-lcd",
	.id   = 0,
	.set_sequence = boe_tft320320_set_sequence,
	.ioctl    = boe_tft320320_ioctl,
	.probe    = boe_tft320320_probe,
	.suspend  = boe_tft320320_suspend,
	.power_on = boe_tft320320_power_on,
};

static int boe_tft320320_init(void)
{
	mipi_dsi_register_lcd_driver(&boe_tft320320_dsim_ddi_driver);
	return 0;
}

static void boe_tft320320_exit(void)
{
	return;
}

module_init(boe_tft320320_init);
module_exit(boe_tft320320_exit);

MODULE_DESCRIPTION("BOE TFT320320 lcd driver");
MODULE_LICENSE("GPL");
