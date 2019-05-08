/*
 * sound/soc/ingenic/icodec/icdc_d1.c
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
#include "icdc_d1.h"

static int icdc_d1_debug = 0;
module_param(icdc_d1_debug, int, 0644);
#define DEBUG_MSG(msg...)			\
	do {					\
		if (icdc_d1_debug)		\
			printk(KERN_DEBUG"ICDC: " msg);	\
	} while(0)

static u8 icdc_d1_reg_defcache[DLV_MAX_REG_NUM] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xd3, 0xd3, 0x00, 0x90, 0x00, 0xb0, 0x00, 0x00,
	0x00, 0xb0, 0x30, 0x10, 0x10, 0x00, 0x00, 0x90,
	0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00,
	0x06, 0x06, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00/*0x36*/, 0x00 /*0x07*/,
	/*extend register mixer*/
	0x00, 0x00, 0x00, 0x00,
	/*extend register agc*/
	0x34, 0x34, 0x34, 0x34, 0x00,
};

static int icdc_d1_volatile(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg > DLV_MAX_REG_NUM)
		return 1;

	switch (reg) {
	case DLV_REG_SR:
	case DLV_REG_SR2:
	case DLV_REG_MR:
	case DLV_REG_IFR:
	case DLV_REG_IFR2:
		return 1;
	default:
		return 0;
	}
}

static int icdc_d1_writable(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg > DLV_MAX_REG_NUM)
		return 0;

	switch (reg) {
	case DLV_REG_SR:
	case DLV_REG_SR2:
	case DLV_REG_MR:
	case DLV_REG_DR_MIX:
	case DLV_REG_DR_ADC_AGC:
	case DLV_REG_MISSING_RNG1S ... DLV_REG_MISSING_RNG1E:
	case DLV_REG_MISSING_REG1:
	case DLV_REG_MISSING_REG2:
	case DLV_REG_MISSING_REG3:
	case DLV_REG_MISSING_REG4:
	case DLV_REG_MISSING_REG5:
	case DLV_REG_MISSING_REG6:
	case DLV_REG_MISSING_REG7:
	case DLV_REG_MISSING_REG8:
	case DLV_REG_MISSING_REG9:
	case DLV_REG_MISSING_REG10:
	case DLV_REG_MISSING_REG11:
	case DLV_REG_MISSING_REG12:
		return 0;
	default:
		return 1;
	}
}

static int icdc_d1_readable(struct snd_soc_codec *codec, unsigned int reg)
{
	if (reg > DLV_MAX_REG_NUM)
		return 0;

	switch (reg) {
	case DLV_REG_DR_MIX:
	case DLV_REG_DR_ADC_AGC:
	case DLV_REG_MISSING_RNG1S ... DLV_REG_MISSING_RNG1E:
	case DLV_REG_MISSING_REG1:
	case DLV_REG_MISSING_REG2:
	case DLV_REG_MISSING_REG3:
	case DLV_REG_MISSING_REG4:
	case DLV_REG_MISSING_REG5:
	case DLV_REG_MISSING_REG6:
	case DLV_REG_MISSING_REG7:
	case DLV_REG_MISSING_REG8:
	case DLV_REG_MISSING_REG9:
	case DLV_REG_MISSING_REG10:
	case DLV_REG_MISSING_REG11:
	case DLV_REG_MISSING_REG12:
		return 0;
	default:
		return 1;
	}
}

static void dump_registers_hazard(struct icdc_d1 *icdc_d1)
{
	int reg = 0;
	dev_info(icdc_d1->dev, "-------------------register:");
	for ( ; reg < DLV_MAX_REG_NUM; reg++) {
		if (reg % 8 == 0)
			printk("\n");
		if (icdc_d1_readable(icdc_d1->codec, reg))
			printk(" 0x%02x:0x%02x,", reg, icdc_d1_hw_read(icdc_d1, reg));
		else
			printk(" 0x%02x:0x%02x,", reg, 0x0);
	}
	printk("\n");
	dev_info(icdc_d1->dev, "----------------------------\n");
	return;
}

static int icdc_d1_write(struct snd_soc_codec *codec, unsigned int reg,
			unsigned int value)
{
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;
	BUG_ON(reg > DLV_MAX_REG_NUM);

	if (icdc_d1_writable(codec, reg)) {
		if (!icdc_d1_volatile(codec,reg)) {
			ret = snd_soc_cache_write(codec, reg, value);
			if (ret != 0)
				dev_err(codec->dev, "Cache write to %x failed: %d\n",
						reg, ret);
		}
		return icdc_d1_hw_write(icdc_d1, reg, value);
	}

	return 0;
}

static unsigned int icdc_d1_read(struct snd_soc_codec *codec, unsigned int reg)
{

	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);
	int val = 0, ret = 0;
	BUG_ON(reg > DLV_MAX_REG_NUM);

	if (!icdc_d1_volatile(codec,reg)) {
		ret = snd_soc_cache_read(codec, reg, &val);
		if (ret >= 0)
			return val;
		else
			dev_err(codec->dev, "Cache read from %x failed: %d\n",
					reg, ret);
	}

	if (icdc_d1_readable(codec, reg))
		return icdc_d1_hw_read(icdc_d1, reg);

	return 0;
}

/* DLV4780 CODEC's gain controller init, it's a neccessary program */
static void icdc_d1_reset_gain(struct snd_soc_codec *codec)
{
	int start = DLV_REG_GCR_HPL, end = DLV_REG_GCR_ADCR;
	int i, reg_value;

	for (i = start; i <= end; i++) {
		reg_value = snd_soc_read(codec, i);
		snd_soc_write(codec, i, reg_value);
	}
	return;
}

