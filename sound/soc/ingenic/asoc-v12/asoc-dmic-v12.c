/*
 *  sound/soc/ingenic/asoc-dmic.c
 *  ALSA Soc Audio Layer -- ingenic dmic (part of aic controller) driver
 *
 *  Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cscheng <shicheng.cheng@ingenic.com>
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
#include <linux/io.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include "asoc-dmic-v12.h"
#include "asoc-aic-v12.h"

static int jz_dmic_debug = 0;
module_param(jz_dmic_debug, int, 0644);
#define DMIC_DEBUG_MSG(msg...)			\
	do {					\
		if (jz_dmic_debug)		\
			printk(KERN_DEBUG"dmic: " msg);	\
	} while(0)

#define DMIC_FIFO_DEPTH 64
#define JZ_DMIC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)
#define JZ_DMIC_RATE (SNDRV_PCM_RATE_8000|SNDRV_PCM_RATE_16000|SNDRV_PCM_RATE_48000)

static void dump_registers(struct device *dev)
{
	struct jz_dmic *jz_dmic = dev_get_drvdata(dev);
	pr_info("DMICCR0  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICCR0),dmic_read_reg(dev, DMICCR0));
	pr_info("DMICGCR  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICGCR),dmic_read_reg(dev, DMICGCR));
	pr_info("DMICIMR  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICIMR),dmic_read_reg(dev, DMICIMR));
	pr_info("DMICINTCR  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICINTCR),dmic_read_reg(dev, DMICINTCR));
	pr_info("DMICTRICR  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICTRICR),dmic_read_reg(dev, DMICTRICR));
	pr_info("DMICTHRH  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICTHRH),dmic_read_reg(dev, DMICTHRH));
	pr_info("DMICTHRL  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICTHRL),dmic_read_reg(dev, DMICTHRL));
	pr_info("DMICTRIMMAX  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICTRIMMAX),dmic_read_reg(dev, DMICTRIMMAX));
	pr_info("DMICTRINMAX  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICTRINMAX),dmic_read_reg(dev, DMICTRINMAX));
	pr_info("DMICDR  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICDR),dmic_read_reg(dev, DMICDR));
	pr_info("DMICFTHR  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICFTHR),dmic_read_reg(dev, DMICFTHR));
	pr_info("DMICFSR  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICFSR),dmic_read_reg(dev, DMICFSR));
	pr_info("DMICCGDIS  %p : 0x%08x\n", (jz_dmic->vaddr_base+DMICCGDIS),dmic_read_reg(dev, DMICCGDIS));
	return;
}

static int jz_dmic_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct jz_dmic *jz_dmic = dev_get_drvdata(dai->dev);

	DMIC_DEBUG_MSG("enter %s, substream capture\n",__func__);

	if (!jz_dmic->dmic_mode) {
		__dmic_enable(dev);
	}

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		jz_dmic->dmic_mode |= DMIC_READ;
	} else {
		dev_err(dai->dev, "dmic is a capture device\n");
		return -EINVAL;
	}

	clk_enable(jz_dmic->clk_gate_dmic);
	regulator_enable(jz_dmic->vcc_dmic);
	jz_dmic->en = 1;

//	dump_registers(jz_dmic->dev);
	printk("start set dmic register....\n");
	return 0;
}

static int dmic_set_rate(struct device *dev, int rate){
	switch(rate){
		case 8000:
			__dmic_set_sr_8k(dev);
			break;
		case 16000:
			__dmic_set_sr_16k(dev);
			break;
		case 48000:
			__dmic_set_sr_48k(dev);
			break;
		default:
			dev_err(dev,"dmic unsupport rate %d\n",rate);
	}
	return 0;
}

static int jz_dmic_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	int channels = params_channels(params);
	int rate = params_rate(params);
	struct jz_dmic *jz_dmic = dev_get_drvdata(dai->dev);
	int fmt_width = snd_pcm_format_width(params_format(params));
	enum dma_slave_buswidth buswidth;
	int trigger;

	DMIC_DEBUG_MSG("enter %s, substream = %s\n",__func__,"capture");

	if (!((1 << params_format(params)) & JZ_DMIC_FORMATS)
			||channels < 1||channels > 2|| rate > 48000||rate < 8000
			||fmt_width != 16) {
		dev_err(dai->dev, "hw params not inval channel %d params %x rate %d fmt_width %d\n",
				channels, params_format(params),rate,fmt_width);
		return -EINVAL;
	}

	buswidth = DMA_SLAVE_BUSWIDTH_4_BYTES;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		jz_dmic->rx_dma_data.buswidth = buswidth;
		jz_dmic->rx_dma_data.max_burst = (DMIC_FIFO_DEPTH * buswidth)/2;
		trigger = jz_dmic->rx_dma_data.max_burst/(int)buswidth;
		__dmic_set_request(dev, trigger);
		__dmic_set_chnum(dev, channels - 1);
		dmic_set_rate(dev,rate);
		snd_soc_dai_set_dma_data(dai, substream, (void *)&jz_dmic->rx_dma_data);
	} else {
		dev_err(dai->dev, "DMIC is a capture device\n");
	}
	return 0;
}

static void jz_dmic_start_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	DMIC_DEBUG_MSG("enter %s, substream start capture\n", __func__);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		__dmic_enable_rdms(dev);
		__dmic_enable(dev);
	} else {
		dev_err(dai->dev, "DMIC is a capture device\n");
	}
	return;
}

static void jz_dmic_stop_substream(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	DMIC_DEBUG_MSG("enter %s, substream stop capture\n",__func__);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (__dmic_is_enable_rdms(dev)) {
			__dmic_disable_rdms(dev);
		}
		__dmic_disable(dev);
	}else{
		dev_err(dai->dev, "DMIC is a capture device\n");
	}
	return;
}

static int jz_dmic_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	struct jz_pcm_runtime_data *prtd = substream->runtime->private_data;
	DMIC_DEBUG_MSG("enter %s, substream capture cmd = %d\n", __func__,cmd);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
		if (atomic_read(&prtd->stopped_pending))
			return -EPIPE;
#endif
		printk(KERN_DEBUG"dmic start\n");
		jz_dmic_start_substream(substream, dai);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
#ifndef CONFIG_JZ_ASOC_DMA_HRTIMER_MODE
		if (atomic_read(&prtd->stopped_pending))
			return 0;
#endif
		printk(KERN_DEBUG"dmic stop\n");
		jz_dmic_stop_substream(substream, dai);
		break;
	}
	return 0;
}

static void jz_dmic_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai) {
	struct jz_dmic *jz_dmic = dev_get_drvdata(dai->dev);
	struct device *dev = dai->dev;

	DMIC_DEBUG_MSG("enter %s, substream = capture\n", __func__);
	jz_dmic_stop_substream(substream, dai);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		jz_dmic->dmic_mode &= ~DMIC_READ;

	if (!jz_dmic->dmic_mode) {
		__dmic_disable(dev);
	}
	regulator_disable(jz_dmic->vcc_dmic);
	clk_disable(jz_dmic->clk_gate_dmic);
	jz_dmic->en = 0;
	return;
}

static int jz_dmic_probe(struct snd_soc_dai *dai)
{
	struct device *dev = dai->dev;
	struct jz_dmic *jz_dmic = dev_get_drvdata(dai->dev);
	clk_enable(jz_dmic->clk_gate_dmic);
	/*gain: 0, ..., e*/
	__dmic_reset(dev);
	while(__dmic_get_reset(dev));
	__dmic_set_sr_8k(dev);
	__dmic_enable_hpf1(dev);
