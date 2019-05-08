/*
 * sound/soc/ingenic/icodec/icdc_d2.c
 * ALSA SoC Audio driver -- ingenic internal codec (icdc_d2) driver

 * Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cscheng <shicheng.cheng@ingenic.com>
 *
 * Note: icdc_d2 is an internal codec for jz SOC
 *	 used for jz4775 m150 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <asm/div64.h>
#include <sound/soc-dai.h>
#include <soc/irq.h>
#include "icdc_d2.h"

static int icdc_d2_debug = 0;
module_param(icdc_d2_debug, int, 0644);
#define DEBUG_MSG(msg...)			\
	do {					\
		if (icdc_d2_debug)		\
			printk("ICDC: " msg);	\
	} while(0)

static u8 icdc_d2_reg_defcache[DLV_MAX_REG_NUM] = {
/*reg	 0    1    2    3    4    5    6    7    8    9  */
		0x00,0xc3,0xc3,0x90,0x98,0x00,0x00,0xb1,0x11,0x10,
/*reg	10   11   12   13   14   15   16   17   18   19  */
		0x00,0x03,0x00,0x00,0x40,0x00,0xff,0x00,0x06,0x06,
/*reg	20   21   22   23   24   25   26   27   28   29  */
		0x06,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
/*reg	30   31   32   33   34   35  */
		0x00,0x34,0x07,0x44,0x1f,0x00
};

static int icdc_d2_volatile(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg > DLV_MAX_REG_NUM)
		return 1;

	switch (reg) {
	case DLV_REG_SR:
	case DLV_REG_IFR:
		return 1;
	default:
		return 0;
	}
}

static int icdc_d2_writable(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg > DLV_MAX_REG_NUM)
		return 0;

	switch (reg) {
	case DLV_REG_SR:
	case DLV_REG_MISSING_REG1:
	case DLV_REG_MISSING_REG2:
		return 0;
	default:
		return 1;
	}
}

static int icdc_d2_readable(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg > DLV_MAX_REG_NUM)
		return 0;

	switch (reg) {
	case DLV_REG_MISSING_REG1:
	case DLV_REG_MISSING_REG2:
		return 0;
	default:
		return 1;
	}
}

static void dump_registers_hazard(struct icdc_d2 *icdc_d2)
{
	int reg = 0;
	dev_info(icdc_d2->dev, "-------------------register:");
	for ( ; reg < DLV_MAX_REG_NUM; reg++) {
		if (reg % 8 == 0)
			printk("\n");
		if (icdc_d2_readable(icdc_d2->codec, reg))
			printk(" 0x%02x:0x%02x,", reg, icdc_d2_hw_read(icdc_d2, reg));
		else
			printk(" 0x%02x:0x%02x,", reg, 0x0);
	}
	printk("\n");
	dev_info(icdc_d2->dev, "----------------------------\n");
	return;
}

static int icdc_d2_write(struct snd_soc_codec *codec, unsigned int reg,
			unsigned int value)
{
	struct icdc_d2 *icdc_d2 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;
	int tmp_val = value;
	BUG_ON(reg > DLV_MAX_REG_NUM);
	dev_dbg(icdc_d2->dev,"%s reg = %x value = %x \n",__func__,reg,tmp_val);
	if (icdc_d2_writable(codec, reg)) {
		if (!icdc_d2_volatile(codec,reg)) {
			if((reg == DLV_REG_GCR_DACL)||(reg == DLV_REG_GCR_DACR)){
					tmp_val = (~tmp_val)&0x3f;
			}
			ret = snd_soc_cache_write(codec, reg, tmp_val);
			if (ret != 0)
				dev_err(codec->dev, "Cache write to %x failed: %d\n",
						reg, ret);
		}
		return icdc_d2_hw_write(icdc_d2, reg, tmp_val);
	}
	return 0;
}

static unsigned int icdc_d2_read(struct snd_soc_codec *codec, unsigned int reg)
{

	struct icdc_d2 *icdc_d2 = snd_soc_codec_get_drvdata(codec);
	int val = 0, ret = 0;
	BUG_ON(reg > DLV_MAX_REG_NUM);

	if (!icdc_d2_volatile(codec,reg)) {
		ret = snd_soc_cache_read(codec, reg, &val);
		if (ret >= 0){
			if((reg == DLV_REG_GCR_DACL)||(reg == DLV_REG_GCR_DACR)){
				val = (~val)&0x3f;
			}
			return val;
		}else
			dev_err(codec->dev, "Cache read from %x failed: %d\n",
					reg, ret);
	}

	if (icdc_d2_readable(codec, reg))
		return icdc_d2_hw_read(icdc_d2, reg);

	return 0;
}

/* DLV4780 CODEC's gain controller init, it's a neccessary program */
static void icdc_d2_reset_gain(struct snd_soc_codec *codec)
{
	int start = DLV_REG_GCR_HPL, end = DLV_REG_GCR_ADCR;
	int i, reg_value;

	for (i = start; i <= end; i++) {
		reg_value = snd_soc_read(codec, i);
		snd_soc_write(codec, i, reg_value);
	}
	return;
}

