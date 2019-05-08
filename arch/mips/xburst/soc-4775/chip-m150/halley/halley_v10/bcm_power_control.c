#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/wlan_plat.h>
#include <mach/jzmmc.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/bcm_pm_core.h>
#include "board.h"

#ifdef CONFIG_BCM_PM_CORE
static void enable_clk32k(void)
{
	jzrtc_enable_clk32k();
}

static void disable_clk32k(void)
{
	jzrtc_disable_clk32k();
}

static struct bcm_power_platform_data bcm_power_platform_data = {
	.wlan_pwr_en = -1,
	.clk_enable = enable_clk32k,
	.clk_disable = disable_clk32k,
};

struct platform_device	bcm_power_platform_device = {
	.name = "bcm_power",
	.id = -1,
	.num_resources = 0,
	.dev = {
		.platform_data = &bcm_power_platform_data,
	},
};
#endif

/*For BlueTooth*/
#ifdef CONFIG_BROADCOM_RFKILL
#include <linux/bt-rfkill.h>
/*restore BT_UART_RTS when resume because it is output 0 when suspend*/
static void restore_pin_status(int bt_power_state)
{
	jzgpio_set_func(BLUETOOTH_UART_GPIO_PORT, BLUETOOTH_UART_GPIO_FUNC, BLUETOOTH_UART_FUNC_SHIFT);
}

static struct bt_rfkill_platform_data  bt_gpio_data = {
	.gpio = {
		.bt_rst_n = HOST_BT_RST,
		.bt_reg_on = BT_REG_EN,
		.bt_wake = HOST_WAKE_BT,
		.bt_int = BT_WAKE_HOST,
		.bt_uart_rts = BT_UART_RTS,
#if 0
		.bt_int_flagreg = -1,
		.bt_int_bit = -1,
#endif
	},

	.restore_pin_status = restore_pin_status,
	.set_pin_status = NULL,
#if 0
	.suspend_gpio_set = NULL,
	.resume_gpio_set = NULL,
#endif
};

struct platform_device bt_power_device  = {
	.name = "bt_power" ,
	.id = -1 ,
	.dev   = {
		.platform_data = &bt_gpio_data,
	},
};

struct platform_device bluesleep_device = {
	.name = "bluesleep" ,
	.id = -1 ,
	.dev   = {
		.platform_data = &bt_gpio_data,
	},

};

#ifdef CONFIG_BT_BLUEDROID_SUPPORT
int bluesleep_tty_strcmp(const char* name)
{
	if(!strcmp(name,BLUETOOTH_UPORT_NAME)){
		return 0;
	} else {
		return -1;
	}
}
EXPORT_SYMBOL(bluesleep_tty_strcmp);
#endif
#endif /* CONFIG_BROADCOM_RFKILL */

static void wifi_le_set_io(void)
{
	jzgpio_set_func(GPIO_PORT_B, GPIO_INPUT, (0x3<<20)|(0xF<<30));
}

static void wifi_le_restore_io(void)
{
	jzgpio_set_func(GPIO_PORT_B, GPIO_FUNC_0, (0x3<<20)|(0xF<<30));
}

#define RESET               0
#define NORMAL              1

int bcm_wlan_power_on(int flag)
{
	int wl_reg_on	= WL_REG_EN;
	pr_debug("wlan power on:%d\n", flag);
	wifi_le_restore_io();
	switch(flag) {
		case RESET:
			gpio_direction_output(wl_reg_on,1);
			break;
		case NORMAL:
			gpio_request(wl_reg_on, "wl_reg_on");
			gpio_direction_output(wl_reg_on,0);/* halley reset no power down ,need sofe control wifi reg on*/
			gpio_direction_output(wl_reg_on,1);
			jzmmc_manual_detect(2, 1);
			break;
	}
	return 0;
}
EXPORT_SYMBOL(bcm_wlan_power_on);

