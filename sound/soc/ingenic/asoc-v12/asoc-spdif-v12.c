/*
 *  sound/soc/ingenic/asoc-spdif.c
 *  ALSA Soc Audio Layer -- ingenic spdif (part of aic controller) driver
 *
 *  Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *     cscheng <shicheng.cheng@ingenic.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/slab.h>
#include "asoc-aic-v12.h"
static int jz_spdif_debug = 0;
module_param(jz_spdif_debug, int, 0644);
#define spdif_DEBUG_MSG(msg...)			\
	do {					\
		if (jz_spdif_debug)		\
			printk("SPDIF: " msg);	\
	} while(0)

struct jz_spdif {
	struct device *aic;
	struct jz_pcm_dma_params tx_dma_data;
};

struct clk {
	const char *name;
	unsigned long rate;
	struct clk *parent;
	unsigned long flags;
	struct clk_ops *ops;
	int count;
	struct clk *source;
};

#define SPDIF_TFIFO_DEPTH 32
#define JZ_SPDIF_FORMATS (SNDRV_PCM_FMTBIT_S24_LE|SNDRV_PCM_FMTBIT_S16_LE)
#define JZ_SPDIF_RATE (SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000|SNDRV_PCM_RATE_96000|SNDRV_PCM_RATE_192000)

static void dump_registers(struct device *aic)
{
	struct jz_aic *jz_aic = dev_get_drvdata(aic);

	pr_info("AIC_FR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICFR),jz_aic_read_reg(aic, AICFR));
	pr_info("AIC_CR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICCR),jz_aic_read_reg(aic, AICCR));
	pr_info("AIC_SPCTRL\t%p : 0x%08x\n",(jz_aic->vaddr_base+SPCTRL),jz_aic_read_reg(aic, SPCTRL));
	pr_info("AIC_SR\t\t%p : 0x%08x\n", (jz_aic->vaddr_base+AICSR),jz_aic_read_reg(aic, AICSR));
	pr_info("AIC_SPSTATE\t%p : 0x%08x\n",(jz_aic->vaddr_base+SPSTATE),jz_aic_read_reg(aic, SPSTATE));
	pr_info("AIC_SPCFG1\t%p : 0x%08x\n",(jz_aic->vaddr_base+SPCFG1),jz_aic_read_reg(aic, SPCFG1));
	pr_info("AIC_SPCFG2\t%p : 0x%08x\n",(jz_aic->vaddr_base+SPCFG2),jz_aic_read_reg(aic, SPCFG2));
	return;
}

static int jz_spdif_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_spdif *jz_spdif = dev_get_drvdata(dai->dev);
	struct device *aic = jz_spdif->aic;
	enum aic_mode work_mode = AIC_SPDIF_MODE;

	spdif_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
	work_mode = aic_set_work_mode(aic, work_mode, true);
	if (work_mode != AIC_SPDIF_MODE) {
		dev_warn(jz_spdif->aic, "Aic now is working on %s mode, open spdif mode failed\n",
				aic_work_mode_str(work_mode));
		return -EPERM;
	}
	printk("start set AIC register....\n");
	return 0;
}

static unsigned long calculate_cgu_aic_rate(unsigned long parent_clk ,unsigned long *rate)
{
       unsigned long i2s_parent_clk;
       unsigned int i2s_dv = 0, i2scdr = 0, div_exp = 0, div_tmp = 0;
	   unsigned int i2s_dv_tmp = 512,i2scdr_tmp = 256;
       i2s_parent_clk = parent_clk;
       if (i2s_parent_clk < (*rate * 64)) {
               pr_err("SPDIF:i2s parent clk rate is error, "
                               "please chose right\n");
               return i2s_parent_clk;
       }

	   div_exp = i2s_parent_clk/(*rate*64);
	   div_tmp = div_exp;

		for(i2scdr = 10 ; i2scdr <= 256;i2scdr++)
			for(i2s_dv = 1; i2s_dv <= 512; i2s_dv++)
				if(( abs((i2scdr*i2s_dv) - div_exp) ) < div_tmp){
					div_tmp = i2scdr*i2s_dv - div_exp;
					i2s_dv_tmp = i2s_dv;
					i2scdr_tmp = i2scdr;
				}

       return i2s_parent_clk / (i2scdr_tmp) ;
}


static unsigned long  spdif_set_clk(struct jz_aic* jz_aic, unsigned long sys_clk, unsigned long sync){
	unsigned long tmp_val;
	int div = sys_clk/(64*sync);
	if(sys_clk%(64*sync) >= (32*sync))
		div++;

	tmp_val = readl(jz_aic->vaddr_base + I2SDIV);
	tmp_val &= ~I2SDIV_DV_MASK;
	tmp_val |= (div-1);
	writel(tmp_val,jz_aic->vaddr_base + I2SDIV);

	return sys_clk/(64*div);
}

static unsigned long  spdif_select_ori_sample_freq(struct device *aic,unsigned long sync)
{
	int div = 0;
	switch(sync) {
		case 8000: div = 0x6;
			break;
		case 11025: div = 0xa;
			break;
		case 16000: div = 0x8;
			break;
		case 22050: div = 0xb;
			break;
		case 24000: div = 0x9;
			break;
		case 32000: div = 0xc;
			break;
		case 44100: div = 0xf;
			break;
		case 48000: div = 0xd;
			break;
		case 96000: div = 0x5;
			break;
		case 192000: div = 0x1;
			break;
		default :
			div = 0xf;
			break;
	}
	__spdif_set_ori_sample_freq(aic,div);
	return div;
}

static unsigned long  spdif_select_sample_freq(struct device* aic,unsigned long sync)
{
	int div = 0;
	switch(sync) {
		case 8000: div = 0x9;
			break;
		case 11025: div = 0x5;
			break;
		case 16000: div = 0x7;
			break;
		case 22050: div = 0x4;
			break;
		case 24000: div = 0x6;
			break;
		case 32000: div = 0x3;
			break;
		case 44100: div = 0x0;
			break;
		case 48000: div = 0x2;
			break;
		case 96000: div = 0xa;
			break;
		case 192000: div = 0xe;
			break;
		default :
			div = 0;
			break;
	}
	__spdif_set_sample_freq(aic,div);
	return div;
}
static int jz_spdif_set_rate(struct device *aic ,struct jz_aic* jz_aic, unsigned long sample_rate){
	unsigned long sysclk;
	struct clk* cgu_aic_clk = jz_aic->clk;
	__i2s_stop_bitclk(aic);
#ifndef CONFIG_PRODUCT_X1000_FPGA
	sysclk = calculate_cgu_aic_rate(clk_get_rate(cgu_aic_clk->parent), &sample_rate);
	clk_set_rate(cgu_aic_clk, (sysclk*2));
	jz_aic->sysclk = clk_get_rate(cgu_aic_clk);

	if (jz_aic->sysclk > (sysclk*2)) {
		printk("external codec set sysclk fail aic = %ld want is %ld .\n",jz_aic->sysclk,sysclk);
		return -1;
	}

	spdif_set_clk(jz_aic, jz_aic->sysclk/2, sample_rate);
#else
	audio_write((253<<13)|(4482),I2SCDR_PRE);
	*(volatile unsigned int*)0xb0000070 = *(volatile unsigned int*)0xb0000070;
	audio_write((253<<13)|(4482)|(1<<29),I2SCDR_PRE);
	audio_write(0,I2SDIV_PRE);
#endif
	__i2s_start_bitclk(aic);
	spdif_select_ori_sample_freq(aic,sample_rate);
	spdif_select_sample_freq(aic,sample_rate);

	return sample_rate;
}

static int jz_spdif_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	int channels = params_channels(params);
	int fmt_width = snd_pcm_format_width(params_format(params));
	struct jz_spdif *jz_spdif = dev_get_drvdata(dai->dev);
	struct device *aic = jz_spdif->aic;
	struct jz_aic *jz_aic = dev_get_drvdata(aic);
	enum dma_slave_buswidth buswidth;
	int trigger;
	unsigned long sample_rate = params_rate(params);

	spdif_DEBUG_MSG("enter %s, substream = %s\n", __func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (!((1 << params_format(params)) & JZ_SPDIF_FORMATS) || channels > 2) {
		dev_err(dai->dev, "hw params not inval channel %d params %x\n",
				channels, params_format(params));
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		__i2s_channel(aic, channels);
		/* format */
		if (fmt_width == 16){
			buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;
			__spdif_set_max_wl(aic,0);
			__spdif_set_sample_size(aic,1);
		}else if(fmt_width == 24){
			buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;
			__spdif_set_max_wl(aic,1);
			__spdif_set_sample_size(aic,5);
		}else{
			return -EINVAL;
		}

		jz_spdif->tx_dma_data.buswidth = buswidth;
		jz_spdif->tx_dma_data.max_burst = (SPDIF_TFIFO_DEPTH * buswidth)/2;
		trigger = SPDIF_TFIFO_DEPTH - (jz_spdif->tx_dma_data.max_burst/(int)buswidth);
		__i2s_set_transmit_trigger(aic,trigger/2);
		snd_soc_dai_set_dma_data(dai, substream, (void *)&jz_spdif->tx_dma_data);

	} else {
		printk("spdif is not a capture device!\n");
		return -EINVAL;
	}

	/* sample rate */
	jz_aic->sample_rate = jz_spdif_set_rate(aic,jz_aic,sample_rate);
	if(jz_aic->sample_rate < 0)
		printk("set spdif clk failed!!\n");
	/* signed transfer */
	if(snd_pcm_format_signed(params_format(params)))
		__spdif_clear_signn(aic);
	else
		__spdif_set_signn(aic);
	return 0;
}

