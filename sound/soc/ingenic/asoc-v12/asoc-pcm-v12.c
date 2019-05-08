/*
 *  sound/soc/ingenic/asoc-pcm.c
 *  ALSA Soc Audio Layer -- ingenic pcm (part of aic controller) driver
 *
 *  Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
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
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/slab.h>
#include "asoc-pcm-v12.h"

static int jz_pcm_debug = 0;
module_param(jz_pcm_debug, int, 0644);
#define PCM_DEBUG_MSG(msg...)			\
	do {					\
		if (jz_pcm_debug)		\
			printk(KERN_DEBUG"PCM: " msg);	\
	} while(0)

#define PCM_RFIFO_DEPTH 16
#define PCM_TFIFO_DEPTH 16
#define JZ_PCM_FORMATS (SNDRV_PCM_FMTBIT_S8 |  SNDRV_PCM_FMTBIT_S16_LE)

static void dump_registers(struct device *dev)
{
	struct jz_pcm *jz_pcm = dev_get_drvdata(dev);
	pr_info("PCMCTL  %p : 0x%08x\n", (jz_pcm->vaddr_base+PCMCTL),pcm_read_reg(dev, PCMCTL));
	pr_info("PCMCFG  %p : 0x%08x\n", (jz_pcm->vaddr_base+PCMCFG),pcm_read_reg(dev, PCMCFG));
	pr_info("PCMINTC %p : 0x%08x\n", (jz_pcm->vaddr_base+PCMINTC),pcm_read_reg(dev,PCMINTC));
	pr_info("PCMINTS %p : 0x%08x\n",(jz_pcm->vaddr_base+PCMINTS),pcm_read_reg(dev, PCMINTS));
	pr_info("PCMDIV  %p : 0x%08x\n",(jz_pcm->vaddr_base+PCMDIV),pcm_read_reg(dev, PCMDIV));
	return;
}

static int jz_pcm_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct jz_pcm *jz_pcm = dev_get_drvdata(dai->dev);

	PCM_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (!jz_pcm->pcm_mode) {
		clk_enable(jz_pcm->clk_gate);
#if 1
		__pcm_as_slaver(dev);
#else
		clk_set_rate(jz_pcm->clk, 9600000);
		clk_enable(jz_pcm->clk);
		__pcm_as_master(dev);
		__pcm_set_clkdiv(dev, 9600000/20/8000 - 1);
		__pcm_set_syndiv(dev, 20 - 1);
#endif
		__pcm_set_msb_one_shift_in(dev);
		__pcm_set_msb_one_shift_out(dev);
		__pcm_play_lastsample(dev);
		__pcm_enable(dev);
		__pcm_clock_enable(dev);
	}

	if (substream->stream ==
			SNDRV_PCM_STREAM_PLAYBACK) {
		__pcm_disable_transmit_dma(dev);
		__pcm_disable_replay(dev);
		__pcm_clear_tur(dev);
		jz_pcm->pcm_mode |= PCM_WRITE;
	} else {
		__pcm_disable_receive_dma(dev);
		__pcm_disable_record(dev);
		__pcm_clear_ror(dev);
		jz_pcm->pcm_mode |= PCM_READ;
	}
	printk("start set PCM register....\n");
	return 0;
}

static int jz_pcm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct jz_pcm *jz_pcm = dev_get_drvdata(dai->dev);
	int fmt_width = snd_pcm_format_width(params_format(params));
	enum dma_slave_buswidth buswidth;
	int trigger;

	PCM_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (!((1 << params_format(params)) & JZ_PCM_FORMATS) ||
			params_channels(params) != 1 ||
			params_rate(params) != 8000) {
		dev_err(dai->dev, "hw params not inval channel %d params %x\n",
				params_channels(params), params_format(params));
		return -EINVAL;
	}

	/* format 8 bit or 16 bit*/
	if (fmt_width == 8)
		buswidth = DMA_SLAVE_BUSWIDTH_1_BYTE;
	else
		buswidth = DMA_SLAVE_BUSWIDTH_2_BYTES;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		jz_pcm->tx_dma_data.buswidth = buswidth;
		jz_pcm->tx_dma_data.max_burst = (PCM_TFIFO_DEPTH * buswidth)/2;
		trigger = PCM_TFIFO_DEPTH - (jz_pcm->tx_dma_data.max_burst/(int)buswidth);
		__pcm_set_oss_sample_size(dev, fmt_width == 8 ? 0 : 1);
		__pcm_set_transmit_trigger(dev, trigger);
		snd_soc_dai_set_dma_data(dai, substream, (void *)&jz_pcm->tx_dma_data);
	} else {
		jz_pcm->rx_dma_data.buswidth = buswidth;
		jz_pcm->rx_dma_data.max_burst = (PCM_RFIFO_DEPTH * buswidth)/2;
		trigger = jz_pcm->rx_dma_data.max_burst/(int)buswidth;
		__pcm_set_iss_sample_size(dev, fmt_width == 8 ? 0 : 1);
		__pcm_set_receive_trigger(dev, trigger);
		snd_soc_dai_set_dma_data(dai, substream, (void *)&jz_pcm->rx_dma_data);
	}
	return 0;
}

