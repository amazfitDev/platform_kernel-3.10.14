/*
 * drivers/power/sm5007-charger.c
 *
 * Charger driver for SILICONMITUS SM5007 power management chip.
 *
 * Copyright (C) 2012-2014 SILICONMITUS COMPANY,LTD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#define SM5007_BATTERY_VERSION "SM5007_BATTERY_VERSION: 2015.6.30 V0.0.0.0"


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/power_supply.h>
#include <linux/mfd/sm5007.h>
#include <linux/power/sm5007_charger.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#ifdef CONFIG_SM5007_CHARGER_HOLD_WAKE_LOCK
#include <linux/wakelock.h>

struct wake_lock smsuspend_lock;
int smsuspend_lock_locked;

#endif

#if defined(CONFIG_USB_DWC2) && defined(CONFIG_DWC2_BOND_TO_CHARGER)
extern void dwc2_gadget_plug_change(int plugin);
#endif

struct sm5007_charger_info {
	struct device      *dev;
	struct power_supply	charger;

	struct work_struct	irq_work;

	struct mutex		lock;

    uint8_t chg_status; /* charging status */
	int		status;
	int		irq_base;

	struct sm5007_charger_type_data type;
};

static void sm5007_enable_autostop(struct sm5007_charger_info *info,
		int onoff)
{
	pr_info("%s:[BATT] Autostop set(%d)\n", __func__, onoff);

	if (onoff)
		sm5007_set_bits(info->dev->parent, SM5007_CHGCNTL1, SM5007_AUTOSTOP_MASK);
	else
		sm5007_clr_bits(info->dev->parent, SM5007_CHGCNTL1, SM5007_AUTOSTOP_MASK);
}

static void sm5007_set_rechg(struct sm5007_charger_info *info,
		int threshold)
{
	pr_info("%s:[BATT] RECHG threshold(%d)\n", __func__, threshold);

	if (threshold == SM5007_RECHG_M50)
		sm5007_set_bits(info->dev->parent, SM5007_CHGCNTL1, SM5007_RECHG_MASK);
	else
		sm5007_clr_bits(info->dev->parent, SM5007_CHGCNTL1, SM5007_RECHG_MASK);
}

static void sm5007_enable_chgen(struct sm5007_charger_info *info,
		int onoff)
{
	pr_info("%s:[BATT] CHGEN set(%d)\n", __func__, onoff);

	if (onoff)
		sm5007_set_bits(info->dev->parent, SM5007_CHGCNTL1, SM5007_CHGEN_MASK);
	else
		sm5007_clr_bits(info->dev->parent, SM5007_CHGCNTL1, SM5007_CHGEN_MASK);
}

static void sm5007_set_topoff_current(struct sm5007_charger_info *info,
		int topoff_current_ma)
{
	int temp = 0, reg_val = 0, data = 0;

	if(topoff_current_ma <= 10)
		topoff_current_ma = 10;
	else if (topoff_current_ma >= 80)
		topoff_current_ma = 80;

	data = (topoff_current_ma - 10) / 10;

	sm5007_read(info->dev->parent, SM5007_CHGCNTL3, &reg_val);

	temp = ((reg_val & ~SM5007_TOPOFF_MASK) | (data << SM5007_TOPOFF_SHIFT));

	sm5007_write(info->dev->parent, SM5007_CHGCNTL3, temp);

	pr_info("%s : SM5007_CHGCNTL3 (topoff current) : 0x%02x\n",
		__func__, data);
}

static void sm5007_set_fast_charging_current(struct sm5007_charger_info *info,
		int charging_current_ma)
{
	int temp = 0, reg_val = 0, data = 0;

	if(charging_current_ma <= 60)
		charging_current_ma = 60;
	else if (charging_current_ma >= 600)
		charging_current_ma = 600;

	data = (charging_current_ma - 60) / 20;

	sm5007_read(info->dev->parent, SM5007_CHGCNTL3, &reg_val);

	temp = ((reg_val & ~SM5007_FASTCHG_MASK) | (data << SM5007_FASTCHG_SHIFT));

	sm5007_write(info->dev->parent, SM5007_CHGCNTL3, temp);

	pr_info("%s : SM5007_CHGCNTL3 (fastchg current) : 0x%02x\n",
		__func__, data);
}

