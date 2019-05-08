/*
 * driver/mfd/sm5007-irq.c
 *
 * Interrupt driver for SILICONMITUS SM5007 power management chip.
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
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mfd/sm5007.h>

#define SM5007_INT1_MASK     SM5007_INT1MSK
#define SM5007_INT2_MASK     SM5007_INT2MSK
#define SM5007_INT3_MASK     SM5007_INT3MSK
#define SM5007_INT4_MASK     SM5007_INT4MSK
#define SM5007_INT5_MASK     SM5007_INT5MSK
#define SM5007_INT6_MASK     SM5007_INT6MSK


#define IRQ_NAME(irq) [irq] = irq##_NAME
static const char *sm5007_irq_names[] = {
	IRQ_NAME(SM5007_IRQ_VBATOVP),
	IRQ_NAME(SM5007_IRQ_TRITMROFF),
	IRQ_NAME(SM5007_IRQ_FASTTMROFF),
	IRQ_NAME(SM5007_IRQ_THEMREG),
	IRQ_NAME(SM5007_IRQ_THEMSHDN),
	IRQ_NAME(SM5007_IRQ_1STTHEMWARN),
	IRQ_NAME(SM5007_IRQ_2NDTHEMWARN),
	IRQ_NAME(SM5007_IRQ_THEMSAFE),

	IRQ_NAME(SM5007_IRQ_VBUSPOK),
	IRQ_NAME(SM5007_IRQ_VBUSUVLO),
	IRQ_NAME(SM5007_IRQ_VBUSOVP),
	IRQ_NAME(SM5007_IRQ_CHGON),
	IRQ_NAME(SM5007_IRQ_TOPOFF),
	IRQ_NAME(SM5007_IRQ_DONE),
	IRQ_NAME(SM5007_IRQ_CHGRSTF),

	IRQ_NAME(SM5007_IRQ_SHORTKEY),
	IRQ_NAME(SM5007_IRQ_LNGKEY_nONKEY),
	IRQ_NAME(SM5007_IRQ_MANUALRST),
	IRQ_NAME(SM5007_IRQ_VBATNG),

	IRQ_NAME(SM5007_IRQ_BUCK2POK),
	IRQ_NAME(SM5007_IRQ_BUCK3POK),
	IRQ_NAME(SM5007_IRQ_BUCK4POK),
	IRQ_NAME(SM5007_IRQ_LDO1POK),
	IRQ_NAME(SM5007_IRQ_LDO2POK),
	IRQ_NAME(SM5007_IRQ_LDO3POK),
	IRQ_NAME(SM5007_IRQ_LDO4POK),

	IRQ_NAME(SM5007_IRQ_LDO5POK),
	IRQ_NAME(SM5007_IRQ_LDO6POK),
	IRQ_NAME(SM5007_IRQ_LDO7POK),
	IRQ_NAME(SM5007_IRQ_LDO8POK),
	IRQ_NAME(SM5007_IRQ_LDO9POK),
	IRQ_NAME(SM5007_IRQ_PS1POK),
	IRQ_NAME(SM5007_IRQ_PS2POK),
	IRQ_NAME(SM5007_IRQ_PS3POK),

	IRQ_NAME(SM5007_IRQ_PS4POK),
	IRQ_NAME(SM5007_IRQ_PS5POK),
	IRQ_NAME(SM5007_IRQ_CPPOK),
	IRQ_NAME(SM5007_IRQ_PWRSTMF),
	IRQ_NAME(SM5007_IRQ_PWRSTMR),
};

const char *sm5007_get_irq_name_by_index(int index)
{
	return sm5007_irq_names[index];
}
EXPORT_SYMBOL(sm5007_get_irq_name_by_index);

enum SM5007_IRQ_OFFSET {
	SM5007_INT1_OFFSET = 0,
	SM5007_INT2_OFFSET,
	SM5007_INT3_OFFSET,
	SM5007_INT4_OFFSET,
	SM5007_INT5_OFFSET,
	SM5007_INT6_OFFSET,
};

struct sm5007_irq_data {
	u8	int_en_bit;
	u8	mask_reg_index;
};

#define SM5007_IRQ(_int_bit, _mask_ind) \
	{						            \
		.int_en_bit	= _int_bit,		    \
		.mask_reg_index	= _mask_ind,	\
	}

static const struct sm5007_irq_data sm5007_irqs[SM5007_NR_IRQS] = {
	[SM5007_IRQ_VBATOVP]    = SM5007_IRQ(0, 0),
	[SM5007_IRQ_TRITMROFF]  = SM5007_IRQ(1, 0),
	[SM5007_IRQ_FASTTMROFF] = SM5007_IRQ(2, 0),
	[SM5007_IRQ_THEMREG]    = SM5007_IRQ(3, 0),
	[SM5007_IRQ_THEMSHDN]   = SM5007_IRQ(4, 0),
	[SM5007_IRQ_1STTHEMWARN] = SM5007_IRQ(5, 0),
	[SM5007_IRQ_2NDTHEMWARN] = SM5007_IRQ(6, 0),
	[SM5007_IRQ_THEMSAFE]   = SM5007_IRQ(7, 0),

	[SM5007_IRQ_VBUSPOK]    = SM5007_IRQ(0, 1),
	[SM5007_IRQ_VBUSUVLO]   = SM5007_IRQ(1, 1),
	[SM5007_IRQ_VBUSOVP]    = SM5007_IRQ(2, 1),
	[SM5007_IRQ_CHGON]      = SM5007_IRQ(3, 1),
	[SM5007_IRQ_TOPOFF]     = SM5007_IRQ(4, 1),
	[SM5007_IRQ_DONE]       = SM5007_IRQ(5, 1),
	[SM5007_IRQ_CHGRSTF]    = SM5007_IRQ(6, 1),

	[SM5007_IRQ_SHORTKEY]       = SM5007_IRQ(0, 2),
	[SM5007_IRQ_LNGKEY_nONKEY]  = SM5007_IRQ(1, 2),
	[SM5007_IRQ_MANUALRST]      = SM5007_IRQ(2, 2),
	[SM5007_IRQ_VBATNG]         = SM5007_IRQ(3, 2),

	[SM5007_IRQ_BUCK2POK] = SM5007_IRQ(1, 3),
	[SM5007_IRQ_BUCK3POK] = SM5007_IRQ(2, 3),
	[SM5007_IRQ_BUCK4POK] = SM5007_IRQ(3, 3),
	[SM5007_IRQ_LDO1POK] = SM5007_IRQ(4, 3),
	[SM5007_IRQ_LDO2POK] = SM5007_IRQ(5, 3),
	[SM5007_IRQ_LDO3POK] = SM5007_IRQ(6, 3),
	[SM5007_IRQ_LDO4POK] = SM5007_IRQ(7, 3),

	[SM5007_IRQ_LDO5POK] = SM5007_IRQ(0, 4),
	[SM5007_IRQ_LDO6POK] = SM5007_IRQ(1, 4),
	[SM5007_IRQ_LDO7POK] = SM5007_IRQ(2, 4),
	[SM5007_IRQ_LDO8POK] = SM5007_IRQ(3, 4),
	[SM5007_IRQ_LDO9POK] = SM5007_IRQ(4, 4),
	[SM5007_IRQ_PS1POK] = SM5007_IRQ(5, 4),
	[SM5007_IRQ_PS2POK] = SM5007_IRQ(6, 4),
	[SM5007_IRQ_PS3POK] = SM5007_IRQ(7, 4),

	[SM5007_IRQ_PS4POK] = SM5007_IRQ(0, 5),
	[SM5007_IRQ_PS5POK] = SM5007_IRQ(1, 5),
	[SM5007_IRQ_CPPOK]  = SM5007_IRQ(2, 5),
	[SM5007_IRQ_PWRSTMF] = SM5007_IRQ(3, 5),
	[SM5007_IRQ_PWRSTMR] = SM5007_IRQ(4, 5)

};

static void sm5007_irq_lock(struct irq_data *irq_data)
{
	struct sm5007 *sm5007 = irq_data_get_irq_chip_data(irq_data);

	mutex_lock(&sm5007->irq_lock);
}

static void sm5007_irq_unmask(struct irq_data *irq_data)
{
	struct sm5007 *sm5007 = irq_data_get_irq_chip_data(irq_data);
	unsigned int __irq = irq_data->irq - sm5007->irq_base;
	const struct sm5007_irq_data *data = &sm5007_irqs[__irq];

	sm5007->irq_masks_cache[data->mask_reg_index] &= ~(1 << data->int_en_bit);
}

static void sm5007_irq_mask(struct irq_data *irq_data)
{
	struct sm5007 *sm5007 = irq_data_get_irq_chip_data(irq_data);
	unsigned int __irq = irq_data->irq - sm5007->irq_base;
	const struct sm5007_irq_data *data = &sm5007_irqs[__irq];

	sm5007->irq_masks_cache[data->mask_reg_index] |= (1 << data->int_en_bit);
}

static void sm5007_irq_sync_unlock(struct irq_data *irq_data)
{
	struct sm5007 *sm5007 = irq_data_get_irq_chip_data(irq_data);

	sm5007_write(sm5007->dev,
			SM5007_INT1_MASK,
			sm5007->irq_masks_cache[SM5007_INT1_OFFSET]);

	sm5007_write(sm5007->dev,
			SM5007_INT2_MASK,
            sm5007->irq_masks_cache[SM5007_INT2_OFFSET]);

	sm5007_write(sm5007->dev,
			SM5007_INT3_MASK,
            sm5007->irq_masks_cache[SM5007_INT3_OFFSET]);

	sm5007_write(sm5007->dev,
			SM5007_INT4_MASK,
            sm5007->irq_masks_cache[SM5007_INT4_OFFSET]);

	sm5007_write(sm5007->dev,
			SM5007_INT5_MASK,
            sm5007->irq_masks_cache[SM5007_INT5_OFFSET]);

	sm5007_write(sm5007->dev,
			SM5007_INT6_MASK,
            sm5007->irq_masks_cache[SM5007_INT6_OFFSET]);

	mutex_unlock(&sm5007->irq_lock);
}

static int sm5007_irq_set_type(struct irq_data *irq_data, unsigned int type)
{
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int sm5007_irq_set_wake(struct irq_data *irq_data, unsigned int on)
{
	struct sm5007 *sm5007 = irq_data_get_irq_chip_data(irq_data);
	return irq_set_irq_wake(sm5007->chip_irq, on);	/* i2c->irq */
}
#else
#define sm5007_irq_set_wake NULL
#endif