static void jz_pcm_start_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	PCM_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		__pcm_enable_transmit_dma(dev);
		__pcm_enable_replay(dev);
	} else {
		__pcm_enable_record(dev);
		__pcm_enable_receive_dma(dev);
	}
	return;
}

static void jz_pcm_stop_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	PCM_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (__pcm_transmit_dma_is_enable(dev)) {
			__pcm_disable_transmit_dma(dev);
			__pcm_clear_tur(dev);
#ifdef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
			/* Hrtimer mode: stop will be happen in any where, make sure there is
			 *	no data transfer on ahb bus before stop dma
			 * Harzard:
			 *	In pcm slave mode, the clk maybe stop before here, we will dead here
			 */
			while(!__pcm_test_tur(dev));
#endif
		}
		__pcm_disable_replay(dev);
		__pcm_clear_tur(dev);
	} else {
		if (__pcm_receive_dma_is_enable(dev)) {
			__pcm_disable_receive_dma(dev);
			__pcm_clear_ror(dev);
#ifdef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
			while(!__pcm_test_ror(dev));
#endif
		}
		__pcm_disable_record(dev);
		__pcm_clear_ror(dev);
	}
	return;
}

static int jz_pcm_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
#endif
	PCM_DEBUG_MSG("enter %s, substream = %s cmd = %d\n",
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
		PCM_DEBUG_MSG("pcm start\n");
		jz_pcm_start_substream(substream, dai);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
		if (atomic_read(&prtd->stopped_pending))
			return 0;
#endif
		PCM_DEBUG_MSG("pcm stop\n");
		jz_pcm_stop_substream(substream, dai);
		break;
	}
	return 0;
}