static int icdc_d1_set_bias_level(struct snd_soc_codec *codec,
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
			icdc_d1_reset_gain(codec);
		}
		snd_soc_update_bits(codec, DLV_REG_AICR_ADC, DLV_AICR_AICR_SB_MASK, 0);
		snd_soc_update_bits(codec, DLV_REG_AICR_DAC, DLV_AICR_AICR_SB_MASK, 0);
		break;
	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec, DLV_REG_AICR_ADC, DLV_AICR_AICR_SB_MASK, DLV_AICR_AICR_SB_MASK);
		snd_soc_update_bits(codec, DLV_REG_AICR_DAC, DLV_AICR_AICR_SB_MASK, DLV_AICR_AICR_SB_MASK);
		snd_soc_update_bits(codec, DLV_REG_CR_VIC, DLV_CR_VIC_SB_SLEEP_MASK, DLV_CR_VIC_SB_SLEEP_MASK);
		snd_soc_update_bits(codec, DLV_REG_CR_VIC, DLV_CR_VIC_SB_MASK, DLV_CR_VIC_SB_MASK);
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static int icdc_d1_hw_params(struct snd_pcm_substream *substream,
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
	int sample_attr[] = {	8000,  11025,  12000, 16000, 22050,
				24000, 32000,  44100, 48000, 88200,
				96000, 176400, 192000	};
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
	for (speed_sel = 0; speed > sample_attr[speed_sel]; speed_sel++)
		;

	snd_soc_update_bits(codec, aicr_reg, DLV_AICR_ADWL_MASK,
			(bit_width_sel << DLV_AICR_ADWL_SHIFT));
	snd_soc_update_bits(codec, fcr_reg, DLV_FCR_FREQ_MASK,
			(speed_sel << DLV_FCR_FREQ_SHIFT));
	return 0;
}

static int icdc_d1_adc_mute_unlock(struct snd_soc_codec *codec, int mute)
{
	int timeout = 0;
	int wait_mute_state =  mute ? DLV_MR_ADC_IN_MUTE : DLV_MR_DAC_NOT_MUTE;

	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_ADC_MUTE_MASK);

	snd_soc_update_bits(codec, DLV_REG_CR_ADC, DLV_CR_ADC_MUTE_MASK,
			(!!mute) << DLV_CR_ADC_MUTE_SHIFT);

	timeout = 5;
	while ((!snd_soc_test_bits(codec, DLV_REG_IFR, DLV_IFR_ADC_MUTE_MASK, 0)) && timeout--) {
		msleep(1);
	}
	if (timeout == 0)
		dev_warn(codec->dev,"wait adc being enter %s state seq timeout\n",
				mute ? "mute" : "unmute");
	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_ADC_MUTE_MASK);

	timeout = 5;
	while (snd_soc_test_bits(codec, DLV_REG_MR, DLV_MR_JADC_MUTE_STATE_MASK,
				wait_mute_state << DLV_MR_JADC_MUTE_STATE_SHIFT) && timeout--) {
		msleep(5);
	}

	if (timeout == 0) {
		dev_err(codec->dev,"wait adc leaving enter %s state seq timeout\n",
				mute ? "mute" : "unmute");
	}
	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_ADC_MUTE_MASK);

	return 0;
}

static int icdc_d1_digital_mute_unlocked(struct snd_soc_codec *codec,
		int mute, bool usr)
{
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);
	int wait_mute_state =  mute ? DLV_MR_DAC_IN_MUTE : DLV_MR_DAC_NOT_MUTE;
	int timeout = 0;

	if (usr)
		icdc_d1->dac_user_mute = mute;

	if (snd_soc_test_bits(codec, DLV_REG_CR_DAC, DLV_CR_DAC_SB_MASK, 0))
		return 0;

	DEBUG_MSG("%s enter real mute %d\n", __func__, mute);

	if (snd_soc_test_bits(codec, DLV_REG_CR_DAC, DLV_CR_DAC_MUTE_MASK, 0) == mute)
		goto out;

	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_DAC_MUTE_MASK);

	snd_soc_update_bits(codec, DLV_REG_CR_DAC, DLV_CR_DAC_MUTE_MASK,
			(!!mute) << DLV_CR_DAC_MUTE_SHIFT);

	timeout = 5;
	while ((!snd_soc_test_bits(codec, DLV_REG_IFR, DLV_IFR_DAC_MUTE_MASK, 0)) && timeout--) {
		mdelay(1);
	}

	if (timeout == 0)
		dev_warn(codec->dev,"wait dac being enter %s state seq timeout\n",
				mute ? "mute" : "unmute");

	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_DAC_MUTE_MASK);

	timeout = 5;
	while (snd_soc_test_bits(codec, DLV_REG_MR, DLV_MR_DAC_MUTE_STATE_MASK,
			wait_mute_state << DLV_MR_DAC_MUTE_STATE_SHIFT) && timeout--) {
		msleep(5);
	}

	if (timeout == 0) {
		dev_err(codec->dev,"wait dac leaving enter %s state seq timeout\n",
				mute ? "mute" : "unmute");
	}
	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_DAC_MUTE_MASK);
out:
	DEBUG_MSG("%s leave real mute %d\n", __func__, mute);

	return 0;
}

static int icdc_d1_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	int ret;

	DEBUG_MSG("%s enter mute %d\n", __func__, mute);
	ret = icdc_d1_digital_mute_unlocked(codec, mute, true);
	return ret;
}