static void sm5007_set_regulation_voltage(struct sm5007_charger_info *info,
		int float_voltage_mv)
{
	int data = 0;

	if ((float_voltage_mv) <= 4000)
		float_voltage_mv = 4000;
	else if ((float_voltage_mv) >= 4430)
		float_voltage_mv = 4300;

	data = ((float_voltage_mv - 4000) / 10);

	sm5007_write(info->dev->parent, SM5007_CHGCNTL2, data);

	pr_info("%s : SM5007_CHGCNTL3 (Battery regulation voltage) : 0x%02x\n",
		__func__, data);
}

static int get_power_supply_status(struct sm5007_charger_info *info)
{
	uint8_t status;
	int ret = 0;

	/* get  power supply status */
	ret = sm5007_read(info->dev->parent, SM5007_STATUS, &status);
	if (ret < 0) {
		dev_err(info->dev, "Error in reading the control register\n");
		return ret;
	}

    if ((status & SM5007_STATE_TOPOFF) || (status & SM5007_STATE_DONE)) {
        info->chg_status
            = POWER_SUPPLY_STATUS_FULL;
		pr_info("%s : Status, Power Supply Full \n", __func__);
    }else if (status & SM5007_STATE_CHGON) {
        info->chg_status
            = POWER_SUPPLY_STATUS_CHARGING;
    }else if ((status & SM5007_STATE_VBATOVP) || (status & SM5007_STATE_VBUSOVP)) {
        info->chg_status
            = POWER_SUPPLY_STATUS_NOT_CHARGING;
    }else if (status & SM5007_STATE_VBUSUVLO) {
        info->chg_status
            = POWER_SUPPLY_STATUS_DISCHARGING;
    }else {
        pr_info("%s : Status, ELSE \n", __func__);
    }

	return status;
}


/* Initial setting of charger */
static int sm5007_init_charger(struct sm5007_charger_info *info)
{
    int err;

    sm5007_enable_autostop(info, info->type.chg_autostop); //AUTOSTOP : Enable
    sm5007_set_rechg(info, info->type.chg_autostop); //RECHG : -100mV
    sm5007_set_regulation_voltage(info, info->type.chg_batreg); //BATREG : 4350mV
    sm5007_set_topoff_current(info, info->type.chg_topoff); //Topoff current : 10mA
    sm5007_set_fast_charging_current(info, info->type.chg_fastchg); //Fastcharging current : 200mA

	return err;
}

static int get_power_supply_Android_status(struct sm5007_charger_info *info)
{
	get_power_supply_status(info);
	union power_supply_propval value;

	/* get  power supply status */
	switch (info->chg_status) {
		case	POWER_SUPPLY_STATUS_NOT_CHARGING:
				return POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;

		case	POWER_SUPPLY_STATUS_DISCHARGING:
				return POWER_SUPPLY_STATUS_DISCHARGING;
				break;

		case	POWER_SUPPLY_STATUS_CHARGING:
				return POWER_SUPPLY_STATUS_CHARGING;
				break;

		case	POWER_SUPPLY_STATUS_FULL:
                psy_do_property("sm5007-fuelgauge", get,
                    POWER_SUPPLY_PROP_CHARGE_FULL, value);
                pr_debug("%s : info->chg_status = %d, value.intval = %d\n",__func__, info->chg_status, value.intval);
				if(value.intval == 1) { //If soc is 100, status is changed to FULL
					return POWER_SUPPLY_STATUS_FULL;
				} else {
					return POWER_SUPPLY_STATUS_CHARGING;
				}
				break;
		default:
				return POWER_SUPPLY_STATUS_UNKNOWN;
				break;
	}

	return POWER_SUPPLY_STATUS_UNKNOWN;
}

