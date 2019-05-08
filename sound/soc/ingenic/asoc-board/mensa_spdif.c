 /*
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: cli <chen.li@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <linux/gpio.h>

static struct snd_soc_ops mensa_i2s_ops = {

};

#ifndef GPIO_PG
#define GPIO_PG(n)      (5*32 + 23 + n)
#endif

static struct snd_soc_dai_link mensa_dais[] = {
	[0] = {
		.name = "MENSA ICDC",
		.stream_name = "MENSA ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-spdif",
		.codec_dai_name = "spdif dump dai",
		.codec_name = "spdif dump",
		.ops = &mensa_i2s_ops,
	},
	[1] = {
		.name = "MENSA PCMBT",
		.stream_name = "MENSA PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
};

static struct snd_soc_card mensa = {
	.name = "mensa",
	.owner = THIS_MODULE,
	.dai_link = mensa_dais,
	.num_links = ARRAY_SIZE(mensa_dais),
};

static struct platform_device *mensa_snd_device;


static struct snd_soc_card mensa = {
	.name = "mensa",
	.owner = THIS_MODULE,
	.dai_link = mensa_dais,
	.num_links = ARRAY_SIZE(mensa_dais),
};

static int snd_mensa_probe(struct platform_device *pdev)
{
	int ret = 0;
	mensa.dev = &pdev->dev;
	ret = snd_soc_register_card(&mensa);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
	return ret;
}

static int snd_mensa_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&mensa);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_mensa_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-mensa",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_mensa_probe,
	.remove = snd_mensa_remove,
};
module_platform_driver(snd_mensa_driver);

MODULE_AUTHOR("shicheng.cheng<shicheng.cheng@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC mensa Snd Card");
MODULE_LICENSE("GPL");