static void jz_pcm_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai) {
	struct jz_pcm *jz_pcm = dev_get_drvdata(dai->dev);
	struct device *dev = dai->dev;

	PCM_DEBUG_MSG("enter %s, substream = %s\n",
			__func__,
			(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? "playback" : "capture");
	jz_pcm_stop_substream(substream, dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		jz_pcm->pcm_mode &= ~PCM_WRITE;
	else
		jz_pcm->pcm_mode &= ~PCM_READ;

	if (!jz_pcm->pcm_mode) {
		__pcm_disable(dev);
		__pcm_clock_disable(dev);
		clk_disable(jz_pcm->clk_gate);
	}
	return;
}

static int jz_pcm_probe(struct snd_soc_dai *dai)
{
	return 0;
}


static struct snd_soc_dai_ops jz_pcm_dai_ops = {
	.startup	= jz_pcm_startup,
	.trigger 	= jz_pcm_trigger,
	.hw_params 	= jz_pcm_hw_params,
	.shutdown	= jz_pcm_shutdown,
};

#define jz_pcm_suspend	NULL
#define jz_pcm_resume	NULL

static ssize_t jz_pcm_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct jz_pcm *jz_pcm = dev_get_drvdata(dev);
	dump_registers(jz_pcm->dev);
	return 0;
}

static struct device_attribute jz_pcm_sysfs_attrs[] = {
	__ATTR(pcm_regs, S_IRUGO, jz_pcm_regs_show, NULL),
};

static struct snd_soc_dai_driver jz_pcm_dai = {
		.probe   = jz_pcm_probe,
		.suspend = jz_pcm_suspend,
		.resume  = jz_pcm_resume,
		.playback = {
			.channels_min = 1,
			.channels_max = 1,
			.rates = SNDRV_PCM_RATE_8000,
			.formats = JZ_PCM_FORMATS,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 1,
			.rates = SNDRV_PCM_RATE_8000,
			.formats = JZ_PCM_FORMATS,
		},
		.ops = &jz_pcm_dai_ops,
};

static const struct snd_soc_component_driver jz_pcm_component = {
	.name		= "jz-pcm",
};

static int jz_pcm_platfrom_probe(struct platform_device *pdev)
{
	struct jz_pcm *jz_pcm;
	struct resource *res = NULL;
	int i = 0, ret;

	jz_pcm = devm_kzalloc(&pdev->dev, sizeof(struct jz_pcm), GFP_KERNEL);
	if (!jz_pcm)
		return -ENOMEM;

	jz_pcm->dev = &pdev->dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOENT;
	if (!devm_request_mem_region(&pdev->dev,
				res->start, resource_size(res),
				pdev->name))
		return -EBUSY;

	jz_pcm->res_start = res->start;
	jz_pcm->res_size = resource_size(res);
	jz_pcm->vaddr_base = devm_ioremap_nocache(&pdev->dev,
			jz_pcm->res_start, jz_pcm->res_size);
	if (!jz_pcm->vaddr_base) {
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		return -ENOMEM;
	}

	jz_pcm->clk_gate = devm_clk_get(&pdev->dev, "pcm");
	if (IS_ERR_OR_NULL(jz_pcm->clk_gate)) {
		ret = PTR_ERR(jz_pcm->clk_gate);
		dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
		jz_pcm->clk_gate = NULL;
		return ret;
	}
	jz_pcm->clk = devm_clk_get(&pdev->dev, "cgu_pcm");
	if (IS_ERR_OR_NULL(jz_pcm->clk)) {
		ret = PTR_ERR(jz_pcm->clk);
		dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
		return ret;
	}
	platform_set_drvdata(pdev, (void *)jz_pcm);

	jz_pcm->pcm_mode = 0;
	jz_pcm->tx_dma_data.dma_addr = (dma_addr_t)jz_pcm->res_start + PCMDP;
	jz_pcm->rx_dma_data.dma_addr = (dma_addr_t)jz_pcm->res_start + PCMDP;
	platform_set_drvdata(pdev, (void *)jz_pcm);

	for (; i < ARRAY_SIZE(jz_pcm_sysfs_attrs); i++) {
		ret = device_create_file(&pdev->dev, &jz_pcm_sysfs_attrs[i]);
		if (ret)
			dev_warn(&pdev->dev,"attribute %s create failed %x",
					attr_name(jz_pcm_sysfs_attrs[i]), ret);
	}

	ret = snd_soc_register_component(&pdev->dev, &jz_pcm_component,
			&jz_pcm_dai, 1);
	if (ret) {
		platform_set_drvdata(pdev, NULL);
		return ret;
	}
	dev_info(&pdev->dev, "pcm platform probe success\n");
	return ret;
}

static int jz_pcm_platfom_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(jz_pcm_sysfs_attrs); i++)
		device_remove_file(&pdev->dev, &jz_pcm_sysfs_attrs[i]);
	platform_set_drvdata(pdev, NULL);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static struct platform_driver jz_pcm_plat_driver = {
	.probe  = jz_pcm_platfrom_probe,
	.remove = jz_pcm_platfom_remove,
	.driver = {
		.name = "jz-asoc-pcm",
		.owner = THIS_MODULE,
	},
};

static int jz_pcm_init(void)
{
        return platform_driver_register(&jz_pcm_plat_driver);
}
module_init(jz_pcm_init);

static void jz_pcm_exit(void)
{
	platform_driver_unregister(&jz_pcm_plat_driver);
}
module_exit(jz_pcm_exit);

MODULE_AUTHOR("cli <chen.li@ingenic.com>");
MODULE_DESCRIPTION("JZ AIC PCM SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz-pcm");
