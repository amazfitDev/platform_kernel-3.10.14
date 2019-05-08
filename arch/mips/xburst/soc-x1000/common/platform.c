/*
 * Platform device support for Jz4780 SoC.
 *
 * Copyright 2007, <zpzhong@ingenic.cn>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/resource.h>
#include <linux/i2c-gpio.h>
#include <linux/spi/spi_gpio.h>

#include <gpio.h>

#include <soc/gpio.h>
#include <soc/base.h>
#include <soc/irq.h>

#include <mach/platform.h>
#include <mach/jzdma.h>
#include <mach/jzsnd.h>
#include <mach/jzssi.h>

/* device IO define array */
struct jz_gpio_func_def platform_devio_array[] = {
#ifdef CONFIG_JZMMC_V12_MMC0_PA_4BIT
	MSC0_PORTA_4BIT,
#endif
#ifdef CONFIG_JZMMC_V12_MMC0_PA_8BIT
	MSC0_PORTA_8BIT,
#endif
#ifdef CONFIG_JZMMC_V12_MMC1_PC_4BIT
	MSC1_PORTC,
#endif
#ifdef CONFIG_I2C0_V12_JZ
	I2C0_PORTB,
#endif
#ifdef CONFIG_I2C1_V12_PA
	I2C1_PORTA,
#endif
#ifdef CONFIG_I2C1_V12_PC
	I2C1_PORTC,
#endif
#ifdef CONFIG_I2C2_V12_JZ
	I2C2_PORTD,
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART0
	UART0_PORTC,
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART1_PA
	UART1_PORTA,
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART1_PD
	UART1_PORTD,
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART2_PA
	UART2_PORTA,
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART2_PD
	UART2_PORTD,
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART2_PC
	UART2_PORTC,
#endif

#if (defined(JZ_EXTERNAL_CODEC_V12) || (defined(CONFIG_SND_SOC_EXTERN_CODEC)))
	I2S_PORTB,
#endif

#if (defined(CONFIG_SOUND_JZ_PCM_V12) || defined(CONFIG_SND_ASOC_JZ_PCM_V13))
	PCM_PORTC,
#endif

#if  (defined(CONFIG_SOUND_JZ_SPDIF_V12)||defined(CONFIG_SND_ASOC_JZ_AIC_SPDIF_V13))
	I2S_PORTB,
#endif

#if (defined(CONFIG_JZ_DMIC0) || defined(CONFIG_SND_ASOC_JZ_DMIC_V13))
    DMIC0_PORTB,
#endif
#if (defined(CONFIG_JZ_DMIC1) || defined(CONFIG_SND_ASOC_JZ_DMIC_V13))
    DMIC1_PORTB,
#endif

#ifdef CONFIG_LCD_V13_SLCD_8BIT
	SLCDC_PORTAB_8BIT,
#endif
#ifdef CONFIG_LCD_V13_SLCD_9BIT
	SLCDC_PORTAB_9BIT,
#endif
#ifdef CONFIG_LCD_V13_SLCD_16BIT
	SLCDC_PORTAB_16BIT,
#endif
#ifdef CONFIG_JZ_PWM_BIT0
	PWM_PORTC_BIT0,
#endif
#ifdef CONFIG_JZ_PWM_BIT1
	PWM_PORTC_BIT1,
#endif
#ifdef CONFIG_JZ_PWM_BIT2
	PWM_PORTC_BIT2,
#endif
#ifdef CONFIG_JZ_PWM_BIT3
	PWM_PORTB_BIT3,
#endif
#ifdef CONFIG_JZ_PWM_BIT4
	PWM_PORTC_BIT4,
#endif

#ifdef CONFIG_JZ_MAC
	RMII_PORTB,
#endif

#ifdef CONFIG_USB_DWC2_DRVVBUS_PIN
	OTG_DRVVUS,
#endif

#ifdef CONFIG_VIDEO_JZ_CIM_HOST_V13
	CIM_PORTA,
#endif

#ifdef CONFIG_JZ_SFC
	SFC_PORTA,
#endif

#ifdef CONFIG_JZ_SPI0_PA_22BIT
	SSI0_PORTA_22BIT,
#endif
#ifdef CONFIG_JZ_SPI0_PA_26BIT
	SSI0_PORTA_26BIT,
#endif
#ifdef CONFIG_JZ_SPI0_PD
	SSI0_PORTD,
#endif
};

