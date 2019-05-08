/*
 * sound/soc/ingenic/icodec/icdc_d1.h
 * ALSA SoC Audio driver -- ingenic internal codec (icdc_d1) driver

 * Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 * Note: icdc_d1 is an internal codec for jz SOC
 *	 used for jz4780 m200 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef __ICDC_D1_REG_H__
#define __ICDC_D1_REG_H__

#include <linux/spinlock.h>
#include <sound/soc.h>
#include "../asoc-v12/asoc-aic-v12.h"

struct icdc_d1 {
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
 * Note: icdc_d1 codec just only support detect headphone jack
 * detected_type: detect event treat as detected_type
 *	 example: SND_JACK_HEADSET detect event treat as SND_JACK_HEADSET
 */
int icdc_d1_hp_detect(struct snd_soc_codec *codec,
		struct snd_soc_jack *jack, int detected_type);

/*  icdc_d1 internal register space */
enum {
	DLV_REG_SR		= 0x00,
	DLV_REG_SR2		= 0x01,
	DLV_REG_MISSING_RNG1S	= 0x02,
	DLV_REG_MISSING_RNG1E	= 0x06,
	DLV_REG_MR		= 0x07,
	DLV_REG_AICR_DAC	= 0x08,
	DLV_REG_AICR_ADC	= 0x09,
	DLV_REG_MISSING_REG1,
	DLV_REG_CR_LO		= 0x0b,
	DLV_REG_MISSING_REG2,
	DLV_REG_CR_HP		= 0x0d,
	DLV_REG_MISSING_REG3,
	DLV_REG_MISSING_REG4,
	DLV_REG_CR_DMIC		= 0x10,
	DLV_REG_CR_MIC1		= 0x11,
	DLV_REG_CR_MIC2		= 0x12,
	DLV_REG_CR_LI1		= 0x13,
	DLV_REG_CR_LI2		= 0x14,
	DLV_REG_MISSING_REG5,
	DLV_REG_MISSING_REG6,
	DLV_REG_CR_DAC		= 0x17,
	DLV_REG_CR_ADC		= 0x18,
	DLV_REG_CR_MIX		= 0x19,
	DLV_REG_DR_MIX		= 0x1a,
	DLV_REG_CR_VIC		= 0x1b,
	DLV_REG_CR_CK		= 0x1c,
	DLV_REG_FCR_DAC		= 0x1d,
	DLV_REG_MISSING_REG7,
	DLV_REG_MISSING_REG8,
	DLV_REG_FCR_ADC		= 0x20,
	DLV_REG_CR_TIMER_MSB	= 0x21,
	DLV_REG_CR_TIMER_LSB	= 0x22,
	DLV_REG_ICR		= 0x23,
	DLV_REG_IMR		= 0x24,
	DLV_REG_IFR		= 0x25,
	DLV_REG_IMR2		= 0x26,
	DLV_REG_IFR2		= 0x27,
	DLV_REG_GCR_HPL		= 0x28,
	DLV_REG_GCR_HPR		= 0x29,
	DLV_REG_GCR_LIBYL	= 0x2a,
	DLV_REG_GCR_LIBYR	= 0x2b,
	DLV_REG_GCR_DACL	= 0x2c,
	DLV_REG_GCR_DACR	= 0x2d,
	DLV_REG_GCR_MIC1	= 0x2e,
	DLV_REG_GCR_MIC2	= 0x2f,
	DLV_REG_GCR_ADCL	= 0x30,
	DLV_REG_GCR_ADCR	= 0x31,
	DLV_REG_MISSING_REG9,
	DLV_REG_MISSING_REG10,
	DLV_REG_GCR_MIXDACL	= 0x34,
	DLV_REG_GCR_MIXDACR	= 0x35,
	DLV_REG_GCR_MIXADCL	= 0x36,
	DLV_REG_GCR_MIXADCR	= 0x37,
	DLV_REG_MISSING_REG11,
	DLV_REG_MISSING_REG12,
	DLV_REG_CR_ADC_AGC	= 0x3a,
	DLV_REG_DR_ADC_AGC	= 0x3b,
	/*extensional register*/
	DLV_EXREG_RNG_S		= 0x3c,
	DLV_EXREG_MIX0		= DLV_EXREG_RNG_S,
	DLV_EXREG_MIX1		= 0x3d,
	DLV_EXREG_MIX2		= 0x3e,
	DLV_EXREG_MIX3		= 0x3f,
	DLV_EXREG_AGC0		= 0x40,
	DLV_EXREG_AGC1		= 0x41,
	DLV_EXREG_AGC2		= 0x42,
	DLV_EXREG_AGC3		= 0x43,
	DLV_EXREG_AGC4		= 0x44,
	DLV_EXREG_RNG_E		= DLV_EXREG_AGC4,
	DLV_MAX_REG_NUM,
};