static int icdc_d1_trigger(struct snd_pcm_substream * stream, int cmd,
		struct snd_soc_dai *dai)
{
#ifdef DEBUG
	struct snd_soc_codec *codec = dai->codec;
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		dump_registers_hazard(icdc_d1);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		dump_registers_hazard(icdc_d1);
		break;
	}
#endif
	return 0;
}

#define DLV4780_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S18_3LE | \
			 SNDRV_PCM_FMTBIT_S20_3LE |SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops icdc_d1_dai_ops = {
	.hw_params	= icdc_d1_hw_params,
	.digital_mute	= icdc_d1_digital_mute,
	.trigger = icdc_d1_trigger,
};

static struct snd_soc_dai_driver  icdc_d1_codec_dai = {
	.name = "icdc-d1-hifi",
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
	.ops = &icdc_d1_dai_ops,
};

/* unit: 0.01dB */
static const DECLARE_TLV_DB_SCALE(dac_tlv, -3100, 100, 0);
static const DECLARE_TLV_DB_SCALE(adc_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(diff_out_tlv, -2500, 100, 0);
static const DECLARE_TLV_DB_SCALE(bypassl_tlv, -2500, 100, 0);
static const DECLARE_TLV_DB_SCALE(bypassr_tlv, -2500, 100, 0);
static const DECLARE_TLV_DB_SCALE(mic1_tlv, 0, 400, 0);
static const DECLARE_TLV_DB_SCALE(mic2_tlv, 0, 400, 0);
static const char *icdc_d1_ail_sel[] = {"AIPN1", "AIP2"};
static const char *icdc_d1_adc_sel[] = {"AIL/L", "AIR/R", "AIL/R", "AIR/L"};
static const char *icdc_d1_ailatt_sel[] = {"AIPN1", "AIP2"};
static const char *icdc_d1_aohp_sel[] = {"DACL/R", "DACL", "AIL/Ratt" ,"AILatt", "AIRatt"};
static const char *icdc_d1_aolo_sel[] = {"DACL", "DACR", "DACL/R", "AILatt", "AIRatt", "AIL/Ratt"};
static const char *icdc_d1_adc_wnf[] = {"Inactive", "Mode1", "Mode2", "Mode3"};
static const char *icdc_d1_aohp_vir_sel[] = { "HP-ON", "HP-OFF"};
static const char *icdc_d1_aolo_vir_sel[] = { "LO-ON", "LO-OFF"};

static const unsigned int icdc_d1_aolo_sel_value[] = {0x0, 0x1, 0x2, 0x4, 0x5, 0x6};
static const unsigned int icdc_d1_adc_sel_value[] = {0x0, 0x1, 0x0, 0x1};

static const struct soc_enum icdc_d1_enum[] = {
	SOC_ENUM_SINGLE(DLV_REG_CR_MIC1, 0, ARRAY_SIZE(icdc_d1_ail_sel), icdc_d1_ail_sel),
	SOC_VALUE_ENUM_SINGLE(DLV_REG_CR_ADC, 0, 0x3,  ARRAY_SIZE(icdc_d1_adc_sel),
			icdc_d1_adc_sel, icdc_d1_adc_sel_value),
	SOC_ENUM_SINGLE(DLV_REG_CR_LI1, 0, ARRAY_SIZE(icdc_d1_ailatt_sel), icdc_d1_ailatt_sel),
	SOC_ENUM_SINGLE(DLV_REG_CR_HP, 0, ARRAY_SIZE(icdc_d1_aohp_sel), icdc_d1_aohp_sel),
	SOC_VALUE_ENUM_SINGLE(DLV_REG_CR_LO, 0, 0x7, ARRAY_SIZE(icdc_d1_aolo_sel),
			icdc_d1_aolo_sel, icdc_d1_aolo_sel_value),
	SOC_ENUM_SINGLE(DLV_REG_FCR_ADC, 4, ARRAY_SIZE(icdc_d1_adc_wnf), icdc_d1_adc_wnf),
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(icdc_d1_aohp_vir_sel), icdc_d1_aohp_vir_sel),
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0, ARRAY_SIZE(icdc_d1_aolo_vir_sel), icdc_d1_aolo_vir_sel),
};

static const struct snd_kcontrol_new icdc_d1_snd_controls[] = {
	/* playback gain control */
	SOC_DOUBLE_R_TLV("Digital Playback Volume", DLV_REG_GCR_DACL, DLV_REG_GCR_DACR, 0, 31, 1, dac_tlv),
	SOC_DOUBLE_R_TLV("Headphone Volume", DLV_REG_GCR_HPL, DLV_REG_GCR_HPR,
			 0, 31, 1, diff_out_tlv),
	/* record gain control */
	SOC_DOUBLE_R_TLV("Digital Capture Volume", DLV_REG_GCR_ADCL, DLV_REG_GCR_ADCR, 0, 23, 0, adc_tlv),
	SOC_SINGLE_TLV("Mic1 Volume", DLV_REG_GCR_MIC1, 0, 5, 0, mic1_tlv),
	SOC_SINGLE_TLV("Mic2 Volume", DLV_REG_GCR_MIC2, 0, 5, 0, mic2_tlv),
	SOC_SINGLE_TLV("Bypass1 Volume", DLV_REG_GCR_LIBYL, 0, 31, 1, bypassl_tlv),
	SOC_SINGLE_TLV("Bypass2 Volume", DLV_REG_GCR_LIBYR, 0, 31, 1, bypassr_tlv),
	SOC_SINGLE("ADC High Pass Filter Switch", DLV_REG_FCR_ADC, 6, 1, 0),
	SOC_ENUM("ADC Wind Noise Filter Switch", icdc_d1_enum[5]),
	SOC_SINGLE("Mic1 Diff Switch", DLV_REG_CR_MIC1, 6, 1, 0),
	SOC_SINGLE("Mic Stereo Switch", DLV_REG_CR_MIC1, 7, 1, 0),
	SOC_SINGLE("ADC Stereo Switch", DLV_REG_CR_ADC, 5, 1, 1),
	SOC_SINGLE("Bypass1 Diff Switch", DLV_REG_CR_LI1, 6, 1, 0),
	SOC_SINGLE("Mic1 Bias Switch", DLV_REG_CR_MIC1, 5, 1, 1),	/*some times we want open micbias first*/
	SOC_SINGLE("Mic2 Bias Switch", DLV_REG_CR_MIC2, 5, 1, 1),
};

