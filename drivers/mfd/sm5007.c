/*
 * driver/mfd/sm5007.c
 *
 * Core driver implementation to access SILICONMITUS SM5007 power management chip.
 *
 * Copyright (C) 2012-2014 SILICONMITUS COMPANY,LTD
 *
 * Based on code
 *	Copyright (C) 2011 NVIDIA Corporation
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
/*#define DEBUG			1*/
/*#define VERBOSE_DEBUG		1*/
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/mfd/core.h>
#include <linux/mfd/sm5007.h>
#include <linux/delay.h>
#include <jz_notifier.h>

#ifndef CONFIG_BATTERY_SM5007
/*
 * Really ugly when CONFIG_BATTERY_SM5007 is not selected.
 */
//int g_soc;
//int g_fg_on_mode;
#endif

static inline int __sm5007_read(struct i2c_client *client,
				  u8 reg, uint8_t *val)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (uint8_t)ret;
	dev_dbg(&client->dev, "sm5007: reg read  reg=%x, val=%x\n",
				reg, *val);
	return 0;
}

static inline int __sm5007_bulk_reads(struct i2c_client *client, u8 reg,
				int len, uint8_t *val)
{
	int ret;
	int i;

	ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading from 0x%02x\n", reg);
		return ret;
	}
	for (i = 0; i < len; ++i) {
		dev_dbg(&client->dev, "sm5007: reg read  reg=%x, val=%x\n",
				reg + i, *(val + i));
	}
	return 0;
}

static inline int __sm5007_write(struct i2c_client *client,
				 u8 reg, uint8_t val)
{
	int ret;

	dev_dbg(&client->dev, "sm5007: reg write  reg=%x, val=%x\n",
				reg, val);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
				val, reg);
		return ret;
	}

	return 0;
}

static inline int __sm5007_bulk_writes(struct i2c_client *client, u8 reg,
				  int len, uint8_t *val)
{
	int ret;
	int i;

	for (i = 0; i < len; ++i) {
		dev_dbg(&client->dev, "sm5007: reg write  reg=%x, val=%x\n",
				reg + i, *(val + i));
	}

	ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writings to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}
/*
static inline int set_bank_sm5007(struct device *dev, int bank)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret;

	if (bank != (bank & 1))
		return -EINVAL;
	if (bank == sm5007->bank_num)
		return 0;
	ret = __sm5007_write(to_i2c_client(dev), SM5007_REG_BANKSEL, bank);
	if (!ret)
		sm5007->bank_num = bank;

	return ret;
}
*/
int sm5007_write(struct device *dev, u8 reg, uint8_t val)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 0);
	//if (!ret)
		ret = __sm5007_write(to_i2c_client(dev), reg, val);
	mutex_unlock(&sm5007->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_write);

int sm5007_write_bank1(struct device *dev, u8 reg, uint8_t val)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 1);
	//if (!ret)
		ret = __sm5007_write(to_i2c_client(dev), reg, val);
	mutex_unlock(&sm5007->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_write_bank1);

int sm5007_bulk_writes(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 0);
	//if (!ret)
		ret = __sm5007_bulk_writes(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&sm5007->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_bulk_writes);

int sm5007_bulk_writes_bank1(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 1);
	//if (!ret)
		ret = __sm5007_bulk_writes(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&sm5007->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_bulk_writes_bank1);

int sm5007_read(struct device *dev, u8 reg, uint8_t *val)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 0);
	//if (!ret)
		ret = __sm5007_read(to_i2c_client(dev), reg, val);
	mutex_unlock(&sm5007->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_read);

int sm5007_read_bank1(struct device *dev, u8 reg, uint8_t *val)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 1);
	//if (!ret)
		ret =  __sm5007_read(to_i2c_client(dev), reg, val);
	mutex_unlock(&sm5007->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_read_bank1);

int sm5007_bulk_reads(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 0);
	//if (!ret)
		ret = __sm5007_bulk_reads(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&sm5007->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_bulk_reads);

int sm5007_bulk_reads_bank1(struct device *dev, u8 reg, u8 len, uint8_t *val)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 1);
	//if (!ret)
		ret = __sm5007_bulk_reads(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&sm5007->io_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_bulk_reads_bank1);