static int inline register_is_extend(int reg)
{
	if (reg < DLV_EXREG_RNG_S || reg > DLV_EXREG_RNG_E)
		return 0;
	return 1;
}

/*For Codec*/
#define RGADW		(0x4)
#define RGDATA		(0x8)

static inline void icdc_d1_mapped_reg_set(void __iomem * xreg, int xmask, int xval)
{
	int val = readl(xreg);
	val &= ~(xmask);
	val |= xval;
	writel(val, xreg);
}

static inline int icdc_d1_mapped_test_bits(void __iomem * xreg, int xmask, int xval)
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
#define DLV_ICRST_BIT		(31)
#define DLV_ICRST_MASK		(0x1 << DLV_ICRST_BIT)

#define icdc_d1_test_rw_inval(icdc_d1)      \
	icdc_d1_mapped_test_bits((icdc_d1->mapped_base + RGADW), DLV_RGWR_MASK, (1 << DLV_RGDIN_BIT))
#define icdc_d1_rst_begin(reg)	\
	icdc_d1_mapped_reg_set((icdc_d1->mapped_base + RGADW), DLV_ICRST_MASK, (1 << DLV_ICRST_BIT))
#define icdc_d1_rst_end(reg)		\
	icdc_d1_mapped_reg_set((icdc_d1->mapped_base + RGADW), DLV_ICRST_MASK, 0)

/*
 * RGDATA
 */
#define DLV_RGDOUT_BIT		(0)
#define DLV_RGDOUT_MASK		(0xff << DLV_RGDOUT_BIT)
#define DLV_IRQ_BIT		(8)
#define DLV_IRQ_MASK		(0x1  << DLV_IRQ_BIT)

#define icdc_d1_test_irq(icdc_d1)	\
	icdc_d1_mapped_test_bits((icdc_d1->mapped_base + RGDATA),	\
			DLV_IRQ_MASK, (1 << DLV_IRQ_BIT))

static inline u8 icdc_d1_hw_read_normal(struct icdc_d1 *icdc_d1, u8 reg)
{
	void __iomem * mapped_base = icdc_d1->mapped_base;
	int reval;
	int timeout = 0xfffff;
	unsigned long flags;

	spin_lock_irqsave(&icdc_d1->io_lock, flags);

	while(icdc_d1_test_rw_inval(icdc_d1)) {
		timeout--;
		if (!timeout) pr_err("icdc_d1 test_rw_inval timeout\n");
	}

	icdc_d1_mapped_reg_set((mapped_base + RGADW), DLV_RGADDR_MASK,
			(reg << DLV_RGADDR_BIT));

	reval = readl((mapped_base + RGDATA));
	reval = readl((mapped_base + RGDATA));
	reval = readl((mapped_base + RGDATA));
	reval = readl((mapped_base + RGDATA));
	reval = readl((mapped_base + RGDATA));
	//printk("reg %x reval %x\n", reg, reval);
	reval = ((reval & DLV_RGDOUT_MASK) >> DLV_RGDOUT_BIT);
	//printk("reg %x reval %x\n", reg, reval);
	spin_unlock_irqrestore(&icdc_d1->io_lock, flags);
	return (u8) reval;
}

