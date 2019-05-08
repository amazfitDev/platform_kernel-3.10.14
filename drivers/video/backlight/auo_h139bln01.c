/*
 * Copyright (C) 2015 Ingenic Electronics
 *
 * AUO 1.39 400*400 MIPI LCD Driver
 *
 * Model : H139BLN01.1
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

struct auo_h139bln01_dev {
	struct device *dev;
	unsigned int id;
	unsigned int power;

	struct lcd_device *ld;
	struct backlight_device *bd;
	struct mipi_dsim_lcd_device *dsim_dev;
	struct lcd_platform_data    *ddi_pd;
	struct mutex lock;
};

#ifdef CONFIG_PM
static void auo_h139bln01_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power);
#endif

static void auo_h139bln01_brightness_setting(struct auo_h139bln01_dev *lcd, int brightness)
{
	unsigned char brightness_cmd_table[] = {
			0x15, 0x51, (unsigned char)brightness,
			0x15, 0x53, 0x20,
	};
	int array_size = ARRAY_SIZE(brightness_cmd_table);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, brightness_cmd_table, array_size);
}

static int auo_h139bln01_update_status(struct backlight_device *bd)
{
	struct auo_h139bln01_dev *lcd = dev_get_drvdata(&bd->dev);
	int brightness = bd->props.brightness;

	auo_h139bln01_brightness_setting(lcd, brightness);
	return 0;
}

static struct backlight_ops auo_h139bln01_backlight_ops = {
	.update_status = auo_h139bln01_update_status,
};

void auo_h139bln01_nop(struct auo_h139bln01_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x00, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void auo_h139bln01_sleep_in(struct auo_h139bln01_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x10, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void auo_h139bln01_sleep_out(struct auo_h139bln01_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x11, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void auo_h139bln01_display_on(struct auo_h139bln01_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x29, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void auo_h139bln01_display_off(struct auo_h139bln01_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x28, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(dsi, data_to_send, array_size);
}

static void auo_h139bln01_panel_condition_setting(struct auo_h139bln01_dev *lcd)
{
	int array_size;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	unsigned char auo_h139bln01_cmd_list[] = {
			0x15, 0xFE, 0x05,
			0x15, 0x05, 0x00,
			0x15, 0xFE, 0x07,
			0x15, 0x07, 0x4F,
			0x15, 0xFE, 0x0A,
			0x15, 0x1C, 0x1B,
			0x15, 0xFE, 0x00,
			0x15, 0x35, 0x00,
	};
	array_size = ARRAY_SIZE(auo_h139bln01_cmd_list);

	auo_h139bln01_nop(lcd); // do nothing, just for logic integrity
	mdelay(4); // used to avoid screen corrosion/pitting

	ops->cmd_write(dsi, auo_h139bln01_cmd_list, array_size);

	auo_h139bln01_sleep_out(lcd);
//	mdelay(2);
	auo_h139bln01_display_on(lcd);

	return;
}

static void auo_h139bln01_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct auo_h139bln01_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd->ddi_pd->lcd_enabled) {
		mutex_lock(&lcd->lock);
		auo_h139bln01_panel_condition_setting(lcd);
		auo_h139bln01_sleep_out(lcd);
		lcd->power = FB_BLANK_UNBLANK;
		mutex_unlock(&lcd->lock);
	}

	return;
}

static int auo_h139bln01_ioctl(struct mipi_dsim_lcd_device *dsim_dev, int cmd)
{
	struct auo_h139bln01_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd) {
		pr_err(" auo_h139bln01_ioctl get drv failed\n");
		return -EFAULT;
	}

	mutex_lock(&lcd->lock);
	switch (cmd) {
	case CMD_MIPI_DISPLAY_ON:
		auo_h139bln01_display_on(lcd);
#ifdef CONFIG_PM
		auo_h139bln01_power_on(dsim_dev, POWER_ON_BL);
#endif
		break;
	default:
		break;
	}
	mutex_unlock(&lcd->lock);

	return 0;
}

static int auo_h139bln01_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct auo_h139bln01_dev *lcd = NULL;
	struct backlight_properties props;

	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct auo_h139bln01_dev), GFP_KERNEL);
	if (!lcd) {
		dev_err(&dsim_dev->dev, "failed to allocate auo_h139bln01_dev structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->ddi_pd = (struct lcd_platform_data *)dsim_dev->platform_data;
	lcd->dev = &dsim_dev->dev;

	mutex_init(&lcd->lock);

	lcd->ld = lcd_device_register("auo_h139bln01_dev", lcd->dev, lcd, NULL);
	if (IS_ERR(lcd->ld)) {
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	props.type = BACKLIGHT_RAW;
	props.max_brightness = 255;
	lcd->bd = backlight_device_register("pwm-backlight.0", lcd->dev, lcd,
										&auo_h139bln01_backlight_ops, &props);
	if (IS_ERR(lcd->bd)) {
		dev_err(lcd->dev, "failed to register 'pwm-backlight.0'.\n");
		return PTR_ERR(lcd->bd);
	}
	dev_set_drvdata(&dsim_dev->dev, lcd);
	dev_dbg(lcd->dev, "probed auo_h139bln01_dev panel driver.\n");

	return 0;
}

#ifdef CONFIG_PM
static void auo_h139bln01_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct auo_h139bln01_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd->ddi_pd->lcd_enabled) {
		/* lcd power on */
		if (lcd->ddi_pd->power_on) {
			lcd->ddi_pd->power_on(lcd->ld, power);
		}

		if (power != POWER_ON_BL) {
			/* lcd reset */
			if (lcd->ddi_pd->reset) {
				lcd->ddi_pd->reset(lcd->ld);
			}
		}
	}

	return;
}

static int auo_h139bln01_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct auo_h139bln01_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	lcd->ddi_pd->lcd_enabled = 0;

	mutex_lock(&lcd->lock);

	auo_h139bln01_sleep_in(lcd);
	msleep(120);
	auo_h139bln01_display_off(lcd);
	auo_h139bln01_power_on(dsim_dev, !POWER_ON_LCD);

	mutex_unlock(&lcd->lock);

	return 0;
}
#else
#define auo_h139bln01_suspend		NULL
#define auo_h139bln01_resume		NULL
#define auo_h139bln01_power_on		NULL
#endif


static struct mipi_dsim_lcd_driver auo_h139bln01_dsim_ddi_driver = {
	.name = "auo_h139bln01-lcd",
	.id   = 0,
	.set_sequence = auo_h139bln01_set_sequence,
	.ioctl    = auo_h139bln01_ioctl,
	.probe    = auo_h139bln01_probe,
	.suspend  = auo_h139bln01_suspend,
	.power_on = auo_h139bln01_power_on,
};

static int auo_h139bln01_init(void)
{
	mipi_dsi_register_lcd_driver(&auo_h139bln01_dsim_ddi_driver);
	return 0;
}

static void auo_h139bln01_exit(void)
{
	return;
}

module_init(auo_h139bln01_init);
module_exit(auo_h139bln01_exit);

MODULE_DESCRIPTION("AUO 1.39 400*400 MIPI LCD Driver");
MODULE_LICENSE("GPL");
