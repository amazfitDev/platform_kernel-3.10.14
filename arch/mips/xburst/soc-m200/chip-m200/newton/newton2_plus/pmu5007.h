#ifndef __PMU5007_H__
#define __PMU5007_H__
//#ifdef CONFIG_REGULATOR_SM5007

#define PMU_I2C_BUSNUM5007  0
/* ****************************PMU BUCK/LDO/PS NAME******************************* */
#define BUCK1_NAME "cpu_core"
#define BUCK1_DVS_NAME "buck1_dvs"
#define BUCK2_NAME "cpu_mem12"
#define BUCK3_NAME "cpu_vddio"
#define BUCK4_NAME "emmc_vcc"
#define LDO1_NAME "hr_1v8"
#define LDO2_NAME "lcd_1v8"
#define LDO3_NAME "bt3v0"
#define LDO4_NAME "lcd_2v8"
#define LDO5_NAME "mipi_2v5"
#define LDO6_NAME "dmic"
#define LDO7_NAME "camera_vdd"
#define LDO8_NAME "camera_vddio"
#define LDO9_NAME "hr_3v3"
#define PS1_NAME "sensorhub_core"
#define PS2_NAME "ps2"
#define PS3_NAME "bcm_vddio_1v8"
#define PS4_NAME "motor"
#define PS5_NAME "tp2v8"

/* ****************************PMU BUCK/LDO/PS NAME END*************************** */

/* ****************************PMU BUCK/LDO/PS DEFAULT V************************** */
#define BUCK1_INIT_UV     1100
#define BCUK1_DVS_UV      BUCK1_INIT_UV
#define BUCK2_INIT_UV     1200
#define BUCK3_INIT_UV     1800
#ifdef CONFIG_JZ_EPD_V12
#define BUCK4_INIT_UV     3300
#else
#define BUCK4_INIT_UV     3300
#endif
#define LDO1_INIT_UV    3300
#define LDO2_INIT_UV    1800
#define LDO3_INIT_UV    3000
#define LDO4_INIT_UV    2800
#define LDO5_INIT_UV    2500
#define LDO6_INIT_UV    1800
#define LDO7_INIT_UV    2800
#define LDO8_INIT_UV    1800
#define LDO9_INIT_UV    1800
#define PS1_INIT_UV     BUCK1_INIT_UV
#define PS2_INIT_UV     BUCK2_INIT_UV
#define PS3_INIT_UV     BUCK3_INIT_UV
#define PS4_INIT_UV     LDO3_INIT_UV
#define PS5_INIT_UV     LDO4_INIT_UV
/* ****************************PMU BUCK/LDO/PS DEFAULT V END********************** */

/* ****************************PMU BUCK/LDO/PS ALWAYS ON************************** */
#define BUCK1_ALWAYS_ON     1
#define BUCK2_ALWAYS_ON     1
#define BUCK3_ALWAYS_ON     1
#define BUCK4_ALWAYS_ON     1
#define LDO1_ALWAYS_ON    0
#define LDO2_ALWAYS_ON    0
#define LDO3_ALWAYS_ON    0
#define LDO4_ALWAYS_ON    0
#define LDO5_ALWAYS_ON    1
#define LDO6_ALWAYS_ON    0
#define LDO7_ALWAYS_ON    0
#define LDO8_ALWAYS_ON    0
#define LDO9_ALWAYS_ON    0
#define PS1_ALWAYS_ON     0
#define PS2_ALWAYS_ON     0
#define PS3_ALWAYS_ON     0
#define PS4_ALWAYS_ON     0
#define PS5_ALWAYS_ON     0
/* ****************************PMU BUCK/LDO ALWAYS ON END********************** */

/* ****************************PMU BUCK/LDO BOOT ON**************************** */
#define BUCK1_BOOT_ON     1
#define BUCK2_BOOT_ON     1
#define BUCK3_BOOT_ON     1
#define BUCK4_BOOT_ON     1
#define LDO1_BOOT_ON    0
#define LDO2_BOOT_ON    0
#define LDO3_BOOT_ON    0
#define LDO4_BOOT_ON    0
#define LDO5_BOOT_ON    1
#define LDO6_BOOT_ON    0
#define LDO7_BOOT_ON    0
#define LDO8_BOOT_ON    0
#define LDO9_BOOT_ON    0
#define PS1_BOOT_ON     0
#define PS2_BOOT_ON     0
#define PS3_BOOT_ON     0
#define PS4_BOOT_ON     0
#define PS5_BOOT_ON     0
/* ****************************PMU BUCK/LDO BOOT ON END************************ */

/* ****************************PMU LDO LPM_ATTACH_TO_STM AND BUCKS AUTO_MODE************************ */
#define BUCK1_LPM	LPM_BUCK_AUTO
#define BUCK2_LPM	LPM_BUCK_AUTO
#define BUCK3_LPM	LPM_BUCK_AUTO
#define BUCK4_LPM	LPM_BUCK_AUTO
#define LDO1_LPM	LPM_LDO_ATTACH_TO_STM
#define LDO2_LPM	LPM_LDO_ATTACH_TO_STM
#define LDO3_LPM	LPM_LDO_ATTACH_TO_STM
#define LDO4_LPM	LPM_LDO_ATTACH_TO_STM
#define LDO5_LPM	LPM_LDO_ATTACH_TO_STM
#define LDO6_LPM	LPM_LDO_ATTACH_TO_STM
#define LDO7_LPM	LPM_LDO_ATTACH_TO_STM
#define LDO8_LPM	LPM_LDO_ATTACH_TO_STM
#define LDO9_LPM	LPM_LDO_ATTACH_TO_STM
#define PS1_LPM		LPM_IGNORE //fixed PSx do not have the LPM property
#define PS2_LPM		LPM_IGNORE
#define PS3_LPM		LPM_IGNORE
#define PS4_LPM		LPM_IGNORE
#define PS5_LPM		LPM_IGNORE
/* ****************************PMU LDO LPM_ATTACH_TO_STM AND BUCKS AUTO_MODE END************************ */

/* ****************************PMU BUCK/LDO INIT ENABLE************************ */
#define BUCK1_INIT_ENABLE	BUCK1_BOOT_ON
#define BUCK2_INIT_ENABLE	BUCK2_BOOT_ON
#define BUCK3_INIT_ENABLE	BUCK3_BOOT_ON
#define BUCK4_INIT_ENABLE	BUCK4_BOOT_ON
#define LDO1_INIT_ENABLE	LDO1_BOOT_ON
#define LDO2_INIT_ENABLE	LDO2_BOOT_ON
#define LDO3_INIT_ENABLE	LDO3_BOOT_ON
#define LDO4_INIT_ENABLE	LDO4_BOOT_ON
#define LDO5_INIT_ENABLE	LDO5_BOOT_ON
#define LDO6_INIT_ENABLE	LDO6_BOOT_ON
#define LDO7_INIT_ENABLE	LDO7_BOOT_ON
#define LDO8_INIT_ENABLE	LDO8_BOOT_ON
#define LDO9_INIT_ENABLE	LDO9_BOOT_ON
#define PS1_INIT_ENABLE		PS1_BOOT_ON
#define PS2_INIT_ENABLE		PS2_BOOT_ON
#define PS3_INIT_ENABLE		PS3_BOOT_ON
#define PS4_INIT_ENABLE		PS4_BOOT_ON
#define PS5_INIT_ENABLE		PS5_BOOT_ON
/* ****************************PMU BUCK/LDO INIT ENABLE END******************** */
//#endif	/* CONFIG_REGULATOR_SM */
#endif
