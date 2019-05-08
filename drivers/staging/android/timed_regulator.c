/* drivers/misc/timed_regulator.c
 *
 * Copyright (C) 2013 Ingenic, Inc.
 * Author: Aaron Wang<hfwang@ingenic.cn>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>

#include <linux/timed_output.h>
#include <linux/timed_regulator.h>

struct timed_regulator_data {
	struct timed_output_dev dev;
	struct hrtimer timer;
	spinlock_t lock;
	struct regulator *regulator;
	int max_timeout;

	struct work_struct  work;
	struct workqueue_struct *workqueue;
};

static void timed_regulator_work_handler(struct work_struct *work)
{
	struct timed_regulator_data *data = container_of(work, struct timed_regulator_data, work);
	if (!IS_ERR(data->regulator)) {
		if (regulator_is_enabled(data->regulator)) {
			regulator_disable(data->regulator);
		}
	}
}

static enum hrtimer_restart timed_regulator_timer_func(struct hrtimer *timer)
{
	struct timed_regulator_data *data =
		container_of(timer, struct timed_regulator_data, timer);

	schedule_work(&data->work);
	return HRTIMER_NORESTART;
}

static int timed_regulator_get_time(struct timed_output_dev *dev)
{
	struct timed_regulator_data *data =
		container_of(dev, struct timed_regulator_data, dev);

	if (hrtimer_active(&data->timer)) {
		ktime_t r = hrtimer_get_remaining(&data->timer);
		struct timeval t = ktime_to_timeval(r);
		return t.tv_sec * 1000 + t.tv_usec / 1000;
	} else {
		return 0;
	}
}

static void timed_regulator_enable(struct timed_output_dev *dev, int value)
{
	struct timed_regulator_data *data =
		container_of(dev, struct timed_regulator_data, dev);
	unsigned long flags;

	cancel_work_sync(&data->work);

	if (!IS_ERR(data->regulator)) {
		if (value) {
			if (!regulator_is_enabled(data->regulator))
				regulator_enable(data->regulator);
		} else {
			if (regulator_is_enabled(data->regulator))
				regulator_disable(data->regulator);
		}
	}

	spin_lock_irqsave(&data->lock, flags);
	/* cancel previous timer and set GPIO according to value */
	hrtimer_cancel(&data->timer);

	if (value > 0) {
		if (value > data->max_timeout)
			value = data->max_timeout;

		hrtimer_start(&data->timer,
				ktime_set(value / 1000, (value % 1000) * 1000000),
				HRTIMER_MODE_REL);
	}

	spin_unlock_irqrestore(&data->lock, flags);
}

static int timed_regulator_probe(struct platform_device *pdev)
{
	struct timed_regulator_platform_data *pdata = pdev->dev.platform_data;
	struct timed_regulator *cur_regulator;

	struct timed_regulator_data *regulator_data, *cur_regulator_data;
	int i, ret = 0;

	if (!pdata)
		return -EBUSY;

	regulator_data = kzalloc(sizeof(struct timed_regulator_data) * pdata->num_regulators,GFP_KERNEL);

	if (!regulator_data)
		return -ENOMEM;

	for (i = 0; i < pdata->num_regulators; i++) {
		cur_regulator = &pdata->regs[i];
		cur_regulator_data = &regulator_data[i];

		hrtimer_init(&cur_regulator_data->timer, CLOCK_MONOTONIC,HRTIMER_MODE_REL);
		cur_regulator_data->timer.function = timed_regulator_timer_func;
		spin_lock_init(&cur_regulator_data->lock);

		cur_regulator_data->dev.name = cur_regulator->name;
		cur_regulator_data->dev.get_time = timed_regulator_get_time;
		cur_regulator_data->dev.enable = timed_regulator_enable;

		ret = timed_output_dev_register(&cur_regulator_data->dev);
		if (ret != 0) {
			pr_err("%s: register timed regulator device failed!\n",__func__);
			memset(cur_regulator_data, 0, sizeof(struct timed_regulator_data));
			continue;
		} else if (cur_regulator->reg_name != NULL) {
			cur_regulator_data->regulator = regulator_get(cur_regulator_data->dev.dev,cur_regulator->reg_name);
			if (IS_ERR(cur_regulator_data->regulator)) {
				pr_err("%s: get regulator %s failed!\n",__func__,cur_regulator->reg_name);
				memset(cur_regulator_data, 0, sizeof(struct timed_regulator_data));
				continue;
			}
		}

		cur_regulator_data->max_timeout = cur_regulator->max_timeout;

		INIT_WORK(&cur_regulator_data->work, timed_regulator_work_handler);
	}

	platform_set_drvdata(pdev, regulator_data);

	return 0;
}

static int timed_regulator_remove(struct platform_device *pdev)
{
	struct timed_regulator_platform_data *pdata = pdev->dev.platform_data;
	struct timed_regulator_data *regulator_data = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < pdata->num_regulators; i++) {
		timed_output_dev_unregister(&regulator_data[i].dev);
		if (!IS_ERR(regulator_data->regulator)) {
			regulator_put(regulator_data[i].regulator);
		}
	}

	kfree(regulator_data);

	return 0;
}

static struct platform_driver timed_regulator_driver = {
	.probe		= timed_regulator_probe,
	.remove		= timed_regulator_remove,
	.driver		= {
		.name	= TIMED_REGULATOR_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init timed_regulator_init(void)
{
	return platform_driver_register(&timed_regulator_driver);
}

static void __exit timed_regulator_exit(void)
{
	platform_driver_unregister(&timed_regulator_driver);
}

module_init(timed_regulator_init);
module_exit(timed_regulator_exit);

MODULE_AUTHOR("Aaron Wang <hfwang@ingenic.cn>");
MODULE_DESCRIPTION("timed regulator driver");
MODULE_LICENSE("GPL");