static int icdc_d2_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level) {
	DEBUG_MSG("%s enter set level %d\n", __func__, level);
	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		if (snd_soc_update_bits(codec, DLV_REG_CR_VIC, DLV_CR_VIC_SB_MASK, 0))
			msleep(250);
		if (snd_soc_update_bits(codec, DLV_REG_CR_VIC, DLV_CR_VIC_SB_SLEEP_MASK, 0)) {
			msleep(400);
			icdc_d2_reset_gain(codec);
		}
		break;
	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec, DLV_REG_CR_VIC, DLV_CR_VIC_SB_SLEEP_MASK, 1);
		snd_soc_update_bits(codec, DLV_REG_CR_VIC, DLV_CR_VIC_SB_MASK, 1);
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static int icdc_d2_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	int playback = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK);
	int bit_width_sel = 3;
	int speed_sel = 0;
	int aicr_reg = playback ? DLV_REG_AICR_DAC : DLV_REG_AICR_ADC;
	int fcr_reg = playback ? DLV_REG_FCR_DAC : DLV_REG_FCR_ADC;
	int sample_attr[] = {	96000,  48000,  44100, 32000, 24000,
				22050, 16000,  12000, 11025, 8000,
					};
	int speed = params_rate(params);
	int bit_width = params_format(params);
	DEBUG_MSG("%s enter  set bus width %d , sample rate %d\n",
			__func__, bit_width, speed);
	/* bit width */
	switch (bit_width) {
	case SNDRV_PCM_FORMAT_S16_LE:
		bit_width_sel = 0;
		break;
	case SNDRV_PCM_FORMAT_S18_3LE:
		bit_width_sel = 1;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		bit_width_sel = 2;
		break;
	default:
	case SNDRV_PCM_FORMAT_S24_3LE:
		bit_width_sel = 3;
		break;
	}

	/*sample rate*/
	for (speed_sel = 9; speed > sample_attr[speed_sel]; speed_sel--)
		;
	snd_soc_update_bits(codec, aicr_reg, DLV_AICR_DAC_ADWL_MASK,
			(bit_width_sel << DLV_AICR_DAC_ADWL_SHIFT));
	snd_soc_update_bits(codec, fcr_reg, DLV_FCR_FREQ_MASK,
			(speed_sel << DLV_FCR_FREQ_SHIFT));
	return 0;
}



static int icdc_d2_digital_mute_unlocked(struct snd_soc_codec *codec,
		int mute, bool usr)
{
	struct icdc_d2 *icdc_d2 = snd_soc_codec_get_drvdata(codec);
	int timeout = 0;

	if (usr)
		icdc_d2->dac_user_mute = mute;

	if (snd_soc_test_bits(codec, DLV_REG_CR_DAC, DLV_CR_DAC_SB_MASK, 0))
		return 0;

	DEBUG_MSG("%s enter real mute %d\n", __func__, mute);

	if (snd_soc_test_bits(codec, DLV_REG_CR_DAC, DLV_CR_DAC_MUTE_MASK, 0) == mute)
		goto out;

	if(mute){
		snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_GDO_MASK);

		snd_soc_update_bits(codec, DLV_REG_CR_DAC, DLV_CR_DAC_MUTE_MASK, (!!mute) << DLV_CR_DAC_MUTE_SHIFT);

		timeout = 5;
		while ((!snd_soc_test_bits(codec, DLV_REG_IFR, DLV_IFR_GDO_MASK, 0)) && timeout--) {
			mdelay(1);
		}
	}else{
		snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_GUP_MASK);

		snd_soc_update_bits(codec, DLV_REG_CR_DAC, DLV_CR_DAC_MUTE_MASK, (!!mute) << DLV_CR_DAC_MUTE_SHIFT);
		timeout = 5;
		while ((!snd_soc_test_bits(codec, DLV_REG_IFR, DLV_IFR_GUP_MASK, 0)) && timeout--) {
			mdelay(1);
		}

	}
	if (timeout == 0)
		dev_warn(codec->dev,"wait dac enter %s state seq timeout\n",
				mute ? "mute" : "unmute");
	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_GDO_MASK | DLV_IFR_GUP_MASK);
out:
	DEBUG_MSG("%s leave real mute %d\n", __func__, mute);

	return 0;
}

static int icdc_d2_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	int ret;

	DEBUG_MSG("%s enter mute %d\n", __func__, mute);
	ret = icdc_d2_digital_mute_unlocked(codec, mute, true);
	return ret;
}

static int icdc_d2_trigger(struct snd_pcm_substream * stream, int cmd,
		struct snd_soc_dai *dai)
{
#ifdef DEBUG
	struct snd_soc_codec *codec = dai->codec;
	struct icdc_d2 *icdc_d2 = snd_soc_codec_get_drvdata(codec);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		dump_registers_hazard(icdc_d2);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		dump_registers_hazard(icdc_d2);
		break;
	}
#endif
	return 0;
}

#define DLV4780_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S18_3LE | \
			 SNDRV_PCM_FMTBIT_S20_3LE |SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops icdc_d2_dai_ops = {
	.hw_params	= icdc_d2_hw_params,
	.digital_mute	= icdc_d2_digital_mute,
	.trigger = icdc_d2_trigger,
};

