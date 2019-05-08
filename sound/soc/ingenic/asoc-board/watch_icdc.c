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
#include <mach/jzsnd.h>
#include "../icodec/icdc_d1.h"
static struct snd_codec_data *codec_platform_data = NULL;
static struct snd_soc_ops watch_i2s_ops = {
};

static int watch_spk_power(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
	    if (codec_platform_data && (codec_platform_data->gpio_spk_en.gpio) != -1) {
	        gpio_direction_output(codec_platform_data->gpio_spk_en.gpio,
	                codec_platform_data->gpio_spk_en.active_level);
	        printk("gpio speaker enable %d\n", gpio_get_value(codec_platform_data->gpio_spk_en.gpio));
	    } else
	        printk("set speaker enable failed. please check codec_platform_data\n");
	} else {
	    if (codec_platform_data && (codec_platform_data->gpio_spk_en.gpio) != -1) {
	        gpio_direction_output(codec_platform_data->gpio_spk_en.gpio, !(codec_platform_data->gpio_spk_en.active_level));
	        printk("gpio speaker disable %d\n", gpio_get_value(codec_platform_data->gpio_spk_en.gpio));
	    } else
	        printk("set speaker disable failed. please check codec_platform_data\n");
	}
	return 0;
}

static const struct snd_soc_dapm_widget watch_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", watch_spk_power),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_MIC("Mic Buildin", NULL),
};

static struct snd_soc_jack watch_icdc_d1_jack;
static struct snd_soc_jack_pin watch_icdc_d1_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
	{
		.pin = "Mic Jack",
		.mask = SND_JACK_MICROPHONE,
	},
};

static struct snd_soc_jack_gpio watch_icdc_d1_jack_gpio[] = {
	{
		.name = "Headphone detection",
		.report = SND_JACK_HEADPHONE,
		.debounce_time = 150,
	}
};

/* watch machine audio_map */
static const struct snd_soc_dapm_route audio_map[] = {
	/* headphone connected to AOHPL/R */
	{"Headphone Jack", NULL, "AOHPL"},
	{"Headphone Jack", NULL, "AOHPR"},

	/* ext speaker connected to AOLOP/N  */
	{"Speaker", NULL, "AOLOP"},
	{"Speaker", NULL, "AOLON"},

	/* mic is connected to AIP/N1 */
	{"AIP1", NULL, "MICBIAS1"},
	{"AIN1", NULL, "MICBIAS1"},
	{"AIP2", NULL, "MICBIAS1"},

	{"MICBIAS1", NULL, "Mic Buildin"},
	{"MICBIAS1", NULL, "Mic Jack"},
};

static int watch_dlv_dai_link_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	struct snd_soc_card *card = rtd->card;
	int err;
	int jack = 0;

	if (codec_platform_data) {
	    err = devm_gpio_request(card->dev, codec_platform_data->gpio_spk_en.gpio, "Speaker_en");
	    if (err)
	        return err;
	} else {
	    pr_err("codec_platform_data gpio_spk_en is NULL\n");
	    return err;
	}


	err = snd_soc_dapm_new_controls(dapm, watch_dapm_widgets,
			ARRAY_SIZE(watch_dapm_widgets));
	if (err)
		return err;

	/* Set up rx1950 specific audio path audio_mapnects */
	err = snd_soc_dapm_add_routes(dapm, audio_map,
			ARRAY_SIZE(audio_map));
	if (err)
		return err;

	snd_soc_jack_new(codec, "Headset Jack", SND_JACK_HEADSET, &watch_icdc_d1_jack);
	snd_soc_jack_add_pins(&watch_icdc_d1_jack,
			ARRAY_SIZE(watch_icdc_d1_jack_pins),
			watch_icdc_d1_jack_pins);
	if (codec_platform_data){
        if (gpio_is_valid(codec_platform_data->gpio_hp_detect.gpio)) {
            watch_icdc_d1_jack_gpio[jack].gpio = codec_platform_data->gpio_hp_detect.gpio;
            watch_icdc_d1_jack_gpio[jack].invert = !(codec_platform_data->gpio_hp_detect.active_level);
            snd_soc_jack_add_gpios(&watch_icdc_d1_jack,
                    1,
                    watch_icdc_d1_jack_gpio);
        } else {
            icdc_d1_hp_detect(codec, &watch_icdc_d1_jack, SND_JACK_HEADSET);
        }
	} else
	    pr_debug("codec_platform_data gpio_hp_detect is NULL\n");


	snd_soc_dapm_sync(dapm);
	return 0;
}

static struct snd_soc_dai_link watch_dais[] = {
	[0] = {
		.name = "WATCH ICDC",
		.stream_name = "WATCH ICDC",
		.platform_name = "jz-asoc-aic-dma",
		.cpu_dai_name = "jz-asoc-aic-i2s",
		.init = watch_dlv_dai_link_init,
		.codec_dai_name = "icdc-d1-hifi",
		.codec_name = "icdc-d1",
		.ops = &watch_i2s_ops,
	},
	[1] = {
		.name = "WATCH PCMBT",
		.stream_name = "WATCH PCMBT",
		.platform_name = "jz-asoc-pcm-dma",
		.cpu_dai_name = "jz-asoc-pcm",
		.codec_dai_name = "pcm dump dai",
		.codec_name = "pcm dump",
	},
};

static struct snd_soc_card watch = {
	.name = "watch",
	.owner = THIS_MODULE,
	.dai_link = watch_dais,
	.num_links = ARRAY_SIZE(watch_dais),
};

static int snd_watch_probe(struct platform_device *pdev)
{
    int ret = 0;
    watch.dev = &pdev->dev;
    codec_platform_data = (struct snd_codec_data *)watch.dev->platform_data;
    ret = snd_soc_register_card(&watch);
    if (ret)
        dev_err(&pdev->dev, "snd_soc_register_card failed %d\n", ret);
    return ret;
}

static int snd_watch_remove(struct platform_device *pdev)
{
	snd_soc_unregister_card(&watch);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver snd_watch_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ingenic-watch",
		.pm = &snd_soc_pm_ops,
	},
	.probe = snd_watch_probe,
	.remove = snd_watch_remove,
};
module_platform_driver(snd_watch_driver);

MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_DESCRIPTION("ALSA SoC Watch Snd Card");
MODULE_LICENSE("GPL");
