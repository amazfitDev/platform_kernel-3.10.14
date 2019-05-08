/* drivers/pwm/pwm-jz.c
 * PWM driver of Ingenic's SoC M200
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author:	King liuyang <liuyang@ingenic.cn>
 * Modified:	Sun Jiwei <jiwei.sun@ingenic.com>
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <soc/extal.h>
#include <soc/gpio.h>
#include <mach/jztcu.h>

#define NUM_PWM		(NR_TCU_CH)

struct jz_pwm_device{
	struct tcu_device *tcu_cha;
};

struct jz_pwm_chip {
	struct jz_pwm_device pwm_chrs[NUM_PWM];
	struct pwm_chip chip;
};

static inline struct jz_pwm_chip *to_jz(struct pwm_chip *chip)
{
	return container_of(chip, struct jz_pwm_chip, chip);
}

static int jz_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_chip *jz_pwm = to_jz(chip);
	struct tcu_device *tcu_pwm;
	int id = pwm -> hwpwm;
	if ((id < 0) || (id > 7) || (id == 5)) {
		pr_err("%s, tcu channel %d is not exit\n", __func__, id);
		return -ENODEV;
	}

#if 0
	tcu_pwm= tcu_request(id,NULL);
#else
	tcu_pwm= tcu_request(id);
#endif

	if(IS_ERR(tcu_pwm)) {
		pr_err("%s request tcu channel %d failed!\n", __func__, id);
		return -ENODEV;
	}

	jz_pwm->pwm_chrs[id].tcu_cha = tcu_pwm;

	tcu_pwm->pwm_flag = pwm->flags;
	tcu_pwm->init_level = 0;
	pr_debug("%s, request pwm channel %d successfully\n", __func__, id);

	return 0;
}

static void jz_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_chip *jz_pwm = to_jz(chip);
	struct tcu_device *tcu_pwm = jz_pwm->pwm_chrs[pwm->hwpwm].tcu_cha;
	tcu_free(tcu_pwm);
}

static int jz_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			     int duty_ns, int period_ns)
{
	struct jz_pwm_chip *jz_pwm = to_jz(chip);
	int id = pwm->hwpwm;
	struct tcu_device *tcu_pwm = jz_pwm->pwm_chrs[id].tcu_cha;
	unsigned long long tmp;
	unsigned long period, duty;
	int prescaler = 0; /*prescale = 0,1,2,3,4,5*/

	if (duty_ns < 0 || duty_ns > period_ns) {
		pr_err("%s, duty_ns(%d)< 0 or duty_ns > period_ns(%d)\n",
				__func__, duty_ns, period_ns);
		return -EINVAL;
	}
	if (period_ns < 200 || period_ns > 1000000000) {
		pr_err("%s, period_ns(%d) > 100000000 or < 200\n",
				__func__, period_ns);
		return -EINVAL;
	}

	tcu_pwm->id = id;
	tcu_pwm->pwm_flag = pwm->flags;

#ifndef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
	tmp = JZ_EXTAL;
#else
	tmp = JZ_EXTAL;//32768 RTC CLOCK failure!
#endif
	tmp = tmp * period_ns;

	do_div(tmp, 1000000000);
	period = tmp;

	while (period > 0xffff && prescaler < 6) {
		period >>= 2;
		++prescaler;
	}

	if (prescaler == 6)
		return -EINVAL;

	tmp = (unsigned long long)period * duty_ns;
	do_div(tmp, period_ns);
	duty = tmp;

	if (duty >= period)
		duty = period - 1;
	tcu_pwm->full_num = period;
	tcu_pwm->half_num = (period - duty);
	tcu_pwm->divi_ratio = prescaler;
#ifdef CONFIG_SLCD_SUSPEND_ALARM_WAKEUP_REFRESH
	tcu_pwm->clock = RTC_EN;
#else
	tcu_pwm->clock = EXT_EN;
#endif
	tcu_pwm->count_value = 0;
	tcu_pwm->pwm_shutdown = 1;

	if(tcu_as_timer_config(tcu_pwm) != 0)
		return -EINVAL;

	return 0;
}

static int jz_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_chip *jz_pwm = to_jz(chip);
	struct tcu_device *tcu_pwm = jz_pwm->pwm_chrs[pwm->hwpwm].tcu_cha;

	tcu_enable(tcu_pwm);

	return 0;
}

static void jz_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz_pwm_chip *jz_pwm = to_jz(chip);
	struct tcu_device *tcu_pwm = jz_pwm->pwm_chrs[pwm->hwpwm].tcu_cha;

	tcu_disable(tcu_pwm);
}

static const struct pwm_ops jz_pwm_ops = {
	.request = jz_pwm_request,
	.free = jz_pwm_free,
	.config = jz_pwm_config,
	.enable = jz_pwm_enable,
	.disable = jz_pwm_disable,
	.owner = THIS_MODULE,
};

static int jz_pwm_probe(struct platform_device *pdev)
{
	struct jz_pwm_chip *jz_pwm;
	int ret;

	jz_pwm = devm_kzalloc(&pdev->dev,
			sizeof(struct jz_pwm_chip), GFP_KERNEL);
	if (!jz_pwm) {
		pr_err("%s %d,malloc jz_pwm_chip error\n",
				__func__, __LINE__);
		return -ENOMEM;
	}

	jz_pwm->chip.dev = &pdev->dev;
	jz_pwm->chip.ops = &jz_pwm_ops;
	jz_pwm->chip.npwm = NUM_PWM;
	jz_pwm->chip.base = -1;

	ret = pwmchip_add(&jz_pwm->chip);
	if (ret < 0) {
		devm_kfree(&pdev->dev, jz_pwm);
		return ret;
	}

	platform_set_drvdata(pdev, jz_pwm);

	return 0;
}

static int jz_pwm_remove(struct platform_device *pdev)
{
	struct jz_pwm_chip *jz_pwm = platform_get_drvdata(pdev);
	int ret;

	ret = pwmchip_remove(&jz_pwm->chip);
	if (ret < 0)
		return ret;

	devm_kfree(&pdev->dev, jz_pwm);

	return 0;
}

static struct platform_driver jz_pwm_driver = {
	.driver = {
		.name = "jz-pwm",
		.owner = THIS_MODULE,
	},
	.probe = jz_pwm_probe,
	.remove = jz_pwm_remove,
};
module_platform_driver(jz_pwm_driver);

MODULE_DESCRIPTION("Ingenic SoC PWM driver");
MODULE_ALIAS("platform:jz-pwm");
MODULE_LICENSE("GPL");