static inline int icdc_d1_hw_write_normal(struct icdc_d1 *icdc_d1, u8 reg, u8 data)
{
	void __iomem * mapped_base = icdc_d1->mapped_base;
	int ret = 0;
	int timeout = 0xfffff;
	unsigned long flags;

	spin_lock_irqsave(&icdc_d1->io_lock, flags);

	while(icdc_d1_test_rw_inval(icdc_d1)) {
		timeout--;
		if (!timeout) pr_err("icdc_d1 test_rw_inval timeout\n");
	}

	icdc_d1_mapped_reg_set((mapped_base + RGADW), DLV_RGADDR_MASK | DLV_RGDIN_MASK,
			(1 << DLV_RGWR_BIT) | (reg << DLV_RGADDR_BIT) | (data << DLV_RGDIN_BIT));

	spin_unlock_irqrestore(&icdc_d1->io_lock, flags);

	if (data != icdc_d1_hw_read_normal(icdc_d1, reg))
		ret = -1;
	return ret;
}

static inline int icdc_d1_hw_write_extend(struct icdc_d1 *icdc_d1, u8 sreg, u8 sdata)
{
	u8 creg, cdata, dreg, ddata;
	switch (sreg) {
	case DLV_EXREG_MIX0 ... DLV_EXREG_MIX3:
		creg = DLV_REG_CR_MIX;
		dreg = DLV_REG_DR_MIX;
		cdata = (sreg - DLV_EXREG_MIX0) & 0x3;
		break;
	case DLV_EXREG_AGC0 ... DLV_EXREG_AGC4:
		creg = DLV_REG_CR_ADC_AGC;
		dreg = DLV_REG_DR_ADC_AGC;
		cdata = (sreg - DLV_EXREG_AGC0) & 0x7;
		break;
	default:
		return -1;
	}
	ddata = sdata;

	icdc_d1_hw_write_normal(icdc_d1, dreg, ddata);
	icdc_d1_hw_write_normal(icdc_d1, creg, (cdata | 1 << 6));
	icdc_d1_hw_write_normal(icdc_d1, creg, cdata);
	if (ddata != icdc_d1_hw_read_normal(icdc_d1, dreg)) {
		return -1;
	}
	return 0;
}

static inline u8 icdc_d1_hw_read_extend(struct icdc_d1 *icdc_d1, u8 sreg)
{
	int creg, cdata, dreg, ddata;
	switch (sreg) {
	case DLV_EXREG_MIX0 ... DLV_EXREG_MIX3:
		creg = DLV_REG_CR_MIX;
		dreg = DLV_REG_DR_MIX;
		cdata = (sreg - DLV_EXREG_MIX0) & 0x3;
		break;
	case DLV_EXREG_AGC0 ... DLV_EXREG_AGC4:
		creg = DLV_REG_CR_ADC_AGC;
		dreg = DLV_REG_DR_ADC_AGC;
		cdata = (sreg - DLV_EXREG_AGC0) & 0x4;
		break;
	default:
		return 0;
	}
	icdc_d1_hw_write_normal(icdc_d1, creg, cdata);
	ddata = icdc_d1_hw_read_normal(icdc_d1, dreg);
	return (u8) ddata;
}

static inline u8 icdc_d1_hw_read(struct icdc_d1 *icdc_d1, int reg)
{
	if (register_is_extend(reg))
		return icdc_d1_hw_read_extend(icdc_d1, reg);
	else
		return icdc_d1_hw_read_normal(icdc_d1, reg);
}