static const struct snd_kcontrol_new icdc_d1_ail_mux_controls =
	SOC_DAPM_ENUM("Route", icdc_d1_enum[0]);

static const struct snd_kcontrol_new icdc_d1_adc_mux_controls =
	SOC_DAPM_VALUE_ENUM("Route",  icdc_d1_enum[1]);

static const struct snd_kcontrol_new icdc_d1_ailatt_mux_controls =
	SOC_DAPM_ENUM("Route", icdc_d1_enum[2]);

static const struct snd_kcontrol_new icdc_d1_aohp_mux_controls =
	SOC_DAPM_ENUM("Route", icdc_d1_enum[3]);

static const struct snd_kcontrol_new icdc_d1_aolo_mux_controls =
	SOC_DAPM_VALUE_ENUM("Route", icdc_d1_enum[4]);

static const struct snd_kcontrol_new icdc_d1_aohp_vmux_controls =
	SOC_DAPM_ENUM_VIRT("Route", icdc_d1_enum[6]);

static const struct snd_kcontrol_new icdc_d1_aolo_vmux_controls =
	SOC_DAPM_ENUM_VIRT("Route", icdc_d1_enum[7]);

static int icdc_d1_wait_hp_mode_unlocked(struct snd_soc_codec *codec)
{
	int timeout = 5;
	DEBUG_MSG("%s enter\n", __func__);

	while ((!snd_soc_test_bits(codec, DLV_REG_IFR, DLV_IFR_DAC_MODE_MASK, 0)) && timeout--)
		msleep(1);

	if (timeout == 0)
		dev_warn(codec->dev,"wait dac mode state timeout\n");
	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_DAC_MODE_MASK);

	timeout = 10;		/*takes 700ms*/
	while (snd_soc_test_bits(codec, DLV_REG_MR, DLV_MR_DAC_MODE_STATE_MASK,
			DLV_MR_DAC_PGMED << DLV_MR_DAC_MODE_STATE_SHIFT) && timeout--) {
		msleep(100);
	}

	if (timeout == 0)
		dev_warn(codec->dev,"wait dac mode state timeout\n");

	snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_DAC_MODE_MASK);
	DEBUG_MSG("%s leave\n", __func__);
	return 0;
}

static int icdc_d1_aohp_anti_pop_event_sub(struct snd_soc_codec *codec,
		int event) {
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);
	DEBUG_MSG("%s enter %d\n", __func__ , event);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (snd_soc_test_bits(codec, DLV_REG_CR_HP,
					DLV_CR_HP_SB_MASK, 0)) {
			icdc_d1->aohp_in_pwsq = 1;
			udelay(500);

			/*dac mute*/
			icdc_d1_digital_mute_unlocked(codec, 1 , false);

			/*hp unmute*/
			snd_soc_update_bits(codec, DLV_REG_CR_HP, DLV_CR_HP_MUTE_MASK, 0);

			/*hp wished to +6db*/
			icdc_d1->hpl_wished_gain =
				snd_soc_read(codec, DLV_REG_GCR_HPL);
			icdc_d1->hpr_wished_gain =
				snd_soc_read(codec, DLV_REG_GCR_HPR);
			snd_soc_write(codec, DLV_REG_GCR_HPL, 0);
			snd_soc_write(codec, DLV_REG_GCR_HPR, 0);

			/*clear ifr
			snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_DAC_MODE_MASK);*/
			mdelay(1);
			printk(KERN_DEBUG"codec mr before1 set hp sb %x ifr %x\n",
					snd_soc_read(codec, DLV_REG_MR),
					snd_soc_read(codec, DLV_REG_IFR));
		}
		break;
	case SND_SOC_DAPM_POST_PMU:
		if (icdc_d1->aohp_in_pwsq) {

			/*wait aohp power on*/
			icdc_d1_wait_hp_mode_unlocked(codec);

			/*hp +6db to wished :ingnore hp gain set when aohp power up seq*/
			snd_soc_write(codec, DLV_REG_GCR_HPL, icdc_d1->hpl_wished_gain);
			snd_soc_write(codec, DLV_REG_GCR_HPR, icdc_d1->hpr_wished_gain);

			/*user mute avail*/
			icdc_d1_digital_mute_unlocked(codec, icdc_d1->dac_user_mute, false);

			icdc_d1->aohp_in_pwsq = 0;
		}
		break;
	case SND_SOC_DAPM_PRE_PMD:
		if (!snd_soc_test_bits(codec, DLV_REG_CR_HP,
					DLV_CR_HP_SB_MASK, 0)) {
			icdc_d1->aohp_in_pwsq = 1;

			/*dac mute*/
			icdc_d1_digital_mute_unlocked(codec, 1, false);

			/*hp wished to +6db*/
			icdc_d1->hpl_wished_gain =
				snd_soc_read(codec, DLV_REG_GCR_HPL);
			icdc_d1->hpr_wished_gain =
				snd_soc_read(codec, DLV_REG_GCR_HPR);
			snd_soc_write(codec, DLV_REG_GCR_HPL, 0);
			snd_soc_write(codec, DLV_REG_GCR_HPR, 0);

			/*clear ifr*/
			//snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_DAC_MODE_MASK);
			printk(KERN_DEBUG"codec mr before1 set hp sb %x ifr %x\n",
					snd_soc_read(codec, DLV_REG_MR),
					snd_soc_read(codec, DLV_REG_IFR));
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (icdc_d1->aohp_in_pwsq) {
			/*wait aohp power down*/
			icdc_d1_wait_hp_mode_unlocked(codec);

			/*hp mute*/
			snd_soc_update_bits(codec, DLV_REG_CR_HP, DLV_CR_HP_MUTE_MASK, DLV_CR_HP_MUTE_MASK);

			/*hp +6db to wished :ingnore hp gain set when aohp power up seq*/
			snd_soc_write(codec, DLV_REG_GCR_HPL, icdc_d1->hpl_wished_gain);
			snd_soc_write(codec, DLV_REG_GCR_HPR, icdc_d1->hpr_wished_gain);

			/*user mute avail*/
			icdc_d1_digital_mute_unlocked(codec, icdc_d1->dac_user_mute, false);
			icdc_d1->aohp_in_pwsq = 0;
		}
		break;
	}

	return 0;
}

