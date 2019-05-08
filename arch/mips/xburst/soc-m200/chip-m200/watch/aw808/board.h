#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>
#include <linux/jz_dwc.h>

#if defined(CONFIG_REGULATOR_SM5007)
#include "pmu5007.h"
#elif defined(CONFIG_REGULATOR_RICOH619)
#include "pmu.h"
#endif
/* ****************************GPIO SLEEP START******************************* */
#define GPIO_REGULATOR_SLP	GPIO_PB(0)
#define GPIO_OUTPUT_TYPE	GPIO_OUTPUT1
/* ****************************GPIO SLEEP END******************************** */

/* ****************************SENSOR HUB START********************************** */
#ifdef CONFIG_FRIZZ
#define FRIZZ_IRQ GPIO_PA(9)
#define FRIZZ_RESET GPIO_PA(8)
#define FRIZZ_WAKEUP GPIO_PA(15)

#if defined(CONFIG_AW808_HW_X3)
#define GSENSOR_CHIP_ORIENTATION 7
#elif defined(CONFIG_AW808_HW_IN901)
#define GSENSOR_CHIP_ORIENTATION 2
#else
#define GSENSOR_CHIP_ORIENTATION 4
#endif //define g_chip_orientation

#endif
/* ****************************SENSOR HUB END********************************** */


/* ****************************GPIO LCD START******************************** */
#ifdef CONFIG_LCD_BYD_8991FTGF
#define GPIO_LCD_DISP		GPIO_PB(0)
#define GPIO_LCD_DE		0
#define GPIO_LCD_VSYNC		0
#define GPIO_LCD_HSYNC		0
#define GPIO_LCD_CS		GPIO_PA(11)
#define GPIO_LCD_CLK	        GPIO_PD(28)
//#define GPIO_LCD_SDO		GPIO_PE(3)
//#define GPIO_LCD_SDI		GPIO_PE(0)
#define GPIO_LCD_BACK_SEL	GPIO_PC(20)
#endif
#ifdef CONFIG_LCD_LH155
#define GPIO_LCD_BLK            GPIO_PC(17)
#define GPIO_LCD_RST            GPIO_PA(12)
#endif
#ifdef CONFIG_LCD_TRULY_TDO_HD0499K
#define GPIO_MIPI_RST_N         GPIO_PA(12)
#define GPIO_MIPI_PWR           GPIO_PC(17)
#endif
#ifdef CONFIG_LCD_BYD_9177AA
#define GPIO_MIPI_RST_N         GPIO_PC(3)
#define GPIO_MIPI_PWR           GPIO_PC(2)
#endif
#ifdef CONFIG_BACKLIGHT_PWM
#define GPIO_LCD_PWM		GPIO_PE(1)
#endif
#ifdef CONFIG_BACKLIGHT_DIGITAL_PULSE
#define GPIO_GIGITAL_PULSE      GPIO_PE(1)
#endif
#ifdef CONFIG_LCD_CV90_M5377_P30
#define GPIO_LCD_BLK		GPIO_PC(17)
#define GPIO_LCD_RST            GPIO_PA(12)
#define GPIO_LCD_NRD_E		GPIO_PC(8)
#define GPIO_LCD_NWR_SCL	GPIO_PC(25)
#define GPIO_LCD_DNC		GPIO_PC(26)
#define SLCD_NBUSY_PIN		GPIO_PA(11)
#endif
#ifdef CONFIG_JZ_EPD_V12
#define GPIO_EPD_PWR0           GPIO_PC(22)
#define GPIO_EPD_PWR1           GPIO_PC(23)
#define GPIO_EPD_PWR2           GPIO_PC(24)
#define GPIO_EPD_PWR3           GPIO_PC(25)
#define GPIO_EPD_EN             GPIO_PC(23)
#define GPIO_EPD_ENOP           GPIO_PC(24)
#endif
#ifdef  CONFIG_LCD_X163
#define GPIO_LCD_RST                 GPIO_PC(19)
#define GPIO_LCD_BLK_EN              GPIO_PC(23)
#define VCC_LCD_1V8_NAME             LDO4_NAME
#define VCC_LCD_3V0_NAME             LDO6_NAME
#define VCC_LCD_BLK_NAME             "lcd_blk_vcc"
#define DSI_TE_GPIO                  GPIO_PC(18)
#define GPIO_LCD_SWIRE               GPIO_PC(24)
#define GPIO_LCD_SWIRE_ACTIVE_LEVEL  1
#endif
#ifdef CONFIG_LCD_AUO_H139BLN01
#define GPIO_MIPI_RST_N        GPIO_PC(19)
#define DSI_TE_GPIO            GPIO_PC(18)
#define GPIO_LCD_BLK_EN        -1
#define VCC_LCD_1V8_NAME       LDO4_NAME
#define VCC_LCD_2V8_NAME       LDO6_NAME
#endif
#ifdef CONFIG_JZ_EPD_V12
#define GPIO_EPD_PWR0           -1
#define GPIO_EPD_PWR1           -1
#define GPIO_EPD_PWR2           -1
#define GPIO_EPD_PWR3           -1
#define GPIO_EPD_EN             -1
#define GPIO_EPD_ENOP           -1
#endif
/* ****************************GPIO LCD END********************************** */

