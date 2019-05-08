#include <linux/mmc/host.h>
#include <linux/fs.h>
#include <linux/wlan_plat.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <mach/jzmmc.h>
#include <linux/bcm_pm_core.h>

#include "board_base.h"

#define WLAN_SDIO_INDEX			1

#define RESET               		0
#define NORMAL              		1

struct wifi_data {
	struct wake_lock                wifi_wake_lock;
	struct regulator                *wifi_vbat;
	struct regulator                *wifi_vddio;
	int                             wifi_reset;
} bcm_ap6212_data;

/* static int wl_pw_en = 0; */

extern void rtc32k_enable(void);
extern void rtc32k_disable(void);

void wlan_pw_en_enable(void)
{
}

void wlan_pw_en_disable(void)
{
}

int bcm_ap6212_wlan_init(void)
{
	static struct wake_lock	*wifi_wake_lock = &bcm_ap6212_data.wifi_wake_lock;
	int reset;

	reset = GPIO_WIFI_RST_N;
	if (gpio_request(GPIO_WIFI_RST_N, "wifi_reset")) {
		pr_err("ERROR: no wifi_reset pin available !!\n");
		goto err_put_vddio;
	} else {
		gpio_direction_output(reset, 1);
	}
	bcm_ap6212_data.wifi_reset = reset;

	wake_lock_init(wifi_wake_lock, WAKE_LOCK_SUSPEND, "wifi_wake_lock");

	return 0;

err_put_vddio:
	return -EINVAL;
}

int bcm_wlan_power_on(int flag)
{
	static struct wake_lock	*wifi_wake_lock = &bcm_ap6212_data.wifi_wake_lock;
	int reset = bcm_ap6212_data.wifi_reset;
	if (wifi_wake_lock == NULL)
		pr_warn("%s: invalid wifi_wake_lock\n", __func__);
	else if (!gpio_is_valid(reset))
		pr_warn("%s: invalid reset\n", __func__);
	else
		goto start;
	return -ENODEV;
start:
	pr_debug("wlan power on:%d\n", flag);
	rtc32k_enable();

	switch(flag) {
		case RESET:
			jzmmc_clk_ctrl(WLAN_SDIO_INDEX, 1);

			gpio_set_value(reset, 0);
			msleep(10);

			gpio_set_value(reset, 1);

			break;
		case NORMAL:

			gpio_set_value(reset, 0);
			msleep(10);

			gpio_set_value(reset, 1);

			jzmmc_manual_detect(WLAN_SDIO_INDEX, 1);

			break;
	}
	wake_lock(wifi_wake_lock);
	return 0;
}

int bcm_wlan_power_off(int flag)
{
	static struct wake_lock	*wifi_wake_lock = &bcm_ap6212_data.wifi_wake_lock;
	int reset = bcm_ap6212_data.wifi_reset;

	if (wifi_wake_lock == NULL)
		pr_warn("%s: invalid wifi_wake_lock\n", __func__);
	else if (!gpio_is_valid(reset))
		pr_warn("%s: invalid reset\n", __func__);
	else
		goto start;
	return -ENODEV;
start:
	pr_debug("wlan power off:%d\n", flag);
	switch(flag) {
		case RESET:
			gpio_set_value(reset, 0);

			break;
		case NORMAL:
			gpio_set_value(reset, 0);

			break;
	}

	wake_unlock(wifi_wake_lock);
	rtc32k_disable();

	return 0;
}

static ssize_t bcm_ap6212_manual_set(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
        int ret = 0;
        /* rlt8723bs default state is power on */
        static int init_state = 1;

        if (strncmp(buf, "0", 1) == 0) {
                if (!init_state)
                        goto out;
                ret = bcm_wlan_power_off(NORMAL);
                init_state = 0;
        } else if (strncmp(buf, "1", 1) == 0) {
                if (init_state)
                        goto out;
                ret = bcm_wlan_power_on(NORMAL);
                init_state = 1;
        } else
                ret = -1;
        if (ret)
                pr_warn("===>%s bcm_ap6212 power ctl fail\n", __func__);

out:
	return count;
}

static DEVICE_ATTR(manual, S_IWUSR | S_IRGRP | S_IROTH,
                   NULL, bcm_ap6212_manual_set);

int bcm_ap6212_manual_sysfs_init(struct device *dev)
{
        int ret = 0;

        ret = sysfs_create_file(&dev->kobj, &dev_attr_manual.attr);
        if (ret)
                pr_warn("===>%s create sysfs file fail!\n", __func__);

        return ret;
}

EXPORT_SYMBOL(bcm_ap6212_wlan_init);
EXPORT_SYMBOL(bcm_ap6212_manual_sysfs_init);
EXPORT_SYMBOL(bcm_wlan_power_on);
EXPORT_SYMBOL(bcm_wlan_power_off);