static inline int icdc_d1_hw_write(struct icdc_d1 *icdc_d1, int reg, int data)
{
	if (register_is_extend(reg)) {
		return icdc_d1_hw_write_extend(icdc_d1, reg, data);
	} else {
		if (reg == DLV_REG_DR_ADC_AGC || reg == DLV_REG_DR_MIX)
			data &= ~(1 << 6);
		return icdc_d1_hw_write_normal(icdc_d1, reg, data);
	}
}
/*SR*/
#define DLV_SR_JACK_SHIFT	5
#define DLV_SR_JACK_MASK	(0x1 << DLV_SR_JACK_SHIFT)
/*MR*/
#define DLV_MR_ADC_MODE_UPD_SHIFT	5
#define DLV_MR_ADC_MODE_UPD_MASK	(0x1 << DLV_MR_ADC_MODE_UPD_SHIFT)
#define DLV_MR_ADC_PGMED		0
#define DLV_MR_ADC_OPING		1
#define DLV_MR_JADC_MUTE_STATE_SHIFT	3
#define DLV_MR_JADC_MUTE_STATE_MASK	(0x3 << DLV_MR_JADC_MUTE_STATE_SHIFT)
#define DLV_MR_ADC_NOT_MUTE		0x0
#define DLV_MR_ADC_BEING_MUTE		0x1
#define DLV_MR_ADC_LEAVING_MUTE		0x2
#define DLV_MR_ADC_IN_MUTE		0x3
#define DLV_MR_DAC_MODE_STATE_SHIFT	2
#define DLV_MR_DAC_MODE_STATE_MASK	(0x1 << DLV_MR_DAC_MODE_STATE_SHIFT)
#define DLV_MR_DAC_PGMED		0
#define DLV_MR_DAC_OPING		1
#define DLV_MR_DAC_MUTE_STATE_SHIFT	0
#define DLV_MR_DAC_MUTE_STATE_MASK	(0x3 << DLV_MR_DAC_MUTE_STATE_SHIFT)
#define DLV_MR_DAC_NOT_MUTE		0x0
#define DLV_MR_DAC_BEING_MUTE		0x1
#define DLV_MR_DAC_LEAVING_MUTE		0x2
#define DLV_MR_DAC_IN_MUTE		0x3
/*DA/ADC AICR*/
#define DLV_AICR_ADWL_SHIFT	6
#define DLV_AICR_ADWL_MASK	(0x3 << DLV_AICR_ADWL_SHIFT)
#define DLV_AICR_MODE_SHIFT	5
#define DLV_AICR_MODE_MASK	(0x1 << DLV_AICR_MODE_SHIFT)
#define DLV_AICR_MODE_SLAVE	1
#define DLV_AICR_MODE_MASTER	0
#define DLV_AICR_AUDIOIF_SHIFT	0
#define DLV_AICR_AUDIOIF_MASK	(0x3 << DLV_AICR_AUDIOIF_SHIFT)
#define DLV_AICR_AUDIOIF_I2S	3
#define DLV_AICR_AICR_SB_SHIFT	4
#define DLV_AICR_AICR_SB_MASK	(0x1 << DLV_AICR_AICR_SB_SHIFT)
/*CR HP*/
#define DLV_CR_HP_MUTE_SHIFT	7
#define DLV_CR_HP_MUTE_MASK	(0x1 << DLV_CR_HP_MUTE_SHIFT)
#define DLV_CR_HP_SB_SHIFT	4
#define DLV_CR_HP_SB_MASK	(0x1 << DLV_CR_HP_SB_SHIFT)
#define DLV_CR_HP_MUX_SHIFT	0
#define DLV_CR_HP_MUX_MASK	(0x7 << DLV_CR_HP_MUX_SHIFT)
#define DLV_CR_HP_DACL		0
#define DLV_CR_HP_DACLR		1
#define DLV_CR_HP_AILATT	2
#define DLV_CR_HP_AIRATT	3
#define DLV_CR_HP_AILRATT	4
/*CR MIC1*/
#define DLV_CR_MIC1_DIFF_SHIFT	6
#define DLV_CR_MIC1_DIFF_MASK	(0x1 << DLV_CR_MIC1_DIFF_SHIFT)
#define DLV_CR_MIC1_MUX_SHIFT	0
#define DLV_CR_MIC1_MUX_MASK	(0x1 << DLV_CR_MIC1_MUX_SHIFT)
/*CR LI1*/
#define DLV_CR_LI1_DIFF_SHIFT	6
#define DLV_CR_LI1_DIFF_MASK	(0x1 << DLV_CR_LI1_DIFF_SHIFT)
#define DLV_CR_LI1_MUX_SHIFT	0
#define DLV_CR_LI1_MUX_MASK	(0x1 << DLV_CR_LI1_MUX_SHIFT)
/*CR DAC*/
#define DLV_CR_DAC_MUTE_SHIFT	7
#define DLV_CR_DAC_MUTE_MASK	(0x1 << DLV_CR_DAC_MUTE_SHIFT)
#define DLV_CR_DAC_SB_SHIFT	4
#define DLV_CR_DAC_SB_MASK	(0x1 << DLV_CR_DAC_SB_SHIFT)
/*CR ADC*/
#define DLV_CR_ADC_MUTE_SHIFT	7
#define DLV_CR_ADC_MUTE_MASK	(0x1 << DLV_CR_ADC_MUTE_SHIFT)
/*FCR AD/DAC*/
#define DLV_FCR_FREQ_SHIFT	0
#define DLV_FCR_FREQ_MASK	(0xf << DLV_FCR_FREQ_SHIFT)
/*CR VIC*/
#define DLV_CR_VIC_SB_SLEEP_SHIFT	1
#define DLV_CR_VIC_SB_SLEEP_MASK	(0x1 << DLV_CR_VIC_SB_SLEEP_SHIFT)
#define DLV_CR_VIC_SB_SHIFT		0
#define DLV_CR_VIC_SB_MASK		(0x1 << DLV_CR_VIC_SB_SHIFT)
/*CR CK*/
#define DLV_CR_CK_SB_SHIFT	4
#define DLV_CR_CK_SB_MASK	(0x1 << DLV_CR_CK_SB_SHIFT)
#define DLV_CR_CK_SEL_SHIFT	0
#define DLV_CR_CK_SEL_MASK	(0xf << DLV_CR_CK_SEL_SHIFT)
#define DLV_CR_CK_SEL_12M	0
/*ICR*/
#define DLV_ICR_INT_FORM_SHIFT	6
#define DLV_ICR_INT_FORM_MASK	(0x3 << DLV_ICR_INT_FORM_SHIFT)
#define DLV_ICR_INT_FORM_HIGH_LEVEL 0