static int icdc_d1_aohp_anti_pop_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event) {
	struct snd_soc_codec *codec = w->codec;
	icdc_d1_aohp_anti_pop_event_sub(codec, event);
	return 0;
}

static int icdc_d1_adc_unmute_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) {

	struct snd_soc_codec *codec = w->codec;
	int mute;
	switch (event) {
		case SND_SOC_DAPM_POST_PMU:
			mute = 0;
			break;
		case SND_SOC_DAPM_PRE_PMD:
			mute = 1;
			break;
		default:
			return 0;
	}
	return icdc_d1_adc_mute_unlock(codec, mute);
}

static const struct snd_soc_dapm_widget icdc_d1_dapm_widgets[] = {
	SND_SOC_DAPM_ADC_E("ADC", "Capture", DLV_REG_CR_ADC, 4, 1, icdc_d1_adc_unmute_event,
			SND_SOC_DAPM_POST_PMU|SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_MUX("ADC Mux", SND_SOC_NOPM, 0, 0, &icdc_d1_adc_mux_controls),
	SND_SOC_DAPM_DAC("DAC", "Playback", DLV_REG_CR_DAC, 4, 1),
	SND_SOC_DAPM_PGA_E("AOHP", DLV_REG_CR_HP, 4, 1, NULL, 0,
			icdc_d1_aohp_anti_pop_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MUX("AOHP Mux", SND_SOC_NOPM, 0, 0, &icdc_d1_aohp_mux_controls),
	SND_SOC_DAPM_MUX("AOLO Mux", DLV_REG_CR_LO, 4, 1, &icdc_d1_aolo_mux_controls),
	SND_SOC_DAPM_VIRT_MUX("AOHP Vmux", SND_SOC_NOPM, 0, 0, &icdc_d1_aohp_vmux_controls),
	SND_SOC_DAPM_VIRT_MUX("AOLO Vmux", SND_SOC_NOPM, 0, 0, &icdc_d1_aolo_vmux_controls),
	SND_SOC_DAPM_MUX("AIL Mux", DLV_REG_CR_MIC1, 4, 1, &icdc_d1_ail_mux_controls),
	SND_SOC_DAPM_PGA("AIR", DLV_REG_CR_MIC2, 4, 1, NULL, 0),
	SND_SOC_DAPM_MUX("AILatt Mux", DLV_REG_CR_LI1, 4, 1, &icdc_d1_ailatt_mux_controls),
	SND_SOC_DAPM_PGA("AIRatt", DLV_REG_CR_LI2, 4, 1, NULL, 0),
	SND_SOC_DAPM_MICBIAS("MICBIAS1", DLV_REG_CR_MIC1, 5, 1),
	SND_SOC_DAPM_MICBIAS("MICBIAS2", DLV_REG_CR_MIC2, 5, 1),
	/*SND_SOC_DAPM_MIC("DMIC_IN", dmic_en_event),*/
	/*SND_SOC_DAPM_MIXER();*/
	/*SND_SOC_DAPM_MIXER();*/
	/*SND_SOC_DAPM_MIXER();*/
	/*SND_SOC_DAPM_MIXER();*/
	SND_SOC_DAPM_OUTPUT("AOHPL"),
	SND_SOC_DAPM_OUTPUT("AOHPR"),
	SND_SOC_DAPM_OUTPUT("AOLOP"),
	SND_SOC_DAPM_OUTPUT("AOLON"),
	SND_SOC_DAPM_INPUT("AIP1"),
	SND_SOC_DAPM_INPUT("AIN1"),
	SND_SOC_DAPM_INPUT("AIP2"),
	SND_SOC_DAPM_INPUT("AIP3"),
};

static const struct snd_soc_dapm_route intercon[] = {
	/*input*/
	{ "AIL Mux", "AIPN1", "AIP1" },
	{ "AIL Mux", "AIPN1", "AIN1" },
	{ "AIL Mux", "AIP2", "AIP2" },
	{ "AIR", NULL, "AIP3"},

	{ "ADC Mux", "AIR/R", "AIR"  },
	{ "ADC Mux", "AIL/R", "AIR"  },
	{ "ADC Mux", "AIR/L", "AIR"  },
	{ "ADC Mux", "AIL/L", "AIL Mux"},
	{ "ADC Mux", "AIL/R", "AIL Mux"},
	{ "ADC Mux", "AIR/L", "AIL Mux"},

	{ "ADC", NULL, "ADC Mux"},

        { "AILatt Mux", "AIPN1", "AIP1" },
        { "AILatt Mux", "AIPN1", "AIN1" },
        { "AILatt Mux", "AIP2", "AIP2" },
	{ "AIRatt", NULL, "AIP3"},

	/*bypass*/
	{ "AOHP Mux", "AIL/Ratt", "AILatt Mux"},
	{ "AOHP Mux", "AILatt", "AILatt Mux"},
	{ "AOHP Mux", "AIL/Ratt", "AIRatt"},
	{ "AOHP Mux", "AIRatt", "AIRatt"},

	{ "AOLO Mux", "AILatt", "AILatt Mux"},
	{ "AOLO Mux", "AIL/Ratt", "AILatt Mux"},
	{ "AOLO Mux", "AIRatt", "AIRatt"},
	{ "AOLO Mux", "AIL/Ratt", "AIRatt"},

	/*output*/

	{ "AOHP Mux", "DACL/R", "DAC"},
	{ "AOHP Mux", "DACL", "DAC"},

	{ "AOHP",  NULL, "AOHP Mux"},
	{ "AOHP Vmux", "HP-ON", "AOHP"},

	{ "AOHPR", NULL, "AOHP Vmux"},
	{ "AOHPL", NULL, "AOHP Vmux"},

	{ "AOLO Mux", "DACL", "DAC"},
	{ "AOLO Mux", "DACR", "DAC"},
	{ "AOLO Mux", "DACL/R", "DAC"},

	{ "AOLO Vmux", "LO-ON", "AOLO Mux"},

	{ "AOLOP", NULL, "AOLO Vmux"},
	{ "AOLON", NULL, "AOLO Vmux"}
};

static int icdc_d1_suspend(struct snd_soc_codec *codec)
{
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);

	icdc_d1_set_bias_level(codec, SND_SOC_BIAS_OFF);
	snd_soc_update_bits(codec, DLV_REG_CR_CK, DLV_CR_CK_SB_MASK, DLV_CR_CK_SB_MASK);
	if (icdc_d1_debug)
		dump_registers_hazard(icdc_d1);
	return 0;
}

static int icdc_d1_resume(struct snd_soc_codec *codec)
{
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);

	snd_soc_update_bits(codec, DLV_REG_CR_CK, DLV_CR_CK_SB_MASK, 0);
	icdc_d1_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	if (icdc_d1_debug)
		dump_registers_hazard(icdc_d1);
	return 0;
}

