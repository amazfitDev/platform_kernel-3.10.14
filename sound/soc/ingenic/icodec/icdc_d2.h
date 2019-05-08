/*
 * sound/soc/ingenic/icodec/icdc_d2.h
 * ALSA SoC Audio driver -- ingenic internal codec (icdc_d2) driver

 * Copyright 2015 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 * Note: icdc_d2 is an internal codec for jz SOC
 *	 used for jz4775 m150 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __ICDC_D2_REG_H__
#define __ICDC_D2_REG_H__

#include <linux/spinlock.h>
#include <sound/soc.h>
#include "../asoc-v12/asoc-aic-v12.h"

struct icdc_d2 {
	struct device		*dev;		/*aic device used to access register*/
	struct snd_soc_codec	*codec;
	spinlock_t       io_lock;		/*codec hw io lock,
							  Note codec cannot opt in irq context*/
	void * __iomem mapped_base;				/*vir addr*/
	resource_size_t mapped_resstart;				/*resource phy addr*/
	resource_size_t mapped_ressize;				/*resource phy addr size*/

	int dac_user_mute;				/*dac user mute state*/
	/*aohp power on anti pop event*/
	volatile int aohp_in_pwsq;				/*aohp in power up/down seq*/
	int hpl_wished_gain;				/*keep original hpl/r gain register value*/
	int hpr_wished_gain;
	int linl_wished_gain;				/*keep original hpl/r gain register value*/
	int linr_wished_gain;
	/*codec irq*/
	int irqno;
	int irqflags;
	int codec_imr;
	struct work_struct irq_work;
	/*headphone detect*/
	struct snd_soc_jack *jack;
	int report_mask;
};

/*
 * Note: icdc_d2 codec just only support detect headphone jack
 * detected_type: detect event treat as detected_type
 *	 example: SND_JACK_HEADSET detect event treat as SND_JACK_HEADSET
 */
int icdc_d2_hp_detect(struct snd_soc_codec *codec,
		struct snd_soc_jack *jack, int detected_type);

/*  icdc_d2 internal register space */
enum {
	DLV_REG_SR		= 0x0,
	DLV_REG_AICR_DAC	= 0x1,
	DLV_REG_AICR_ADC	= 0x2,
	DLV_REG_CR_LO	= 0x3,
	DLV_REG_CR_HP	= 0x4,
	DLV_REG_MISSING_REG1	=0x5,
	DLV_REG_CR_DAC	= 0x6,
	DLV_REG_CR_MIC	= 0x7,
	DLV_REG_CR_LI	= 0x8,
	DLV_REG_CR_ADC	= 0x9,
	DLV_REG_CR_MIX	= 0xA,
	DLV_REG_CR_VIC	= 0xB,
	DLV_REG_CCR		= 0xC,
	DLV_REG_FCR_DAC	= 0xD,
	DLV_REG_FCR_ADC	= 0xE,
	DLV_REG_ICR		= 0xF,
	DLV_REG_IMR		= 0x10,
	DLV_REG_IFR		= 0x11,
	DLV_REG_GCR_HPL	= 0x12,
	DLV_REG_GCR_HPR	= 0x13,
	DLV_REG_GCR_LIBYL	= 0x14,
	DLV_REG_GCR_LIBYR	= 0x15,
	DLV_REG_GCR_DACL	= 0x16,
	DLV_REG_GCR_DACR	= 0x17,
	DLV_REG_GCR_MIC1	= 0x18,
	DLV_REG_GCR_MIC2	= 0x19,
	DLV_REG_GCR_ADCL	= 0x1A,
	DLV_REG_GCR_ADCR	= 0x1B,
	DLV_REG_MISSING_REG2	= 0x1C,
	DLV_REG_GCR_MIXADC	= 0x1D,
	DLV_REG_GCR_MIXDAC	= 0x1E,
	DLV_REG_AGC1	= 0x1F,
	DLV_REG_AGC2	= 0x20,
	DLV_REG_AGC3	= 0x21,
	DLV_REG_AGC4	= 0x22,
	DLV_REG_AGC5	= 0x23,
	DLV_MAX_REG_NUM,
};

