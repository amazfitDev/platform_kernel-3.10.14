#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/i2c-gpio.h>
#include <linux/interrupt.h>
#include <linux/i2c/pca953x.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include "board_base.h"

/* *****************************sensor hub start************************** */
#if defined(CONFIG_FRIZZ)
#include <linux/i2c/frizz.h>
static struct frizz_platform_data frizz_pdata = {
	.irq_gpio = FRIZZ_IRQ,
	.reset_gpio = FRIZZ_RESET,
	.wakeup_frizz_gpio = FRIZZ_WAKEUP,
	.g_chip_orientation = GSENSOR_CHIP_ORIENTATION,
};
#endif
/* *****************************sensor hub end**************************** */

/* *****************************pedometer start ************************** */
#if defined(CONFIG_SENSORS_MMA955XL_I2C)
#include <linux/i2c/mma955xl_pedometer.h>
static struct mma955xl_platform_data mma955xl_pdata = {
		.sensor_int0_n = MMA955XL_INTO_N,
		.ap_wakeup_mcu = AP_WAKEUP_MMA955XL,
		.mcu_wakeup_ap = MMA955XL_WAKEUP_AP,
};
#endif
/* *****************************pedometer  end *************************** */

/* *****************************touchscreen******************************* */
#ifdef CONFIG_TOUCHSCREEN_GWTC9XXXB
static struct jztsc_pin fpga_tsc_gpio[] = {
	[0] = {GPIO_TP_INT, LOW_ENABLE},
	[1] = {GPIO_TP_WAKE, HIGH_ENABLE},
};