static int sm5007_chg_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sm5007_charger_info *info = dev_get_drvdata(psy->dev->parent);
	int ret = 0;
	uint8_t status;

	mutex_lock(&info->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = sm5007_read(info->dev->parent, SM5007_STATUS, &status);
		if (ret < 0) {
			dev_err(info->dev, "Error in reading the control register\n");
			mutex_unlock(&info->lock);
			return ret;
		}
        val->intval = (status & SM5007_STATE_VBUSPOK ? 1 : 0);
		break;

	case POWER_SUPPLY_PROP_STATUS:
		ret = get_power_supply_Android_status(info);
		val->intval = ret;
		info->status = ret;
		/* dev_dbg(info->dev, "Power Supply Status is %d\n",
							info->status); */
		break;

	 case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		ret = 0;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		ret = 0;
		break;

	default:
		mutex_unlock(&info->lock);
		return -ENODEV;
	}

	mutex_unlock(&info->lock);

	return ret;
}

static int sm5007_chg_set_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sm5007_charger_info *info = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	mutex_lock(&info->lock);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		break;

	case POWER_SUPPLY_PROP_STATUS:
		break;

    case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		ret = 0;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		break;

	default:
		mutex_unlock(&info->lock);
		return -ENODEV;
	}

	mutex_unlock(&info->lock);

	return ret;
}


static enum power_supply_property sm5007_batt_props[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_ONLINE,
//    POWER_SUPPLY_PROP_CURRENT_MAX,
//    POWER_SUPPLY_PROP_CURRENT_AVG,
//    POWER_SUPPLY_PROP_CURRENT_NOW,
//    POWER_SUPPLY_PROP_VOLTAGE_MAX,
};

static enum power_supply_property sm5007_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

struct sm5007_charger_irq_handler {
	char *name;
	int irq_index;
	irqreturn_t (*handler)(int irq, void *data);
};


static irqreturn_t sm5703_chg_vbuspok_irq_handler(int irq, void *data)
{
	struct sm5007_charger_info *info = data;
	uint8_t status;
	int ret = 0;

    printk("%s : VBUSPOK\n", __func__);

	ret = sm5007_read(info->dev->parent, SM5007_STATUS, &status);
	if (ret < 0) {
		dev_err(info->dev, "Error in reading the control register\n");
		return ret;
	}

    if (status & SM5007_STATE_VBUSPOK) {
		pr_info("%s : VBUSPOK STATUS \n", __func__);

		sm5007_set_rechg(info, info->type.chg_rechg); //RECHG : -100mV
		sm5007_set_regulation_voltage(info, info->type.chg_batreg); //BATREG : 4350mV
		sm5007_set_topoff_current(info, info->type.chg_topoff); //Topoff current : 10mA
		sm5007_set_fast_charging_current(info, info->type.chg_fastchg); //Fastcharging current : 200mA

		sm5007_enable_chgen(info, SM5007_CHGEN_ENABLED);

		power_supply_changed(&info->charger);
#if defined(CONFIG_USB_DWC2) && defined(CONFIG_DWC2_BOND_TO_CHARGER)
//turn on dwc2
		dwc2_gadget_plug_change(1);
#endif
	}
#ifdef CONFIG_SM5007_CHARGER_HOLD_WAKE_LOCK
    if(smsuspend_lock_locked == 0) {
        wake_lock(&smsuspend_lock);
        smsuspend_lock_locked = 1;
    }
#endif

	return IRQ_HANDLED;
}

static irqreturn_t sm5703_chg_vbusuvlo_irq_handler(int irq, void *data)
{
	struct sm5007_charger_info *info = data;
	uint8_t status;
	int ret = 0;

    printk("%s : VBUSUVLO\n", __func__);

	ret = sm5007_read(info->dev->parent, SM5007_STATUS, &status);
	if (ret < 0) {
		dev_err(info->dev, "Error in reading the control register\n");
		return ret;
	}

    if (status & SM5007_STATE_VBUSUVLO) {
		pr_info("%s : VBUSUVLO STATUS \n", __func__);
		sm5007_enable_chgen(info, SM5007_CHGEN_DISABLED);

		power_supply_changed(&info->charger);
#if defined(CONFIG_USB_DWC2) && defined(CONFIG_DWC2_BOND_TO_CHARGER)
//turn off dwc2
		dwc2_gadget_plug_change(0);
#endif
	}
#ifdef CONFIG_SM5007_CHARGER_HOLD_WAKE_LOCK
    if(smsuspend_lock_locked == 1){
        wake_unlock(&smsuspend_lock);
        smsuspend_lock_locked = 0;
    }
#endif
	return IRQ_HANDLED;
}

