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
#include "../icodec/icdc_d3.h"

#define GPIO_PG(n)      (5*32 + 23 + n)
#define phoenix_SPK_GPIO GPIO_PB(0)
#define phoenix_SPK_EN 0

unsigned long codec_sysclk = -1;
static int phoenix_spk_power(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		gpio_direction_output(phoenix_SPK_GPIO, phoenix_SPK_EN);
		printk("gpio speaker enable %d\n", gpio_get_value(phoenix_SPK_GPIO));
	} else {
		gpio_direction_output(phoenix_SPK_GPIO, !phoenix_SPK_EN);
		printk("gpio speaker disable %d\n", gpio_get_value(phoenix_SPK_GPIO));
	}
	return 0;
}

void phoenix_spk_sdown(struct snd_pcm_substream *sps){
		gpio_direction_output(phoenix_SPK_GPIO, !phoenix_SPK_EN);
		return;
}

int phoenix_spk_sup(struct snd_pcm_substream *sps){
		gpio_direction_output(phoenix_SPK_GPIO, phoenix_SPK_EN);
		return 0;
}

int phoenix_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	/*FIXME snd_soc_dai_set_pll*/
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S|SND_SOC_DAIFMT_CBM_CFM);
	if (ret)
		return ret;
	ret = snd_soc_dai_set_sysclk(cpu_dai, JZ_I2S_INNER_CODEC, 24000000, SND_SOC_CLOCK_OUT);
	if (ret)
		return ret;
	return 0;
};

int phoenix_i2s_hw_free(struct snd_pcm_substream *substream)
{
	/*notify release pll*/
	return 0;
};


static struct snd_soc_ops phoenix_i2s_ops = {
	.startup = phoenix_spk_sup,
	.shutdown = phoenix_spk_sdown,
	.hw_params = phoenix_i2s_hw_params,
	.hw_free = phoenix_i2s_hw_free,

};
static const struct snd_soc_dapm_widget phoenix_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", phoenix_spk_power),
	SND_SOC_DAPM_MIC("Mic Buildin", NULL),
	SND_SOC_DAPM_MIC("DMic", NULL),
};

static struct snd_soc_jack phoenix_icdc_d3_hp_jack;
static struct snd_soc_jack_pin phoenix_icdc_d3_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

#ifdef HAVE_HEADPHONE
static struct snd_soc_jack_gpio phoenix_icdc_d3_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};
#endif

/* phoenix machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* ext speaker connected to DO_LO_PWM  */
	{"Speaker", NULL, "DO_LO_PWM"},

	/* mic is connected to AIP/N1 */
	{"MICBIAS", NULL, "Mic Buildin"},
	{"DMIC", NULL, "DMic"},

};

static int phoenix_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err;
	err = devm_gpio_request(card->dev, phoenix_SPK_GPIO, "Speaker_en");
	if (err)
		return err;
	err = snd_soc_dapm_new_controls(dapm, phoenix_dapm_widgets,
			ARRAY_SIZE(phoenix_dapm_widgets));
	if (err){
		printk("phoenix dai add controls err!!\n");
		return err;
	}
	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err){
		printk("add phoenix dai routes err!!\n");
		return err;
	}
	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &phoenix_icdc_d3_hp_jack);
	snd_soc_jack_add_pins(&phoenix_icdc_d3_hp_jack,ARRAY_SIZE(phoenix_icdc_d3_hp_jack_pins),phoenix_icdc_d3_hp_jack_pins);
#ifdef HAVE_HEADPHONE
	if (gpio_is_valid(DORADO_HP_DET)) {
		phoenix_icdc_d3_jack_gpio[jack].gpio = PHOENIX_HP_DET;
		phoenix_icdc_d3_jack_gpio[jack].invert = !PHOENIX_HP_DET_LEVEL;
		snd_soc_jack_add_gpios(&phoenix_icdc_d3_hp_jack, 1, phoenix_icdc_d3_jack_gpio);
	}
#else
	snd_soc_dapm_disable_pin(dapm, "Headphone Jack");
#endif

	snd_soc_dapm_force_enable_pin(dapm, "Speaker");
	snd_soc_dapm_force_enable_pin(dapm, "Mic Buildin");
	snd_soc_dapm_force_enable_pin(dapm, "DMic");

	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link phoenix_dais[] = {
	[0] = {
		.name = "phoenix ICDC",
		.stream_name = "phoenix ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = phoenix_dlv_dai_link_init,
		.codec_dai_name = "icdc-d3-hifi",
		.codec_name = "icdc-d3",
		.ops = &phoenix_i2s_ops,
	},
	[1] = {
		.name = "phoenix PCMBT",
		.stream_name = "phoenix PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
	[2] = {
		.name = "PHOENIX DMIC",
		.stream_name = "PHOENIX DMIC",
		.platform_name = "jz-asoc-dmic-dma",
		.cpu_dai_name = "jz-asoc-dmic",
		.codec_dai_name = "dmic dump dai",
		.codec_name = "dmic dump",
	},

};

static struct snd_soc_card phoenix = {
	.name = "phoenix",
	.owner = THIS_MODULE,
	.dai_link = phoenix_dais,
	.num_links = ARRAY_SIZE(phoenix_dais),
};
static struct platform_device *phoenix_snd_device;


static int phoenix_init(void)
{
	int ret;
	phoenix_snd_device = platform_device_alloc("soc-audio", -1);
	if (!phoenix_snd_device) {
		gpio_free(phoenix_SPK_GPIO);
		return -ENOMEM;
	}

	platform_set_drvdata(phoenix_snd_device, &phoenix);
	ret = platform_device_add(phoenix_snd_device);
	if (ret) {
		platform_device_put(phoenix_snd_device);
		gpio_free(phoenix_SPK_GPIO);
	}

	dev_info(&phoenix_snd_device->dev, "Alsa sound card:phoenix init ok!!!\n");
	return ret;
}

static void phoenix_exit(void)
{
	platform_device_unregister(phoenix_snd_device);
	gpio_free(phoenix_SPK_GPIO);
}

module_init(phoenix_init);
module_exit(phoenix_exit);

MODULE_AUTHOR("sccheng<shicheng.cheng@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC phoenix Snd Card");
MODULE_LICENSE("GPL");