static struct snd_soc_dai_driver  icdc_d2_codec_dai = {
	.name = "icdc-d2-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
#if defined(CONFIG_SOC_4780)
		.rates = SNDRV_PCM_RATE_8000_96000,
#else
		.rates = SNDRV_PCM_RATE_8000_192000,
#endif
		.formats = DLV4780_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
#if defined(CONFIG_SOC_4780)
		.rates = SNDRV_PCM_RATE_8000_96000,
#else
		.rates = SNDRV_PCM_RATE_8000_192000,
#endif
		.formats = DLV4780_FORMATS,
	},
	.ops = &icdc_d2_dai_ops,
};

/* unit: 0.01dB */
static const DECLARE_TLV_DB_SCALE(dac_tlv, -3100, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(diff_out_tlv, -2500, 100, 0);
static const DECLARE_TLV_DB_SCALE(bypass_tlv, -2500, 100, 0);
static const DECLARE_TLV_DB_SCALE(mic_tlv, 0, 400, 0);

static const char *icdc_d2_adc_sel[] = {"MIC1/LR", "MIC2/LR", "MIC12/LR", "MIC12/RL","LININ"};
static const char *icdc_d2_aohp_sel[] = {"MIC1/LR", "MIC2/LR", "MIC12/LR" ,"MIC12/RL", "AIL/Ratt", "DACL/R"};
static const char *icdc_d2_aolo_sel[] = {"MIC1", "MIC2", "MIC1/2", "MIC1/2", "AIL/Ratt", "DACL/R"};

static const char *icdc_d2_aohp_vir_sel[] = { "HP-ON", "HP-OFF"};
static const char *icdc_d2_aolo_vir_sel[] = { "LO-ON", "LO-OFF"};

static const unsigned int icdc_d2_adc_sel_value[] = {0x0, 0x1, 0x0, 0x1, 0x2};
static const unsigned int icdc_d2_hp_sel_value[] = {0x0, 0x1, 0x0, 0x1, 0x2, 0x3};
static const unsigned int icdc_d2_aolo_sel_value[] = {0x0, 0x1, 0x0, 0x1, 0x2, 0x3};

static const struct soc_enum icdc_d2_enum[] = {
	SOC_VALUE_ENUM_SINGLE(DLV_REG_CR_ADC, 0, 0x3,  ARRAY_SIZE(icdc_d2_adc_sel),icdc_d2_adc_sel, icdc_d2_adc_sel_value),
	SOC_VALUE_ENUM_SINGLE(DLV_REG_CR_HP, 0, 0x3,  ARRAY_SIZE(icdc_d2_aohp_sel),icdc_d2_aohp_sel, icdc_d2_hp_sel_value),
	SOC_VALUE_ENUM_SINGLE(DLV_REG_CR_LO, 0, 0x3, ARRAY_SIZE(icdc_d2_aolo_sel),icdc_d2_aolo_sel, icdc_d2_aolo_sel_value),
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(icdc_d2_aohp_vir_sel), icdc_d2_aohp_vir_sel),
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(icdc_d2_aolo_vir_sel), icdc_d2_aolo_vir_sel),
};


static const struct snd_kcontrol_new icdc_d2_adc_mux_controls =
	SOC_DAPM_VALUE_ENUM("Route",  icdc_d2_enum[0]);

static const struct snd_kcontrol_new icdc_d2_aohp_mux_controls =
	SOC_DAPM_VALUE_ENUM("Route", icdc_d2_enum[1]);

static const struct snd_kcontrol_new icdc_d2_aolo_mux_controls =
	SOC_DAPM_VALUE_ENUM("Route", icdc_d2_enum[2]);

static const struct snd_kcontrol_new icdc_d2_aohp_vmux_controls =
	SOC_DAPM_ENUM_VIRT("Route", icdc_d2_enum[3]);

static const struct snd_kcontrol_new icdc_d2_aolo_vmux_controls =
	SOC_DAPM_ENUM_VIRT("Route", icdc_d2_enum[4]);


static const struct snd_kcontrol_new icdc_d2_snd_controls[] = {
	/* Volume controls */
	SOC_DOUBLE_R_SX_TLV("Digital Playback Volume",DLV_REG_GCR_DACL , DLV_REG_GCR_DACR , 6,-32,31,dac_tlv),
	SOC_DOUBLE_R_TLV("Headphone Volume", DLV_REG_GCR_HPL, DLV_REG_GCR_HPR,0, 31, 1, diff_out_tlv),
	SOC_DOUBLE_R_TLV("Bypass Volume", DLV_REG_GCR_LIBYL,DLV_REG_GCR_LIBYR, 0, 31, 1, bypass_tlv),
	SOC_DOUBLE_R_TLV("Mic Volume", DLV_REG_GCR_MIC1,DLV_REG_GCR_MIC2, 0, 5, 0, mic_tlv),
	SOC_DOUBLE_R_TLV("Digital Capture Volume", DLV_REG_GCR_ADCL, DLV_REG_GCR_ADCR, 0, 43, 0, adc_tlv),
	/* ADC private controls */
	SOC_SINGLE("ADC High Pass Filter Switch", DLV_REG_FCR_ADC, 6, 1, 0),
	SOC_SINGLE("ADC mono", DLV_REG_CR_ADC, 6, 1, 0),
	/* mic private controls */
	SOC_SINGLE("Mic Diff Switch", DLV_REG_CR_MIC, 6, 1, 0),
	SOC_SINGLE("Mic Stereo Switch", DLV_REG_CR_MIC, 7, 1, 0),
	SOC_SINGLE("MIC switch", DLV_REG_CR_ADC, 7, 1, 0),

	SOC_SINGLE("Digital Playback mute", DLV_REG_CR_DAC, 7, 1, 0),
};

