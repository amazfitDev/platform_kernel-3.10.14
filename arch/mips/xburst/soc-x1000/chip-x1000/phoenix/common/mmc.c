#include <linux/mmc/host.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/delay.h>

#include <mach/jzmmc.h>

#include "board.h"

#ifdef CONFIG_BCMDHD_1_141_66
int bcm_ap6212_wlan_init(void);
#endif

#ifndef CONFIG_NAND
#ifdef CONFIG_JZMMC_V12_MMC0
static struct card_gpio tf_gpio = {
	.cd				= {GPIO_SD0_CD_N,   LOW_ENABLE},
	.wp             = {-1,	-1},
	.pwr			= {-1,	-1},
	.rst        = {-1, -1},
};
/* common pdata for both tf_card and sdio wifi on fpga board */

struct jzmmc_platform_data tf_pdata = {
	.removal  			= REMOVABLE,
	.sdio_clk			= 0,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA ,
	.pm_flags			= 0,
	.recovery_info			= NULL,
	.gpio				= &tf_gpio,
	.max_freq                       = CONFIG_MMC0_MAX_FREQ,
#ifdef CONFIG_MMC1_PIO_MODE
	.pio_mode                       = 1,
#else
	.pio_mode                       = 0,
#endif
};
#endif

#ifdef CONFIG_JZMMC_V12_MMC1
struct jzmmc_platform_data sdio_pdata = {
	.removal  			= MANUAL,
	.sdio_clk			= 1,
	.ocr_avail			= MMC_VDD_32_33 | MMC_VDD_33_34,
	.capacity  			= MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_4_BIT_DATA,
	.pm_flags			= MMC_PM_IGNORE_PM_NOTIFY,
	.recovery_info			= NULL,
	.gpio				= NULL,
	.max_freq                       = CONFIG_MMC1_MAX_FREQ,
#ifdef CONFIG_MMC1_PIO_MODE
	.pio_mode                       = 1,
#else
	.pio_mode                       = 0,
#endif
#ifdef CONFIG_BCMDHD_1_141_66
	.private_init			= bcm_ap6212_wlan_init,
#endif
};
#endif
#else /* CONFIG NAND*/


#endif