/* ****************************GPIO I2C START******************************** */
#ifndef CONFIG_I2C0_V12_JZ
#define GPIO_I2C0_SDA GPIO_PD(30)
#define GPIO_I2C0_SCK GPIO_PD(31)
#endif
#ifndef CONFIG_I2C1_V12_JZ
#define GPIO_I2C1_SDA GPIO_PE(30)
#define GPIO_I2C1_SCK GPIO_PE(31)
#endif
#ifndef CONFIG_I2C2_V12_JZ
#define GPIO_I2C2_SDA GPIO_PE(00)
#define GPIO_I2C2_SCK GPIO_PE(03)
#endif
#ifndef CONFIG_I2C3_V12_JZ
#define GPIO_I2C3_SDA GPIO_PB(7)
#define GPIO_I2C3_SCK GPIO_PB(8)
#endif
/* ****************************GPIO I2C END********************************** */

/* ****************************GPIO SPI START******************************** */
#ifdef CONFIG_SPI_GPIO
#define GPIO_SPI_SCK  GPIO_PC(13)
#define GPIO_SPI_MISO GPIO_PC(14)
#define GPIO_SPI_MOSI GPIO_PC(15)
#endif
/* ****************************GPIO SPI END********************************** */

#ifdef  CONFIG_AFE4403
#define SPI_STE GPIO_PB(8)
#define AFE_ADC_RDY GPIO_PB(7)
#define AFE_PWR_EN  GPIO_PC(12)
#define AFE_ACG_CLK GPIO_PE(2)
#endif

/* ****************************GPIO TOUCHSCREEN START************************ */
#ifdef CONFIG_TOUCHSCREEN_GWTC9XXXB
#define GPIO_TP_INT		GPIO_PB(0)
#define GPIO_TP_WAKE		GPIO_PE(10)
#endif
#ifdef CONFIG_TOUCHSCREEN_FT6X0X
#define GPIO_TP_INT			GPIO_PA(12)
#define GPIO_TP_WAKE			GPIO_PA(14)
#define GPIO_TP_EN			-1
#define VCC_TOUCHSCREEN LDO10_NAME
#define VIO_TOUCHSCREEN "" /* not use */
#endif
#ifdef CONFIG_TOUCHSCREEN_FT6X06
#define GPIO_TP_INT		GPIO_PD(17)
#define GPIO_TP_RESET		GPIO_PD(27)
#endif
#ifdef CONFIG_TOUCHSCREEN_FT5336
#define GPIO_TP_INT		GPIO_PD(17)
#define GPIO_TP_RESET		GPIO_PD(27)
#endif
#ifdef  CONFIG_TOUCHSCREEN_ITE7258
#define GPIO_TP_INT             GPIO_PA(12)
#define GPIO_TP_RESET             GPIO_PA(14)
#define GPIO_TP_EN			-1
#define VCC_TOUCHSCREEN LDO10_NAME
#define VIO_TOUCHSCREEN LDO9_NAME
#endif  /* CONFIG_TOUCHSCREEN_ITE7258 */

/* ****************************GPIO TOUCHSCREEN END************************** */

/* ****************************GPIO KEY START******************************** */
#define GPIO_BACK_KEY		 GPIO_PD(18)
#define ACTIVE_LOW_BACK		0

//#define GPIO_VOLUMEDOWN_KEY        -1
//#define ACTIVE_LOW_VOLUMEDOWN	0

#define GPIO_ENDCALL_KEY            GPIO_PA(30)
#define ACTIVE_LOW_ENDCALL      1

/* ****************************GPIO KEY END********************************** */