int platform_devio_array_size = ARRAY_SIZE(platform_devio_array);

int jz_device_register(struct platform_device *pdev,void *pdata)
{
	pdev->dev.platform_data = pdata;

	return platform_device_register(pdev);
}
#ifdef CONFIG_XBURST_DMAC
static struct resource jz_pdma_res[] = {
	[0] = {
		.flags = IORESOURCE_MEM,
		.start = PDMA_IOBASE,
		.end = PDMA_IOBASE + 0x10000 - 1,
	},
	[1] = {
		.name  = "irq",
		.flags = IORESOURCE_IRQ,
		.start = IRQ_PDMA,
	},
	[2] = {
		.name  = "pdmam",
		.flags = IORESOURCE_IRQ,
		.start = IRQ_PDMAM,
	},
	[3] = {
		.name  = "mcu",
		.flags = IORESOURCE_IRQ,
		.start = IRQ_MCU,
	},
	[4] = {
		.name  = "irq_desc",
		.flags = IORESOURCE_IRQ,
		.start = IRQ_PDMAD,
	},
};

static struct jzdma_platform_data jzdma_pdata = {
	.irq_base = IRQ_MCU_BASE,
	.irq_end = IRQ_MCU_END,
	.map = {
                JZDMA_REQ_AUTO_TXRX,
                JZDMA_REQ_AUTO_TXRX,
		JZDMA_REQ_I2S1,
		JZDMA_REQ_I2S0,
		JZDMA_REQ_I2S0,
/* #ifdef CONFIG_SERIAL_JZ47XX_UART2_DMA */
/* 		JZDMA_REQ_UART2, */
/* 		JZDMA_REQ_UART2, */
/* #endif */
/* #ifdef CONFIG_SERIAL_JZ47XX_UART1_DMA */
/* 		JZDMA_REQ_UART1, */
/* 		JZDMA_REQ_UART1, */
/* #endif */
/* #ifdef CONFIG_SERIAL_JZ47XX_UART0_DMA */
/* 		JZDMA_REQ_UART0, */
/* 		JZDMA_REQ_UART0, */
/* #endif */
		/* JZDMA_REQ_SSI0, */
		/* JZDMA_REQ_SSI0, */
		/* JZDMA_REQ_SSI1, */
		/* JZDMA_REQ_SSI1, */
		JZDMA_REQ_PCM0,
		JZDMA_REQ_PCM0,
		/* JZDMA_REQ_PCM1, */
		/* JZDMA_REQ_PCM1, */
		/* JZDMA_REQ_I2C0, */
		/* JZDMA_REQ_I2C0, */
		/* JZDMA_REQ_I2C1, */
		/* JZDMA_REQ_I2C1, */
		/* JZDMA_REQ_I2C2, */
		/* JZDMA_REQ_I2C2, */
		JZDMA_REQ_DES,
	},
};

struct platform_device jz_pdma_device = {
	.name = "jz-dma",
	.id = -1,
	.dev = {
		.platform_data = &jzdma_pdata,
	},
	.resource = jz_pdma_res,
	.num_resources = ARRAY_SIZE(jz_pdma_res),
};
#endif

#ifdef CONFIG_JZMMC_V12
static u64 jz_msc_dmamask =  ~(u32)0;
#define DEF_MSC(NO)								\
	static struct resource jz_msc##NO##_resources[] = {			\
		{								\
			.start          = MSC##NO##_IOBASE,			\
			.end            = MSC##NO##_IOBASE + 0x1000 - 1,	\
			.flags          = IORESOURCE_MEM,			\
		},								\
		{								\
			.start          = IRQ_MSC##NO,				\
			.end            = IRQ_MSC##NO,				\
			.flags          = IORESOURCE_IRQ,			\
		},								\
	};									\