/*	__dmic_disable_hpf1(dev);*/
	__dmic_set_gcr(dev,8);
	__dmic_mask_all_int(dev);
	__dmic_enable_rdms(dev);
	__dmic_enable_pack(dev);
	__dmic_disable_sw_lr(dev);
	__dmic_enable_lp(dev);
	__dmic_set_request(dev,48);
	__dmic_enable_hpf2(dev);
	__dmic_set_thr_high(dev,32);
	__dmic_set_thr_low(dev,16);
	__dmic_enable_tri(dev);
	__dmic_unpack_dis(dev);

	clk_disable(jz_dmic->clk_gate_dmic);

	return 0;
}


static struct snd_soc_dai_ops jz_dmic_dai_ops = {
	.startup	= jz_dmic_startup,
	.trigger 	= jz_dmic_trigger,
	.hw_params 	= jz_dmic_hw_params,
	.shutdown	= jz_dmic_shutdown,
};

#define jz_dmic_suspend	NULL
#define jz_dmic_resume	NULL

static ssize_t jz_dmic_regs_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct jz_dmic *jz_dmic = dev_get_drvdata(dev);
	dump_registers(jz_dmic->dev);
	return 0;
}

static struct device_attribute jz_dmic_sysfs_attrs[] = {
	__ATTR(dmic_regs, S_IRUGO, jz_dmic_regs_show, NULL),
};

static struct snd_soc_dai_driver jz_dmic_dai = {
		.probe   = jz_dmic_probe,
		.suspend = jz_dmic_suspend,
		.resume  = jz_dmic_resume,
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = JZ_DMIC_RATE,
			.formats = JZ_DMIC_FORMATS,
		},
		.ops = &jz_dmic_dai_ops,
};

static const struct snd_soc_component_driver jz_dmic_component = {
	.name		= "jz-dmic",
};