static irqreturn_t sm5703_chg_chgrstf_irq_handler(int irq, void *data)
{
	struct sm5007_charger_info *info = data;

    printk("%s : CHGRSTF\n", __func__);

    sm5007_set_rechg(info, info->type.chg_rechg); //RECHG : -100mV
    sm5007_set_regulation_voltage(info, info->type.chg_batreg); //BATREG : 4350mV
    sm5007_set_topoff_current(info, info->type.chg_topoff); //Topoff current : 10mA
    sm5007_set_fast_charging_current(info, info->type.chg_fastchg); //Fastcharging current : 200mA

	sm5007_enable_chgen(info, SM5007_CHGEN_DISABLED);
	sm5007_enable_chgen(info, SM5007_CHGEN_ENABLED);
	power_supply_changed(&info->charger);

	return IRQ_HANDLED;
}

static irqreturn_t sm5703_chg_topoff_irq_handler(int irq, void *data)
{
	struct sm5007_charger_info *info = data;

    printk("%s : TOPOFF\n", __func__);
	power_supply_changed(&info->charger);

	return IRQ_HANDLED;
}

static irqreturn_t sm5703_chg_done_irq_handler(int irq, void *data)
{
	struct sm5007_charger_info *info = data;

    printk("%s : DONE\n", __func__);
	power_supply_changed(&info->charger);

	return IRQ_HANDLED;
}

const struct sm5007_charger_irq_handler sm5007_chg_irq_handlers[] = {
	{
		.name = "VBUSPOK",
		.irq_index = SM5007_IRQ_VBUSPOK,
		.handler = sm5703_chg_vbuspok_irq_handler,
	},
	{
		.name = "VBUSUVLO",
		.irq_index = SM5007_IRQ_VBUSUVLO,
		.handler = sm5703_chg_vbusuvlo_irq_handler,
	},
	{
		.name = "CHGRSTF",
		.irq_index = SM5007_IRQ_CHGRSTF,
		.handler = sm5703_chg_chgrstf_irq_handler,
	},
	{
		.name = "TOPOFF",
		.irq_index = SM5007_IRQ_TOPOFF,
		.handler = sm5703_chg_topoff_irq_handler,
	},
	{
		.name = "DONE",
		.irq_index = SM5007_IRQ_DONE,
		.handler = sm5703_chg_done_irq_handler,
	},
};

static int register_irq(struct platform_device *pdev,
		struct sm5007_charger_info *info)
{
	int irq;
	int i, j;
	int ret;
	const struct sm5007_charger_irq_handler *irq_handler = sm5007_chg_irq_handlers;
	const char *irq_name;

	for (i = 0; i < ARRAY_SIZE(sm5007_chg_irq_handlers); i++) {
		irq_name = sm5007_get_irq_name_by_index(irq_handler[i].irq_index);
		irq = platform_get_irq_byname(pdev, irq_name);
		if(irq < 0) {
			pr_err("%s, ERROR irq = [%d] \n", __func__, irq);
			goto err_irq;
		}
		irq = irq + info->irq_base;
		ret = request_threaded_irq(irq, NULL, irq_handler[i].handler,
					   IRQF_ONESHOT | IRQF_TRIGGER_FALLING |
					   IRQF_NO_SUSPEND, irq_name, info);
		if (ret < 0) {
			pr_err("%s : Failed to request IRQ (%s): #%d: %d\n",
					__func__, irq_name, irq, ret);
			goto err_irq;
		}

		pr_info("%s : Register IRQ%d(%s) successfully\n",
				__func__, irq, irq_name);
	}

	return 0;
err_irq:
	for (j = 0; j < i; j++) {
		irq_name = sm5007_get_irq_name_by_index(irq_handler[j].irq_index);
		irq = platform_get_irq_byname(pdev, irq_name);
		free_irq(irq, info);
	}

	return ret;
}