/*For Codec*/
#define RGADW		(0x4)
#define RGDATA		(0x8)

static inline void icdc_d2_mapped_reg_set(void __iomem * xreg, int xmask, int xval)
{
	int val = readl(xreg);
	val &= ~(xmask);
	val |= xval;
	writel(val, xreg);
}

static inline int icdc_d2_mapped_test_bits(void __iomem * xreg, int xmask, int xval)
{
	int val = readl(xreg);
	val &= xmask;
	return (val == xval);
}

/*
 * RGADW
 */
#define DLV_RGDIN_BIT		(0)
#define DLV_RGDIN_MASK		(0xff << DLV_RGDIN_BIT)
#define DLV_RGADDR_BIT		(8)
#define DLV_RGADDR_MASK		(0x7f << DLV_RGADDR_BIT)
#define DLV_RGWR_BIT		(16)
#define DLV_RGWR_MASK		(0x1  << DLV_RGWR_BIT)

#define icdc_d2_test_rw_inval(icdc_d2)      \
	icdc_d2_mapped_test_bits((icdc_d2->mapped_base + RGADW), DLV_RGWR_MASK, (1 << DLV_RGWR_BIT))
/*
 * RGDATA
 */
#define DLV_RGDOUT_BIT		(0)
#define DLV_RGDOUT_MASK		(0xff << DLV_RGDOUT_BIT)
#define DLV_IRQ_BIT		(8)
#define DLV_IRQ_MASK		(0x1  << DLV_IRQ_BIT)

#define icdc_d2_test_irq(icdc_d2)	\
	icdc_d2_mapped_test_bits((icdc_d2->mapped_base + RGDATA),	\
			DLV_IRQ_MASK, (1 << DLV_IRQ_BIT))

static inline u8 icdc_d2_hw_read(struct icdc_d2 *icdc_d2, int reg)
{
	void __iomem * mapped_base = icdc_d2->mapped_base;
	int reval;
	int timeout = 0xfffff;
	unsigned long flags;

	spin_lock_irqsave(&icdc_d2->io_lock, flags);

	while(icdc_d2_test_rw_inval(icdc_d2)) {
		timeout--;
		if (!timeout) pr_err("icdc_d2 test_rw_inval timeout\n");
	}

	icdc_d2_mapped_reg_set((mapped_base + RGADW), DLV_RGWR_MASK,(0 << DLV_RGWR_BIT));

	icdc_d2_mapped_reg_set((mapped_base + RGADW), DLV_RGADDR_MASK,(reg << DLV_RGADDR_BIT));

	reval = readl((mapped_base + RGDATA));
	reval = readl((mapped_base + RGDATA));
	reval = readl((mapped_base + RGDATA));
	reval = readl((mapped_base + RGDATA));
	reval = readl((mapped_base + RGDATA));
	//printk("reg %x reval %x\n", reg, reval);
	reval = ((reval & DLV_RGDOUT_MASK) >> DLV_RGDOUT_BIT);
	//printk("reg %x reval %x\n", reg, reval);
	spin_unlock_irqrestore(&icdc_d2->io_lock, flags);
	return (u8) reval;
}

static inline int icdc_d2_hw_write(struct icdc_d2 *icdc_d2, int reg, int data)
{
	void __iomem * mapped_base = icdc_d2->mapped_base;
	int ret = 0;
	int timeout = 0xfffff;
	unsigned long flags;

	spin_lock_irqsave(&icdc_d2->io_lock, flags);

	while(icdc_d2_test_rw_inval(icdc_d2)) {
		timeout--;
		if (!timeout) pr_err("icdc_d2 test_rw_inval timeout\n");
	}
	icdc_d2_mapped_reg_set((mapped_base + RGADW),DLV_RGDIN_MASK|DLV_RGADDR_MASK,(data << DLV_RGDIN_BIT)|(reg << DLV_RGADDR_BIT));
	icdc_d2_mapped_reg_set((mapped_base + RGADW), DLV_RGWR_MASK , 1 << DLV_RGWR_BIT);
	spin_unlock_irqrestore(&icdc_d2->io_lock, flags);
	if (data != icdc_d2_hw_read(icdc_d2, reg))
		ret = -1;
	return ret;
}

