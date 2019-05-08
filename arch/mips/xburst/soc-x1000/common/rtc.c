#include <linux/module.h>
#include <gpio.h>
#include <asm/atomic.h>
#include <soc/gpio.h>

static atomic_t rtc32k_ref;
static int rtc32k_initialized = 0;

void rtc32k_init(void)
{
	atomic_set(&rtc32k_ref, 0);
	if (gpio_request(GPIO_PB(26), "rtc32k")) {
		printk("[Error] request rtc32k gpio fail!\n");
		return ;
	}
	rtc32k_initialized = 1;
}

void rtc32k_enable(void)
{
	if (rtc32k_initialized == 0)
		rtc32k_init();

	if (atomic_inc_return(&rtc32k_ref) == 1)
		jzgpio_set_func(GPIO_PORT_B, GPIO_FUNC_0, (0x1 << 26));
}

void rtc32k_disable(void)
{
	if (rtc32k_initialized == 0)
		rtc32k_init();

	if (atomic_dec_return(&rtc32k_ref) == 0)
		jzgpio_set_func(GPIO_PORT_B, GPIO_INPUT, (0x1 << 26));
}

EXPORT_SYMBOL(rtc32k_enable);
EXPORT_SYMBOL(rtc32k_disable);
