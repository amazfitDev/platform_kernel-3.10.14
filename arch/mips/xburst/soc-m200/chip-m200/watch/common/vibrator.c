#include <linux/platform_device.h>
#include <mach/platform.h>
#include "board_base.h"

#if defined(CONFIG_ANDROID_TIMED_REGULATOR)
#include <../../../../../drivers/staging/android/timed_regulator.h>
static struct timed_regulator vibrator_timed_regulator = {
        .name           = "vibrator",
        .reg_name       = REG_VDDIO,
        .max_timeout    = MAX_TIMEOUT,
};

static struct timed_regulator_platform_data regulator_vibrator_platform_data = {
        .num_regulators = 1,
        .regs           = &vibrator_timed_regulator,
};

struct platform_device jz_timed_regulator_device = {
        .name   = TIMED_REGULATOR_NAME,
        .id     = 0,
        .dev    = {
                .platform_data  = &regulator_vibrator_platform_data,
        },
};
#endif

#if defined(CONFIG_ANDROID_TIMED_GPIO)
#include <../../../../../drivers/staging/android/timed_gpio.h>
struct timed_gpio vibrator_timed_gpio = {
        .name           = "vibrator",
        .gpio           = VIBRATOR_EN,
        .active_low     = ACTIVE_LEVEL,
        .max_timeout    = MAX_TIMEOUT,
};

static struct timed_gpio_platform_data vibrator_platform_data = {
        .num_gpios      = 1,
        .gpios          = &vibrator_timed_gpio,
};

struct platform_device jz_timed_gpio_device = {
        .name   = TIMED_GPIO_NAME,
        .id     = 0,
        .dev    = {
                .platform_data  = &vibrator_platform_data,
        },
};
#endif

