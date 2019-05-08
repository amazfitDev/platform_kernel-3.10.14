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
#define halley2_SPK_GPIO GPIO_PC(23)
#define halley2_SPK_EN 0

unsigned long codec_sysclk = -1;
static int halley2_spk_power(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		gpio_direction_output(halley2_SPK_GPIO, halley2_SPK_EN);
		printk("gpio speaker enable %d\n", gpio_get_value(halley2_SPK_GPIO));
	} else {
		gpio_direction_output(halley2_SPK_GPIO, !halley2_SPK_EN);
		printk("gpio speaker disable %d\n", gpio_get_value(halley2_SPK_GPIO));
	}
	return 0;
}

void halley2_spk_sdown(struct snd_pcm_substream *sps){
		gpio_direction_output(halley2_SPK_GPIO, !halley2_SPK_EN);
		return;
}

int halley2_spk_sup(struct snd_pcm_substream *sps){
		gpio_direction_output(halley2_SPK_GPIO, halley2_SPK_EN);
		return 0;
}

int halley2_i2s_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params) {
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

int halley2_i2s_hw_free(struct snd_pcm_substream *substream)
{
	/*notify release pll*/
	return 0;
};


static struct snd_soc_ops halley2_i2s_ops = {
	.startup = halley2_spk_sup,
	.shutdown = halley2_spk_sdown,
	.hw_params = halley2_i2s_hw_params,
	.hw_free = halley2_i2s_hw_free,

};
static const struct snd_soc_dapm_widget halley2_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", halley2_spk_power),
	SND_SOC_DAPM_MIC("Mic Buildin", NULL),
	SND_SOC_DAPM_MIC("DMic", NULL),
};

static struct snd_soc_jack halley2_icdc_d3_hp_jack;
static struct snd_soc_jack_pin halley2_icdc_d3_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

#ifdef HAVE_HEADPHONE
static struct snd_soc_jack_gpio halley2_icdc_d3_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};
#endif

/* halley2 machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* ext speaker connected to DO_LO_PWM  */
	{"Speaker", NULL, "DO_LO_PWM"},

	/* mic is connected to AIP/N1 */
	{"MICBIAS", NULL, "Mic Buildin"},
	{"DMIC", NULL, "DMic"},

};

static int halley2_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err;
	err = devm_gpio_request(card->dev, halley2_SPK_GPIO, "Speaker_en");
	if (err)
		return err;
	err = snd_soc_dapm_new_controls(dapm, halley2_dapm_widgets,
			ARRAY_SIZE(halley2_dapm_widgets));
	if (err){
		printk("halley2 dai add controls err!!\n");
		return err;
	}
	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err){
		printk("add halley2 dai routes err!!\n");
		return err;
	}
	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &halley2_icdc_d3_hp_jack);
	snd_soc_jack_add_pins(&halley2_icdc_d3_hp_jack,ARRAY_SIZE(halley2_icdc_d3_hp_jack_pins),halley2_icdc_d3_hp_jack_pins);
#ifdef HAVE_HEADPHONE
	if (gpio_is_valid(DORADO_HP_DET)) {
		halley2_icdc_d3_jack_gpio[jack].gpio = HALLEY2_HP_DET;
		halley2_icdc_d3_jack_gpio[jack].invert = !HALLEY2_HP_DET_LEVEL;
		snd_soc_jack_add_gpios(&halley2_icdc_d3_hp_jack, 1, halley2_icdc_d3_jack_gpio);
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

static struct snd_soc_dai_link halley2_dais[] = {
	[0] = {
		.name = "halley2 ICDC",
		.stream_name = "halley2 ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = halley2_dlv_dai_link_init,
		.codec_dai_name = "icdc-d3-hifi",
		.codec_name = "icdc-d3",
		.ops = &halley2_i2s_ops,
	},
	[1] = {
		.name = "halley2 PCMBT",
		.stream_name = "halley2 PCMBT",
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
	int ret;
	halley2_snd_device = platform_device_alloc("soc-audio", -1);
	if (!halley2_snd_device) {
		gpio_free(halley2_SPK_GPIO);
		return -ENOMEM;
	}

	platform_set_drvdata(halley2_snd_device, &halley2);
	ret = platform_device_add(halley2_snd_device);
	if (ret) {
		platform_device_put(halley2_snd_device);
		gpio_free(halley2_SPK_GPIO);
	}

	dev_info(&halley2_snd_device->dev, "Alsa sound card:halley2 init ok!!!\n");
	return ret;
}

static void halley2_exit(void)
{
	platform_device_unregister(halley2_snd_device);
	gpio_free(halley2_SPK_GPIO);
}

module_init(halley2_init);
module_exit(halley2_exit);

MODULE_AUTHOR("sccheng<shicheng.cheng@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC halley2 Snd Card");
MODULE_LICENSE("GPL");