struct platform_device jz_msc##NO##_device = {					\
	.name = "jzmmc_v1.2",							\
	.id = NO,								\
	.dev = {								\
		.dma_mask               = &jz_msc_dmamask,			\
		.coherent_dma_mask      = 0xffffffff,				\
	},									\
	.resource       = jz_msc##NO##_resources,				\
	.num_resources  = ARRAY_SIZE(jz_msc##NO##_resources),			\
};
#ifdef CONFIG_JZMMC_V12_MMC0
DEF_MSC(0);
#endif
#ifdef CONFIG_JZMMC_V12_MMC1
DEF_MSC(1);
#endif
#endif
#if defined(CONFIG_I2C_V12_JZ)
static u64 jz_i2c_dmamask =  ~(u32)0;
#define DEF_I2C(NO,SPEED)							\
	static struct resource jz_i2c##NO##_resources[] = {		\
		[0] = {							\
			.start          = I2C##NO##_IOBASE,		\
			.end            = I2C##NO##_IOBASE + 0x1000 - 1, \
			.flags          = IORESOURCE_MEM,		\
		},							\
		[1] = {							\
			.start          = IRQ_I2C##NO,			\
			.end            = IRQ_I2C##NO,			\
			.flags          = IORESOURCE_IRQ,		\
		},							\
		[2] = {							\
			.start          = JZDMA_REQ_I2C##NO,		\
			.flags          = IORESOURCE_DMA,		\
		},							\
		[3] = {							\
			.start          = SPEED,	\
			.flags          = IORESOURCE_BUS,		\
		},							\
	};								\
	struct platform_device jz_i2c##NO##_device = {			\
		.name = "jz-i2c",					\
		.id = NO,						\
		.dev = {						\
			.dma_mask               = &jz_i2c_dmamask,	\
			.coherent_dma_mask      = 0xffffffff,		\
		},							\
		.num_resources  = ARRAY_SIZE(jz_i2c##NO##_resources),	\
		.resource       = jz_i2c##NO##_resources,		\
	};
#ifdef CONFIG_I2C0_V12_JZ
DEF_I2C(0,CONFIG_I2C0_SPEED);
#endif
#ifdef CONFIG_I2C1_V12_JZ
DEF_I2C(1,CONFIG_I2C1_SPEED);
#endif
#ifdef CONFIG_I2C2_V12_JZ
DEF_I2C(2,CONFIG_I2C2_SPEED);
#endif
#endif

/**
 * sound devices, include i2s,pcm, mixer0 - 1(mixer is used for debug) and an internal codec
 * note, the internal codec can only access by i2s0
 **/
