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
#include "../icodec/icdc_d1.h"

static const struct snd_soc_dapm_widget newton2_plus_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
};

static struct snd_soc_jack newton2_plus_icdc_d1_jack;
static struct snd_soc_jack_pin newton2_plus_icdc_d1_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

static struct snd_soc_jack_gpio newton2_plus_icdc_d1_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};

/* newton2_plus machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* headphone connected to AOHPL/R */
	{"Headphone Jack", NULL, "AOHPL"},
	{"Headphone Jack", NULL, "AOHPR"},
};

static int newton2_plus_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err;
	int jack = 0;

	err = snd_soc_dapm_new_controls(dapm, newton2_plus_dapm_widgets,
			ARRAY_SIZE(newton2_plus_dapm_widgets));
	if (err)
		return err;

	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err)
		return err;

	snd_soc_jack_new(codec, "Headphone Jack", SND_JACK_HEADPHONE, &newton2_plus_icdc_d1_jack);
	snd_soc_jack_add_pins(&newton2_plus_icdc_d1_jack,
			ARRAY_SIZE(newton2_plus_icdc_d1_jack_pins),
			newton2_plus_icdc_d1_jack_pins);
	icdc_d1_hp_detect(codec, &newton2_plus_icdc_d1_jack, SND_JACK_HEADPHONE);

	snd_soc_dapm_sync(dapm);
	return 0;
}

int newton2_plus_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/*FIXME snd_soc_dai_set_pll*/
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM);
	if (ret)
		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, 12000000, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;
	return 0;
};

int newton2_plus_i2s_hw_free(struct snd_pcm_substream *substream)
{
	/*notify release pll*/
	return 0;
};

static struct snd_soc_ops newton2_plus_i2s_ops = {
	.hw_params = newton2_plus_i2s_hw_params,
	.hw_free = newton2_plus_i2s_hw_free,
};

static struct snd_soc_dai_link newton2_plus_dais[] = {
	[0] = {
		.name = "Newton2_plus ICDC",
		.stream_name = "Newton2_plus ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = newton2_plus_dlv_dai_link_init,
		.codec_dai_name = "icdc-d1-hifi",
		.codec_name = "icdc-d1",
		.ops = &newton2_plus_i2s_ops,
	},
	[1] = {
		.name = "Newton2_plus PCMBT",
		.stream_name = "Newton2_plus PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "Newton2_plus",
		.stream_name = "Newton2_plus DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic dump dai",
		.codec_name = "dmic dump",
	},

};

static struct snd_soc_card newton2_plus = {
	.name = "newton2_plus",
	.owner = THIS_MODULE,
	.dai_link = newton2_plus_dais,
	.num_links = ARRAY_SIZE(newton2_plus_dais),
};

static int snd_newton2_plus_probe(struct platform_device *pdev)
{
	int ret = 0;
	newton2_plus.dev = &pdev->dev;
	ret = snd_soc_register_card(&newton2_plus);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
	return ret;
}

static int snd_newton2_plus_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&newton2_plus);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_newton2_plus_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-newton2-plus",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_newton2_plus_probe,
	.remove = snd_newton2_plus_remove,
};
module_platform_driver(snd_newton2_plus_driver);

MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC Dorado Snd Card");
MODULE_LICENSE("GPL");