/*
 * CODEC short circut handler
 *
 * To protect CODEC, CODEC will be shutdown when short circut occured.
 * Then we have to restart it.
 */
static inline void icdc_d1_short_circut_handler(struct icdc_d1 *icdc_d1)
{
	struct snd_soc_codec *codec = icdc_d1->codec;
	int hp_is_on;
	int bias_level;

	/*make sure hp power on/down seq was completed*/
	while (1) {
		if (!icdc_d1->aohp_in_pwsq)
			break;
		msleep(10);
	};

	/*shut down codec seq*/
	if (!(snd_soc_read(codec, DLV_REG_CR_HP) & DLV_CR_HP_SB_MASK)) {
		hp_is_on = 1;
		icdc_d1_aohp_anti_pop_event_sub(codec , SND_SOC_DAPM_PRE_PMD);
		snd_soc_update_bits(codec, DLV_REG_CR_HP, DLV_CR_HP_SB_MASK, 1);
		icdc_d1_aohp_anti_pop_event_sub(codec , SND_SOC_DAPM_POST_PMD);
	}

	bias_level =  codec->dapm.bias_level;

	icdc_d1_set_bias_level(codec, SND_SOC_BIAS_OFF);

	msleep(10);

	/*power on codec seq*/
	icdc_d1_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	icdc_d1_set_bias_level(codec, bias_level);

	if (hp_is_on) {
		icdc_d1_aohp_anti_pop_event_sub(codec , SND_SOC_DAPM_PRE_PMU);
		snd_soc_update_bits(codec, DLV_REG_CR_HP, DLV_CR_HP_SB_MASK, 0);
		icdc_d1_aohp_anti_pop_event_sub(codec , SND_SOC_DAPM_POST_PMU);
	}
	return;
}

int icdc_d1_hp_detect(struct snd_soc_codec *codec, struct snd_soc_jack *jack,
		int type)
{
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);
	int report = 0;

	icdc_d1->jack = jack;
	icdc_d1->report_mask = type;

	if (jack) {
		snd_soc_update_bits(codec, DLV_REG_IMR, DLV_IMR_JACK, 0);
		/*initial headphone detect*/
#ifdef CONFIG_JZ_ASOC_CODEC_HP_INSERT_REV
		if (!(snd_soc_read(codec, DLV_REG_SR) & DLV_SR_JACK_MASK)) {
#else
		if (!!(snd_soc_read(codec, DLV_REG_SR) & DLV_SR_JACK_MASK)) {
#endif
			report = icdc_d1->report_mask;
			dev_info(codec->dev, "codec initial headphone detect --> headphone was inserted\n");
			snd_soc_jack_report(icdc_d1->jack, report, icdc_d1->report_mask);
		}
	} else {
		snd_soc_update_bits(codec, DLV_REG_IMR, DLV_IMR_JACK, DLV_IMR_JACK);
	}
	return 0;
}
EXPORT_SYMBOL_GPL(icdc_d1_hp_detect);

