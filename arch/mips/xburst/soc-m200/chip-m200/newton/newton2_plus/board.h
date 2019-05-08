#ifndef __BOARD_H__
#define __BOARD_H__
#include <gpio.h>
#include <soc/gpio.h>
#include <linux/jz_dwc.h>

#include "pmu.h"

/* ****************************GPIO SLEEP START******************************* */
#define GPIO_REGULATOR_SLP	(-1)
#define GPIO_OUTPUT_TYPE	GPIO_OUTPUT1
/* ****************************GPIO SLEEP END******************************** */

/* ****************************GPIO LCD START******************************** */
#ifdef CONFIG_LCD_BYD_8991FTGF
#define GPIO_LCD_DISP		(-1)
#define GPIO_LCD_DE		0
#define GPIO_LCD_VSYNC		0
#define GPIO_LCD_HSYNC		0
#define GPIO_LCD_CS		(-1)
#define GPIO_LCD_CLK	        (-1)
#define GPIO_LCD_SDO		(-1)
#define GPIO_LCD_SDI		(-1)
#define GPIO_LCD_BACK_SEL	(-1)
#endif
#ifdef CONFIG_LCD_LH155
#define GPIO_LCD_BLK            (-1)
#define GPIO_LCD_RST            (-1)
#endif
#ifdef CONFIG_LCD_TRULY_TDO_HD0499K
#define GPIO_MIPI_RST_N         (-1)
#define GPIO_MIPI_PWR           (-1)
#endif
#ifdef CONFIG_LCD_BYD_9177AA
#define GPIO_MIPI_RST_N         (-1)
#define GPIO_MIPI_PWR           (-1)
#endif
#ifdef CONFIG_BACKLIGHT_PWM
#define GPIO_LCD_PWM		GPIO_PE(1)
#define GPIO_LCD_PWR_ON		GPIO_PD(0)
#endif
#ifdef CONFIG_BACKLIGHT_DIGITAL_PULSE
#define GPIO_GIGITAL_PULSE      (-1)
#endif
#ifdef CONFIG_LCD_CV90_M5377_P30
#define GPIO_LCD_BLK		(-1)
#define GPIO_LCD_RST            (-1)
#define GPIO_LCD_NRD_E		(-1)
#define GPIO_LCD_NWR_SCL	(-1)
#define GPIO_LCD_DNC		(-1)
#define SLCD_NBUSY_PIN		(-1)
#endif
#ifdef CONFIG_JZ_EPD_V12
#define GPIO_EPD_PWR0           (-1)
#define GPIO_EPD_PWR1           (-1)
#define GPIO_EPD_PWR2           (-1)
#define GPIO_EPD_PWR3           (-1)
#define GPIO_EPD_EN             (-1)
#define GPIO_EPD_ENOP           (-1)
#endif
#ifdef CONFIG_LCD_X163
#define GPIO_LCD_RST            (-1)
#define VCC_LCD_1V8_NAME        NULL
#define VCC_LCD_3V0_NAME        NULL
#define DSI_TE_GPIO             (-1)
#endif

#ifdef CONFIG_LCD_BOE_TFT320320
#define GPIO_LCD_RST            GPIO_PD(3)
#define GPIO_MIPI_RST_N         GPIO_LCD_RST
#define GPIO_LCD_BLK_EN		GPIO_LCD_PWR_ON
#define DSI_TE_GPIO             GPIO_PD(4)
#define VCC_LCD_1V8_NAME        LDO2_NAME
#define VCC_LCD_2V8_NAME        LDO4_NAME
#endif
#ifdef CONFIG_LCD_AUO_H139BLN01
#define GPIO_MIPI_RST_N        GPIO_PC(19)
#define DSI_TE_GPIO            GPIO_PC(18)
#define GPIO_LCD_BLK_EN        -1
#define VCC_LCD_1V8_NAME       LDO4_NAME
#define VCC_LCD_2V8_NAME       LDO6_NAME
#endif
#ifdef CONFIG_LCD_EDO_E1392AM1
#define GPIO_MIPI_RST_N        GPIO_PD(03)
#define DSI_TE_GPIO            GPIO_PD(04)
#define GPIO_LCD_BLK_EN        GPIO_PD(0)
#define VCC_LCD_1V8_NAME       LDO2_NAME
#define VCC_LCD_2V8_NAME       LDO4_NAME
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
#ifndef CONFIG_SPI_GPIO
#define GPIO_SPI_SCK  GPIO_PE(15)
#define GPIO_SPI_MOSI GPIO_PE(17)
#define GPIO_SPI_MISO GPIO_PE(14)
#endif
/* ****************************GPIO SPI END********************************** */