static int jz_dmic_platfrom_probe(struct platform_device *pdev)
{
	struct jz_dmic *jz_dmic;
	struct resource *res = NULL;
	int i = 0, ret;

	jz_dmic = devm_kzalloc(&pdev->dev, sizeof(struct jz_dmic), GFP_KERNEL);
	if (!jz_dmic)
		return -ENOMEM;

	jz_dmic->dev = &pdev->dev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOENT;
	if (!devm_request_mem_region(&pdev->dev,
				res->start, resource_size(res),pdev->name))
		return -EBUSY;

	jz_dmic->res_start = res->start;
	jz_dmic->res_size = resource_size(res);
	jz_dmic->vaddr_base = devm_ioremap_nocache(&pdev->dev,
			jz_dmic->res_start, jz_dmic->res_size);
	if (!jz_dmic->vaddr_base) {
		dev_err(&pdev->dev, "Failed to ioremap mmio memory\n");
		return -ENOMEM;
	}

	jz_dmic->dmic_mode = 0;
	jz_dmic->rx_dma_data.dma_addr = (dma_addr_t)jz_dmic->res_start + DMICDR;

	jz_dmic->vcc_dmic = regulator_get(&pdev->dev,"dmic_1v8");
	platform_set_drvdata(pdev, (void *)jz_dmic);

	for (; i < ARRAY_SIZE(jz_dmic_sysfs_attrs); i++) {
		ret = device_create_file(&pdev->dev, &jz_dmic_sysfs_attrs[i]);
		if (ret)
			dev_warn(&pdev->dev,"attribute %s create failed %x",
					attr_name(jz_dmic_sysfs_attrs[i]), ret);
	}

	jz_dmic->clk_gate_dmic = clk_get(&pdev->dev, "dmic");
	if (IS_ERR_OR_NULL(jz_dmic->clk_gate_dmic)) {
		ret = PTR_ERR(jz_dmic->clk_gate_dmic);
		jz_dmic->clk_gate_dmic = NULL;
		dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
		return ret;
	}

	jz_dmic->clk_pwc_dmic = clk_get(&pdev->dev, "pwc_dmic");
	if (IS_ERR_OR_NULL(jz_dmic->clk_pwc_dmic)) {
		ret = PTR_ERR(jz_dmic->clk_pwc_dmic);
		jz_dmic->clk_pwc_dmic = NULL;
		dev_err(&pdev->dev, "Failed to get clock: %d\n", ret);
		return ret;
	}
	clk_enable(jz_dmic->clk_pwc_dmic);

	ret = snd_soc_register_component(&pdev->dev, &jz_dmic_component,
					 &jz_dmic_dai, 1);
	if (ret)
		goto err_register_cpu_dai;
	dev_dbg(&pdev->dev, "dmic platform probe success\n");
	return ret;

err_register_cpu_dai:
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static int jz_dmic_platfom_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(jz_dmic_sysfs_attrs); i++)
		device_remove_file(&pdev->dev, &jz_dmic_sysfs_attrs[i]);
	snd_soc_unregister_component(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}


#ifdef CONFIG_PM
static int jz_dmic_platfom_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct jz_dmic *jz_dmic = platform_get_drvdata(pdev);
	if(jz_dmic->en){
		regulator_disable(jz_dmic->vcc_dmic);
		clk_disable(jz_dmic->clk_gate_dmic);
	}
	return 0;
}

static int jz_dmic_platfom_resume(struct platform_device *pdev)
{
	struct jz_dmic *jz_dmic = platform_get_drvdata(pdev);
	struct device *aic = pdev->dev.parent;
	struct jz_aic *jz_aic = dev_get_drvdata(aic);
	if(jz_dmic->en){
		regulator_enable(jz_dmic->vcc_dmic);
		clk_enable(jz_dmic->clk_gate_dmic);
	}
	return 0;
}
#endif
static struct platform_driver jz_dmic_plat_driver = {
	.probe  = jz_dmic_platfrom_probe,
	.remove = jz_dmic_platfom_remove,
#ifdef CONFIG_PM
	.suspend = jz_dmic_platfom_suspend,
	.resume = jz_dmic_platfom_resume,
#endif
	.driver = {
		.name = "jz-asoc-dmic",
		.owner = THIS_MODULE,
	},
};

static int jz_dmic_init(void)
{
        return platform_driver_register(&jz_dmic_plat_driver);
}
module_init(jz_dmic_init);

static void jz_dmic_exit(void)
{
	platform_driver_unregister(&jz_dmic_plat_driver);
}
module_exit(jz_dmic_exit);

MODULE_AUTHOR("shicheng.cheng@ingenic.com");
MODULE_DESCRIPTION("JZ AIC dmic SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:jz-dmic");