static int jz_spdif_start_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_spdif *jz_spdif = dev_get_drvdata(dai->dev);
	struct device *aic = jz_spdif->aic;
	int rst_test = 0xfff;
	spdif_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		__spdif_reset(aic);
		while(__spdif_get_reset(aic)) {
			if (rst_test-- <= 0){
				printk("spdif rst err\n");
				return -EINVAL;
			}
		}
		__spdif_enable_transmit_dma(aic);
		__spdif_clear_underrun(aic);
		__spdif_enable(aic);
		__aic_enable(aic);
	} else {
		printk("spdif is not a capture device!\n");
		return -EINVAL;
	}
	return 0;
}

static int jz_spdif_stop_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct jz_spdif *jz_spdif = dev_get_drvdata(dai->dev);
	struct device *aic = jz_spdif->aic;
	spdif_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (jz_spdif_debug) __aic_dis_tur_int(aic);
		if (__spdif_is_enable_transmit_dma(aic)) {
			__spdif_disable_transmit_dma(aic);
			__spdif_clear_underrun(aic);
#ifdef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
			/*hrtime mode: stop will be happen in any where, make sure there is
			 *	no data transfer on ahb bus before stop dma
			 */
			while(!__spdif_test_underrun(aic));
#endif
		}
		__spdif_disable(aic);
		__spdif_clear_underrun(aic);
	} else {
		printk("spdif is not a capture device!\n");
		return -EINVAL;
	}

	return 0;
}

