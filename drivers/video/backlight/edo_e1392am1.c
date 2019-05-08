/*
 * Copyright (C) 2015 Ingenic Electronics
 *
 * EDO 1.4 400*400 MIPI LCD Driver
 *
 * Model : E1393AM1.A
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

struct edo_e1392am1_dev {
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
static void edo_e1392am1_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power);
#endif

static void edo_e1392am1_brightness_setting(struct edo_e1392am1_dev *lcd, int brightness)
{
	int array_size;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	unsigned char brightness_cmd_table[] = {
			0x15, 0x51, (unsigned char)brightness,
			0x15, 0x53, 0x20,
	};

	array_size = ARRAY_SIZE(brightness_cmd_table);
	ops->cmd_write(lcd_to_master(lcd), brightness_cmd_table, array_size);
}

static int edo_e1392am1_update_status(struct backlight_device *bd)
{
	struct edo_e1392am1_dev *lcd = dev_get_drvdata(&bd->dev);
	int brightness = bd->props.brightness;

	edo_e1392am1_brightness_setting(lcd, brightness);
	return 0;
}

static struct backlight_ops edo_e1392am1_backlight_ops = {
	.update_status = edo_e1392am1_update_status,
};

void edo_e1392am1_nop(struct edo_e1392am1_dev *lcd) /* nop */
{
	unsigned char data_to_send[] = {0x15, 0x00, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(lcd_to_master(lcd), data_to_send, array_size);
}

static void edo_e1392am1_sleep_in(struct edo_e1392am1_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x10, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(lcd_to_master(lcd), data_to_send, array_size);
}

static void edo_e1392am1_sleep_out(struct edo_e1392am1_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x11, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(lcd_to_master(lcd), data_to_send, array_size);
}

static void edo_e1392am1_display_on(struct edo_e1392am1_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x29, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(lcd_to_master(lcd), data_to_send, array_size);
}

static void edo_e1392am1_display_off(struct edo_e1392am1_dev *lcd)
{
	unsigned char data_to_send[] = {0x15, 0x28, 0x00};
	int array_size = ARRAY_SIZE(data_to_send);
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);
	ops->cmd_write(lcd_to_master(lcd), data_to_send, array_size);
}