#ifdef CONFIG_SOUND_JZ_I2S_V12
static u64 jz_i2s_dmamask =  ~(u32)0;
static struct resource jz_i2s_resources[] = {
	[0] = {
		.start          = AIC0_IOBASE,
		.end            = AIC0_IOBASE + 0x1000 -1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start			= IRQ_AIC0,
		.end			= IRQ_AIC0,
		.flags			= IORESOURCE_IRQ,
	},
};
struct platform_device jz_i2s_device = {
	.name		= DEV_DSP_NAME,
	.id			= minor2index(SND_DEV_DSP0),
	.dev = {
		.dma_mask               = &jz_i2s_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.resource       = jz_i2s_resources,
	.num_resources  = ARRAY_SIZE(jz_i2s_resources),
};
#endif
#ifdef CONFIG_SOUND_JZ_PCM_V12
static u64 jz_pcm_dmamask =  ~(u32)0;
static struct resource jz_pcm_resources[] = {
	[0] = {
		.start          = PCM0_IOBASE,
		.end            = PCM0_IOBASE,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start			= IRQ_PCM0,
		.end			= IRQ_PCM0,
		.flags			= IORESOURCE_IRQ,
	},
};
struct platform_device jz_pcm_device = {
	.name		= DEV_DSP_NAME,
	.id			= minor2index(SND_DEV_DSP1),
	.dev = {
		.dma_mask               = &jz_pcm_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.resource       = jz_pcm_resources,
	.num_resources  = ARRAY_SIZE(jz_pcm_resources),
};
#endif
#ifdef CONFIG_SOUND_JZ_SPDIF_V12
static u64 jz_spdif_dmamask = ~(u32) 0;
static struct resource jz_spdif_resources[] = {
	[0] = {
		.start = SPDIF0_IOBASE,
		.end = SPDIF0_IOBASE + 0x20 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_AIC0,
		.end = IRQ_AIC0,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device jz_spdif_device = {
	.name = DEV_DSP_NAME,
	.id = minor2index(SND_DEV_DSP2),
	.dev = {
		.dma_mask = &jz_spdif_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.resource = jz_spdif_resources,
	.num_resources = ARRAY_SIZE(jz_spdif_resources),
};
#endif
#ifdef CONFIG_SOUND_JZ_DMIC_V12
static u64 jz_dmic_dmamask =  ~(u32)0;
static struct resource jz_dmic_resources[] = {
	[0] = {
		.start          = DMIC_IOBASE,
		.end            = DMIC_IOBASE + 0x70 -1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_DMIC,
		.end            = IRQ_DMIC,
		.flags          = IORESOURCE_IRQ,
	},
};

struct platform_device jz_dmic_device = {
	.name       = DEV_DSP_NAME,
	.id         = minor2index(SND_DEV_DSP3),
	.dev = {
		.dma_mask               = &jz_dmic_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.resource       = jz_dmic_resources,
	.num_resources  = ARRAY_SIZE(jz_dmic_resources),
};
#endif

#ifdef CONFIG_SOUND_OSS_XBURST
#define DEF_MIXER(NO)						\
	struct platform_device jz_mixer##NO##_device = {	\
		.name	= DEV_MIXER_NAME,			\
		.id	= minor2index(SND_DEV_MIXER##NO),	\
	};
DEF_MIXER(0);
DEF_MIXER(1);
DEF_MIXER(2);
DEF_MIXER(3);
#endif

#if defined(CONFIG_SOUND_JZ_PCM_V12) || defined(CONFIG_SOUND_JZ_I2S_V12)
struct platform_device jz_codec_device = {
	.name		= "jz_codec",
};
#endif

/* only for ALSA platform devices */
#if defined(CONFIG_SND) && defined(CONFIG_SND_ASOC_INGENIC)
#if defined(CONFIG_SND_ASOC_JZ_AIC_V12)
static u64 jz_asoc_dmamask =  ~(u64)0;
static struct resource jz_aic_dma_resources[] = {
	[0] = {
		.start          = JZDMA_REQ_I2S0,
		.end		= JZDMA_REQ_I2S0,
		.flags          = IORESOURCE_DMA,
	},
};
struct platform_device jz_aic_dma_device = {
	.name		= "jz-asoc-aic-dma",
	.id		= -1,
	.dev = {
		.dma_mask               = &jz_asoc_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.resource       = jz_aic_dma_resources,
	.num_resources  = ARRAY_SIZE(jz_aic_dma_resources),
};

static struct resource jz_aic_resources[] = {
	[0] = {
		.start          = AIC0_IOBASE,
		.end            = AIC0_IOBASE + 0xA0 -1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_AIC0,
		.end		= IRQ_AIC0,
		.flags		= (IORESOURCE_IRQ| IORESOURCE_IRQ_SHAREABLE),
	},
};
struct platform_device jz_aic_device = {
	.name		= "jz-asoc-aic",
	.id		= -1,
	.resource       = jz_aic_resources,
	.num_resources  = ARRAY_SIZE(jz_aic_resources),
};
#endif

#if defined(CONFIG_SND_ASOC_JZ_ICDC_D3)

static struct resource jz_icdc_resources[] = {
	[0] = {
		.start          = AIC0_IOBASE + 0xA0,
		.end            = AIC0_IOBASE + 0xAA -1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start      = IRQ_AIC0,
		.end        = IRQ_AIC0,
		.flags      = (IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE),
	},
};

struct platform_device jz_icdc_device = {   /*jz internal codec*/
	.name       = "icdc-d3",
	.id     = -1,
	.resource   = jz_icdc_resources,
	.num_resources  = ARRAY_SIZE(jz_icdc_resources),
};
#endif

#if defined(CONFIG_SND_ASOC_JZ_DMIC_V13)
static struct resource jz_dmic_dma_resources[] = {
	[0] = {
		.start          = JZDMA_REQ_I2S1,
		.end		= JZDMA_REQ_I2S1,
		.flags          = IORESOURCE_DMA,
	},
};

struct platform_device jz_dmic_dma_device = {
	.name		= "jz-asoc-dmic-dma",
	.id		= -1,
	.dev = {
		.dma_mask               = &jz_asoc_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.resource       = jz_dmic_dma_resources,
	.num_resources  = ARRAY_SIZE(jz_dmic_dma_resources),
};

static struct resource jz_dmic_resources[] = {
	[0] = {
		.start          = DMIC_IOBASE,
		.end            = DMIC_IOBASE + 0x70 -1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_DMIC,
		.end            = IRQ_DMIC,
		.flags          = IORESOURCE_IRQ,
	},
};
struct platform_device jz_dmic_device = {
	.name           = "jz-asoc-dmic",
	.id             = -1,
	.resource       = jz_dmic_resources,
	.num_resources  = ARRAY_SIZE(jz_dmic_resources),
};
#endif

#if defined(CONFIG_SND_ASOC_JZ_PCM_V13)
static struct resource jz_pcm_dma_resources[] = {
	[0] = {
		.start          = JZDMA_REQ_PCM0,
		.end		= JZDMA_REQ_PCM0,
		.flags          = IORESOURCE_DMA,
	},
};
struct platform_device jz_pcm_dma_device = {
	.name		= "jz-asoc-pcm-dma",
	.id		= -1,
	.dev = {
		.dma_mask               = &jz_asoc_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.resource       = jz_pcm_dma_resources,
	.num_resources  = ARRAY_SIZE(jz_pcm_dma_resources),
};

static struct resource jz_pcm_resources[] = {
	[0] = {
		.start          = PCM0_IOBASE,
		.end            = PCM0_IOBASE + 0x18 -1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_PCM0,
		.end            = IRQ_PCM0,
		.flags          = IORESOURCE_IRQ,
	},
};
struct platform_device jz_pcm_device = {
	.name           = "jz-asoc-pcm",
	.id             = -1,
	.resource       = jz_pcm_resources,
	.num_resources  = ARRAY_SIZE(jz_pcm_resources),
};
#endif

#if defined(CONFIG_SND_ASOC_JZ_SPDIF_DUMP_CDC)

struct platform_device jz_spdif_dump_cdc_device = {   /*jz dump codec*/
	.name           = "spdif dump",
	.id             = -1,
};
#endif

#if defined(CONFIG_SND_ASOC_JZ_PCM_DUMP_CDC)
struct platform_device jz_pcm_dump_cdc_device = {   /*jz dump codec*/
	.name           = "pcm dump",
	.id             = -1,
};
#endif

#if defined(CONFIG_SND_ASOC_JZ_DMIC_DUMP_CDC)
struct platform_device jz_dmic_dump_cdc_device = {   /*jz dump codec*/
	.name           = "dmic dump",
	.id             = -1,
};
#endif


#endif /* CONFIG_SND && CONFIG_SND_ASOC_INGENIC */
#ifdef CONFIG_FB_JZ_V13
static u64 jz_fb_dmamask = ~(u64) 0;
static struct resource jz_fb_resources[] = {
	[0] = {
		.start = LCDC_IOBASE,
		.end = LCDC_IOBASE + 0x1800 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_LCD,
		.end = IRQ_LCD,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device jz_fb_device = {
	.name = "jz-fb",
	.id = -1,
	.dev = {
		.dma_mask = &jz_fb_dmamask,
		.coherent_dma_mask = 0xffffffff,
	},
	.num_resources = ARRAY_SIZE(jz_fb_resources),
	.resource = jz_fb_resources,
};
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART0
/* UART ( uart controller) */
static struct resource jz_uart0_resources[] = {
	[0] = {
		.start          = UART0_IOBASE,
		.end            = UART0_IOBASE + 0x1000 - 1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_UART0,
		.end            = IRQ_UART0,
		.flags          = IORESOURCE_IRQ,
	},
#ifdef CONFIG_SERIAL_JZ47XX_UART0_DMA
	[2] = {
		.start          = JZDMA_REQ_UART0,
		.flags          = IORESOURCE_DMA,
	},
#endif
};

struct platform_device jz_uart0_device = {
	.name = "jz-uart",
	.id = 0,
	.num_resources  = ARRAY_SIZE(jz_uart0_resources),
	.resource       = jz_uart0_resources,
};
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART1
static struct resource jz_uart1_resources[] = {
	[0] = {
		.start          = UART1_IOBASE,
		.end            = UART1_IOBASE + 0x1000 - 1,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = IRQ_UART1,
		.end            = IRQ_UART1,
		.flags          = IORESOURCE_IRQ,
	},
#ifdef CONFIG_SERIAL_JZ47XX_UART1_DMA
	[2] = {
		.start          = JZDMA_REQ_UART1,
		.flags          = IORESOURCE_DMA,
	},
#endif
};
struct platform_device jz_uart1_device = {
	.name = "jz-uart",
	.id = 1,
	.num_resources  = ARRAY_SIZE(jz_uart1_resources),
	.resource       = jz_uart1_resources,
};
#endif
#ifdef CONFIG_SERIAL_JZ47XX_UART2
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
#ifdef CONFIG_SERIAL_JZ47XX_UART2_DMA
	[2] = {
		.start          = JZDMA_REQ_UART2,
		.flags          = IORESOURCE_DMA,
	},
#endif
};

struct platform_device jz_uart2_device = {
	.name = "jz-uart",
	.id = 2,
	.num_resources  = ARRAY_SIZE(jz_uart2_resources),
	.resource       = jz_uart2_resources,
};
#endif
#ifndef CONFIG_SPI0_PIO_ONLY
static u64 jz_ssi_dmamask =  ~(u32)0;
#endif

#define DEF_SSI(NO)							\
	static struct resource jz_ssi##NO##_resources[] = {		\
		[0] = {							\
			.flags	       = IORESOURCE_MEM,		\
			.start	       = SSI##NO##_IOBASE,		\
			.end	       = SSI##NO##_IOBASE + 0x1000 - 1,	\
		},							\
		[1] = {							\
			.flags	       = IORESOURCE_IRQ,		\
			.start	       = IRQ_SSI##NO,			\
			.end	       = IRQ_SSI##NO,			\
		},							\
		[2] = {							\
			.flags	       = IORESOURCE_DMA,		\
			.start	       = JZDMA_REQ_SSI##NO,		\
		},							\
	};								\
	struct platform_device jz_ssi##NO##_device = {			\
		.name = "jz-ssi",					\
		.id = NO,						\
		.dev = {						\
			.dma_mask	       = &jz_ssi_dmamask,	\
			.coherent_dma_mask      = 0xffffffff,		\
		},							\
		.resource       = jz_ssi##NO##_resources,		\
		.num_resources  = ARRAY_SIZE(jz_ssi##NO##_resources),	\
	};

#define DEF_PIO_SSI(NO)							\
	static struct resource jz_ssi##NO##_resources[] = {		\
		[0] = {							\
			.flags	       = IORESOURCE_MEM,		\
			.start	       = SSI##NO##_IOBASE,		\
			.end	       = SSI##NO##_IOBASE + 0x1000 - 1,	\
		},							\
		[1] = {							\
			.flags	       = IORESOURCE_IRQ,		\
			.start	       = IRQ_SSI##NO,			\
			.end	       = IRQ_SSI##NO,			\
		},							\
	};								\
	struct platform_device jz_ssi##NO##_device = {			\
		.name = "jz-ssi",					\
		.id = NO,						\
		.resource       = jz_ssi##NO##_resources,		\
		.num_resources  = ARRAY_SIZE(jz_ssi##NO##_resources),	\
	};

#ifdef CONFIG_SPI0_PIO_ONLY
DEF_PIO_SSI(0);
#else
DEF_SSI(0);
#endif

#ifdef CONFIG_JZ_SFC
static struct resource jz_sfc_resources[] = {
	[0] = {
		.flags = IORESOURCE_MEM,
		.start = SFC_IOBASE,
		.end = SFC_IOBASE + 0x10000 - 1,
	},
	[1] = {
		.flags = IORESOURCE_IRQ,
		.start = IRQ_SFC,
	},
	[2] = {
		.start = CONFIG_SFC_SPEED,
		.flags = IORESOURCE_BUS,
	}
};

struct platform_device jz_sfc_device = {
	.name = "jz-sfc",
	.id = 0,
	.resource = jz_sfc_resources,
	.num_resources = ARRAY_SIZE(jz_sfc_resources),
};
#endif
#if defined(CONFIG_VIDEO_JZ_CIM_HOST_V13)
static struct resource jz_cim_resources[] = {
	[0] = {
		.flags = IORESOURCE_MEM,
		.start = CIM_IOBASE,
		.end = CIM_IOBASE + 0x10000 - 1,
	},
	[1] = {
		.flags = IORESOURCE_IRQ,
		.start = IRQ_CIM,
	}
};

struct platform_device jz_cim_device = {
	.name = "jz-cim",
	.id = 0,
	.resource = jz_cim_resources,
	.num_resources = ARRAY_SIZE(jz_cim_resources),
};
#endif
#ifdef CONFIG_RTC_DRV_JZ
static struct resource jz_rtc_resource[] = {
	[0] = {
		.start = RTC_IOBASE,
		.end   = RTC_IOBASE + 0xff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_RTC,
		.end   = IRQ_RTC,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device jz_rtc_device = {
	.name             = "jz-rtc",
	.id               = 0,
	.num_resources    = ARRAY_SIZE(jz_rtc_resource),
	.resource         = jz_rtc_resource,
};
#endif
#ifdef CONFIG_JZ_VPU_V13
static struct resource jz_vpu_resource[] = {
	[0] = {
		.start = SCH_IOBASE,
		.end = SCH_IOBASE + 0xF0000 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_VPU,
		.end   = IRQ_VPU,
		.flags = IORESOURCE_IRQ,
	},

};

struct platform_device jz_vpu_device = {
	.name             = "jz-vpu",
	.id               = 0,
	.num_resources    = ARRAY_SIZE(jz_vpu_resource),
	.resource         = jz_vpu_resource,
};
#endif
#ifdef CONFIG_USB_JZ_DWC2
static struct resource jz_dwc_otg_resources[] = {
	[0] = {
		.start	= OTG_IOBASE,
		.end	= OTG_IOBASE + 0x40000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.flags = IORESOURCE_IRQ,
		.start = IRQ_OTG,
		.end   = IRQ_OTG,
	},
};

struct platform_device  jz_dwc_otg_device = {
	.name = "jz-dwc2",
	.id = -1,
	.num_resources	= ARRAY_SIZE(jz_dwc_otg_resources),
	.resource	= jz_dwc_otg_resources,
};
#endif
#ifdef CONFIG_JZ_EFUSE_V13
/* efuse */
struct platform_device jz_efuse_device = {
	.name = "jz-efuse-v13",
};
#endif
#ifdef CONFIG_JZ_SECURITY
struct platform_device jz_security_device = {
	.name = "jz-security",
};
#endif

#ifdef CONFIG_JZ_PWM
struct platform_device jz_pwm_device = {
	.name = "jz-pwm",
	.id   = -1,
};
#endif
