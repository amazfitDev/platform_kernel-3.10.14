/*
 *  LCD control code for KFM701A21_1A
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/lcd.h>
#include <linux/fb.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <linux/kfm701a21_1a.h>

#define POWER_IS_ON(pwr)        ((pwr) <= FB_BLANK_NORMAL)

struct kfm701a21_1a_data {
	int lcd_power;
	int reset_enable;
	struct lcd_device *lcd;
	struct platform_kfm701a21_1a_data *pdata;
	struct regulator *lcd_vcc_reg;
};


static void kfm701a21_1a_on(struct kfm701a21_1a_data *dev)
{
        dev->lcd_power = 1;
	regulator_enable(dev->lcd_vcc_reg);

#if 0
	if (dev->pdata->gpio_lcd_cs) {
		gpio_direction_output(dev->pdata->gpio_lcd_cs, 0);
		mdelay(20);
	}
	printk("+++++zzzzssss++++++ dev->reset_enable = %d\n",dev->reset_enable);
	if (dev->reset_enable) {
	printk("+++++++zzzzssss++++++++++:%s,%d\n",__func__,__LINE__);
		if (dev->pdata->gpio_lcd_reset) {
			gpio_direction_output(dev->pdata->gpio_lcd_reset, 1);
			mdelay(10);
			gpio_direction_output(dev->pdata->gpio_lcd_reset, 0);
			mdelay(10);
			gpio_direction_output(dev->pdata->gpio_lcd_reset, 1);
		}
		mdelay(120);
	} else {
		/* Need to reinitialization panel when late resume */
		dev->reset_enable = 1;
	}
#endif
	if(dev->pdata->gpio_lcd_cs && dev->pdata->gpio_lcd_reset){
		gpio_direction_output(dev->pdata->gpio_lcd_cs,1);
		gpio_direction_output(dev->pdata->gpio_lcd_reset,0);
		mdelay(10);
		gpio_direction_output(dev->pdata->gpio_lcd_reset,1);
		mdelay(1);
		gpio_direction_output(dev->pdata->gpio_lcd_reset,0);
		mdelay(1);
		gpio_direction_output(dev->pdata->gpio_lcd_reset,1);
		mdelay(10);
		gpio_direction_output(dev->pdata->gpio_lcd_cs,0);
	}else{
		printk("error:please checkout lcd_cs_pin or lcd_reset pin\n");
	}
	dev->reset_enable = 1;
}

static void kfm701a21_1a_off(struct kfm701a21_1a_data *dev)
{
        dev->lcd_power = 0;
	regulator_disable(dev->lcd_vcc_reg);

	mdelay(30);
}

static int kfm701a21_1a_set_power(struct lcd_device *lcd, int power)
{
	struct kfm701a21_1a_data *dev= lcd_get_data(lcd);

	if (!power && !(dev->lcd_power)) {
                kfm701a21_1a_on(dev);
        } else if (power && (dev->lcd_power)) {
                kfm701a21_1a_off(dev);
        }
	return 0;
}

static int kfm701a21_1a_get_power(struct lcd_device *lcd)
{
	struct kfm701a21_1a_data *dev= lcd_get_data(lcd);

	return dev->lcd_power;
}

static int kfm701a21_1a_set_mode(struct lcd_device *lcd, struct fb_videomode *mode)
{
	return 0;
}

static struct lcd_ops kfm701a21_1a_ops = {
	.set_power = kfm701a21_1a_set_power,
	.get_power = kfm701a21_1a_get_power,
	.set_mode = kfm701a21_1a_set_mode,
};

static int kfm701a21_1a_probe(struct platform_device *pdev)
{
	int ret;
	struct kfm701a21_1a_data *dev;

	dev = kzalloc(sizeof(struct kfm701a21_1a_data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->pdata = pdev->dev.platform_data;

	dev_set_drvdata(&pdev->dev, dev);

	dev->lcd_vcc_reg = regulator_get(NULL, "vlcd");
	if (IS_ERR(dev->lcd_vcc_reg)) {
		dev_err(&pdev->dev, "failed to get regulator vlcd\n");
		return PTR_ERR(dev->lcd_vcc_reg);
	}

	if (dev->pdata->gpio_lcd_cs)
		gpio_request(dev->pdata->gpio_lcd_cs, "display_on_off");
	if (dev->pdata->gpio_lcd_reset)
		gpio_request(dev->pdata->gpio_lcd_reset, "lcd_reset");

#ifdef CONFIG_LCD_PANEL_RESET_ENABLE
	dev->reset_enable = 1;
#else
	dev->reset_enable = 0;
#endif

	kfm701a21_1a_on(dev);

	dev->lcd = lcd_device_register("kfm701a21_1a-lcd", &pdev->dev,
				       dev, &kfm701a21_1a_ops);
	if (IS_ERR(dev->lcd)) {
		ret = PTR_ERR(dev->lcd);
		dev->lcd = NULL;
		dev_info(&pdev->dev, "lcd device register error: %d\n", ret);
	} else {
		dev_info(&pdev->dev, "lcd device register success\n");
	}

	return 0;
}

static int kfm701a21_1a_remove(struct platform_device *pdev)
{
	struct kfm701a21_1a_data *dev = dev_get_drvdata(&pdev->dev);

	lcd_device_unregister(dev->lcd);
	kfm701a21_1a_off(dev);

	regulator_put(dev->lcd_vcc_reg);

	if (dev->pdata->gpio_lcd_cs)
		gpio_free(dev->pdata->gpio_lcd_cs);
	if (dev->pdata->gpio_lcd_reset)
		gpio_free(dev->pdata->gpio_lcd_reset);

	dev_set_drvdata(&pdev->dev, NULL);
	kfree(dev);

	return 0;
}

#ifdef CONFIG_PM
static int kfm701a21_1a_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int kfm701a21_1a_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define kfm701a21_1a_suspend	NULL
#define kfm701a21_1a_resume	NULL
#endif

static struct platform_driver kfm701a21_1a_driver = {
	.driver		= {
		.name	= "kfm701a21_1a-lcd",
		.owner	= THIS_MODULE,
	},
	.probe		= kfm701a21_1a_probe,
	.remove		= kfm701a21_1a_remove,
	.suspend	= kfm701a21_1a_suspend,
	.resume		= kfm701a21_1a_resume,
};

static int __init kfm701a21_1a_init(void)
{
	return platform_driver_register(&kfm701a21_1a_driver);
}
module_init(kfm701a21_1a_init);

static void __exit kfm701a21_1a_exit(void)
{
	platform_driver_unregister(&kfm701a21_1a_driver);
}
module_exit(kfm701a21_1a_exit);

MODULE_DESCRIPTION("KFM701A21_1A lcd panel driver");
MODULE_LICENSE("GPL");