int sm5007_set_bits(struct device *dev, u8 reg, uint8_t bit_mask)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 0);
	//if (!ret) {
		ret = __sm5007_read(to_i2c_client(dev), reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & bit_mask) != bit_mask) {
			reg_val |= bit_mask;
			ret = __sm5007_write(to_i2c_client(dev), reg,
								 reg_val);
		}
	//}
out:
	mutex_unlock(&sm5007->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_set_bits);

int sm5007_clr_bits(struct device *dev, u8 reg, uint8_t bit_mask)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 0);
	//if (!ret) {
		ret = __sm5007_read(to_i2c_client(dev), reg, &reg_val);
		if (ret)
			goto out;

		if (reg_val & bit_mask) {
			reg_val &= ~bit_mask;
			ret = __sm5007_write(to_i2c_client(dev), reg,
								 reg_val);
		}
	//}
out:
	mutex_unlock(&sm5007->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_clr_bits);

int sm5007_update(struct device *dev, u8 reg, uint8_t val, uint8_t mask)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 0);
	//if (!ret) {
		ret = __sm5007_read(sm5007->client, reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & mask) != val) {
			reg_val = (reg_val & ~mask) | (val & mask);
			ret = __sm5007_write(sm5007->client, reg, reg_val);
		}
	//}
out:
	mutex_unlock(&sm5007->io_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(sm5007_update);

int sm5007_update_bank1(struct device *dev, u8 reg, uint8_t val, uint8_t mask)
{
	struct sm5007 *sm5007 = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&sm5007->io_lock);
	//ret = set_bank_sm5007(dev, 1);
	//if (!ret) {
		ret = __sm5007_read(sm5007->client, reg, &reg_val);
		if (ret)
			goto out;

		if ((reg_val & mask) != val) {
			reg_val = (reg_val & ~mask) | (val & mask);
			ret = __sm5007_write(sm5007->client, reg, reg_val);
		}
	//}
out:
	mutex_unlock(&sm5007->io_lock);
	return ret;
}

static struct i2c_client *sm5007_i2c_client;
int sm5007_power_off(void)
{
	if (!sm5007_i2c_client)
		return -EINVAL;

	/* Power OFF */
	__sm5007_write(sm5007_i2c_client, SM5007_CNTL2, 0x4);

	return 0;
}

static int sm5007_register_reset_notifier(struct jz_notifier *nb)
{
	return jz_notifier_register(nb, NOTEFY_PROI_HIGH);
}

static int sm5007_unregister_reset_notifier(struct jz_notifier *nb)
{
	return jz_notifier_unregister(nb, NOTEFY_PROI_HIGH);
}
static int sm5007_reset_notifier_handler(struct jz_notifier *nb,void* data)
{
	int ret;
	printk("WARNNING:system will power!\n");
	ret = sm5007_power_off();
	if (ret < 0)
		printk("sm5007_power_off failed \n");
	return ret;
}

static int sm5007_remove_subdev(struct device *dev, void *unused)
{
	platform_device_unregister(to_platform_device(dev));
	return 0;
}

static int sm5007_remove_subdevs(struct sm5007 *sm5007)
{
	return device_for_each_child(sm5007->dev, NULL,
				     sm5007_remove_subdev);
}

static int sm5007_add_subdevs(struct sm5007 *sm5007,
				struct sm5007_platform_data *pdata)
{
	struct sm5007_subdev_info *subdev;
	struct platform_device *pdev;
	int i, ret = 0;

	for (i = 0; i < pdata->num_subdevs; i++) {
		subdev = &pdata->subdevs[i];

		pdev = platform_device_alloc(subdev->name, subdev->id);

		pdev->dev.parent = sm5007->dev;
		pdev->dev.platform_data = subdev->platform_data;

		ret = platform_device_add_resources(pdev, subdev->resources, subdev->num_resources);
		if(ret) {
			goto failed;
		}

		ret = platform_device_add(pdev);
		if (ret)
			goto failed;
	}
	return 0;

failed:
	sm5007_remove_subdevs(sm5007);
	return ret;
}

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
static void print_regs(const char *header, struct seq_file *s,
		struct i2c_client *client, int start_offset,
		int end_offset)
{
	uint8_t reg_val;
	int i;
	int ret;

	seq_printf(s, "%s\n", header);
	for (i = start_offset; i <= end_offset; ++i) {
		ret = __sm5007_read(client, i, &reg_val);
		if (ret >= 0)
			seq_printf(s, "Reg 0x%02x Value 0x%02x\n", i, reg_val);
	}
	seq_printf(s, "------------------\n");
}

