#include <linux/platform_device.h>
#include <linux/gpio.h>

#define GPIO_WIFI_WAKE GPIO_PC(16)

static struct resource wlan_resources[] = {
	[0] = {
		.start = GPIO_WIFI_WAKE,
		.end = GPIO_WIFI_WAKE,
		.name = "bcmdhd_wlan_irq",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE,
	},
};
static struct platform_device wlan_device = {
	.name   = "bcmdhd_wlan",
	.id     = 1,
	.dev    = {
		.platform_data = NULL,
	},
	.resource	= wlan_resources,
	.num_resources	= ARRAY_SIZE(wlan_resources),
};

static int __init wlan_device_init(void)
{
	int ret;

	ret = platform_device_register(&wlan_device);

	return ret;
}

late_initcall(wlan_device_init);