static int jz_spdif_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
#endif
	spdif_DEBUG_MSG("enter %s, substream = %s cmd = %d\n",
		      __func__,
		      (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture",
		      cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
		if (atomic_read(&prtd->stopped_pending))
			return -EPIPE;
#endif
		if(jz_spdif_start_substream(substream, dai))
			return -EINVAL;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
		if (atomic_read(&prtd->stopped_pending))
			return 0;
#endif
		jz_spdif_stop_substream(substream, dai);
		break;
	}
	/*dump_registers(aic);*/
	return 0;
}

static void jz_spdif_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai) {
	struct jz_spdif *jz_spdif = dev_get_drvdata(dai->dev);
	struct device *aic = jz_spdif->aic;
	enum aic_mode work_mode = AIC_SPDIF_MODE;

	spdif_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
	work_mode = aic_set_work_mode(jz_spdif->aic, work_mode, false);
	BUG_ON((work_mode != AIC_NO_MODE));
	jz_spdif_stop_substream(substream, dai);
	__aic_disable(aic);
	return;
}

static int jz_spdif_probe(struct snd_soc_dai *dai)
{
	struct jz_spdif *jz_spdif = dev_get_drvdata(dai->dev);
	struct device *aic = jz_spdif->aic;
	unsigned int reg_tmp;

	spdif_DEBUG_MSG("enter %s\n", __func__);


	__i2s_disable_transmit_dma(aic);
	__i2s_disable_receive_dma(aic);
	__i2s_disable_replay(aic);
	__i2s_disable_record(aic);
	__i2s_disable_loopback(aic);
	__aic_disable(aic);


	reg_tmp = jz_aic_read_reg(aic, SPCTRL);
	reg_tmp |= (SPCTRL_SPDIF_I2S_MASK | SPCTRL_M_TRIG_MASK |
			SPCTRL_M_FFUR_MASK | SPCTRL_INVALID_MASK);
	jz_aic_write_reg(aic, SPCTRL, reg_tmp);

	__i2s_stop_bitclk(aic);
	__i2s_external_codec(aic);
	__i2s_bclk_output(aic);
	__i2s_sync_output(aic);
	__aic_select_i2s(aic);
	__i2s_send_rfirst(aic);

	__spdif_set_dtype(aic,0);
	__spdif_set_ch1num(aic,0);
	__spdif_set_ch2num(aic,1);
	__spdif_set_srcnum(aic,0);

	__interface_select_spdif(aic);
	__spdif_play_lastsample(aic);
	__spdif_init_set_low(aic);
	__spdif_choose_consumer(aic);
	__spdif_clear_audion(aic);
	__spdif_set_copyn(aic);
	__spdif_clear_pre(aic);
	__spdif_choose_chmd(aic);
	__spdif_set_category_code_normal(aic);
	__spdif_set_clkacu(aic, 0);
	__spdif_set_sample_size(aic, 1);
	__spdif_set_valid(aic);
	__spdif_mask_trig(aic);
	__spdif_disable_underrun_intr(aic);
	/*select spdif trans*/
	printk("spdif cpu dai prob ok\n");

	return 0;
}