static int dbg_sm_show(struct seq_file *s, void *unused)
{
	struct sm5007 *sm5007 = s->private;
	struct i2c_client *client = sm5007->client;

	seq_printf(s, "SM5007 Registers\n");
	seq_printf(s, "------------------\n");

	print_regs("0x00-0x03 Regs",	s, client, 0x00, 0x03);
	print_regs("0x0A-0x12 Regs",	s, client, 0x0A, 0x12);
	print_regs("0x20-0x22 Regs",	s, client, 0x20, 0x22);
	print_regs("0x30-0x4A Regs",	s, client, 0x30, 0x4A);
	return 0;
}

static int dbg_sm_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_sm_show, inode->i_private);
}

static const struct file_operations debug_fops = {
	.open		= dbg_sm_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static void __init sm5007_debuginit(struct sm5007 *ricoh)
{
	(void)debugfs_create_file("sm5007", S_IRUGO, NULL,
			ricoh, &debug_fops);
}
#else
static void print_regs(const char *header, struct i2c_client *client,
		int start_offset, int end_offset)
{
	uint8_t reg_val;
	int i;
	int ret;

	printk(KERN_INFO "%s\n", header);
	for (i = start_offset; i <= end_offset; ++i) {
		ret = __sm5007_read(client, i, &reg_val);
		if (ret >= 0)
			printk(KERN_INFO "Reg 0x%02x Value 0x%02x\n",
							 i, reg_val);
	}
	printk(KERN_INFO "------------------\n");
}

static void __init sm5007_debuginit(struct sm5007 *ricoh)
{
	struct i2c_client *client = ricoh->client;

	printk(KERN_INFO "SM5007 Registers\n");
	printk(KERN_INFO "------------------\n");

	print_regs("0x00-0x03 Regs",	client, 0x00, 0x03);
	print_regs("0x0A-0x12 Regs",	client, 0x0A, 0x12);
	print_regs("0x20-0x22 Regs",	client, 0x20, 0x22);
	print_regs("0x30-0x4A Regs",	client, 0x30, 0x4A);

	return 0;
}
#endif

static u8 reg_addr = 0;

static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret;
	unsigned long val;
	uint8_t data;

	if (strict_strtoul(buf, 16, &val))
        return -EINVAL;
	data = val;
	ret = sm5007_write(dev, reg_addr, data);
	if (ret < 0) {
		printk("%s --> Write reg failed!\n",__func__);
		return -EIO;
	}

	return size;
};

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t ret;
	uint8_t data;

	ret = sm5007_read(dev, reg_addr, &data);
	if (ret < 0) {
		printk("%s --> Read reg failed!\n",__func__);
		return -EIO;
	}

	ret = sprintf(buf, "0x%02x\n", data);
	return ret;
};

static ssize_t attr_addr_get(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	ssize_t ret = sprintf(buf, "0x%02x\n", reg_addr);
	return ret;
};

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	reg_addr = val;

	return size;
};

static struct device_attribute attributes[] = {

	__ATTR(reg_value, 0600, attr_reg_get, attr_reg_set),
	__ATTR(reg_addr, 0600, attr_addr_get, attr_addr_set),
};

