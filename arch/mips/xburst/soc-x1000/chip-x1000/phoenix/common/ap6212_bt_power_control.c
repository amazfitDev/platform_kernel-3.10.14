#include <linux/platform_device.h>
#include <linux/bcm_pm_core.h>
#include <linux/bt-rfkill.h>
#include <gpio.h>
#include <soc/gpio.h>
#include "board_base.h"


#ifdef CONFIG_BCM_AP6212_RFKILL
extern struct platform_device bt_power_device;
#endif

/* For BlueTooth */
int bluesleep_tty_strcmp(const char* name)
{
	if (!strcmp(name, BLUETOOTH_UPORT_NAME)) {
		return 0;
	} else {
		return -1;
	}
}
EXPORT_SYMBOL(bluesleep_tty_strcmp);

struct bt_rfkill_platform_data  gpio_data = {
	.gpio = {
		.bt_rst_n = -1,
		.bt_reg_on = GPIO_BT_REG_ON,
		.bt_wake = GPIO_BT_WAKE,
		.bt_int = GPIO_BT_INT,
		.bt_uart_rts = GPIO_BT_UART_RTS,
	},
};

struct platform_device bt_power_device = {
	.name = "bt_power",
	.id = -1,
	.dev = {
		.platform_data = &gpio_data,
	},
};
