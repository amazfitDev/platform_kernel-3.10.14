#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/leds.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <mach/platform.h>
#include <mach/jzsnd.h>
#include <mach/jzmmc.h>
#include <mach/jzssi.h>
#include <mach/jz_efuse.h>
#include <gpio.h>
#include <linux/jz_dwc.h>
#include <linux/interrupt.h>
#include "board_base.h"
#include <board.h>
#include <gpio.h>

#include <soc/gpio.h>
#include <soc/base.h>
#include <soc/irq.h>

#ifdef CONFIG_JZ_EFUSE_V11
struct jz_efuse_platform_data jz_efuse_pdata = {
	    /* supply 2.5V to VDDQ */
	    .gpio_vddq_en_n = GPIO_EFUSE_VDDQ,
};
#endif


#ifdef CONFIG_LEDS_GPIO
struct gpio_led jz_leds[] = {
	[0]={
		.name = "wl_led_r",
		.gpio = WL_LED_R,
		.active_low = 0,
	},
	[1]={
		.name = "wl_led_g",
		.gpio = WL_LED_G,
		.active_low = 0,
	},

	[2]={
		.name = "wl_led_b",
		.gpio = WL_LED_B,
		.active_low = 0,
	},

};

struct gpio_led_platform_data  jz_led_pdata = {
	.num_leds = 3,
	.leds = jz_leds,
};

struct platform_device jz_led_rgb = {
	.name       = "leds-gpio",
	.id     = -1,
	.dev        = {
		.platform_data  = &jz_led_pdata,
	}
};
#endif
#ifdef CONFIG_JZ_IRDA_V11
static struct resource jz_uart2_resources[] = {
	[0] = {
		.start          = UART2_IOBASE,
		.end            = UART2_IOBASE + 0x1000 - 1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_UART2,
		.end            = IRQ_UART2,
		.flags          = IORESOURCE_IRQ,
	},
};
struct platform_device jz_irda_device  = {
	.name = "jz-irda",
	.id = 2 ,
	.num_resources  = ARRAY_SIZE(jz_uart2_resources),
	.resource       = jz_uart2_resources,

};
#endif

#ifdef CONFIG_KEYBOARD_GPIO
struct gpio_keys_button __attribute__((weak)) board_buttons[] = {

#ifdef GPIO_PLAY_KEY
	{
		.gpio       = GPIO_PLAY_KEY,
		.code       = KEY_PLAYPAUSE,
		.desc       = "play key",
		.active_low = ACTIVE_LOW_PLAY,
		.gpio_pullup = 1, // need gpio inter pull up
	},
#endif
#ifdef GPIO_AP_STA_KEY
	{
		.gpio       = GPIO_AP_STA_KEY,
		.code       = KEY_SELECT,
		.desc       = "mode key",
		.active_low = ACTIVE_LOW_AP_STA,
		.gpio_pullup = 1,
	},
#endif
#ifdef GPIO_VOLUMEUP_KEY
	{
		.gpio       = GPIO_VOLUMEUP_KEY,
		.code       = KEY_VOLUMEUP,
		.desc       = "volume up key",
		.active_low = ACTIVE_LOW_VOLUMEUP,
		.gpio_pullup = 1,
	},
#endif
#ifdef GPIO_VOLUMEDOWN_KEY
	{
		.gpio       = GPIO_VOLUMEDOWN_KEY,
		.code       = KEY_VOLUMEDOWN,
		.desc       = "volum down key",
		.active_low = ACTIVE_LOW_VOLUMEDOWN,
		.gpio_pullup = 1,
	},
#endif


};
static struct gpio_keys_platform_data board_button_data = {
	.buttons    = board_buttons,
	.nbuttons   = ARRAY_SIZE(board_buttons),
};

struct platform_device jz_button_device = {
	.name       = "gpio-keys",
	.id     = -1,
	.num_resources  = 0,
	.dev        = {
		.platform_data  = &board_button_data,
	}
};
#endif
