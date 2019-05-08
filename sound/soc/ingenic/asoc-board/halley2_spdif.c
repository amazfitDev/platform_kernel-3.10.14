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

static struct snd_soc_ops halley2_i2s_ops = {

};

#ifndef GPIO_PG
#define GPIO_PG(n)      (5*32 + 23 + n)
#endif

static struct snd_soc_dai_link halley2_dais[] = {
	[0] = {
		.name = "HALLEY2 ICDC",
		.stream_name = "HALLEY2 ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-spdif",
		.codec_dai_name = "spdif dump dai",
		.codec_name = "spdif dump",
		.ops = &halley2_i2s_ops,
	},
	[1] = {
		.name = "HALLEY2 PCMBT",
		.stream_name = "HALLEY2 PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "HALLEY2 DMIC",
		.stream_name = "HALLEY2 DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic dump dai",
		.codec_name = "dmic dump",
	},
};

static struct snd_soc_card halley2 = {
	.name = "halley2",
	.owner = THIS_MODULE,
	.dai_link = halley2_dais,
	.num_links = ARRAY_SIZE(halley2_dais),
};

static struct platform_device *halley2_snd_device;
static int halley2_init(void)
{
	/*struct jz_aic_gpio_func *gpio_info;*/
	int ret;
	halley2_snd_device = platform_device_alloc("soc-audio", -1);
	if (!halley2_snd_device)
		return -ENOMEM;

	platform_set_drvdata(halley2_snd_device, &halley2);
	ret = platform_device_add(halley2_snd_device);
	if (ret) {
		platform_device_put(halley2_snd_device);
	}

	dev_info(&halley2_snd_device->dev, "Alsa sound card:halley2 init ok!!!\n");
	return ret;
}

static void halley2_exit(void)
{
	platform_device_unregister(halley2_snd_device);
}

module_init(halley2_init);
module_exit(halley2_exit);

MODULE_AUTHOR("sccheng<shicheng.cheng@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC halley2 Snd Card");
MODULE_LICENSE("GPL");