unsigned char edo_e1392am1_cmd_list[] = {
		0x15, 0xFE, 0x04,
		0x15, 0x00, 0xdc,
		0x15, 0x01, 0x00,
		0x15, 0x02, 0x02,
		0x15, 0x03, 0x00,
		0x15, 0x04, 0x00,
		0x15, 0x05, 0x03,
		0x15, 0x06, 0x16,
		0x15, 0x07, 0x13,
		0x15, 0x08, 0x08,
		0x15, 0x09, 0xdc,
		0x15, 0x0a, 0x00,
		0x15, 0x0b, 0x02,
		0x15, 0x0c, 0x00,
		0x15, 0x0d, 0x00,
		0x15, 0x0e, 0x02,
		0x15, 0x0f, 0x16,
		0x15, 0x10, 0x18,
		0x15, 0x11, 0x08,
		0x15, 0x12, 0x92,
		0x15, 0x13, 0x00,
		0x15, 0x14, 0x02,
		0x15, 0x15, 0x05,
		0x15, 0x16, 0x40,
		0x15, 0x17, 0x03,
		0x15, 0x18, 0x16,
		0x15, 0x19, 0xd7,
		0x15, 0x1a, 0x01,
		0x15, 0x1b, 0xdc,
		0x15, 0x1c, 0x00,
		0x15, 0x1d, 0x04,
		0x15, 0x1e, 0x00,
		0x15, 0x1f, 0x00,
		0x15, 0x20, 0x03,
		0x15, 0x21, 0x16,
		0x15, 0x22, 0x18,
		0x15, 0x23, 0x08,
		0x15, 0x24, 0xdc,
		0x15, 0x25, 0x00,
		0x15, 0x26, 0x04,
		0x15, 0x27, 0x00,
		0x15, 0x28, 0x00,
		0x15, 0x29, 0x01,
		0x15, 0x2a, 0x16,
		0x15, 0x2b, 0x18,
		0x15, 0x2d, 0x08,
		0x15, 0x4c, 0x99,
		0x15, 0x4d, 0x00,
		0x15, 0x4e, 0x00,
		0x15, 0x4f, 0x00,
		0x15, 0x50, 0x01,
		0x15, 0x51, 0x0A,
		0x15, 0x52, 0x00,
		0x15, 0x5a, 0xe4,
		0x15, 0x5e, 0x77,
		0x15, 0x5f, 0x77,
		0x15, 0x60, 0x34,
		0x15, 0x61, 0x02,
		0x15, 0x62, 0x81,
		0x15, 0xFE, 0x07,
		0x15, 0x07, 0x4F,
		0x15, 0xFE, 0x01,
		0x15, 0x05, 0x15,
		0x15, 0x0E, 0x8B,
		0x15, 0x0F, 0x8B,
		0x15, 0x10, 0x11,
		0x15, 0x11, 0xA2,
		0x15, 0x12, 0x60,
		0x15, 0x14, 0x01,
		0x15, 0x15, 0x82,
		0x15, 0x18, 0x47,
		0x15, 0x19, 0x36,
		0x15, 0x1A, 0x10,
		0x15, 0x1C, 0x57,
		0x15, 0x1D, 0x02,
		0x15, 0x21, 0xF8,
		0x15, 0x22, 0x90,
		0x15, 0x23, 0x00,
		0x15, 0x25, 0x03,
		0x15, 0x26, 0x4a,
		0x15, 0x2A, 0x47,
		0x15, 0X2B, 0xFF,
		0x15, 0x2D, 0XFF,
		0x15, 0x2F, 0xAE,
		0x15, 0x37, 0x0C,
		0x15, 0x3a, 0x00,
		0x15, 0x3b, 0x20,
		0x15, 0x3d, 0x0B,
		0x15, 0x3f, 0x38,
		0x15, 0x40, 0x0B,
		0x15, 0x41, 0x0B,
		0x15, 0x42, 0x33,
		0x15, 0x43, 0x66,
		0x15, 0x44, 0x11,
		0x15, 0x45, 0x44,
		0x15, 0x46, 0x22,
		0x15, 0x47, 0x55,
		0x15, 0x4c, 0x33,
		0x15, 0x4d, 0x66,
		0x15, 0x4e, 0x11,
		0x15, 0x4f, 0x44,
		0x15, 0x50, 0x22,
		0x15, 0x51, 0x55,
		0x15, 0x56, 0x11,
		0x15, 0x58, 0x44,
		0x15, 0x59, 0x22,
		0x15, 0x5a, 0x55,
		0x15, 0x5b, 0x33,
		0x15, 0x5c, 0x66,
		0x15, 0x61, 0x11,
		0x15, 0x62, 0x44,
		0x15, 0x63, 0x22,
		0x15, 0x64, 0x55,
		0x15, 0x65, 0x33,
		0x15, 0x66, 0x66,
		0x15, 0x6d, 0x90,
		0x15, 0x6E, 0x40,
		0x15, 0x70, 0xA5,
		0x15, 0x72, 0x04,
		0x15, 0x73, 0x15,
		0x15, 0xFE, 0x0A,
		0x15, 0x29, 0x10,
		0x15, 0xFE, 0x05,
		0x15, 0x05, 0x1F,
		0x15, 0xFE, 0x00,
		0x15, 0x35, 0x00,
		0x15, 0x11, 0x00,
};

static void edo_e1392am1_panel_condition_setting(struct edo_e1392am1_dev *lcd)
{
	int array_size;
	struct dsi_master_ops *ops = lcd_to_master_ops(lcd);
	struct dsi_device *dsi = lcd_to_master(lcd);

	array_size = ARRAY_SIZE(edo_e1392am1_cmd_list);
	ops->cmd_write(dsi, edo_e1392am1_cmd_list, array_size);
	msleep(120);

	edo_e1392am1_display_on(lcd);

	return;
}

static void edo_e1392am1_set_sequence(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct edo_e1392am1_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd->ddi_pd->lcd_enabled) {
		mutex_lock(&lcd->lock);
		edo_e1392am1_panel_condition_setting(lcd);
		lcd->power = FB_BLANK_UNBLANK;
		mutex_unlock(&lcd->lock);
	}

	return;
}