static int icdc_d2_anit_pop_save_hp_gain(struct snd_soc_codec *codec,struct icdc_d2* icdc_d2){
	icdc_d2->hpl_wished_gain =  snd_soc_read(codec,DLV_REG_GCR_HPL);
	icdc_d2->hpr_wished_gain =  snd_soc_read(codec,DLV_REG_GCR_HPR);
	icdc_d2->linl_wished_gain =  snd_soc_read(codec,DLV_REG_GCR_LIBYL);
	icdc_d2->linr_wished_gain =  snd_soc_read(codec,DLV_REG_GCR_LIBYL);
	snd_soc_write(codec,DLV_REG_GCR_HPL,0);
	snd_soc_write(codec,DLV_REG_GCR_HPR,0);
	snd_soc_write(codec,DLV_REG_GCR_LIBYL,0);
	snd_soc_write(codec,DLV_REG_GCR_LIBYR,0);
	return 0;
}
static int icdc_d2_anit_pop_restore_hp_gain(struct snd_soc_codec *codec,struct icdc_d2* icdc_d2){
	snd_soc_write(codec,DLV_REG_GCR_LIBYL,icdc_d2->linl_wished_gain);
	snd_soc_write(codec,DLV_REG_GCR_LIBYR,icdc_d2->linr_wished_gain);
	snd_soc_write(codec,DLV_REG_GCR_HPL,icdc_d2->hpl_wished_gain);
	snd_soc_write(codec,DLV_REG_GCR_HPR,icdc_d2->hpr_wished_gain);
	return 0;
}
static int icdc_d2_aohp_anti_pop_event_sub(struct snd_soc_codec *codec,int event) {
	struct icdc_d2 *icdc_d2 = snd_soc_codec_get_drvdata(codec);
	int timeout;
	DEBUG_MSG("%s enter %d\n", __func__ , event);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (snd_soc_test_bits(codec, DLV_REG_CR_HP, DLV_CR_HP_SB_MASK, 0)) {
			icdc_d2->aohp_in_pwsq = 1;
			udelay(500);
			icdc_d2_anit_pop_save_hp_gain(codec,icdc_d2);
			snd_soc_update_bits(codec,DLV_REG_CR_HP,DLV_CR_HP_MUTE_MASK,0);
			mdelay(1);
			printk(KERN_DEBUG"codec cr hp before1 set hp sb %x ifr %x\n",
					snd_soc_read(codec, DLV_REG_CR_HP),snd_soc_read(codec, DLV_REG_IFR));
		}
		break;
	case SND_SOC_DAPM_POST_PMU:
		if (icdc_d2->aohp_in_pwsq) {
			timeout	= 5;
			while ((snd_soc_test_bits(codec, DLV_REG_IFR, DLV_IFR_RUP_MASK, 1)) && timeout--)
				msleep(1);
			if (timeout == 0)
				dev_warn(codec->dev,"wait dac mode state timeout\n");
			snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_RUP_MASK);
			icdc_d2_digital_mute_unlocked(codec, icdc_d2->dac_user_mute, false);
			icdc_d2_anit_pop_restore_hp_gain(codec,icdc_d2);
			icdc_d2->aohp_in_pwsq = 0;
		}
		break;
	case SND_SOC_DAPM_PRE_PMD:
		if (!snd_soc_test_bits(codec, DLV_REG_CR_HP,
					DLV_CR_HP_SB_MASK, 0)) {
			icdc_d2->aohp_in_pwsq = 1;
			icdc_d2_anit_pop_save_hp_gain(codec,icdc_d2);
			/*dac mute*/
			icdc_d2_digital_mute_unlocked(codec, 1, false);

			printk(KERN_DEBUG"codec cr hp before1 set hp sb %x ifr %x\n",
					snd_soc_read(codec, DLV_REG_CR_HP), snd_soc_read(codec, DLV_REG_IFR));
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (icdc_d2->aohp_in_pwsq) {
			timeout	= 5;
			while ((snd_soc_test_bits(codec, DLV_REG_IFR, DLV_IFR_RDO_MASK, 1)) && timeout--)
				msleep(1);
			if (timeout == 0)
				dev_warn(codec->dev,"wait dac mode state timeout\n");
			snd_soc_update_bits(codec, DLV_REG_IFR, DLV_IFR_RDO_MASK,1);
			/*hp mute*/
			snd_soc_update_bits(codec, DLV_REG_CR_HP, DLV_CR_HP_MUTE_MASK,1);

			/*user mute avail*/
			icdc_d2_digital_mute_unlocked(codec, icdc_d2->dac_user_mute, false);
			icdc_d2_anit_pop_restore_hp_gain(codec,icdc_d2);
			icdc_d2->aohp_in_pwsq = 0;
		}
		break;
	}
	return 0;
}