/*SR*/
#define DLV_SR_JACK_SHIFT	5
#define DLV_SR_JACK_MASK	(0x1 << DLV_SR_JACK_SHIFT)

/*DAC AICR*/
#define DLV_AICR_DAC_ADWL_SHIFT 6
#define DLV_AICR_DAC_ADWL_MASK (0x3 << DLV_AICR_DAC_ADWL_SHIFT)
#define DLV_AICR_DAC_ADWL_16BIT 0
#define DLV_AICR_DAC_ADWL_18BIT 1
#define DLV_AICR_DAC_ADWL_20BIT 2
#define DLV_AICR_DAC_ADWL_24BIT 3
#define DLV_AICR_DAC_SERIAL_SHIFT 1
#define DLV_AICR_DAC_SERIAL_MASK (0x1 << DLV_AICR_DAC_SERIAL_SHIFT)
#define DLV_AICR_DAC_SERIAL_SERIAL 1
#define DLV_AICR_DAC_SERIAL_PARALLEL 0
#define DLV_AICR_DAC_I2S_SHIFT	0
#define DLV_AICR_DAC_I2S_MASK (0x1 << DLV_AICR_DAC_I2S_SHIFT)
#define DLV_AICR_DAC_I2S_MODE 1
#define DLV_AICR_DAC_DSP_MODE 0
/*ADC AICR*/
#define DLV_AICR_ADC_ADWL_SHIFT	6
#define DLV_AICR_ADC_ADWL_MASK	(0x3 << DLV_AICR_ADWL_SHIFT)
#define DLV_AICR_ADC_ADWL_16BIT 0
#define DLV_AICR_ADC_ADWL_18BIT 1
#define DLV_AICR_ADC_ADWL_20BIT 2
#define DLV_AICR_ADC_ADWL_24BIT 3
#define DLV_AICR_ADC_SERIAL_SHIFT	1
#define DLV_AICR_ADC_SERIAL_MASK	(0x1 << DLV_AICR_ADC_SERIAL_SHIFT)
#define DLV_AICR_ADC_SERIAL_SERIAL	1
#define DLV_AICR_ADC_SERIAL_PARALLEL	0
#define DLV_AICR_ADC_I2S_SHIFT	0
#define DLV_AICR_ADC_I2S_MASK	(0x1 << DLV_AICR_ADC_I2S_SHIFT)
#define DLV_AICR_ADC_I2S_MODE 1
#define DLV_AICR_ADC_DSP_MODE 0

/*CR LO*/
#define DLV_CR_LO_MUTE_SHIFT	7
#define DLV_CR_LO_MUTE_MASK	(0x1 << DLV_CR_LO_MUTE_SHIFT)
#define DLV_CR_LO_SB_SHIFT	4
#define DLV_CR_LO_SB_MASK	(0x1 << DLV_CR_LO_SB_SHIFT)
#define DLV_CR_LO_SEL_SHIFT	0
#define DLV_CR_LO_SEL_MASK	(0x3 << DLV_CR_LO_SEL_SHIFT)
#define DLV_CR_LO_MIC1		0	/*mono mic1 / stereo mic1/2*/
#define DLV_CR_LO_MIC2		1	/*mono mic2 / stereo mic1/2 */
#define DLV_CR_LO_BYPASS	2	/*AIL/Ratt*/
#define DLV_CR_LO_DAC		3	/*DACR/L*/

