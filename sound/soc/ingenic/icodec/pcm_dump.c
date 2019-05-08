/*
 * sound/soc/ingenic/icodec/dump.c
 * ALSA SoC Audio driver -- dump codec driver used for devices like bt, hdmi

 * Copyright 2014 Ingenic Semiconductor Co.,Ltd
 *	cli <chen.li@ingenic.com>
 *
 * Note: dlv4780 is an internal codec for jz SOC
 *	 used for dlv4780 m200 and so on
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>

static struct snd_soc_codec_driver dump_codec;

static struct snd_soc_dai_driver dump_codec_dai = {
	.name = "pcm dump dai",
	.playback = {
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE |
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S8,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE |
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S8,
	},
};

static int dump_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
	ret = snd_soc_register_codec(&pdev->dev, &dump_codec,
			&dump_codec_dai, 1);
	dev_dbg(&pdev->dev, "codec dump codec platfrom probe success\n");
	return ret;
}

static int dump_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver dlv4780_codec_driver = {
	.driver = {
		.name = "pcm dump",
		.owner = THIS_MODULE,
	},
	.probe = dump_platform_probe,
	.remove = dump_platform_remove,
};

static int dump_modinit(void)
{
	return platform_driver_register(&dlv4780_codec_driver);
}
module_init(dump_modinit);

static void dump_exit(void)
{
	platform_driver_unregister(&dlv4780_codec_driver);
}
module_exit(dump_exit);

MODULE_DESCRIPTION("Dump Codec Driver");
MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_LICENSE("GPL");