static int icdc_d2_aohp_anti_pop_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event) {
	struct snd_soc_codec *codec = w->codec;
	icdc_d2_aohp_anti_pop_event_sub(codec, event);
	return 0;
}
static const struct snd_soc_dapm_widget icdc_d2_dapm_widgets[] = {
	SND_SOC_DAPM_MICBIAS("MICBIAS", DLV_REG_CR_MIC, 0, 1),

	SND_SOC_DAPM_MUX("ADC Mux", SND_SOC_NOPM, 0, 0, &icdc_d2_adc_mux_controls),
	SND_SOC_DAPM_MUX("AOHP Mux", SND_SOC_NOPM, 0, 0, &icdc_d2_aohp_mux_controls),
	SND_SOC_DAPM_MUX("AOLO Mux", DLV_REG_CR_LO, 4, 1, &icdc_d2_aolo_mux_controls),
	SND_SOC_DAPM_VIRT_MUX("AOHP Vmux", SND_SOC_NOPM, 0, 0, &icdc_d2_aohp_vmux_controls),
	SND_SOC_DAPM_VIRT_MUX("AOLO Vmux", SND_SOC_NOPM, 0, 0, &icdc_d2_aolo_vmux_controls),

	SND_SOC_DAPM_DAC("DAC", "Playback", DLV_REG_CR_DAC, 4, 1),

	SND_SOC_DAPM_PGA("MIC1", DLV_REG_CR_MIC, 4, 1, NULL, 0),
	SND_SOC_DAPM_PGA("MIC2", DLV_REG_CR_MIC, 5, 1, NULL, 0),
	SND_SOC_DAPM_PGA("AIR", DLV_REG_CR_LI, 0, 1, NULL, 0),
	SND_SOC_DAPM_PGA("AIL", DLV_REG_CR_LI, 0, 1, NULL, 0),
	SND_SOC_DAPM_PGA("AIRatt", DLV_REG_CR_LI, 4, 1, NULL, 0),
	SND_SOC_DAPM_PGA("AILatt", DLV_REG_CR_LI, 4, 1, NULL, 0),
	SND_SOC_DAPM_PGA_E("AOHP", DLV_REG_CR_HP, 4, 1, NULL, 0, icdc_d2_aohp_anti_pop_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_ADC("ADC", "Capture", DLV_REG_CR_ADC, 4, 1),

	SND_SOC_DAPM_OUTPUT("AOHPL"),
	SND_SOC_DAPM_OUTPUT("AOHPR"),
	SND_SOC_DAPM_OUTPUT("AOLOP"),
	SND_SOC_DAPM_OUTPUT("AOLON"),
	SND_SOC_DAPM_INPUT("MICP1"),
	SND_SOC_DAPM_INPUT("MICN1"),
	SND_SOC_DAPM_INPUT("MICN2"),
	SND_SOC_DAPM_INPUT("MICP2"),
	SND_SOC_DAPM_INPUT("AIL1"),
	SND_SOC_DAPM_INPUT("AIR1"),
};

static const struct snd_soc_dapm_route intercon[] = {
	{ "MIC1",  NULL,  "MICN1" },
	{ "MIC1",  NULL,  "MICP1" },
	{ "MIC2",  NULL,  "MICN2" },
	{ "MIC2",  NULL,  "MICP2" },

	{ "AIL",      NULL,  "AIL1" },
	{ "AILatt",   NULL,  "AIL1" },
	{ "AIR",      NULL,  "AIR1" },
	{ "AIRatt",   NULL,  "AIR1" },

	/*input*/
	{ "ADC Mux", "MIC1/LR", "MIC1"  },
	{ "ADC Mux", "MIC2/LR", "MIC2"  },
	{ "ADC Mux", "MIC12/LR", "MIC1"  },
	{ "ADC Mux", "MIC12/LR", "MIC2"  },
	{ "ADC Mux", "MIC12/RL", "MIC1"  },
	{ "ADC Mux", "MIC12/RL", "MIC2"  },
	{ "ADC Mux", "LININ", "AIL"  },
	{ "ADC Mux", "LININ", "AIR"  },

	{ "ADC", NULL, "ADC Mux"},
	/*bypass*/
	{ "AOHP Mux", "MIC1/LR", "MIC1"},
	{ "AOHP Mux", "MIC2/LR", "MIC2"},
	{ "AOHP Mux", "MIC12/LR", "MIC1"},
	{ "AOHP Mux", "MIC12/LR", "MIC2"},
	{ "AOHP Mux", "MIC12/RL", "MIC1"},
	{ "AOHP Mux", "MIC12/RL", "MIC2"},
	{ "AOHP Mux", "AIL/Ratt", "AILatt"},
	{ "AOHP Mux", "AIL/Ratt", "AIRatt"},

	{ "AOLO Mux", "MIC1", "MIC1"},
	{ "AOLO Mux", "MIC2", "MIC2"},
	{ "AOLO Mux", "MIC1/2", "MIC1"},
	{ "AOLO Mux", "MIC1/2", "MIC2"},
	{ "AOLO Mux", "AIL/Ratt", "AILatt"},
	{ "AOLO Mux", "AIL/Ratt", "AIRatt"},

	/*output*/
	{ "AOHP Mux", "DACL/R", "DAC"},
	{ "AOHP",  NULL, "AOHP Mux"},
	{ "AOHP Vmux", "HP-ON", "AOHP"},

	{ "AOHPR", NULL, "AOHP Vmux"},
	{ "AOHPL", NULL, "AOHP Vmux"},

	{ "AOLO Mux", "DACL/R", "DAC"},
	{ "AOLO Vmux", "LO-ON", "AOLO Mux"},

	{ "AOLOP", NULL, "AOLO Vmux"},
	{ "AOLON", NULL, "AOLO Vmux"}
};

static int icdc_d2_suspend(struct snd_soc_codec *codec, pm_message_t state)
{
	return 0;
}

static int icdc_d2_resume(struct snd_soc_codec *codec)
{
	return 0;
}

/*
 * CODEC short circut handler
 *
 * To protect CODEC, CODEC will be shutdown when short circut occured.
 * Then we have to restart it.
 */
static inline void icdc_d2_short_circut_handler(struct icdc_d2 *icdc_d2)
{
	struct snd_soc_codec *codec = icdc_d2->codec;
	int hp_is_on;
	int bias_level;

	/*make sure hp power on/down seq was completed*/
	while (1) {
		if (!icdc_d2->aohp_in_pwsq)
			break;
		msleep(10);
	};

	/*shut down codec seq*/
	if (!(snd_soc_read(codec, DLV_REG_CR_HP) & DLV_CR_HP_SB_MASK)) {
		hp_is_on = 1;
		icdc_d2_aohp_anti_pop_event_sub(codec , SND_SOC_DAPM_PRE_PMD);
		snd_soc_update_bits(codec, DLV_REG_CR_HP, DLV_CR_HP_SB_MASK,1);
		icdc_d2_aohp_anti_pop_event_sub(codec , SND_SOC_DAPM_POST_PMD);
	}

	bias_level =  codec->dapm.bias_level;

	icdc_d2_set_bias_level(codec, SND_SOC_BIAS_OFF);

	msleep(10);

	/*power on codec seq*/
	icdc_d2_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	icdc_d2_set_bias_level(codec, bias_level);

	if (hp_is_on) {
		icdc_d2_aohp_anti_pop_event_sub(codec , SND_SOC_DAPM_PRE_PMU);
		snd_soc_update_bits(codec, DLV_REG_CR_HP, DLV_CR_HP_SB_MASK, 0);
		icdc_d2_aohp_anti_pop_event_sub(codec , SND_SOC_DAPM_POST_PMU);
	}
	return;
}

int icdc_d2_hp_detect(struct snd_soc_codec *codec, struct snd_soc_jack *jack,
		int type)
{
	struct icdc_d2 *icdc_d2 = snd_soc_codec_get_drvdata(codec);
	int report = 0;

	icdc_d2->jack = jack;
	icdc_d2->report_mask = type;

	if (jack) {
		snd_soc_update_bits(codec, DLV_REG_IMR, DLV_IMR_JACK, 0);
		/*initial headphone detect*/
		if (!!(snd_soc_read(codec, DLV_REG_SR) & DLV_SR_JACK_MASK)) {
			report = icdc_d2->report_mask;
			dev_info(codec->dev, "codec initial headphone detect --> headphone was inserted\n");
			snd_soc_jack_report(icdc_d2->jack, report, icdc_d2->report_mask);
		}
	} else {
		snd_soc_update_bits(codec, DLV_REG_IMR, 0, DLV_IMR_JACK);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(icdc_d2_hp_detect);

/*
 * CODEC work queue handler
 */
static void icdc_d2_irq_work_handler(struct work_struct *work)
{
	struct icdc_d2 *icdc_d2 = (struct icdc_d2 *)container_of(work, struct icdc_d2, irq_work);
	struct snd_soc_codec *codec = icdc_d2->codec;
	unsigned int icdc_d2_ifr, icdc_d2_imr;

	icdc_d2_imr = icdc_d2->codec_imr;
	icdc_d2_ifr = snd_soc_read(codec, DLV_REG_IFR);

	/*short protect interrupt*/
	if ((icdc_d2_ifr & (~icdc_d2_imr)) & DLV_IFR_SCLR_MASK) {
		mutex_lock(&codec->mutex);
		do {
			dev_warn(codec->dev, "codec short circut protect !!!\n");
			icdc_d2_short_circut_handler(icdc_d2);
			snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_SCLR_MASK);
		} while (snd_soc_read(codec, DLV_REG_IFR) & DLV_IFR_SCLR_MASK);
		dev_warn(codec->dev, "codec short circut protect ok!!!\n");
		mutex_unlock(&codec->mutex);
	}

	/*hp detect interrupt*/
	if ((icdc_d2_ifr & (~icdc_d2_imr)) & DLV_IFR_JACK_MASK) {
		int icdc_d2_jack = 0;
		int report = 0;
		msleep(200);
		icdc_d2_jack = !!(snd_soc_read(codec, DLV_REG_SR) & DLV_SR_JACK_MASK);
		dev_info(codec->dev, "codec headphone detect %s\n",
				icdc_d2_jack ? "insert" : "pull out");
		snd_soc_write(codec, DLV_REG_IFR, DLV_SR_JACK_MASK);
		if (icdc_d2_jack)
			report = icdc_d2->report_mask;
		snd_soc_jack_report(icdc_d2->jack, report,icdc_d2->report_mask);
	}
	snd_soc_write(codec,DLV_REG_IMR, icdc_d2_imr);
}

/*
 * IRQ routine
 */
static irqreturn_t dlv_codec_irq(int irq, void *dev_id)
{
	struct icdc_d2 *icdc_d2 = (struct icdc_d2 *)dev_id;
	struct snd_soc_codec *codec = icdc_d2->codec;
	if (icdc_d2_test_irq(icdc_d2)) {
		/*disable interrupt*/
		icdc_d2->codec_imr = snd_soc_read(codec, DLV_REG_IMR);
		snd_soc_write(codec, DLV_REG_IMR, ICR_IMR_MASK);
		if(!work_pending(&icdc_d2->irq_work))
			schedule_work(&icdc_d2->irq_work);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

static int icdc_d2_probe(struct snd_soc_codec *codec)
{
	struct icdc_d2 *icdc_d2 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	dev_info(codec->dev, "codec icdc-d2 probe enter\n");

	/*power on codec*/
	icdc_d2_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

#ifdef DEBUG
	/*dump for debug*/
	dump_registers_hazard(icdc_d2);
#endif
	/* codec select enable 12M clock*/
	snd_soc_update_bits(codec, DLV_REG_CCR, DLV_CCR_CRYSTAL_MASK,
			DLV_CCR_CRYSTAL_12M << DLV_CCR_CRYSTAL_SHIFT);
	snd_soc_update_bits(codec, DLV_REG_CCR, DLV_CCR_DMIC_CLKON_MASK,
			DLV_CCR_DMIC_CLK_ON << DLV_CCR_DMIC_CLKON_SHIFT);

	/*codec select Dac/Adc i2s interface*/
	snd_soc_update_bits(codec, DLV_REG_AICR_ADC, DLV_AICR_ADC_I2S_MASK,
			DLV_AICR_ADC_I2S_MODE << DLV_AICR_ADC_I2S_SHIFT);
	snd_soc_update_bits(codec, DLV_REG_AICR_ADC, DLV_AICR_ADC_SERIAL_MASK,
			DLV_AICR_ADC_SERIAL_SERIAL << DLV_AICR_ADC_SERIAL_SHIFT);

	snd_soc_update_bits(codec, DLV_REG_AICR_DAC, DLV_AICR_DAC_I2S_MASK,
			DLV_AICR_DAC_I2S_MODE << DLV_AICR_DAC_I2S_SHIFT);
	snd_soc_update_bits(codec, DLV_REG_AICR_DAC, DLV_AICR_DAC_SERIAL_MASK,
			DLV_AICR_DAC_SERIAL_SERIAL << DLV_AICR_DAC_SERIAL_SHIFT);

	/*codec generated IRQ is a high level */
	snd_soc_update_bits(codec, DLV_REG_ICR, DLV_ICR_INT_FORM_MASK, 0);

	/*codec irq mask*/
	snd_soc_write(codec, DLV_REG_IMR, ICR_IMR_COMMON_MSK);
	/*codec clear all irq*/
	snd_soc_write(codec, DLV_REG_IFR, ICR_IMR_MASK);

	/*unmute aolo*/
	snd_soc_update_bits(codec, DLV_REG_CR_LO, (1 << 7), 0);

	INIT_WORK(&icdc_d2->irq_work, icdc_d2_irq_work_handler);
	ret = request_irq(icdc_d2->irqno, dlv_codec_irq, icdc_d2->irqflags,
			"audio_codec_irq", (void *)icdc_d2);
	if (ret)
		dev_warn(codec->dev, "Failed to request aic codec irq %d\n", icdc_d2->irqno);

	icdc_d2->codec = codec;
	return 0;
}

static int icdc_d2_remove(struct snd_soc_codec *codec)
{
	struct icdc_d2 *icdc_d2 = snd_soc_codec_get_drvdata(codec);
	dev_info(codec->dev, "codec icdc_d2 remove enter\n");
	if (icdc_d2 && icdc_d2->irqno != -1) {
		free_irq(icdc_d2->irqno, (void *)icdc_d2);
	}
	icdc_d2_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_icdc_d2_codec = {
	.probe = 	icdc_d2_probe,
	.remove = 	icdc_d2_remove,
	.suspend =	icdc_d2_suspend,
	.resume =	icdc_d2_resume,

	.read = 	icdc_d2_read,
	.write = 	icdc_d2_write,
	.volatile_register = icdc_d2_volatile,
	.readable_register = icdc_d2_readable,
	.writable_register = icdc_d2_writable,
	.reg_cache_default = icdc_d2_reg_defcache,
	.reg_word_size = sizeof(u8),
	.reg_cache_step = 1,
	.reg_cache_size = DLV_MAX_REG_NUM,
	.set_bias_level = icdc_d2_set_bias_level,

	.controls = 	icdc_d2_snd_controls,
	.num_controls = ARRAY_SIZE(icdc_d2_snd_controls),
	.dapm_widgets = icdc_d2_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(icdc_d2_dapm_widgets),
	.dapm_routes = intercon,
	.num_dapm_routes = ARRAY_SIZE(intercon),
};

/*Just for debug*/
static ssize_t icdc_d2_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct icdc_d2 *icdc_d2 = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = icdc_d2->codec;
	if (!codec) {
		dev_info(dev, "icdc_d2 is not probe, can not use %s function\n", __func__);
		return 0;
	}
	mutex_lock(&codec->mutex);
	dump_registers_hazard(icdc_d2);
	mutex_unlock(&codec->mutex);
	return 0;
}

static ssize_t icdc_d2_regs_store(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	struct icdc_d2 *icdc_d2 = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = icdc_d2->codec;
	const char *start = buf;
	unsigned int reg, val;
	int ret_count = 0;

	if (!codec) {
		dev_info(dev, "icdc_d2 is not probe, can not use %s function\n", __func__);
		return count;
	}

	while(!isxdigit(*start)) {
		start++;
		if (++ret_count >= count)
			return count;
	}
	reg = simple_strtoul(start, (char **)&start, 16);
	while(!isxdigit(*start)) {
		start++;
		if (++ret_count >= count)
			return count;
	}
	val = simple_strtoul(start, (char **)&start, 16);
	mutex_lock(&codec->mutex);
	icdc_d2_write(codec, reg, val);
	mutex_unlock(&codec->mutex);
	return count;
}

static struct device_attribute icdc_d2_sysfs_attrs =
	__ATTR(hw_regs, S_IRUGO|S_IWUGO, icdc_d2_regs_show, icdc_d2_regs_store);

static int icdc_d2_platform_probe(struct platform_device *pdev)
{
	struct icdc_d2 *icdc_d2 = NULL;
	struct resource *res = NULL;
	int ret = 0;

	icdc_d2 = (struct icdc_d2*)devm_kzalloc(&pdev->dev,
			sizeof(struct icdc_d2), GFP_KERNEL);
	if (!icdc_d2)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Faild to get ioresource mem\n");
		return -ENOENT;
	}

	if (!devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "Failed to request mmio memory region\n");
		return -EBUSY;
	}
	icdc_d2->mapped_resstart = res->start;
	icdc_d2->mapped_ressize = resource_size(res);
	icdc_d2->mapped_base = devm_ioremap_nocache(&pdev->dev,
			icdc_d2->mapped_resstart,
			icdc_d2->mapped_ressize);
	if (!icdc_d2->mapped_base) {
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res)
		return -ENOENT;

	icdc_d2->irqno = res->start;
	icdc_d2->irqflags = res->flags &
		IORESOURCE_IRQ_SHAREABLE ? IRQF_SHARED : 0;
	icdc_d2->dev = &pdev->dev;
	icdc_d2->dac_user_mute = 1;
	icdc_d2->aohp_in_pwsq = 0;
	spin_lock_init(&icdc_d2->io_lock);
	platform_set_drvdata(pdev, (void *)icdc_d2);

	ret = snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_icdc_d2_codec, &icdc_d2_codec_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Faild to register codec\n");
		platform_set_drvdata(pdev, NULL);
		return ret;
	}

	ret = device_create_file(&pdev->dev, &icdc_d2_sysfs_attrs);
	if (ret)
		dev_warn(&pdev->dev,"attribute %s create failed %x",
				attr_name(icdc_d2_sysfs_attrs), ret);
	dev_info(&pdev->dev, "codec icdc-d2 platfrom probe success\n");
	return 0;
}

static int icdc_d2_platform_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "codec icdc-d2 platform remove\n");
	snd_soc_unregister_codec(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver icdc_d2_codec_driver = {
	.driver = {
		.name = "icdc-d2",
		.owner = THIS_MODULE,
	},
	.probe = icdc_d2_platform_probe,
	.remove = icdc_d2_platform_remove,
};

static int icdc_d2_modinit(void)
{
	return platform_driver_register(&icdc_d2_codec_driver);
}
module_init(icdc_d2_modinit);

static void icdc_d2_exit(void)
{
	platform_driver_unregister(&icdc_d2_codec_driver);
}
module_exit(icdc_d2_exit);

MODULE_DESCRIPTION("iCdc d2 Codec Driver");
MODULE_AUTHOR("sccheng<shicheng.cheng@ingenic.com>");
MODULE_LICENSE("GPL");