/*
 * CODEC work queue handler
 */
static void icdc_d1_irq_work_handler(struct work_struct *work)
{
	struct icdc_d1 *icdc_d1 = (struct icdc_d1 *)container_of(work, struct icdc_d1, irq_work);
	struct snd_soc_codec *codec = icdc_d1->codec;
	unsigned int icdc_d1_ifr, icdc_d1_imr;

	icdc_d1_imr = icdc_d1->codec_imr;
	icdc_d1_ifr = snd_soc_read(codec, DLV_REG_IFR);

	/*short protect interrupt*/
	if ((icdc_d1_ifr & (~icdc_d1_imr)) & DLV_IFR_SCLR_MASK) {
		mutex_lock(&codec->mutex);
		do {
			dev_warn(codec->dev, "codec short circut protect !!!\n");
			icdc_d1_short_circut_handler(icdc_d1);
			snd_soc_write(codec, DLV_REG_IFR, DLV_IFR_SCLR_MASK);
		} while (snd_soc_read(codec, DLV_REG_IFR) & DLV_IFR_SCLR_MASK);
		dev_warn(codec->dev, "codec short circut protect ok!!!\n");
		mutex_unlock(&codec->mutex);
	}

	/*hp detect interrupt*/
	if ((icdc_d1_ifr & (~icdc_d1_imr)) & DLV_IFR_JACK_MASK) {
		int icdc_d1_jack = 0;
		int report = 0;
		msleep(200);
#ifdef CONFIG_JZ_ASOC_CODEC_HP_INSERT_REV
		icdc_d1_jack = !(snd_soc_read(codec, DLV_REG_SR) & DLV_SR_JACK_MASK);
#else
		icdc_d1_jack = !!(snd_soc_read(codec, DLV_REG_SR) & DLV_SR_JACK_MASK);
#endif
		dev_info(codec->dev, "codec headphone detect %s\n",
				icdc_d1_jack ? "insert" : "pull out");
		snd_soc_write(codec, DLV_REG_IFR, DLV_SR_JACK_MASK);
		if (icdc_d1_jack)
			report = icdc_d1->report_mask;
		snd_soc_jack_report(icdc_d1->jack, report,
				icdc_d1->report_mask);
	}
	snd_soc_write(codec,DLV_REG_IMR, icdc_d1_imr);
}

/*
 * IRQ routine
 */