static void unregister_irq(struct platform_device *pdev,
		struct sm5007_charger_info *info)
{
	int irq;
	int i;
	const char *irq_name;
	const struct sm5007_charger_irq_handler *irq_handler = sm5007_chg_irq_handlers;

	for (i = 0; i < ARRAY_SIZE(sm5007_chg_irq_handlers); i++) {
		irq_name = sm5007_get_irq_name_by_index(irq_handler[i].irq_index);
		irq = platform_get_irq_byname(pdev, irq_name);
		free_irq(irq, info);
	}
}


static int sm5007_charger_probe(struct platform_device *pdev)
{
	struct sm5007_charger_info *info;
	struct sm5007_charger_platform_data *pdata;
	int ret = 0;
    int status = 0;

	pr_debug("PMU: %s : version is %s\n", __func__,SM5007_BATTERY_VERSION);

	info = kzalloc(sizeof(struct sm5007_charger_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = &pdev->dev;
	info->status = POWER_SUPPLY_STATUS_CHARGING;
	pdata = pdev->dev.platform_data;

    info->type = pdata->type;
	info->irq_base = pdata->irq;

	mutex_init(&info->lock);
	platform_set_drvdata(pdev, info);

	info->charger.name = "sm5007-charger";
	info->charger.type = POWER_SUPPLY_TYPE_USB;
	info->charger.properties = sm5007_batt_props;
	info->charger.num_properties = ARRAY_SIZE(sm5007_batt_props);
	info->charger.get_property = sm5007_chg_get_prop;
	info->charger.set_property = sm5007_chg_set_prop;

	ret = sm5007_init_charger(info);
	if (ret)
		goto out;

	ret = power_supply_register(&pdev->dev, &info->charger);

	if (ret)
		info->charger.dev->parent = &pdev->dev;

#ifdef CONFIG_SM5007_CHARGER_HOLD_WAKE_LOCK
    wake_lock_init(&smsuspend_lock, WAKE_LOCK_SUSPEND, "sm5007-charger");
	ret = sm5007_read(info->dev->parent, SM5007_STATUS, &status);
	if (ret < 0) {
		dev_err(info->dev, "Error in reading the control register\n");
	}else if (status & SM5007_STATE_VBUSPOK) {
        wake_lock(&smsuspend_lock);
        smsuspend_lock_locked = 1;
	}
#endif

	/* Charger init and IRQ setting */
	ret = sm5007_init_charger(info);
	if (ret)
		goto out;

	ret = register_irq(pdev, info);
	if (ret < 0)
        goto err_reg_irq;

	return 0;

err_reg_irq:
    power_supply_unregister(&info->charger);
out:
	kfree(info);
	return ret;
}

static int sm5007_charger_remove(struct platform_device *pdev)
{
	struct sm5007_charger_info *info = platform_get_drvdata(pdev);
	int err;
	unregister_irq(pdev, info);
	power_supply_unregister(&info->charger);
	kfree(info);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#ifdef CONFIG_PM
static int sm5007_charger_suspend(struct device *dev)
{
//	struct sm5007_charger_info *info = dev_get_drvdata(dev);

	return 0;
}

static int sm5007_charger_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops sm5007_charger_pm_ops = {
	.suspend	= sm5007_charger_suspend,
	.resume		= sm5007_charger_resume,
};
#endif

static struct platform_driver sm5007_charger_driver = {
	.driver	= {
				.name	= "sm5007-charger",
				.owner	= THIS_MODULE,
#ifdef CONFIG_PM
				.pm	= &sm5007_charger_pm_ops,
#endif
	},
	.probe	= sm5007_charger_probe,
	.remove	= sm5007_charger_remove,
};

static int __init sm5007_charger_init(void)
{
	return platform_driver_register(&sm5007_charger_driver);
}
module_init(sm5007_charger_init);

static void __exit sm5007_charger_exit(void)
{
	platform_driver_unregister(&sm5007_charger_driver);
}
module_exit(sm5007_charger_exit);

MODULE_DESCRIPTION("SILICONMITUS SM5007 Battery driver");
MODULE_ALIAS("platform:sm5007-charger");
MODULE_LICENSE("GPL");