static int edo_e1392am1_ioctl(struct mipi_dsim_lcd_device *dsim_dev, int cmd)
{
	struct edo_e1392am1_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	if (!lcd) {
		pr_err(" edo_e1392am1_ioctl get drv failed\n");
		return -EFAULT;
	}

	mutex_lock(&lcd->lock);
	switch (cmd) {
	case CMD_MIPI_DISPLAY_ON:
		edo_e1392am1_display_on(lcd);
#ifdef CONFIG_PM
		edo_e1392am1_power_on(dsim_dev, POWER_ON_BL);
#endif
		break;
	default:
		break;
	}
	mutex_unlock(&lcd->lock);

	return 0;
}

static int edo_e1392am1_probe(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct edo_e1392am1_dev *lcd = NULL;
	struct backlight_properties props;

	lcd = devm_kzalloc(&dsim_dev->dev, sizeof(struct edo_e1392am1_dev), GFP_KERNEL);
	if (!lcd) {
		dev_err(&dsim_dev->dev, "failed to allocate edo_e1392am1_dev structure.\n");
		return -ENOMEM;
	}

	lcd->dsim_dev = dsim_dev;
	lcd->ddi_pd = (struct lcd_platform_data *)dsim_dev->platform_data;
	lcd->dev = &dsim_dev->dev;

	mutex_init(&lcd->lock);

	lcd->ld = lcd_device_register("edo_e1392am1_dev", lcd->dev, lcd, NULL);
	if (IS_ERR(lcd->ld)) {
		dev_err(lcd->dev, "failed to register lcd ops.\n");
		return PTR_ERR(lcd->ld);
	}

	props.type = BACKLIGHT_RAW;
	props.max_brightness = 255;
	lcd->bd = backlight_device_register("pwm-backlight.0", lcd->dev, lcd,
										&edo_e1392am1_backlight_ops, &props);
	if (IS_ERR(lcd->bd)) {
		dev_err(lcd->dev, "failed to register 'pwm-backlight.0'.\n");
		return PTR_ERR(lcd->bd);
	}
	dev_set_drvdata(&dsim_dev->dev, lcd);
	dev_dbg(lcd->dev, "probed edo_e1392am1_dev panel driver.\n");

	return 0;
}

#ifdef CONFIG_PM
static void edo_e1392am1_power_on(struct mipi_dsim_lcd_device *dsim_dev, int power)
{
	struct edo_e1392am1_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

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

static int edo_e1392am1_suspend(struct mipi_dsim_lcd_device *dsim_dev)
{
	struct edo_e1392am1_dev *lcd = dev_get_drvdata(&dsim_dev->dev);

	lcd->ddi_pd->lcd_enabled = 0;

	mutex_lock(&lcd->lock);

	edo_e1392am1_display_off(lcd);
	edo_e1392am1_sleep_in(lcd);

//	msleep(120);
	edo_e1392am1_power_on(dsim_dev, !POWER_ON_LCD);

	mutex_unlock(&lcd->lock);

	return 0;
}
#else
#define edo_e1392am1_suspend		NULL
#define edo_e1392am1_resume			NULL
#define edo_e1392am1_power_on		NULL
#endif


static struct mipi_dsim_lcd_driver edo_e1392am1_dsim_ddi_driver = {
	.name = "edo_e1392am1-lcd",
	.id   = 0,
	.set_sequence = edo_e1392am1_set_sequence,
	.ioctl    = edo_e1392am1_ioctl,
	.probe    = edo_e1392am1_probe,
	.suspend  = edo_e1392am1_suspend,
	.power_on = edo_e1392am1_power_on,
};

static int edo_e1392am1_init(void)
{
	mipi_dsi_register_lcd_driver(&edo_e1392am1_dsim_ddi_driver);
	return 0;
}

static void edo_e1392am1_exit(void)
{
	return;
}

module_init(edo_e1392am1_init);
module_exit(edo_e1392am1_exit);

MODULE_DESCRIPTION("EDO 1.4 400*400 MIPI LCD Driver");
MODULE_LICENSE("GPL");
