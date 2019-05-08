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
	.name = "spdif dump dai",
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100|SNDRV_PCM_RATE_48000|SNDRV_PCM_RATE_96000|SNDRV_PCM_RATE_192000,
		.formats = SNDRV_PCM_FMTBIT_S24_LE|SNDRV_PCM_FMTBIT_S16_LE,
	},
};

static int spdif_dump_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
	ret = snd_soc_register_codec(&pdev->dev, &dump_codec,
			&dump_codec_dai, 1);
	dev_dbg(&pdev->dev, "spdif dump codec platfrom probe success\n");
	return ret;
}

static int spdif_dump_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static struct platform_driver spdif_codec_driver = {
	.driver = {
		.name = "spdif dump",
		.owner = THIS_MODULE,
	},
	.probe = spdif_dump_platform_probe,
	.remove = spdif_dump_platform_remove,
};

static int dump_modinit(void)
{
	return platform_driver_register(&spdif_codec_driver);
}
module_init(dump_modinit);

static void dump_exit(void)
{
	platform_driver_unregister(&spdif_codec_driver);
}
module_exit(dump_exit);

MODULE_DESCRIPTION("Dump Codec Driver");
MODULE_AUTHOR("shicheng.cheng<shicheng.cheng@ingenic.com>");
MODULE_LICENSE("GPL");