static int sm5007_read_irq_status(struct sm5007 *sm5007)
{
	int ret;
    u8 int_val[6] = {0,};
    u8 intsource_val = 0;
	union power_supply_propval value;

	ret = sm5007_read(sm5007->dev, SM5007_INTSOURCE,
			&intsource_val);
	if (ret < 0) {
		printk("Failed on reading INTSOURCE\n");
		return ret;
	}
	pr_debug("intsource = 0x%x\n", intsource_val);

    if (intsource_val & 0x02) //For fuelgauge
    {
        value.intval = 1;
        psy_do_property("sm5007-fuelgauge", set,
                    POWER_SUPPLY_PROP_ENERGY_EMPTY, value);
    }

	ret = sm5007_read(sm5007->dev, SM5007_INT1,
			&int_val[0]);
	if (ret < 0) {
		dev_err(sm5007->dev,
				"Failed on reading irq1 status\n");
		return ret;
	}
    sm5007->irq_status_cache[0] = int_val[0];
	pr_debug("irq1 = 0x%x\n", int_val[0]);

	ret = sm5007_read(sm5007->dev, SM5007_INT2,
            &int_val[1]);
	if (ret < 0) {
		dev_err(sm5007->dev,
				"Failed on reading irq2 status\n");
		return ret;
	}
    sm5007->irq_status_cache[1] = int_val[1];
	pr_debug("irq2 = 0x%x\n", int_val[1]);

	ret = sm5007_read(sm5007->dev, SM5007_INT3,
            &int_val[2]);
	if (ret < 0) {
		dev_err(sm5007->dev,
				"Failed on reading irq3 status\n");
		return ret;
	}
    sm5007->irq_status_cache[2] = int_val[2];
	pr_debug("irq3 = 0x%x\n", int_val[2]);

	ret = sm5007_read(sm5007->dev, SM5007_INT4,
			&int_val[3]);
	if (ret < 0) {
		dev_err(sm5007->dev,
				"Failed on reading irq4 status\n");
		return ret;
	}
    sm5007->irq_status_cache[3] = int_val[3];
	pr_debug("irq4 = 0x%x\n", int_val[3]);

	ret = sm5007_read(sm5007->dev, SM5007_INT5,
            &int_val[4]);
	if (ret < 0) {
		dev_err(sm5007->dev,
				"Failed on reading irq5 status\n");
		return ret;
	}
    sm5007->irq_status_cache[4] = int_val[4];
	pr_debug("irq5 = 0x%x\n", int_val[4]);

	ret = sm5007_read(sm5007->dev, SM5007_INT6,
            &int_val[5]);
	if (ret < 0) {
		dev_err(sm5007->dev,
				"Failed on reading irq6 status\n");
		return ret;
	}
    sm5007->irq_status_cache[5] = int_val[5];
	pr_debug("irq6 = 0x%x\n", int_val[5]);

	return 0;
}