static irqreturn_t dlv_codec_irq(int irq, void *dev_id)
{
	struct icdc_d1 *icdc_d1 = (struct icdc_d1 *)dev_id;
	struct snd_soc_codec *codec = icdc_d1->codec;
	if (icdc_d1_test_irq(icdc_d1)) {
		/*disable interrupt*/
		icdc_d1->codec_imr = icdc_d1_hw_read_normal(icdc_d1, DLV_REG_IMR);
		icdc_d1_hw_write_normal(icdc_d1, DLV_REG_IMR, ICR_IMR_MASK);

		if(!work_pending(&icdc_d1->irq_work))
			schedule_work(&icdc_d1->irq_work);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

static int icdc_d1_probe(struct snd_soc_codec *codec)
{
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	dev_info(codec->dev, "codec icdc-d1 probe enter\n");

	/*power on codec*/
	icdc_d1_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

#ifdef DEBUG
	/*dump for debug*/
	dump_registers_hazard(icdc_d1);
#endif
	/* codec select enable 12M clock*/
	snd_soc_update_bits(codec, DLV_REG_CR_CK, DLV_CR_CK_SEL_MASK,
			DLV_CR_CK_SEL_12M << DLV_CR_CK_SEL_SHIFT);
	snd_soc_update_bits(codec, DLV_REG_CR_CK, DLV_CR_CK_SB_MASK, 0);

	/*codec select master mode*/
	snd_soc_update_bits(codec, DLV_REG_AICR_ADC, DLV_AICR_MODE_MASK,
			DLV_AICR_MODE_MASTER << DLV_AICR_MODE_SHIFT);

	/*codec select i2s interface*/
	snd_soc_update_bits(codec, DLV_REG_AICR_ADC, DLV_AICR_AUDIOIF_MASK,
			DLV_AICR_AUDIOIF_I2S << DLV_AICR_AUDIOIF_SHIFT);
	snd_soc_update_bits(codec, DLV_REG_AICR_DAC, DLV_AICR_AUDIOIF_MASK,
			DLV_AICR_AUDIOIF_I2S << DLV_AICR_AUDIOIF_SHIFT);

	/*codec mixer in input normal default*/
	snd_soc_write(codec, DLV_EXREG_MIX0, 0x50);	/*AIDACX_SEL should be configured to 01 in normal mode*/
	snd_soc_write(codec, DLV_EXREG_MIX2, 0x10);	/*MIXADCX_SEL should be configured to 01 in normal mode*/

	/*codec generated IRQ is a high level */
	snd_soc_update_bits(codec, DLV_REG_ICR, DLV_ICR_INT_FORM_MASK, 0);

	/*codec irq mask*/
	snd_soc_write(codec, DLV_REG_IMR, ICR_IMR_COMMON_MSK);

	/*codec clear all irq*/
	snd_soc_write(codec, DLV_REG_IFR, ICR_IMR_MASK);

	/*unmute aolo*/
	snd_soc_update_bits(codec, DLV_REG_CR_LO, (1 << 7), 0);

	INIT_WORK(&icdc_d1->irq_work, icdc_d1_irq_work_handler);
	ret = request_irq(icdc_d1->irqno, dlv_codec_irq, icdc_d1->irqflags,
			"audio_codec_irq", (void *)icdc_d1);
	if (ret)
		dev_warn(codec->dev, "Failed to request aic codec irq %d\n", icdc_d1->irqno);

	icdc_d1->codec = codec;
	dev_info(codec->dev, "codec icdc-d1 probe success!!!\n");
	return 0;
}

static int icdc_d1_remove(struct snd_soc_codec *codec)
{
	struct icdc_d1 *icdc_d1 = snd_soc_codec_get_drvdata(codec);
	dev_info(codec->dev, "codec icdc_d1 remove enter\n");
	if (icdc_d1 && icdc_d1->irqno != -1) {
		free_irq(icdc_d1->irqno, (void *)icdc_d1);
	}
	icdc_d1_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_icdc_d1_codec = {
	.probe = 	icdc_d1_probe,
	.remove = 	icdc_d1_remove,
	.suspend =	icdc_d1_suspend,
	.resume =	icdc_d1_resume,

	.read = 	icdc_d1_read,
	.write = 	icdc_d1_write,
	.volatile_register = icdc_d1_volatile,
	.readable_register = icdc_d1_readable,
	.writable_register = icdc_d1_writable,
	.reg_cache_default = icdc_d1_reg_defcache,
	.reg_word_size = sizeof(u8),
	.reg_cache_step = 1,
	.reg_cache_size = DLV_MAX_REG_NUM,
	.set_bias_level = icdc_d1_set_bias_level,

	.controls = 	icdc_d1_snd_controls,
	.num_controls = ARRAY_SIZE(icdc_d1_snd_controls),
	.dapm_widgets = icdc_d1_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(icdc_d1_dapm_widgets),
	.dapm_routes = intercon,
	.num_dapm_routes = ARRAY_SIZE(intercon),
};

/*Just for debug*/
static ssize_t icdc_d1_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct icdc_d1 *icdc_d1 = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = icdc_d1->codec;
	if (!codec) {
		dev_info(dev, "icdc_d1 is not probe, can not use %s function\n", __func__);
		return 0;
	}
	mutex_lock(&codec->mutex);
	dump_registers_hazard(icdc_d1);
	mutex_unlock(&codec->mutex);
	return 0;
}

static ssize_t icdc_d1_regs_store(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	struct icdc_d1 *icdc_d1 = dev_get_drvdata(dev);
	struct snd_soc_codec *codec = icdc_d1->codec;
	const char *start = buf;
	unsigned int reg, val;
	int ret_count = 0;

	if (!codec) {
		dev_info(dev, "icdc_d1 is not probe, can not use %s function\n", __func__);
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
	icdc_d1_write(codec, reg, val);
	mutex_unlock(&codec->mutex);
	return count;
}

static struct device_attribute icdc_d1_sysfs_attrs =
	__ATTR(hw_regs, S_IRUGO|S_IWUGO, icdc_d1_regs_show, icdc_d1_regs_store);

static int icdc_d1_platform_probe(struct platform_device *pdev)
{
	struct icdc_d1 *icdc_d1 = NULL;
	struct resource *res = NULL;
	int ret = 0;

	icdc_d1 = (struct icdc_d1*)devm_kzalloc(&pdev->dev,
			sizeof(struct icdc_d1), GFP_KERNEL);
	if (!icdc_d1)
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
	icdc_d1->mapped_resstart = res->start;
	icdc_d1->mapped_ressize = resource_size(res);
	icdc_d1->mapped_base = devm_ioremap_nocache(&pdev->dev,
			icdc_d1->mapped_resstart,
			icdc_d1->mapped_ressize);
	if (!icdc_d1->mapped_base) {
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res)
		return -ENOENT;

	icdc_d1->irqno = res->start;
	icdc_d1->irqflags = res->flags &
		IORESOURCE_IRQ_SHAREABLE ? IRQF_SHARED : 0;
	icdc_d1->dev = &pdev->dev;
	icdc_d1->dac_user_mute = 1;
	icdc_d1->aohp_in_pwsq = 0;
	icdc_d1->hpl_wished_gain = 0;
	icdc_d1->hpr_wished_gain = 0;
	spin_lock_init(&icdc_d1->io_lock);
	platform_set_drvdata(pdev, (void *)icdc_d1);

	ret = snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_icdc_d1_codec, &icdc_d1_codec_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Faild to register codec\n");
		platform_set_drvdata(pdev, NULL);
		return ret;
	}

	ret = device_create_file(&pdev->dev, &icdc_d1_sysfs_attrs);
	if (ret)
		dev_warn(&pdev->dev,"attribute %s create failed %x",
				attr_name(icdc_d1_sysfs_attrs), ret);
	dev_info(&pdev->dev, "codec icdc-d1 platfrom probe success\n");
	return 0;
}

static int icdc_d1_platform_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "codec icdc-d1 platform remove\n");
	snd_soc_unregister_codec(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver icdc_d1_codec_driver = {
	.driver = {
		.name = "icdc-d1",
		.owner = THIS_MODULE,
	},
	.probe = icdc_d1_platform_probe,
	.remove = icdc_d1_platform_remove,
};

static int icdc_d1_modinit(void)
{
	return platform_driver_register(&icdc_d1_codec_driver);
}
module_init(icdc_d1_modinit);

static void icdc_d1_exit(void)
{
	platform_driver_unregister(&icdc_d1_codec_driver);
}
module_exit(icdc_d1_exit);

MODULE_DESCRIPTION("iCdc D1 Codec Driver");
MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_LICENSE("GPL");