/*IFR*/
#define DLV_IFR_LOCK_SHIFT	7
#define DLV_IFR_LOCK_MASK	(1 << DLV_IFR_LOCK_SHIFT)
#define DLV_IFR_SCLR_SHIFT	6
#define DLV_IFR_SCLR_MASK	(1 << DLV_IFR_SCLR_SHIFT)
#define DLV_IFR_JACK_SHIFT      5
#define DLV_IFR_JACK_MASK       (1 << DLV_IFR_JACK_SHIFT)
#define DLV_IFR_ADC_MUTE_SHIFT  2
#define DLV_IFR_ADC_MUTE_MASK	(1 << DLV_IFR_ADC_MUTE_SHIFT)
#define DLV_IFR_DAC_MODE_SHIFT  1
#define DLV_IFR_DAC_MODE_MASK   (1 << DLV_IFR_DAC_MODE_SHIFT)
#define DLV_IFR_DAC_MUTE_SHIFT  0
#define DLV_IFR_DAC_MUTE_MASK   (1 << DLV_IFR_DAC_MUTE_SHIFT)

/*IMR*/
#define DLV_IMR_LOCK            (1 << 7)
#define DLV_IMR_SCLR            (1 << 6)
#define DLV_IMR_JACK            (1 << 5)
#define DLV_IMR_ADC_MUTE        (1 << 2)
#define DLV_IMR_DAC_MODE        (1 << 1)
#define DLV_IMR_DAC_MUTE        (1 << 0)
#define ICR_IMR_MASK		0xe7
#define ICR_IMR_COMMON_MSK	(ICR_IMR_MASK & (~(DLV_IMR_SCLR)))
/*GCR HPL*/
#define DLV_GCR_HPL_LRGO_SHIFT	7
#endif	/* __ICDC_D1_REG_H__ */