/* ****************************GPIO TOUCHSCREEN START************************ */
#ifdef CONFIG_TOUCHSCREEN_GWTC9XXXB
#define GPIO_TP_INT		(-1)
#define GPIO_TP_WAKE		(-1)
#endif
#ifdef CONFIG_TOUCHSCREEN_FT6X0X
#define GPIO_TP_INT		(-1)
#define GPIO_TP_WAKE		(-1)
#endif
#ifdef CONFIG_TOUCHSCREEN_FT6X06
#define GPIO_TP_INT		(-1)
#define GPIO_TP_RESET		(-1)
#endif
#ifdef CONFIG_TOUCHSCREEN_FT5336
#define GPIO_TP_INT		(-1)
#define GPIO_TP_RESET		(-1)
#endif
#ifdef  CONFIG_TOUCHSCREEN_ITE7258
#define VIO_TOUCHSCREEN			LDO2_NAME
#define VCC_TOUCHSCREEN         LDO4_NAME
#define GPIO_TP_INT             GPIO_PC(14)
#define GPIO_TP_RESET           GPIO_PC(13)
#endif  /* CONFIG_TOUCHSCREEN_ITE7258 */
#ifdef  CONFIG_TOUCHSCREEN_FOCALTECH
#define GPIO_TP_INT             GPIO_PC(14)
#define GPIO_TP_RESET           GPIO_PC(13)
#define VCC_TOUCHSCREEN         PS5_NAME
#endif  /* CONFIG_TOUCHSCREEN_FOCALTECH */

/* ****************************GPIO TOUCHSCREEN END************************** */

/* ****************************GPIO KEY START******************************** */
/*#define GPIO_HOME_KEY		GPIO_PD(18)
#define ACTIVE_LOW_HOME		1 */

#define GPIO_BACK_KEY		GPIO_PD(18)
#define  ACTIVE_LOW_BACK	0

#define GPIO_ENDCALL_KEY	GPIO_PA(30)
#define ACTIVE_LOW_ENDCALL      1

/* ****************************GPIO KEY END********************************** */

/* ****************************GPIO PMU START******************************** */
/* PMU ricoh619 */
#ifdef CONFIG_REGULATOR_RICOH619
#define PMU_IRQ_N		(-1)
#endif /* CONFIG_REGULATOR_RICOH619 */

/* pmu d2041 or 9024 gpio def*/
#ifdef CONFIG_REGULATOR_DA9024
#define GPIO_PMU_IRQ		(-1)
#endif

/* PMU sm5007 */
#ifdef CONFIG_REGULATOR_SM5007
#define PMU_IRQ_N5007		GPIO_PC(17)
#endif /* CONFIG_REGULATOR_SM5007 */
/* ****************************GPIO PMU END********************************** */

/******************************GPIO PCA9539 START***********************************/
/*GPIO PCA9593*/
#ifdef CONFIG_GPIO_PCA953X
#define PCA9539_IRQ_N       (-1)
#define PCA9539_RST_N       (-1)
#define PCA9539_GPIO_BASE   (-1)
#define PCA9539_EXT_GPIO(x)   (PCA9539_GPIO_BASE + (x))
#endif /* CONFIG_PCA9539 */
/* ****************************GPIO PCA9539 END********************************** */

/* ****************************GPIO GSENSOR START**************************** */
#define GPIO_GSENSOR_INT     (-1)
/* ****************************GPIO GSENSOR END****************************** */


/* ****************************SensorHub ICM30630 END************************ */
#ifdef CONFIG_SENSORHUB_ICM30630
#define GPIO_ICM30630_GPIO0_INT             GPIO_PC(16)
#define GPIO_ICM30630_GPIO1_NOWAKE_INT      GPIO_PC(12)
#define GPIO_ICM30630_GPIO2_WAKEUP_GPIO     GPIO_PC(23)
#define ICM30630_VDD_ANA                    LDO5_NAME
#define ICM30630_VCC_I2C                    BUCK3_NAME
#endif


/* ****************************GPIO EFUSE START****************************** */
#define GPIO_EFUSE_VDDQ      -1
/* ****************************GPIO EFUSE END******************************** */

