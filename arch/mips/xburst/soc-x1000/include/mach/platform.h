/*
 *  Copyright (C) 2010 Ingenic Semiconductor Inc.
 *
 *   In this file, here are some macro/device/function to
 * to help the board special file to organize resources
 * on the chip.
 */

#ifndef __SOC_x1000_H__
#define __SOC_x1000_H__

/* devio define list */
/****************************************************************************************/
#define I2S_PORTB							\
	{ .name = "i2s", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x1f, }
#define DMIC0_PORTB							\
	{ .name = "dmic0", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x3 << 21, }
#define DMIC1_PORTB							\
	{ .name = "dmic1", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x1 << 5, }
#define PCM_PORTC							\
	{ .name = "pcm", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 0xf << 6 }
/****************************************************************************************/
#define UART0_PORTC							\
	{ .name = "uart0", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 0x0f << 10, }
#define UART1_PORTA							\
	{ .name = "uart1-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_2, .pins = 0x3 << 4, }
#define UART1_PORTD							\
	{ .name = "uart1-pd", .port = GPIO_PORT_D, .func = GPIO_FUNC_1, .pins = 0xf << 2, }
#define UART2_PORTA							\
	{ .name = "uart2-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_2, .pins = 0x3 << 2, }
#define UART2_PORTD							\
	{ .name = "uart2-pd", .port = GPIO_PORT_D, .func = GPIO_FUNC_0, .pins = 0x3 << 4 }
#define UART2_PORTC							\
	{ .name = "uart2-pc", .port = GPIO_PORT_C, .func = GPIO_FUNC_1, .pins = 0x1 << 31 }
/****************************************************************************************/
#define MSC0_PORTA_4BIT							\
	{ .name = "msc0-pa-4bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x3f0 << 16, }
#define MSC0_PORTA_8BIT							\
	{ .name = "msc0-pa-8bit", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x3ff << 16, }
#define MSC1_PORTC							\
	{ .name = "msc1", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 0x3f, }
/****************************************************************************************/
#define SFC_PORTA							\
	{ .name = "sfc", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x3f << 26, }
/****************************************************************************************/
#define SSI0_PORTA_22BIT						\
       { .name = "ssi0-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_2, .pins = 0x7 << 22, }
#define SSI0_PORTA_26BIT						\
       { .name = "ssi0-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_2, .pins = 0xd << 26, }
#define SSI0_PORTD							\
       { .name = "ssi0-pd", .port = GPIO_PORT_D, .func = GPIO_FUNC_0, .pins = 0xd, }
/****************************************************************************************/
#define I2C0_PORTB							\
	{ .name = "i2c0", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 0x3 << 23, }
#define I2C1_PORTA							\
	{ .name = "i2c1-pa", .port = GPIO_PORT_A, .func = GPIO_FUNC_2, .pins = 0x3, }
#define I2C1_PORTC							\
	{ .name = "i2c1-pc", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 0x3 << 26, }
#define I2C2_PORTD							\
	{ .name = "i2c2", .port = GPIO_PORT_D, .func = GPIO_FUNC_1, .pins = 0x3, }
/****************************************************************************************/
#define SLCDC_PORTAB_8BIT						\
	{ .name = "slcd", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0xff, }, \
        { .name = "slcd", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x1a << 16, }

#define SLCDC_PORTAB_9BIT						\
	{ .name = "slcd", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0x1ff, }, \
        { .name = "slcd", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x1a << 16, }

#define SLCDC_PORTAB_16BIT						\
        { .name = "slcd", .port = GPIO_PORT_A, .func = GPIO_FUNC_1, .pins = 0xffff, },\
        { .name = "slcd", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x12 << 16, }

#define PWM_PORTC_BIT0							\
	{ .name = "pwm0", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 1 << 25, }
#define PWM_PORTC_BIT1							\
	{ .name = "pwm1", .port = GPIO_PORT_C, .func = GPIO_FUNC_1, .pins = 1 << 26, }
#define PWM_PORTC_BIT2							\
	{ .name = "pwm2", .port = GPIO_PORT_C, .func = GPIO_FUNC_1, .pins = 1 << 27, }
#define PWM_PORTB_BIT3							\
	{ .name = "pwm3", .port = GPIO_PORT_B, .func = GPIO_FUNC_2, .pins = 1 << 6, }
#define PWM_PORTC_BIT4							\
	{ .name = "pwm4", .port = GPIO_PORT_C, .func = GPIO_FUNC_0, .pins = 1 << 24, }
/****************************************************************************************/
#define RMII_PORTB							\
        { .name = "rmii", .port = GPIO_PORT_B, .func = GPIO_FUNC_1, .pins = 0x3ff << 6, }
/****************************************************************************************/
#define OTG_DRVVUS							\
	{ .name = "otg-drvvbus", .port = GPIO_PORT_B, .func = GPIO_FUNC_0, .pins = 1 << 25, }
/****************************************************************************************/
#define CIM_PORTA							\
	{ .name = "cim",    .port = GPIO_PORT_A,  .func = GPIO_FUNC_2, .pins = 0xfff << 8, }
/****************************************************************************************/
#define SCC_PORTB							\
	{ .name = "scc",    .port = GPIO_PORT_B,  .func = GPIO_FUNC_1, .pins = 0x3 << 23, }
/****************************************************************************************/


/* JZ SoC on Chip devices list */
extern struct platform_device jz_msc0_device;
extern struct platform_device jz_msc1_device;

extern struct platform_device jz_i2c0_device;
extern struct platform_device jz_i2c1_device;
extern struct platform_device jz_i2c2_device;

extern struct platform_device jz_i2c0_dma_device;
extern struct platform_device jz_i2c1_dma_device;
extern struct platform_device jz_i2c2_dma_device;

extern struct platform_device jz_i2s_device;
extern struct platform_device jz_pcm_device;
extern struct platform_device jz_spdif_device;
extern struct platform_device jz_codec_device;
extern struct platform_device jz_dmic_device;

extern struct platform_device jz_mixer0_device;
extern struct platform_device jz_mixer1_device;
extern struct platform_device jz_mixer2_device;
extern struct platform_device jz_mixer3_device;

extern struct platform_device jz_fb_device;

extern struct platform_device jz_uart0_device;
extern struct platform_device jz_uart1_device;
extern struct platform_device jz_uart2_device;

extern struct platform_device jz_ssi0_device;
extern struct platform_device jz_sfc_device;

extern struct platform_device jz_pdma_device;
extern struct platform_device jz_cim_device;

extern struct platform_device jz_rtc_device;
extern struct platform_device jz_dwc_otg_device;
extern struct platform_device jz_vpu_device;
extern struct platform_device jz_efuse_device;
extern struct platform_device jz_security_device;
extern struct platform_device jz_pwm_device;
/*alsa*/

extern struct platform_device jz_aic_dma_device;
extern struct platform_device jz_aic_device;
extern struct platform_device jz_pcm_dma_device;
extern struct platform_device jz_pcm_device;
extern struct platform_device jz_dmic_dma_device;
extern struct platform_device jz_dmic_device;
extern struct platform_device jz_icdc_device;
extern struct platform_device jz_pcm_dump_cdc_device;
extern struct platform_device jz_spdif_dump_cdc_device;
extern struct platform_device jz_dmic_dump_cdc_device;

int jz_device_register(struct platform_device *pdev,void *pdata);

#endif