/*CR HP*/
#define DLV_CR_HP_MUTE_SHIFT	7
#define DLV_CR_HP_MUTE_MASK	(0x1 << DLV_CR_HP_MUTE_SHIFT)
#define DLV_CR_HP_LOAD_SHIFT	6
#define DLV_CR_HP_LOAD_MASK	(0x1 << DLV_CR_HP_LOAD_SHIFT)
#define DLV_CR_HP_SB_SHIFT	4
#define DLV_CR_HP_SB_MASK	(0x1 << DLV_CR_HP_SB_SHIFT)
#define DLV_CR_HP_SB_HPCM_SHIFT 3
#define DLV_CR_HP_SB_HPCM_MASK	(0x1 << DLV_CR_HP_SB_HPCM_SHIFT)
#define DLV_CR_HP_MUX_SHIFT	0
#define DLV_CR_HP_MUX_MASK	(0x7 << DLV_CR_HP_MUX_SHIFT)
#define DLV_CR_HP_MIC1		0
#define DLV_CR_HP_MIC2		1
#define DLV_CR_HP_BYPASS	2
#define DLV_CR_HP_DAC		3

/*CR DAC*/
#define DLV_CR_DAC_MUTE_SHIFT	7
#define DLV_CR_DAC_MUTE_MASK	(0x1 << DLV_CR_DAC_MUTE_SHIFT)
#define DLV_CR_DAC_MONO_SHIFT	6
#define DLV_CR_DAC_MONO_MASK	(0x1 << DLV_CR_DAC_MONO_SHIFT)
#define DLV_CR_DAC_LEFT_ONLY_SHIFT 5
#define DLV_CR_DAC_LEFT_ONLY_MASK  (0x1 << DLV_CR_DAC_LEFT_ONLY_SHIFT)
#define DLV_CR_DAC_SB_SHIFT	4
#define DLV_CR_DAC_SB_MASK	(0x1 << DLV_CR_DAC_SB_SHIFT)
#define DLV_CR_DAC_LRSWAP_SHIFT	3
#define DLV_CR_DAC_LRSWAP_MASK	(0x1 << DLV_CR_DAC_LRSWAP_SHIFT)

/*CR MIC*/
#define DLV_CR_MIC_STEREO_SHIFT		7
#define DLV_CR_MIC_STEREO_MASK		(0x1 << DLV_CR_MIC_STEREO_SHIFT)
#define DLV_CR_MIC_DIFF_SHIFT		6
#define DLV_CR_MIC_DIFF_MASK		(0x1 << DLV_CR_MIC_DIFF_SHIFT)
#define DLV_CR_MIC_SB1_SHIFT		5
#define DLV_CR_MIC_SB1_MASK		(0x1 << DLV_CR_MIC_SB1_SHIFT)
#define DLV_CR_MIC_SB2_SHIFT		4
#define DLV_CR_MIC_SB2_MASK		(0x1 << DLV_CR_MIC_SB2_SHIFT)
#define DLV_CR_MIC_MICBIAS_V0_SHIFT	1
#define DLV_CR_MIC_MICBIAS_V0_MASK	(0x1 << DLV_CR_MIC_MICBIAS_V0_SHIFT)
#define DLV_CR_MIC_SB_MICBIAS_SHIFT	0
#define DLV_CR_MIC_SB_MICBIAS_MASK	(0x1 << DLV_CR_MIC_SB_MICBIAS_SHIFT)

/*CR LI*/
#define DLV_CR_LI_SB_LIBY_SHIFT	4
#define DLV_CR_LI_SB_LIBY_MASK	(0x1 << DLV_CR_LI_SB_LIBY_SHIFT)
#define DLV_CR_LI_SB_SHIFT	0
#define DLV_CR_LI_SB_MASK	(0x1 << DLV_CR_LI_SB_SHIFT)

