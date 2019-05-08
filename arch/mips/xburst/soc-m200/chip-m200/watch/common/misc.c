
#include <linux/platform_device.h>
#include <mach/jzsnd.h>
#ifdef CONFIG_MFD_JZ_SADC_V12
#include <linux/jz_adc.h>
#ifdef CONFIG_JZ_BATTERY
#include <linux/power/jz_battery.h>
#include <linux/power/li_ion_charger.h>
#endif
#endif
#ifdef CONFIG_JZ_EFUSE_V12
#include <mach/jz_efuse.h>
#endif
#include "board_base.h"

#ifdef CONFIG_JZ_EFUSE_V12
struct jz_efuse_platform_data jz_efuse_pdata = {
	/* supply 2.5V to VDDQ */
	.gpio_vddq_en = GPIO_EFUSE_VDDQ,
};
#endif

#ifdef CONFIG_CHARGER_LI_ION
/* li-ion charger */
static struct li_ion_charger_platform_data jz_li_ion_charger_pdata = {
	.gpio_charge = GPIO_LI_ION_CHARGE,
	.gpio_ac = GPIO_LI_ION_AC,
	.gpio_active_low = GPIO_ACTIVE_LOW,
};

static struct platform_device jz_li_ion_charger_device = {
	.name = "li-ion-charger",
	.dev = {
		.platform_data = &jz_li_ion_charger_pdata,
	},
};
#endif

#if IS_ENABLED(CONFIG_SND_ASOC_INGENIC_NEWTON_ICDC)
static struct snd_codec_data snd_newton_platform_data = {
    .gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
    .gpio_hp_detect = {.gpio = GPIO_HP_DETECT, .active_level = GPIO_HP_INSERT_LEVEL},
};

struct platform_device snd_newton_device = {
	.name = "ingenic-newton",
    .dev = {
            .platform_data = &snd_newton_platform_data,
    },
};
#endif

#if defined(CONFIG_SND_ASOC_INGENIC)
static struct snd_codec_data snd_watch_platform_data = {
	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
	.gpio_hp_detect = {.gpio = GPIO_HP_DETECT, .active_level = GPIO_HP_INSERT_LEVEL},
};

struct platform_device snd_watch_device = {
    .name = "ingenic-watch",
    .dev = {
            .platform_data = &snd_watch_platform_data,
    },
};
#endif

#ifdef CONFIG_JZ_BATTERY
static struct jz_battery_info  watch_battery_info = {
	.max_vol        = 4050,
	.min_vol        = 3600,
	.usb_max_vol    = 4100,
	.usb_min_vol    = 3760,
	.ac_max_vol     = 4100,
	.ac_min_vol     = 3760,
	.battery_max_cpt = 3000,
	.ac_chg_current = 800,
	.usb_chg_current = 400,
};
#endif
#ifdef CONFIG_MFD_JZ_SADC_V12
struct jz_adc_platform_data adc_platform_data = {
#ifdef CONFIG_JZ_BATTERY
	battery_info = watch_battery_info;
#endif
};
#endif

#if defined(CONFIG_USB_JZ_DWC2)
#if defined(GPIO_USB_ID) && defined(GPIO_USB_ID_LEVEL)
struct jzdwc_pin dwc2_id_pin = {
	.num = GPIO_USB_ID,
	.enable_level = GPIO_USB_ID_LEVEL,
};
#endif

#if defined(GPIO_USB_DETE) && defined(GPIO_USB_DETE_LEVEL)
struct jzdwc_pin dwc2_dete_pin = {
	.num = GPIO_USB_DETE,
	.enable_level = GPIO_USB_DETE_LEVEL,
};
#endif

#if defined(GPIO_USB_DRVVBUS) && defined(GPIO_USB_DRVVBUS_LEVEL) && !defined(USB_DWC2_DRVVBUS_FUNCTION_PIN)
struct jzdwc_pin dwc2_drvvbus_pin = {
	.num = GPIO_USB_DRVVBUS,
	.enable_level = GPIO_USB_DRVVBUS_LEVEL,
};
#endif
#endif /*CONFIG_USB_DWC2*/