/* ****************************GPIO PMU START******************************** */
/* PMU ricoh619 */
#ifdef CONFIG_REGULATOR_RICOH619
#define PMU_IRQ_N		GPIO_PA(0)
#endif /* CONFIG_REGULATOR_RICOH619 */

/* pmu d2041 or 9024 gpio def*/
#ifdef CONFIG_REGULATOR_DA9024
#define GPIO_PMU_IRQ		GPIO_PA(13)
#endif

/* PMU sm5007 */
#ifdef CONFIG_REGULATOR_SM5007
#define PMU_IRQ_N5007		GPIO_PE(2)
#endif /* CONFIG_REGULATOR_SM5007 */
/* ****************************GPIO PMU END********************************** */

/* ****************************SENSOR HUB START********************************** */
#ifdef CONFIG_FRIZZ
#define FRIZZ_IRQ GPIO_PA(9)
#define FRIZZ_RESET GPIO_PA(8)
#define FRIZZ_WAKEUP GPIO_PA(15)

#if defined(CONFIG_AW808_HW_X3)
#define GSENSOR_CHIP_ORIENTATION 7
#elif defined(CONFIG_AW808_HW_IN901)
#define GSENSOR_CHIP_ORIENTATION 2
#else
#define GSENSOR_CHIP_ORIENTATION 4
#endif //define g_chip_orientation

#endif
/* ****************************SENSOR HUB END********************************** */

/* ****************************GPIO GSENSOR START**************************** */
#define GPIO_GSENSOR_INT	-1
#define GSENSOR_VDD_NAME	LDO5_NAME
#define GSENSOR_VIO_NAME	DC4_NAME
/* ****************************GPIO GSENSOR END****************************** */

/* ****************************GPIO UVSENSOR START**************************** */
#ifdef CONFIG_SENSORS_SI1132
#define VCC_SENSOR_1V8_NAME  LDO7_NAME
#define GPIO_UVSENSOR_INT    GPIO_PE(2)
#endif
/* ****************************GPIO UVSENSOR END****************************** */

/* ****************************GPIO HEARTRATE START************************** */
#define GPIO_PAH8001_INT 	GPIO_PA(10)
#define GPIO_PAH8001_RESET 	GPIO_PA(11)
#define GPIO_PAH8001_PD 	-1
#if defined(CONFIG_AW808_HW_IN901)
#define PAH8001_VDD_NAME    LDO7_NAME
#define PAH8001_VIO_NAME    LDO8_NAME
#endif
/* ****************************GPIO HEARTRATE END**************************** */

/* ****************************GPIO EFUSE START****************************** */
#define GPIO_EFUSE_VDDQ      -1
/* ****************************GPIO EFUSE END******************************** */

/* ****************************GPIO LI ION START***************************** */
//#define GPIO_LI_ION_CHARGE   GPIO_PB(1)
//#define GPIO_LI_ION_AC       GPIO_PA(13)
#define GPIO_ACTIVE_LOW      1
#define GPIO_ACTIVE_HIGH     0
#define GPIO_USB_DETE_ACTIVE_LEVEL  GPIO_ACTIVE_LOW
/* ****************************GPIO LI ION END******************************* */

/* ****************************GPIO MMC START******************************** */
#define GPIO_MMC_RST_N			GPIO_PA(29)
#define GPIO_MMC_RST_N_LEVEL	LOW_ENABLE
/* ****************************GPIO MMC END******************************** */

/* ****************************GPIO USB START******************************** */
//#define GPIO_USB_ID			GPIO_PA(13)
#define GPIO_USB_ID_LEVEL		LOW_ENABLE
#define GPIO_USB_DETE			GPIO_PA(1)
#define GPIO_USB_DETE_LEVEL		LOW_ENABLE
//#define GPIO_USB_DRVVBUS		GPIO_PE(10)
//#define GPIO_USB_DRVVBUS_LEVEL		HIGH_ENABLE
/* ****************************GPIO USB END********************************** */

/* ****************************GPIO CAMERA START***************************** */
#if 0
#define CAMERA_RST		GPIO_PC(22)
#define CAMERA_PWDN_N           GPIO_PC(23) /* pin conflict with USB_ID */
//#define CAMERA_MCLK		GPIO_PE(2) /* no use */
#ifdef CONFIG_DVP_OV9712
#define OV9712_POWER	 	GPIO_PC(2) //the power of camera board
#define OV9712_RST		GPIO_PA(11)
#define OV9712_PWDN_EN		GPIO_PD(28)
#endif
#endif
/* ****************************GPIO CAMERA END******************************* */