/*CR ADC*/
#define DLV_CR_DMIC_SEL_SHIFT	7
#define DLV_CR_DMIC_SEL_MASK	(0x1 << DLV_CR_DMIC_SEL_SHIFT)
#define DLV_CR_ADC_MONO_SHIFT	6
#define DLV_CR_ADC_MONO_MASK	(0x1 << DLV_CR_ADC_MONO_SHIFT)
#define DLV_CR_ADC_LEFT_ONLY_SHIFT	5
#define DLV_CR_ADC_LEFT_ONLY_MASK	(0x1 << DLV_CR_ADC_LEFT_ONLY_SHIFT)
#define DLV_CR_ADC_SB_SHIFT		3
#define DLV_CR_ADC_SB_MASK		(0x1 << DLV_CR_ADC_SB_SHIFT)
#define DLV_CR_ADC_LRSWAP_SHIFT		2
#define DLV_CR_ADC_LRSWAP_MASK		(0x1 << DLV_CR_ADC_LRSWAP_SHIFT)
#define DLV_CR_ADC_INSEL_SHIFT		0
#define DLV_CR_ADC_INSEL_MASK		(0x3 << DLV_CR_ADC_INSEL_SHIFT)

/*CR VIC*/
#define DLV_CR_VIC_SB_SLEEP_SHIFT	1
#define DLV_CR_VIC_SB_SLEEP_MASK	(0x1 << DLV_CR_VIC_SB_SLEEP_SHIFT)
#define DLV_CR_VIC_SB_SHIFT		0
#define DLV_CR_VIC_SB_MASK		(0x1 << DLV_CR_VIC_SB_SHIFT)

/*CCR*/
#define DLV_CCR_DMIC_CLKON_SHIFT	7
#define DLV_CCR_DMIC_CLKON_MASK		(0x1 << DLV_CCR_DMIC_CLKON_SHIFT)
#define DLV_CCR_DMIC_CLK_ON			1
#define DLV_CCR_DMIC_CLK_OFF		0
#define DLV_CCR_CRYSTAL_SHIFT		0
#define DLV_CCR_CRYSTAL_MASK		(0x1 << DLV_CCR_CRYSTAL_SHIFT)
#define DLV_CCR_CRYSTAL_12M			0

/*FCR AD/DAC*/
#define DLV_FCR_FREQ_SHIFT	0
#define DLV_FCR_FREQ_MASK	(0xf << DLV_FCR_FREQ_SHIFT)

/*ICR*/
#define DLV_ICR_INT_FORM_SHIFT	6
#define DLV_ICR_INT_FORM_MASK	(0x3 << DLV_ICR_INT_FORM_SHIFT)
#define DLV_ICR_INT_FORM_HIGH_LEVEL 0

/*IFR*/
#define DLV_IFR_SCLR_SHIFT	6
#define DLV_IFR_SCLR_MASK	(1 << DLV_IFR_SCLR_SHIFT)
#define DLV_IFR_JACK_SHIFT      5
#define DLV_IFR_JACK_MASK       (1 << DLV_IFR_JACK_SHIFT)
#define DLV_IFR_SCMC_SHIFT      4
#define DLV_IFR_SCMC_MASK       (1 << DLV_IFR_SCMC_SHIFT)
#define DLV_IFR_RUP_SHIFT	3
#define DLV_IFR_RUP_MASK	(1 << DLV_IFR_RUP_SHIFT)
#define DLV_IFR_RDO_SHIFT  2
#define DLV_IFR_RDO_MASK	(1 << DLV_IFR_RDO_SHIFT)
#define DLV_IFR_GUP_SHIFT  1
#define DLV_IFR_GUP_MASK   (1 << DLV_IFR_GUP_SHIFT)
#define DLV_IFR_GDO_SHIFT  0
#define DLV_IFR_GDO_MASK   (1 << DLV_IFR_GDO_SHIFT)

/*IMR*/
#define DLV_IMR_SCLR            (1 << 6)
#define DLV_IMR_JACK            (1 << 5)
#define DLV_IMR_SCMC            (1 << 4)
#define DLV_IMR_RUP        (1 << 3)
#define DLV_IMR_RDO        (1 << 2)
#define DLV_IMR_GUP        (1 << 1)
#define DLV_IMR_GDO        (1 << 0)
#define ICR_IMR_MASK		0xef
#define ICR_IMR_COMMON_MSK	(ICR_IMR_MASK & (~(DLV_IMR_SCLR)))
#endif	/* __ICDC_D2_REG_H__ */