static int create_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for ( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int sm5007_i2c_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct sm5007 *sm5007;
	struct sm5007_platform_data *pdata = client->dev.platform_data;
	int ret;
	uint8_t data = 0;

	sm5007 = kzalloc(sizeof(struct sm5007), GFP_KERNEL);
	if (sm5007 == NULL)
		return -ENOMEM;

	sm5007->client = client;
	sm5007->dev = &client->dev;
	i2c_set_clientdata(client, sm5007);

	mutex_init(&sm5007->io_lock);

    sm5007->sm5007_notifier.jz_notify = sm5007_reset_notifier_handler;
	sm5007->sm5007_notifier.level = NOTEFY_PROI_NORMAL;
	sm5007->sm5007_notifier.msg = JZ_POST_HIBERNATION;
    /* For init PMIC_IRQ port */
	//ret = pdata->init_port(client->irq);

	if (client->irq) {
		ret = sm5007_irq_init(sm5007, client->irq, pdata->irq_base);
		if (ret) {
			dev_err(&client->dev, "IRQ init failed: %d\n", ret);
			goto err_irq_init;
		}
	}

	ret = sm5007_add_subdevs(sm5007, pdata);
	if (ret) {
		dev_err(&client->dev, "add devices failed: %d\n", ret);
		goto err_add_devs;
	}

	ret = sm5007_register_reset_notifier(&(sm5007->sm5007_notifier));
	if (ret) {
		printk("sm5007_register_reset_notifier failed\n");
		goto err_add_notifier;
	}

	sm5007_debuginit(sm5007);

	sm5007_i2c_client = client;

	create_sysfs_interfaces(&client->dev);

	/* For Support Long Press(7S) wakeup pin to reboot */
	sm5007_read(sm5007->dev, 0x11, &data);
	data |= 1 << 5;
	sm5007_write(sm5007->dev, 0x11, data);

	return 0;

err_add_notifier:
err_add_devs:
	if (client->irq)
		sm5007_irq_exit(sm5007);
err_irq_init:
	kfree(sm5007);
	return ret;
}

static int  sm5007_i2c_remove(struct i2c_client *client)
{
	int ret = 0;
        struct sm5007 *sm5007 = i2c_get_clientdata(client);

	if (client->irq)
		sm5007_irq_exit(sm5007);

        ret = sm5007_unregister_reset_notifier(&(sm5007->sm5007_notifier));
	if (ret) {
		printk("sm5007_unregister_reset_notifier failed\n");
	}

	sm5007_remove_subdevs(sm5007);
	kfree(sm5007);
	return ret;
}

#ifdef CONFIG_PM
static int sm5007_i2c_suspend(struct i2c_client *client, pm_message_t state)
{
	printk(KERN_INFO "PMU: %s:\n", __func__);
#if 0
	if (client->irq)
		disable_irq(client->irq);
#endif
	return 0;
}

static int sm5007_i2c_resume(struct i2c_client *client)
{
	printk(KERN_INFO "PMU: %s:\n", __func__);

#if 0
        uint8_t reg_val;
	int ret;

	/* Disable all Interrupt */
	__sm5007_write(client, SM5007_INTC_INTEN, 0x0);

	ret = __sm5007_read(client, SM5007_INT_IR_SYS, &reg_val);
	if (reg_val & 0x01) { /* If PWR_KEY wakeup */
		printk(KERN_INFO "PMU: %s: PWR_KEY Wakeup\n", __func__);
		pwrkey_wakeup = 1;
		/* Clear PWR_KEY IRQ */
		__sm5007_write(client, SM5007_INT_IR_SYS, 0x0);
	}
	enable_irq(client->irq);

	/* Enable all Interrupt */
	__sm5007_write(client, SM5007_INTC_INTEN, 0xff);
#endif

	return 0;
}

#endif

static const struct i2c_device_id sm5007_i2c_id[] = {
	{"sm5007", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sm5007_i2c_id);

static struct i2c_driver sm5007_i2c_driver = {
	.driver = {
		   .name = "sm5007",
		   .owner = THIS_MODULE,
		   },
	.probe = sm5007_i2c_probe,
	.remove = sm5007_i2c_remove,
#ifdef CONFIG_PM
	.suspend = sm5007_i2c_suspend,
	.resume = sm5007_i2c_resume,
#endif
	.id_table = sm5007_i2c_id,
};


static int __init sm5007_i2c_init(void)
{
	int ret = -ENODEV;

	ret = i2c_add_driver(&sm5007_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);

	return ret;
}

subsys_initcall(sm5007_i2c_init);

static void __exit sm5007_i2c_exit(void)
{
	i2c_del_driver(&sm5007_i2c_driver);
}

module_exit(sm5007_i2c_exit);

MODULE_DESCRIPTION("SILICONMITUS SM5007 PMU multi-function core driver");
MODULE_LICENSE("GPL");