static struct snd_soc_dai_ops jz_spdif_dai_ops = {
	.startup = jz_spdif_startup,
	.trigger 	= jz_spdif_trigger,
	.hw_params 	= jz_spdif_hw_params,
	.shutdown	= jz_spdif_shutdown,
};

#define jz_spdif_suspend NULL
#define jz_spdif_resume	NULL
static struct snd_soc_dai_driver jz_spdif_dai = {
		.probe   = jz_spdif_probe,
		.suspend = jz_spdif_suspend,
		.resume  = jz_spdif_resume,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = JZ_SPDIF_RATE,
			.formats = JZ_SPDIF_FORMATS,
		},
		.ops = &jz_spdif_dai_ops,
};

static ssize_t jz_spdif_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct jz_spdif *jz_spdif = dev_get_drvdata(dev);
	dump_registers(jz_spdif->aic);
	return 0;
}

static struct device_attribute jz_spdif_sysfs_attrs[] = {
	__ATTR(spdif_regs, S_IRUGO, jz_spdif_regs_show, NULL),
};

static const struct snd_soc_component_driver jz_i2s_component = {
	.name		= "jz-spdif",
};

static int jz_spdif_platfrom_probe(struct platform_device *pdev){
	struct jz_aic_subdev_pdata *pdata = dev_get_platdata(&pdev->dev);
	struct jz_spdif *jz_spdif;
	int i = 0, ret;

	jz_spdif = devm_kzalloc(&pdev->dev, sizeof(struct jz_spdif), GFP_KERNEL);
	if (!jz_spdif)
		return -ENOMEM;

	jz_spdif->aic = pdev->dev.parent;

	jz_spdif->tx_dma_data.dma_addr = pdata->dma_base + SPFIFO;
	platform_set_drvdata(pdev, (void *)jz_spdif);

	for (; i < ARRAY_SIZE(jz_spdif_sysfs_attrs); i++) {
		ret = device_create_file(&pdev->dev, &jz_spdif_sysfs_attrs[i]);
		if (ret)
			dev_warn(&pdev->dev,"attribute %s create failed %x",
					attr_name(jz_spdif_sysfs_attrs[i]), ret);
	}
	ret = snd_soc_register_component(&pdev->dev, &jz_spdif_component,
					 &jz_spdif_dai, 1);
	if (!ret)
		dev_dbg(&pdev->dev, "spdif platform probe success\n");
	return ret;
}

static int jz_spdif_platfom_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(jz_spdif_sysfs_attrs); i++)
		device_remove_file(&pdev->dev, &jz_spdif_sysfs_attrs[i]);
	platform_set_drvdata(pdev, NULL);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static struct platform_driver jz_spdif_plat_driver = {
	.probe  = jz_spdif_platfrom_probe,
	.remove = jz_spdif_platfom_remove,
	.driver = {
		.name = "jz-asoc-aic-spdif",
		.owner = THIS_MODULE,
	},
};

static int jz_spdif_init(void)
{

	return platform_driver_register(&jz_spdif_plat_driver);
}
module_init(jz_spdif_init);

static void jz_spdif_exit(void)
{
	platform_driver_unregister(&jz_spdif_plat_driver);
}
module_exit(jz_spdif_exit);

MODULE_AUTHOR("shicheng.cheng <shicheng.cheng@ingenic.com>");
MODULE_DESCRIPTION("JZ AIC SPDIF SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz-aic-spdif");