int bcm_wlan_power_off(int flag)
{
	int wl_reg_on = WL_REG_EN;
	pr_debug("wlan power off:%d\n", flag);
	switch(flag) {
		case RESET:
			gpio_direction_output(wl_reg_on,0);
			break;

		case NORMAL:
			/* *  control wlan reg on pin */
			gpio_direction_output(wl_reg_on,0);
			break;
	}
	wifi_le_set_io();
	return 0;
}
EXPORT_SYMBOL(bcm_wlan_power_off);

static struct wifi_platform_data bcmdhd_wlan_pdata;
#define IMPORT_WIFIMAC_BY_SELF
//#define IMPORT_FWPATH_BY_SELF

#ifdef IMPORT_WIFIMAC_BY_SELF
#define WIFIMAC_ADDR_PATH "/firmware/wifimac.txt"

static int get_wifi_mac_addr(unsigned char* buf)
{
	struct file *fp = NULL;
	mm_segment_t fs;

	unsigned char source_addr[18];
	loff_t pos = 0;
	unsigned char *head, *end;
	int i = 0;

	fp = filp_open(WIFIMAC_ADDR_PATH, O_RDONLY,  0444);
	if (IS_ERR(fp)) {

		printk("Can not access wifi mac file : %s\n",WIFIMAC_ADDR_PATH);
		return -EFAULT;
	}else{
		fs = get_fs();
		set_fs(KERNEL_DS);

		vfs_read(fp, source_addr, 18, &pos);
		source_addr[17] = ':';

		head = end = source_addr;
		for(i=0; i<6; i++) {
			while (end && (*end != ':') )
				end++;

			if (end && (*end == ':') )
				*end = '\0';

			buf[i] = simple_strtoul(head, NULL, 16 );

			if (end) {
				end++;
				head = end;
			}
			printk("wifi mac %02x \n", buf[i]);
		}
		set_fs(fs);
		filp_close(fp, NULL);
	}

	return 0;
}
#endif

#ifdef IMPORT_FWPATH_BY_SELF
#define WIFI_DRIVER_FW_PATH_PARAM "/data/misc/wifi/fwpath"
#define CLJ_DEBUG 1
static unsigned char fwpath[128];

static char* get_firmware_path(void)
{
	struct file *fp = NULL;
	mm_segment_t fs;
	loff_t pos = 0;

	printk("enter  :%s\n",__FUNCTION__);
	fp = filp_open(WIFI_DRIVER_FW_PATH_PARAM, O_RDONLY,  0444);
	if (IS_ERR(fp)) {

		printk("Can not access wifi firmware path file : %s\n",WIFI_DRIVER_FW_PATH_PARAM);
		return NULL;
	}else{
		fs = get_fs();
		set_fs(KERNEL_DS);

		memset(fwpath , 0 ,sizeof(fwpath));

		vfs_read(fp, fwpath, sizeof(fwpath), &pos);

		if(CLJ_DEBUG)
			printk("fetch fwpath = %s\n",fwpath);

		set_fs(fs);
		filp_close(fp, NULL);
		return fwpath;
	}

	return NULL;
}

#endif



static struct resource wlan_resources[] = {
	[0] = {
		.start = WL_WAKE_HOST,
		.end = WL_WAKE_HOST,
		.name = "bcmdhd_wlan_irq",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};
static struct platform_device wlan_device = {
	.name   = "bcmdhd_wlan",
	.id     = 1,
	.dev    = {
		.platform_data = &bcmdhd_wlan_pdata,
	},
	.resource	= wlan_resources,
	.num_resources	= ARRAY_SIZE(wlan_resources),
};

static int __init wlan_device_init(void)
{
	int ret;

	memset(&bcmdhd_wlan_pdata,0,sizeof(bcmdhd_wlan_pdata));

#ifdef IMPORT_WIFIMAC_BY_SELF
	bcmdhd_wlan_pdata.get_mac_addr = get_wifi_mac_addr;
#endif
#ifdef IMPORT_FWPATH_BY_SELF
	bcmdhd_wlan_pdata.get_fwpath = get_firmware_path;
#endif

	ret = platform_device_register(&wlan_device);

	return ret;
}

late_initcall(wlan_device_init);