static irqreturn_t sm5007_irq(int irq, void *data)
{
	struct sm5007 *sm5007 = data;
	int i;
	u8 int_sts[MAX_INTERRUPT_MASKS];

	/* printk("PMU: %s: irq=%d\n", __func__, irq); */
	/* disable_irq_nosync(irq); */
	/* Clear the status */

    sm5007_read_irq_status(sm5007);

	for (i = 0; i < MAX_INTERRUPT_MASKS; i++)
	    int_sts[i] = sm5007->irq_status_cache[i];

	for (i = 0; i < SM5007_NR_IRQS; i++)
		int_sts[i] &= ~sm5007->irq_masks_cache[i];

	/* Call interrupt handler if enabled */
	for (i = 0; i < SM5007_NR_IRQS; ++i) {
		const struct sm5007_irq_data *data = &sm5007_irqs[i];
		if ((int_sts[data->mask_reg_index] & (1 << data->int_en_bit)))
			handle_nested_irq(sm5007->irq_base + i);
	}

	pr_debug(KERN_DEBUG "PMU: %s: out\n", __func__);
	return IRQ_HANDLED;
}

static struct irq_chip sm5007_irq_chip = {
	.name = "sm5007",
	.irq_mask = sm5007_irq_mask,
	.irq_unmask = sm5007_irq_unmask,
	.irq_bus_lock = sm5007_irq_lock,
	.irq_bus_sync_unlock = sm5007_irq_sync_unlock,
	.irq_set_type = sm5007_irq_set_type,
	.irq_set_wake = sm5007_irq_set_wake,
};