static struct jztsc_platform_data fpga_tsc_pdata = {
	.gpio = fpga_tsc_gpio,
	.x_max = 800,
	.y_max = 480,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_FT6X0X
#include <linux/input/ft6x0x_ts.h>
extern int touch_power_init(struct device *dev);
extern int touch_power_on(struct device *dev);
extern int touch_power_off(struct device *dev);

static struct jztsc_pin ft6x0x_tsc_gpio[] = {
	 [0] = {GPIO_TP_INT, LOW_ENABLE},
	 [1] = {GPIO_TP_WAKE, LOW_ENABLE},
};

static struct ft6x0x_platform_data ft6x0x_tsc_pdata = {
	.gpio           = ft6x0x_tsc_gpio,
	.x_max          = 240,
	.y_max          = 240,
	.fw_ver         = 0x21,
#ifdef CONFIG_KEY_SPECIAL_POWER_KEY
	.blight_off_timer = 3000,   //3s
#endif
	.power_init     = touch_power_init,
	.power_on       = touch_power_on,
	.power_off      = touch_power_off,
};

#endif

#ifdef CONFIG_TOUCHSCREEN_FT6X06
#include <linux/input/ft6x06_ts.h>
struct ft6x06_platform_data ft6x06_tsc_pdata = {
	.x_max          = 300,
	.y_max          = 540,
	.va_x_max	= 300,
	.va_y_max	= 480,
	.irqflags = IRQF_TRIGGER_FALLING|IRQF_DISABLED,
	.irq = GPIO_TP_INT,
	.reset = GPIO_TP_RESET,
};
#endif

#ifdef CONFIG_TOUCHSCREEN_ITE7258
#include <linux/tsc.h>
struct jztsc_pin ite7258_tsc_gpio[2] = {
        [0] = {GPIO_TP_INT, LOW_ENABLE},
#if (defined(CONFIG_AW808_HW_V11_NATURAL_ROUND))
        [1] = {GPIO_TP_RESET, HIGH_ENABLE},
#else
        [1] = {GPIO_TP_RESET, LOW_ENABLE},
#endif
};
static struct jztsc_platform_data ite7258_tsc_pdata = {
	.gpio           = ite7258_tsc_gpio,
	.x_max          = 320,
	.y_max          = 320,
	.irqflags = IRQF_TRIGGER_LOW | IRQF_DISABLED,
	.vcc_name = VCC_TOUCHSCREEN,
#if (defined(CONFIG_WATCH_ACRAB) || defined(CONFIG_WATCH_AW808) || defined(CONFIG_WATCH_SOLAR))
	.vccio_name = VIO_TOUCHSCREEN,
#endif
};
#endif  /* CONFIG_TOUCHSCREEN_ITE7258 */

/* *****************************haptics drv2605 start********************* */
#if defined(CONFIG_HAPTICS_DRV2605)
#include <linux/haptics/drv2605.h>
#elif defined(CONFIG_HAPTICS_DRV2604)
#include <linux/haptics/drv2604.h>
#elif defined(CONFIG_HAPTICS_DRV2604L)
#include <linux/haptics/drv2604l.h>
#elif defined(CONFIG_HAPTICS_DRV2667)
#include <linux/haptics/drv2667.h>
#endif

#if defined(CONFIG_HAPTICS_DRV2605)
static struct drv2605_platform_data drv2605_plat_data = {
    .GpioEnable = DRV2605_ENABLE, //enable the chip
    .GpioTrigger = 0,//external trigger pin, (0: internal trigger)
#if defined(CONFIG_HAPTICS_LRA_SEMCO1030)
    //rated = 1.5Vrms, ov=2.1Vrms, f=204hz
    .loop = CLOSE_LOOP,
    .RTPFormat = Signed,
    .BIDIRInput = BiDirectional,
    .actuator = {
        .device_type = LRA,
        .rated_vol = 0x3d,
        .g_effect_bank = LIBRARY_F,
        .over_drive_vol = 0x87,
        .LRAFreq = 204,
    },
    .a2h = {
        .a2h_min_input = AUDIO_HAPTICS_MIN_INPUT_VOLTAGE,
        .a2h_max_input = AUDIO_HAPTICS_MAX_INPUT_VOLTAGE,
        .a2h_min_output = AUDIO_HAPTICS_MIN_OUTPUT_VOLTAGE,
        .a2h_max_output = AUDIO_HAPTICS_MAX_OUTPUT_VOLTAGE,
    },
#elif defined(CONFIG_HAPTICS_ERM_EVOWAVE_Z4TJGB1512658)
    //rated vol = 3.0 v, ov = 3.6 v, risetime = 150 ms
    .loop = CLOSE_LOOP,
    .RTPFormat = Signed,
    .BIDIRInput = BiDirectional,
    .actuator = {
        .device_type = ERM,
        .g_effect_bank = LIBRARY_E,
        .rated_vol = 0x8d,
        .over_drive_vol = 0xa9,
    },
    .a2h = {
        .a2h_min_input = AUDIO_HAPTICS_MIN_INPUT_VOLTAGE,
        .a2h_max_input = AUDIO_HAPTICS_MAX_INPUT_VOLTAGE,
        .a2h_min_output = AUDIO_HAPTICS_MIN_OUTPUT_VOLTAGE,
        .a2h_max_output = AUDIO_HAPTICS_MAX_OUTPUT_VOLTAGE,
    },
#else
#error "please define actuator type"
#endif
};
#elif defined(CONFIG_HAPTICS_DRV2604)
static struct drv2604_platform_data drv2604_plat_data = {
    .GpioEnable = 0, //enable the chip
    .GpioTrigger = 0,//external trigger pin, (0: internal trigger)
#if defined(CONFIG_HAPTICS_LRA_SEMCO1030)
    //rated = 1.5Vrms, ov=2.1Vrms, f=204hz
    .loop = CLOSE_LOOP,
    .RTPFormat = Signed,
    .BIDIRInput = BiDirectional,
    .actuator = {
        .device_type = LRA,
        .rated_vol = 0x3d,
        .over_drive_vol = 0x87,
        .LRAFreq = 204,
    },
#elif defined(CONFIG_HAPTICS_ERM_EVOWAVE_Z4TJGB1512658)
    //rated vol = 3.0 v, ov = 3.6 v, risetime = 150 ms
    .loop = CLOSE_LOOP,
    .RTPFormat = Signed,
    .BIDIRInput = BiDirectional,
    .actuator = {
        .device_type = ERM,
        .rated_vol = 0x8d,
        .over_drive_vol = 0xa9,
    },
#else
#error "please define actuator type"
#endif
};
#elif defined(CONFIG_HAPTICS_DRV2604L)
static struct DRV2604L_platform_data drv2604l_plat_data = {
    .GpioEnable = 0, //enable the chip
    .GpioTrigger = 0,//external trigger pin, (0: internal trigger)
#if defined(CONFIG_HAPTICS_LRA_SEMCO1030)
    //rated = 1.5Vrms, ov=2.1Vrms, f=204hz
    .loop = CLOSE_LOOP,
    .RTPFormat = Signed,
    .BIDIRInput = BiDirectional,
    .actuator = {
        .device_type = LRA,
        .rated_vol = 0x3d,
        .over_drive_vol = 0x87,
        .LRAFreq = 204,
    },
#elif defined(CONFIG_HAPTICS_ERM_EVOWAVE_Z4TJGB1512658)
    //rated vol = 3.0 v, ov = 3.6 v, risetime = 150 ms
    .loop = CLOSE_LOOP,
    .RTPFormat = Signed,
    .BIDIRInput = BiDirectional,
    .actuator = {
        .device_type = ERM,
        .rated_vol = 0x8d,
        .over_drive_vol = 0xa9,
    },
#else
#error "please define actuator type"
#endif
};
#elif defined(CONFIG_HAPTICS_DRV2667)
static struct drv2667_platform_data drv2667_plat_data = {
    .inputGain = DRV2667_GAIN_50VPP,
    .boostTimeout = DRV2667_IDLE_TIMEOUT_20MS,
};
#endif	/* CONFIG_HAPTICS_DRV2605 */
/* *****************************haptics drv2605 end*********************** */

#ifdef CONFIG_TOUCHSCREEN_FT5336
#include <linux/i2c/ft5336_ts.h>
static struct ft5336_platform_data ft5336_tsc_pdata = {
	.x_max          = 540,
	.y_max          = 1020,
	.va_x_max	= 540,
	.va_y_max	= 960,
	.irqflags = IRQF_TRIGGER_FALLING|IRQF_DISABLED,
	.irq = GPIO_TP_INT,
	.reset = GPIO_TP_RESET,
};
#endif
/* *****************************touchscreen end*************************** */

/* *****************************pixart pah8001 start********************** */

#ifdef CONFIG_SENSORS_PIXART_PAH8001
#include <linux/input/pah8001.h>
#endif

#ifdef  CONFIG_SENSORS_PIXART_PAH8001
#if !defined(PAH8001_VDD_NAME)
#define PAH8001_VDD_NAME    NULL
#endif
#if !defined(PAH8001_VIO_NAME)
#define PAH8001_VIO_NAME    NULL
#endif

static struct regulator *pah8001_power_vdd = NULL;
static struct regulator *pah8001_power_vio = NULL;
static atomic_t pah8001_powered = ATOMIC_INIT(0);
static int pah8001_early_init(struct device *dev)
{
    int res;

    if (!pah8001_power_vdd) {
        pah8001_power_vdd = regulator_get(dev, PAH8001_VDD_NAME);
        if (IS_ERR(pah8001_power_vdd)) {
            pr_err("%s -> get regulator VDD failed\n", __func__);
            res = -ENODEV;
            goto err_vdd;
        }
    }

    if (!pah8001_power_vio) {
        pah8001_power_vio = regulator_get(dev, PAH8001_VIO_NAME);
        if (IS_ERR(pah8001_power_vio)) {
            pr_err("%s -> get regulator VIO failed\n", __func__);
            res = -ENODEV;
            goto err_vio;
        }
    }

    return 0;

err_vio:
    regulator_put(pah8001_power_vdd);
    pah8001_power_vdd = NULL;
err_vdd:
    pah8001_power_vio = NULL;
    return res;
}

static int pah8001_exit(struct device *dev)
{
    if (pah8001_power_vio != NULL && !IS_ERR(pah8001_power_vio)) {
        regulator_put(pah8001_power_vio);
        pah8001_power_vio = NULL;       //Must set pointer to NULL
    }

    if (pah8001_power_vdd != NULL && !IS_ERR(pah8001_power_vdd)) {
        regulator_put(pah8001_power_vdd);
        pah8001_power_vdd = NULL;       //Must set pointer to NULL
    }

    atomic_set(&pah8001_powered, 0);

    return 0;
}

static int pah8001_power_on(void)
{
    int res;
    int ret;
    if (!atomic_read(&pah8001_powered)) {
        if (!IS_ERR(pah8001_power_vdd)) {
            ret = regulator_enable(pah8001_power_vdd);
        } else {
            pr_err("%s -> VDD power unavailable!\n", __func__);
            res = -ENODEV;
            goto err_vdd;
        }
        msleep(1);
        if (!IS_ERR(pah8001_power_vio)) {
            ret = regulator_enable(pah8001_power_vio);
        } else {
            pr_err("%s -> VIO power unavailable!\n", __func__);
            res = -ENODEV;
            goto err_vio;
        }
        msleep(1);

        atomic_set(&pah8001_powered, 1);
    }

    return 0;

err_vio:
    regulator_disable(pah8001_power_vdd);
err_vdd:
    return res;
}

static int pah8001_power_off(void)
{
    int res;
    int ret;
    if (atomic_read(&pah8001_powered)) {
        if (!IS_ERR(pah8001_power_vio)) {
            ret = regulator_disable(pah8001_power_vio);
        } else {
            pr_err("%s -> VIO power unavailable!\n", __func__);
            res = -ENODEV;
            goto err_vio;
        }

        if (!IS_ERR(pah8001_power_vdd)) {
            ret = regulator_disable(pah8001_power_vdd);
        } else {
            pr_err("%s -> VDD power unavailable!\n", __func__);
            res = -ENODEV;
            goto err_vdd;
        }

        atomic_set(&pah8001_powered, 0);
    }

    return 0;

err_vio:
err_vdd:
    return res;
}

static struct pah8001_platform_data pah8001_pdata = {
    .gpio_int = GPIO_PAH8001_INT,
    .gpio_reset = GPIO_PAH8001_RESET,
    .gpio_pd = GPIO_PAH8001_PD,
    .board_init = pah8001_early_init,
    .board_exit = pah8001_exit,
    .power_on = pah8001_power_on,
    .power_off = pah8001_power_off,
};
#endif
/* *****************************pixart pah8001 end************************ */

/* *****************************pixart pac7673 start********************** */
#ifdef CONFIG_SENSORS_PIXART_PAC7673
#include <linux/input/pac7673.h>

#if !defined(PAC7673_VDD_NAME)
#define PAC7673_VDD_NAME    NULL
#endif

static struct regulator *pac7673_power_vdd = NULL;
static atomic_t pac7673_powered = ATOMIC_INIT(0);
static int pac7673_early_init(struct device *dev)
{
    int res;

    if (!pac7673_power_vdd) {
        pac7673_power_vdd = regulator_get(dev, PAC7673_VDD_NAME);
        if (IS_ERR(pac7673_power_vdd)) {
            pr_err("%s -> get regulator VDD failed\n", __func__);
            res = -ENODEV;

        }
    }

    return 0;
}

static int pac7673_exit(struct device *dev)
{
    if (pac7673_power_vdd != NULL && !IS_ERR(pac7673_power_vdd)) {
        regulator_put(pac7673_power_vdd);
        pac7673_power_vdd = NULL;       //Must set pointer to NULL
    }

    atomic_set(&pac7673_powered, 0);

    return 0;
}

static int pac7673_power_on(void)
{
    int res;
    static int cnt = 0;
    if (!atomic_read(&pac7673_powered)) {
        if (!IS_ERR(pac7673_power_vdd)) {
            if (cnt == 0) {
                regulator_disable(pac7673_power_vdd);
                cnt++;
            }
            res = regulator_enable(pac7673_power_vdd);
        } else {
            pr_err("%s -> VDD power unavailable!\n", __func__);
            res = -ENODEV;
        }
        msleep(1);

        atomic_set(&pac7673_powered, 1);
    }

    return 0;
}

static int pac7673_power_off(void)
{
    int res;
    if (atomic_read(&pac7673_powered)) {
        if (!IS_ERR(pac7673_power_vdd)) {
            res = regulator_disable(pac7673_power_vdd);
        } else {
            pr_err("%s -> VDD power unavailable!\n", __func__);
            res = -ENODEV;
        }

        atomic_set(&pac7673_powered, 0);
    }

    return 0;
}

static struct pac7673_platform_data pac7673_pdata = {
    .gpio_int = GPIO_PAC7673_INT,
    .board_init = pac7673_early_init,
    .board_exit = pac7673_exit,
    .power_on = pac7673_power_on,
    .power_off = pac7673_power_off,
};

#endif
/* *****************************pixart pac7673 end************************ */

#if (defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C0_V12_JZ))
struct i2c_board_info jz_i2c0_devs[] __initdata = {
#ifdef CONFIG_TOUCHSCREEN_GWTC9XXXB
	{
		I2C_BOARD_INFO("gwtc9xxxb_ts", 0x05),
		.platform_data = &fpga_tsc_pdata,
	},
#endif

#ifdef CONFIG_TOUCHSCREEN_FT6X06
	{
		I2C_BOARD_INFO("ft6x06_ts", 0x38),
		.platform_data = &ft6x06_tsc_pdata,
	},
#endif

#ifdef CONFIG_TOUCHSCREEN_ITE7258
	{
		I2C_BOARD_INFO("ite7258_ts", 0x46),
		.platform_data = &ite7258_tsc_pdata,
	},
#endif


#ifdef CONFIG_TOUCHSCREEN_FT6X0X
	{
		I2C_BOARD_INFO("ft6x0x_tsc", 0x38),
		.platform_data = &ft6x0x_tsc_pdata,
	},
#endif

#ifdef CONFIG_TOUCHSCREEN_FT5336
	{
		I2C_BOARD_INFO("ft5336_ts", 0x38),
		.platform_data = &ft5336_tsc_pdata,
	},
#endif
#if defined(CONFIG_BCM2079X_NFC)
	{
		I2C_BOARD_INFO("bcm2079x-i2c", 0x77),
		.platform_data = &bcm2079x_pdata,
	},
#endif /*CONFIG_BCM2079X_NFC*/
};
#endif  /*I2C0*/


#ifdef CONFIG_GPIO_PCA953X
static struct pca953x_platform_data dorado_pca953x_pdata = {
	.gpio_base  = PCA9539_GPIO_BASE,
        .irq_base  = IRQ_RESERVED_BASE + 101,
        .reset_n  = PCA9539_RST_N,
	.irq_n  = PCA9539_IRQ_N,
 };
#endif

#if (defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C1_V12_JZ))
struct i2c_board_info jz_i2c1_devs[] __initdata = {
#ifdef CONFIG_GPIO_PCA953X
	{
		I2C_BOARD_INFO("pca9539",0x74),
		.platform_data  = &dorado_pca953x_pdata,
	},
#endif
#ifdef CONFIG_TOUCHSCREEN_ITE7258
	{
		I2C_BOARD_INFO("ite7258_ts", 0x46),
		.platform_data = &ite7258_tsc_pdata,
	},
#endif
};
#endif  /*I2C1*/

#if (defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C2_V12_JZ))
struct i2c_board_info jz_i2c2_devs[] __initdata = {

#if defined(CONFIG_FRIZZ)
	{
		I2C_BOARD_INFO("frizz", 0x38),
		.platform_data = &frizz_pdata,
	},
#endif
#ifdef CONFIG_INV_MPU_IIO
	{
		I2C_BOARD_INFO("mpu6500", 0x68),
		.irq = (IRQ_GPIO_BASE + GPIO_GSENSOR_INT),
		.platform_data = &mpu9250_platform_data,
	},
#endif /*CONFIG_INV_MPU_IIO*/
#ifdef CONFIG_SENSORS_MMA955XL_I2C
    {
        I2C_BOARD_INFO("mma955xl", 0x4C),
        .platform_data = &mma955xl_pdata,
    },
#endif
};
#endif  /*I2C2*/

#if (defined(CONFIG_I2C_GPIO) || defined(CONFIG_I2C3_V12_JZ))
struct i2c_board_info jz_i2c3_devs[] __initdata = {

#ifdef  CONFIG_SENSORS_PIXART_PAH8001
    {
        I2C_BOARD_INFO("pixart_ofn", 0x33),
        .irq = (IRQ_GPIO_BASE + GPIO_PAH8001_INT),
        .platform_data = &pah8001_pdata,
    },
#endif

#if defined(CONFIG_HAPTICS_DRV2605)
    {
        I2C_BOARD_INFO(HAPTICS_DEVICE_NAME, 0x5a),
        .platform_data = &drv2605_plat_data,
    },
#elif defined(CONFIG_HAPTICS_DRV2604)
    {
        I2C_BOARD_INFO(HAPTICS_DEVICE_NAME, 0x5a),
        .platform_data = &drv2604_plat_data,
    },
#elif defined(CONFIG_HAPTICS_DRV2604L)
    {
        I2C_BOARD_INFO(HAPTICS_DEVICE_NAME, 0x5a),
        .platform_data = &drv2604l_plat_data,
    },
#elif defined(CONFIG_HAPTICS_DRV2667)
    {
        I2C_BOARD_INFO(HAPTICS_DEVICE_NAME, 0x59),
        .platform_data = &drv2667_plat_data,
    },
#endif

#ifdef  CONFIG_SENSORS_PIXART_PAC7673
    {
        I2C_BOARD_INFO("pac7673", 0x48),
        .irq = (IRQ_GPIO_BASE + GPIO_PAC7673_INT),
        .platform_data = &pac7673_pdata,
    },
#endif

};
#endif  /*I2C3*/

#if     defined(CONFIG_SOFT_I2C0_GPIO_V12_JZ) || defined(CONFIG_I2C0_V12_JZ)
int jz_i2c0_devs_size = ARRAY_SIZE(jz_i2c0_devs);
#endif

#if     defined(CONFIG_SOFT_I2C1_GPIO_V12_JZ) || defined(CONFIG_I2C1_V12_JZ)
int jz_i2c1_devs_size = ARRAY_SIZE(jz_i2c1_devs);
#endif

#if     defined(CONFIG_SOFT_I2C2_GPIO_V12_JZ) || defined(CONFIG_I2C2_V12_JZ)
int jz_i2c2_devs_size = ARRAY_SIZE(jz_i2c2_devs);
#endif

#if     defined(CONFIG_SOFT_I2C3_GPIO_V12_JZ) || defined(CONFIG_I2C3_V12_JZ)
int jz_i2c3_devs_size = ARRAY_SIZE(jz_i2c3_devs);
#endif
/*
 * define gpio i2c,if you use gpio i2c,
 * please enable gpio i2c and disable i2c controller
 */
#ifdef CONFIG_I2C_GPIO
#define DEF_GPIO_I2C(NO)						\
	static struct i2c_gpio_platform_data i2c##NO##_gpio_data = {	\
		.sda_pin	= GPIO_I2C##NO##_SDA,			\
		.scl_pin	= GPIO_I2C##NO##_SCK,			\
	};								\
	struct platform_device i2c##NO##_gpio_device = {		\
		.name	= "i2c-gpio",					\
		.id	= NO,						\
		.dev	= { .platform_data = &i2c##NO##_gpio_data,},	\
	};

#ifdef CONFIG_SOFT_I2C0_GPIO_V12_JZ
DEF_GPIO_I2C(0);
#endif
#ifdef CONFIG_SOFT_I2C1_GPIO_V12_JZ
DEF_GPIO_I2C(1);
#endif
#ifdef CONFIG_SOFT_I2C2_GPIO_V12_JZ
DEF_GPIO_I2C(2);
#endif
#ifdef CONFIG_SOFT_I2C3_GPIO_V12_JZ
DEF_GPIO_I2C(3);
#endif
#endif /*CONFIG_I2C_GPIO*/