/* ****************************GPIO LI ION START***************************** */
#define GPIO_LI_ION_CHARGE   (-1)
#define GPIO_LI_ION_AC       (-1)
#define GPIO_ACTIVE_LOW      1
/* ****************************GPIO LI ION END******************************* */

/* ****************************GPIO MMC START******************************** */
#define GPIO_MMC_RST_N		(-1)
#define GPIO_MMC_RST_N_LEVEL	(-1)
/* ****************************GPIO MMC END******************************** */

/* ****************************GPIO USB START******************************** */
#define GPIO_USB_ID		(-1)/*GPIO_PA(13)*/
#define GPIO_USB_ID_LEVEL	LOW_ENABLE
#define GPIO_USB_DETE           GPIO_PD(7)
#define GPIO_USB_DETE_LEVEL	LOW_ENABLE
#define GPIO_USB_DRVVBUS	(-1)
#define GPIO_USB_DRVVBUS_LEVEL	HIGH_ENABLE
/* ****************************GPIO USB END********************************** */

/* ****************************GPIO CAMERA START***************************** */
#define CAMERA_RST		(-1)
#define CAMERA_PWDN_N	GPIO_PD(2) /* pin conflict with USB_ID */
#define CAMERA_PWEN		(-1)
#define CAMERA_MCLK		(-1) /* no use */
#ifdef CONFIG_DVP_OV9712
#define OV9712_POWER	 	(-1) //the power of camera board
#define OV9712_RST		(-1)
#define OV9712_PWDN_EN		(-1)
#endif
/* ****************************GPIO CAMERA END******************************* */

/* ****************************GPIO AUDIO START****************************** */
#define GPIO_HP_MUTE		(-1)	/*hp mute gpio*/
#define GPIO_HP_MUTE_LEVEL	(-1)	/*vaild level*/

#define GPIO_SPEAKER_EN		(-1)      /*speaker enable gpio*/
#define GPIO_SPEAKER_EN_LEVEL	(-1)

#define GPIO_HANDSET_EN		(-1)	/*handset enable gpio*/
#define GPIO_HANDSET_EN_LEVEL   (-1)

#define	GPIO_HP_DETECT		(-1)		/*hp detect gpio*/
#define GPIO_HP_INSERT_LEVEL    (1)
#define GPIO_MIC_SELECT		(-1)	/*mic select gpio*/
#define GPIO_BUILDIN_MIC_LEVEL	(-1)	/*builin mic select level*/
#define GPIO_MIC_DETECT		(-1)
#define GPIO_MIC_INSERT_LEVEL   (-1)
#define GPIO_MIC_DETECT_EN	(-1)  /*mic detect enable gpio*/
#define GPIO_MIC_DETECT_EN_LEVEL (-1) /*mic detect enable gpio*/

#define HP_SENSE_ACTIVE_LEVEL	(0)
#define HOOK_ACTIVE_LEVEL	(-1)
/* ****************************GPIO AUDIO END******************************** */

/* ****************************GPIO WIFI START******************************* */
#define BCM_PWR_EN		(-1)
#define WL_WAKE_HOST		GPIO_PD(12)
#define WL_REG_EN		GPIO_PD(13)
#define HOST_WAKE_WL		(-1)
/* ****************************GPIO WIFI END********************************* */

#ifdef CONFIG_ANDROID_TIMED_REGULATOR
#define REG_VDDIO   PS4_NAME
#define MAX_TIMEOUT 18000
#endif


/* ****************************GPIO NFC START******************************** */
/*
 * For BCM2079X NFC
 */
#define NFC_REQ         -1//GPIO_PC(26)
#define NFC_REG_PU      -1//GPIO_PC(27)
#define HOST_WAKE_NFC   -1//GPIO_PA(11)
/* ****************************GPIO NFC END********************************** */

/* ****************************GPIO BLUETOOTH START************************** */
/* BT gpio */

#define BLUETOOTH_UART_GPIO_PORT        GPIO_PORT_D
#define BLUETOOTH_UART_GPIO_FUNC        GPIO_FUNC_2
#define BLUETOOTH_UART_FUNC_SHIFT       (1 << 28)

#define HOST_WAKE_BT	GPIO_PD(5)
#define BT_WAKE_HOST	GPIO_PD(10)
#define BT_REG_EN	GPIO_PD(9)
#define BT_UART_RTS	GPIO_PD(28)
#define BLUETOOTH_UPORT_NAME  "ttyS1"
/* ****************************GPIO BLUETOOTH END**************************** */

#endif /* __BOARD_H__ */