static int sm5007_irq_mask_regs[] = {
	SM5007_INT1_MASK,
	SM5007_INT2_MASK,
	SM5007_INT3_MASK,
	SM5007_INT4_MASK,
	SM5007_INT5_MASK,
	SM5007_INT6_MASK,
};

static uint8_t sm5007_irqs_ctrl_mask_all_val[] = {
	0xFF,
	0x8C, //CHGRSTF, DONE, TOPOOF,VBUSPOK, VBUSUVLO
	0xFF,
	0xFF,
	0xFF,
	0xFF,
};

static int sm5007_mask_all_irqs(struct i2c_client *i2c)
{
	int rc;
	int i;
	struct sm5007 *sm5007 = i2c_get_clientdata(i2c);

	for (i=0;i<ARRAY_SIZE(sm5007_irq_mask_regs);i++) {
		rc = sm5007_write(sm5007->dev, sm5007_irq_mask_regs[i],
				sm5007_irqs_ctrl_mask_all_val[i]);
		sm5007->irq_masks_cache[i] = sm5007_irqs_ctrl_mask_all_val[i];
		if (rc<0) {
			pr_info("Error : can't write reg[0x%x] = 0x%x\n",
					sm5007_irq_mask_regs[i],
					sm5007_irqs_ctrl_mask_all_val[i]);
			return rc;
		}
	}

	return 0;
}


static int sm5007_irq_init_read(struct sm5007 *sm5007)
{
	int ret;
	ret = sm5007_read_irq_status(sm5007);

	return ret;
}

int sm5007_irq_init(struct sm5007 *sm5007, int irq,
				int irq_base)
{
	int i, ret;

	if (!irq_base) {
		dev_warn(sm5007->dev, "No interrupt support on IRQ base\n");
		return -EINVAL;
	}

    sm5007_clr_bits(sm5007->dev, SM5007_CNTL1, SM5007_MASK_INT_MASK); //Not Mask Interrupts

	ret = sm5007_mask_all_irqs(sm5007->client);
	if (ret < 0) {
		pr_err("%s : Can't mask all irqs(%d)\n", __func__, ret);
		//goto err_mask_all_irqs;
	}

	sm5007_irq_init_read(sm5007);

	mutex_init(&sm5007->irq_lock);

	sm5007->irq_base = irq_base;
	sm5007->chip_irq = irq;

	for (i = 0; i < SM5007_NR_IRQS; i++) {
		int __irq = i + sm5007->irq_base;
		irq_set_chip_data(__irq, sm5007);
		irq_set_chip_and_handler(__irq, &sm5007_irq_chip,
					 handle_simple_irq);
		irq_set_nested_thread(__irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(__irq, IRQF_VALID);
#endif
	}

	ret = request_threaded_irq(irq, NULL, sm5007_irq,
			IRQ_TYPE_EDGE_FALLING|IRQF_DISABLED|IRQF_ONESHOT,
						"sm5007", sm5007);
	if (ret < 0)
		dev_err(sm5007->dev, "Error in registering interrupt "
				"error: %d\n", ret);
	if (!ret) {
		device_init_wakeup(sm5007->dev, 1);
		enable_irq_wake(irq);
	}

	return ret;
}

int sm5007_irq_exit(struct sm5007 *sm5007)
{
	if (sm5007->chip_irq)
		free_irq(sm5007->chip_irq, sm5007);
	return 0;
}