/* ****************************GPIO AUDIO START****************************** */
#define GPIO_HP_MUTE		-1	/*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL	-1	/*vaild level*/

#define GPIO_SPEAKER_EN		GPIO_PA(2)      /*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL	1

#define GPIO_HANDSET_EN		-1	/*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL   -1

#define GPIO_HP_DETECT		-1		/*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL    -1
#define GPIO_MIC_SELECT		-1	/*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL	-1	/*builin mic select level*/
#define GPIO_MIC_DETECT		-1
#define GPIO_MIC_INSERT_LEVEL   -1
#define GPIO_MIC_DETECT_EN	-1  /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL -1 /*mic detect enable gpio*/

#define HP_SENSE_ACTIVE_LEVEL	0
#define HOOK_ACTIVE_LEVEL		-1
/* ****************************GPIO AUDIO END******************************** */

/* ****************************GPIO WIFI START******************************* */
#define BCM_PWR_EN       GPIO_PC(12)
#define WL_WAKE_HOST     GPIO_PC(17)
#define WL_REG_EN        GPIO_PC(13)
#define HOST_WAKE_WL     (-1)
/* ****************************GPIO WIFI END********************************* */

/* ***************************GPIO VIBRATOR START***************************** */
#ifdef	CONFIG_ANDROID_TIMED_REGULATOR
#define	REG_VDDIO	LDO4_NAME
#define	MAX_TIMEOUT	18000
#endif

#ifdef	CONFIG_ANDROID_TIMED_GPIO

#if defined(CONFIG_AW808_HW_X3) || defined(CONFIG_AW808_HW_IN901) || defined(CONFIG_AW808_HW_MAIN) || defined(CONFIG_AW808_HW_V11_NATURAL_ROUND) || defined(CONFIG_AW808_HW_V11_WISE_SQUARE)
#define	VIBRATOR_EN		GPIO_PE(2)
#define	ACTIVE_LEVEL	0
#else
#define	VIBRATOR_EN		-1
#define	ACTIVE_LEVEL	0
#endif

#define	MAX_TIMEOUT	15000
#endif
/* ****************************GPIO VIBRATOR END******************************* */


/* ****************************GPIO NFC START******************************** */
/*
 * For BCM2079X NFC
 */
//#define NFC_REQ		GPIO_PC(26)
//#define NFC_REG_PU	GPIO_PC(27)
//#define HOST_WAKE_NFC   GPIO_PA(11)
/* ****************************GPIO NFC END********************************** */

/* ****************************GPIO BLUETOOTH START************************** */
/* BT gpio */
#ifdef  CONFIG_BROADCOM_RFKILL

#define HOST_WAKE_BT	GPIO_PC(15)
#define BT_WAKE_HOST	GPIO_PC(16)
#define BT_REG_EN       GPIO_PC(14)
#define BT_UART_RTS     GPIO_PD(28)
#define HOST_BT_RST     -1

//#define GPIO_PB_FLGREG      (0x10010058)
#define GPIO_BT_INT_BIT     (1 << (BT_WAKE_HOST % 32))
/* bluetooth uart set */
#define BLUETOOTH_UPORT_NAME  "ttyS1"
#define BLUETOOTH_UART_GPIO_PORT	GPIO_PORT_D
#define BLUETOOTH_UART_GPIO_FUNC	GPIO_FUNC_2
#define BLUETOOTH_UART_FUNC_SHIFT	(0x1<<28)
#endif
/* ****************************GPIO BLUETOOTH END**************************** */

/* ****************************GPIO DRV2605 START**************************** */
#ifndef VIBRATOR_EN
#ifdef CONFIG_HAPTICS_DRV2605
#define DRV2605_ENABLE		GPIO_PE(2)
#define DRV2605_IN_TRIGGER	GPIO_PB(1)
#endif
#endif
/* ****************************GPIO DRV2605 END****************************** */

/* ****************************GPIO MMA955XL START*************************** */
#if defined(CONFIG_SENSORS_MMA955XL_I2C)
#define MMA955XL_WAKEUP_AP     GPIO_PA(9)
#define AP_WAKEUP_MMA955XL     GPIO_PA(15)
#define MMA955XL_INTO_N        GPIO_PA(13)
#endif
/* ****************************GPIO MMA955XL END***************************** */
#endif /* __BOARD_H__ */
