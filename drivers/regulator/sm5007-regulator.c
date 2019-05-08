/*
 * drivers/regulator/sm5007-regulator.c
 *
 * Regulator driver for SILICONMITUS SM5007 power management chip.
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
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/sm5007.h>
#include <linux/regulator/sm5007-regulator.h>

static const unsigned int sm5703_buck_output_list[] = {
	0*1000,
};

static const unsigned int sm5703_ldo_output_list[] = {
	800*1000,
	900*1000,
	1000*1000,
	1100*1000,
	1200*1000,
	1500*1000,
	1800*1000,
	2000*1000,
	2100*1000,
	2500*1000,
	2600*1000,
	2700*1000,
	2800*1000,
	2900*1000,
	3000*1000,
	3300*1000,
	-1,
};

struct SM5007_regulator {
	int		id;
	int		sleep_id;
	/* Regulator register address.*/
	u8		reg_en_reg;
	u8		en_bit;
	u8		vout_reg;
	u8		vout_mask;
	u8      vout_shift;
	u8		reg_slot_pwr_reg;
	u8		lpm_reg;
	u8		lpm_mask;
	u8      lpm_shift;

	/* chip constraints on regulator behavior */
	int			min_uV;
	int			max_uV;
	int			step_uV;
	int			nsteps;
	unsigned int const *output_list;

	/* regulator specific turn-on delay */
	u16			delay;

	/* used by regulator core */
	struct regulator_desc	desc;

	/* Device */
	struct device		*dev;
};

static unsigned int SM5007_suspend_status;

static inline struct device *to_SM5007_dev(struct regulator_dev *rdev)
{
	return rdev_get_dev(rdev)->parent->parent;
}

static int SM5007_regulator_enable_time(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);

	return ri->delay;
}

static int SM5007_reg_is_enabled(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	uint8_t control;
	int ret = 0, is_enable = 0, tem_enable = 0;

	ret = sm5007_read(parent, ri->reg_en_reg, &control);
	if (ret < 0) {
		dev_err(&rdev->dev, "Error in reading the control register\n");
		return ret;
	}
	//dong
	//Need to add PWRSTM Pin condition
	//If PWRSTM pin is high, condition is disable in case of en_bit 1 or 2.
	/* If we have enable the advanced function of sm5007,
	 * there is no way to adjuest if it is enabled only by the en_bit
	 **/
	switch (ri->id) {
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK1_DVS:
			tem_enable = ((control  >> ri->en_bit) & 0x3);
			is_enable = (tem_enable == 0x03);
			break;
		case SM5007_ID_BUCK2 ... SM5007_ID_LDO1:
		case SM5007_ID_LDO3 ... SM5007_ID_LDO5:
		case SM5007_ID_LDO8:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			tem_enable = ((control  >> ri->en_bit) & 0x3);
			is_enable = ((tem_enable == 0x00) ? 0 :
					(tem_enable == 0x02) ? 1 :
					(tem_enable == 0x03) ? 1 : 0);
			break;
		case SM5007_ID_LDO2:
		case SM5007_ID_LDO6 ... SM5007_ID_LDO7:
		case SM5007_ID_LDO9:
			tem_enable = ((control  >> ri->en_bit) & 0x1);
			is_enable = tem_enable;
			break;
		default:
			break;
	}

	return is_enable;
}

static int SM5007_reg_enable(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	int ret = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK1_DVS:
			ret = sm5007_set_bits(parent, ri->reg_en_reg, (0x3 << ri->en_bit));
			break;
		case SM5007_ID_BUCK2 ... SM5007_ID_LDO1:
		case SM5007_ID_LDO3 ... SM5007_ID_LDO5:
		case SM5007_ID_LDO8:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			ret = sm5007_set_bits(parent, ri->reg_en_reg, (0x3 << ri->en_bit));
			break;
		case SM5007_ID_LDO2:
		case SM5007_ID_LDO6 ... SM5007_ID_LDO7:
		case SM5007_ID_LDO9:
			ret = sm5007_set_bits(parent, ri->reg_en_reg, (0x1 << ri->en_bit));
			break;
		default:
			break;
	}

	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);

	return ret;
}

static int SM5007_reg_disable(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	int ret = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK1_DVS:
			ret = sm5007_clr_bits(parent, ri->reg_en_reg, (0x3 << ri->en_bit));
			break;
		case SM5007_ID_BUCK2 ... SM5007_ID_LDO1:
		case SM5007_ID_LDO3 ... SM5007_ID_LDO5:
		case SM5007_ID_LDO8:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			ret = sm5007_clr_bits(parent, ri->reg_en_reg, (0x3 << ri->en_bit));
			break;
		case SM5007_ID_LDO2:
		case SM5007_ID_LDO6 ... SM5007_ID_LDO7:
		case SM5007_ID_LDO9:
			ret = sm5007_clr_bits(parent, ri->reg_en_reg, (0x1 << ri->en_bit));
			break;
		default:
			break;
	}

	if (ret < 0)
		dev_err(&rdev->dev, "Error in updating the STATE register\n");

	return ret;
}

static int SM5007_list_voltage(struct regulator_dev *rdev, unsigned index)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	int list_vol = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK1_DVS:
		case SM5007_ID_BUCK4:
			list_vol = ri->min_uV + (ri->step_uV * index);
			break;
		case SM5007_ID_LDO1 ... SM5007_ID_LDO2:
		case SM5007_ID_LDO7 ... SM5007_ID_LDO9:
			list_vol = sm5703_ldo_output_list[index];
			break;
		case SM5007_ID_BUCK2 ... SM5007_ID_BUCK3:
		case SM5007_ID_LDO3 ... SM5007_ID_LDO6:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			list_vol = ri->min_uV;
			break;
		default:
			break;
	}

	return list_vol;
}

static int __SM5007_set_voltage(struct device *parent,
		struct SM5007_regulator *ri, int min_uV, int max_uV,
		unsigned *selector)
{
	int vsel = 0;
	int ret = 0;
	int i = 0;
	uint8_t vout_val, read_val;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK1_DVS:
		case SM5007_ID_BUCK4:
			if ((min_uV < ri->min_uV) || (max_uV > ri->max_uV))
				return -EDOM;

			vsel = (min_uV - ri->min_uV + ri->step_uV - 1)/ri->step_uV;
			if (vsel > ri->nsteps)
				return -EDOM;

			if (selector)
				*selector = vsel;

			ret = sm5007_read(parent, ri->vout_reg, &read_val);
			if (ret < 0)
				dev_err(ri->dev, "Error read the voltage register\n");

			vout_val = (read_val & ~ri->vout_mask) |
				(vsel << ri->vout_shift);
			ret = sm5007_write(parent, ri->vout_reg, vout_val);
			sm5007_read(parent, ri->vout_reg, &read_val);
			if (ret < 0)
				dev_err(ri->dev, "Error in writing the Voltage register\n");

			break;
		case SM5007_ID_LDO1 ... SM5007_ID_LDO2:
		case SM5007_ID_LDO7 ... SM5007_ID_LDO9:
			if ((min_uV < ri->min_uV) || (max_uV > ri->max_uV))
				return -EDOM;

			for (i = 0; i < 17; i++)
			{
				if(sm5703_ldo_output_list[i] > min_uV) {
					vsel = i - 1;
					break;
				}
			}
			if (vsel < 0)
				vsel = 0;

			if (selector)
				*selector = vsel;

			ret = sm5007_read(parent, ri->vout_reg, &read_val);
			if (ret < 0)
				dev_err(ri->dev, "Error read the voltage register\n");

			vout_val = (read_val & ~ri->vout_mask) |
				(vsel << ri->vout_shift);
			ret = sm5007_write(parent, ri->vout_reg, vout_val);
			sm5007_read(parent, ri->vout_reg, &read_val);
			if (ret < 0)
				dev_err(ri->dev, "Error in writing the Voltage register\n");

			break;
		case SM5007_ID_BUCK2 ... SM5007_ID_BUCK3:
		case SM5007_ID_LDO3 ... SM5007_ID_LDO6:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			if (selector)
				*selector = vsel;
			ret = 0;
			break;
		default:
			break;
	}

	return ret;
}

static int SM5007_set_voltage(struct regulator_dev *rdev,
		int min_uV, int max_uV, unsigned *selector)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);

	if (SM5007_suspend_status)
		return -EBUSY;

	return __SM5007_set_voltage(parent, ri, min_uV, max_uV, selector);
}

static int SM5007_get_voltage(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	uint8_t read_val;
	uint8_t vsel;
	int ret = 0, vout_val = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK1_DVS:
		case SM5007_ID_BUCK4 ... SM5007_ID_LDO2:
		case SM5007_ID_LDO7 ... SM5007_ID_LDO9:
			ret = sm5007_read(parent, ri->vout_reg, &read_val);
			if (ret < 0)
				dev_err(&rdev->dev, "Error read the voltage register\n");
			vsel = ((read_val & ri->vout_mask) >> ri->vout_shift);
			vout_val = ri->min_uV + vsel * ri->step_uV;
			break;
		case SM5007_ID_BUCK2 ... SM5007_ID_BUCK3:
		case SM5007_ID_LDO3 ... SM5007_ID_LDO6:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			vout_val = ri->min_uV;
			break;
		default:
			break;
	}
	return vout_val;
}

int SM5007_set_slot_pwrstm_function_enable(struct regulator_dev *rdev) //check
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	int ret = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK3:
			ret = sm5007_update(parent, ri->reg_slot_pwr_reg, (0x1 << ri->en_bit), 0x00);
			break;
		case SM5007_ID_BUCK4:
			break;
		case SM5007_ID_LDO1 ... SM5007_ID_LDO2:
			break;
		case SM5007_ID_LDO3 ... SM5007_ID_LDO5:
			ret = sm5007_update(parent, ri->reg_slot_pwr_reg, (0x1 << ri->en_bit), 0x00);
			break;
		case SM5007_ID_LDO6 ... SM5007_ID_LDO7:
		case SM5007_ID_LDO9:
			break;
		case SM5007_ID_LDO8:
			ret = sm5007_update(parent, ri->reg_slot_pwr_reg, (0x1 << ri->en_bit), 0x00);
			break;
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			ret = sm5007_update(parent, ri->reg_en_reg, (0x1 << ri->en_bit), 0x00);
			break;
		default:
			break;
	}

	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);

	return ret;
}
EXPORT_SYMBOL_GPL(SM5007_set_slot_pwrstm_function_enable);

int SM5007_set_lpm_pwrstm_function_enable(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	int ret = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK4:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			break;
		case SM5007_ID_LDO1 ... SM5007_ID_LDO9:
			ret = sm5007_update(parent, ri->lpm_reg, (0x1 << ri->lpm_shift), ri->lpm_mask);
			break;
		default:
			break;
	}

	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);

	return ret;
}
EXPORT_SYMBOL_GPL(SM5007_set_lpm_pwrstm_function_enable);

int SM5007_set_lpm_function_enable(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	int ret = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK4:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			break;
		case SM5007_ID_LDO1 ... SM5007_ID_LDO9:
			ret = sm5007_update(parent, ri->lpm_reg, (0x0 << ri->lpm_shift), ri->lpm_mask);
			break;
		default:
			break;
	}

	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);

	return ret;
}
EXPORT_SYMBOL_GPL(SM5007_set_lpm_function_enable);

int SM5007_set_lpm_function_disable(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	int ret = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK4:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			break;
		case SM5007_ID_LDO1 ... SM5007_ID_LDO9:
			ret = sm5007_update(parent, ri->lpm_reg, (0x3 << ri->lpm_shift), ri->lpm_mask);
			break;
		default:
			break;
	}

	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);
	return ret;
}
EXPORT_SYMBOL_GPL(SM5007_set_lpm_function_disable);

int SM5007_set_buck_auto_function_enable(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	int ret = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK4:
			ret = sm5007_update(parent, ri->lpm_reg, (0x0 << ri->lpm_shift),ri->lpm_mask);
			break;
		case SM5007_ID_LDO1 ... SM5007_ID_LDO9:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			break;
		default:
			break;
	}

	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);

	return ret;
}
EXPORT_SYMBOL_GPL(SM5007_set_buck_auto_function_enable);

int SM5007_set_buck_pwm_function_enable(struct regulator_dev *rdev)
{
	struct SM5007_regulator *ri = rdev_get_drvdata(rdev);
	struct device *parent = to_SM5007_dev(rdev);
	int ret = 0;

	switch (ri->id)
	{
		case SM5007_ID_BUCK1 ... SM5007_ID_BUCK4:
			ret = sm5007_update(parent, ri->lpm_reg, (0x1 << ri->lpm_shift), ri->lpm_mask);
			break;
		case SM5007_ID_LDO1 ... SM5007_ID_LDO9:
		case SM5007_ID_PS1 ... SM5007_ID_PS5:
			break;
		default:
			break;
	}

	if (ret < 0) {
		dev_err(&rdev->dev, "Error in updating the STATE register\n");
		return ret;
	}
	udelay(ri->delay);

	return ret;
}
EXPORT_SYMBOL_GPL(SM5007_set_buck_pwm_function_enable);

static struct regulator_ops SM5007_ops = {
	.list_voltage	= SM5007_list_voltage,
	.set_voltage	= SM5007_set_voltage,
	.get_voltage	= SM5007_get_voltage,
	.enable		= SM5007_reg_enable,
	.disable	= SM5007_reg_disable,
	.is_enabled	= SM5007_reg_is_enabled,
	.enable_time	= SM5007_regulator_enable_time,
};

#define SM5007_REG_BUCK(_id, _en_reg, _en_bit, _slot_pwr_reg, _vout_reg, _vout_mask, _vout_shift,\
		_min_mv, _max_mv, _step_uV, _nsteps, _ops, _delay, out_list,_lpm_reg, _lpm_mask, _lpm_shift) \
{								\
	.reg_en_reg	= _en_reg,				\
	.en_bit		= _en_bit,				\
	.reg_slot_pwr_reg	= _slot_pwr_reg,			\
	.vout_reg	= _vout_reg,				\
	.vout_mask	= _vout_mask,				\
	.vout_shift	= _vout_shift,				\
	.min_uV		= _min_mv * 1000,			\
	.max_uV		= _max_mv * 1000,			\
	.step_uV	= _step_uV,				\
	.nsteps		= _nsteps,				\
	.delay		= _delay,				\
	.output_list = out_list,            \
	.lpm_reg = _lpm_reg,            \
	.lpm_mask = _lpm_mask,            \
	.lpm_shift = _lpm_shift,            \
	.id		= SM5007_ID_##_id,			\
	.desc = {						\
		.name = sm5007_rails(_id),			\
		.id = SM5007_ID_##_id,			\
		.n_voltages = _nsteps,				\
		.ops = &_ops,					\
		.type = REGULATOR_VOLTAGE,			\
		.owner = THIS_MODULE,				\
	},							\
}

#define SM5007_REG_LDO(_id, _en_reg, _en_bit, _slot_pwr_reg, _vout_reg, _vout_mask, _vout_shift,\
		_min_mv, _max_mv, _step_uV, _nsteps, _ops, _delay, out_list,_lpm_reg, _lpm_mask, _lpm_shift) \
{								\
	.reg_en_reg	= _en_reg,				\
	.en_bit		= _en_bit,				\
	.reg_slot_pwr_reg = _slot_pwr_reg,			\
	.vout_reg	= _vout_reg,				\
	.vout_mask	= _vout_mask,				\
	.vout_shift	= _vout_shift,				\
	.min_uV		= _min_mv * 1000,			\
	.max_uV		= _max_mv * 1000,			\
	.step_uV	= _step_uV,				\
	.nsteps		= _nsteps,				\
	.delay		= _delay,				\
	.output_list = out_list,            \
	.lpm_reg = _lpm_reg,            \
	.lpm_mask = _lpm_mask,            \
	.lpm_shift = _lpm_shift,            \
	.id		= SM5007_ID_##_id,			\
	.desc = {						\
		.name = sm5007_rails(_id),			\
		.id = SM5007_ID_##_id,			\
		.n_voltages = _nsteps,				\
		.ops = &_ops,					\
		.type = REGULATOR_VOLTAGE,			\
		.owner = THIS_MODULE,				\
	},							\
}

#define SM5007_REG_PS(_id, _en_reg, _en_bit, _slot_pwr_reg, _vout_reg, _vout_mask, _vout_shift,\
		_min_mv, _max_mv, _step_uV, _nsteps, _ops, _delay, out_list,_lpm_reg, _lpm_mask, _lpm_shift) \
{								\
	.reg_en_reg	= _en_reg,				\
	.en_bit		= _en_bit,				\
	.reg_slot_pwr_reg = _slot_pwr_reg,			\
	.vout_reg	= _vout_reg,				\
	.vout_mask	= _vout_mask,				\
	.vout_shift	= _vout_shift,				\
	.min_uV		= _min_mv * 1000,			\
	.max_uV		= _max_mv * 1000,			\
	.step_uV	= _step_uV,				\
	.nsteps		= _nsteps,				\
	.delay		= _delay,				\
	.output_list = out_list,            \
	.lpm_reg = _lpm_reg,            \
	.lpm_mask = _lpm_mask,            \
	.lpm_shift = _lpm_shift,            \
	.id		= SM5007_ID_##_id,			\
	.desc = {						\
		.name = sm5007_rails(_id),			\
		.id = SM5007_ID_##_id,			\
		.n_voltages = _nsteps,				\
		.ops = &_ops,					\
		.type = REGULATOR_VOLTAGE,			\
		.owner = THIS_MODULE,				\
	},							\
}

static struct SM5007_regulator SM5007_regulator[] = {
	SM5007_REG_BUCK(BUCK1, 0x30, 0, 0x30, 0x31, 0x3F, 0x00,
			700, 1300, 12500, 0x30, SM5007_ops, 500, sm5703_buck_output_list, 0x46, 0x01, 0x00),
	SM5007_REG_BUCK(BUCK1_DVS, 0x30, 0, 0x30, 0x32, 0x3F, 0x00,
			700, 1300, 12500, 0x30, SM5007_ops, 500, sm5703_buck_output_list, 0x46, 0x01, 0x00),
	SM5007_REG_BUCK(BUCK2, 0x33, 0, 0x33, 0xFF, 0xFF, 0x00,
			1200, 1200, 0, 0x0, SM5007_ops, 500, sm5703_buck_output_list, 0x46, 0x02, 0x01),
	SM5007_REG_BUCK(BUCK3, 0x42, 0, 0x42, 0xFF, 0xFF, 0x00,
			1800, 1800, 0, 0x0, SM5007_ops, 500, sm5703_buck_output_list, 0x46, 0x04, 0x02),
	SM5007_REG_BUCK(BUCK4, 0x42, 1, 0x42, 0x34, 0x3F, 0x00,
			1800, 3300, 50000, 0x1E, SM5007_ops, 500, sm5703_buck_output_list, 0x46, 0x08, 0x03),
	SM5007_REG_LDO(LDO1, 0x35, 0, 0x35, 0x3A, 0x0F, 0x00,
			800, 3300, 0, 0x0F, SM5007_ops, 500, sm5703_ldo_output_list, 0x43, 0x03, 0x00),
	SM5007_REG_LDO(LDO2, 0x42, 2, 0x42, 0x3A, 0xF0, 0x04,
			800, 3300, 0, 0x0F, SM5007_ops, 500, sm5703_ldo_output_list, 0x43, 0x0C, 0x02),
	SM5007_REG_LDO(LDO3, 0x36, 0, 0x36, 0xFF, 0xFF, 0x00,
			3000, 3000, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x43, 0x30, 0x04),
	SM5007_REG_LDO(LDO4, 0x37, 0, 0x37, 0xFF, 0xFF, 0x00,
			2800, 2800, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x43, 0xC0, 0x06),
	SM5007_REG_LDO(LDO5, 0x38, 0, 0x38, 0xFF, 0xFF, 0x00,
			2500, 2500, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x44, 0x03, 0x00),
	SM5007_REG_LDO(LDO6, 0x42, 3, 0x42, 0xFF, 0xFF, 0x00,
			1800, 1800, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x44, 0x0C, 0x02),
	SM5007_REG_LDO(LDO7, 0x42, 4, 0x42, 0x3B, 0x0F, 0x00,
			800, 3300, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x44, 0x30, 0x04),
	SM5007_REG_LDO(LDO8, 0x39, 0, 0x39, 0x3B, 0xF0, 0x04,
			800, 3300, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x44, 0xC0, 0x06),
	SM5007_REG_LDO(LDO9, 0x42, 5, 0x42, 0x3C, 0x0F, 0x00,
			800, 3300, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x45, 0x03, 0x00),
	SM5007_REG_PS(PS1, 0x3D, 0, 0x3D, 0x31, 0x3F, 0x00,
			1100, 1100, 12500, 0x30, SM5007_ops, 500, sm5703_buck_output_list, 0x00, 0x00, 0x00),
	SM5007_REG_PS(PS2, 0x3E, 0, 0x3E, 0xFF, 0xFF, 0x00,
			1200, 1200, 0, 0x0, SM5007_ops, 500, sm5703_buck_output_list, 0x00, 0x00, 0x00),
	SM5007_REG_PS(PS3, 0x3F, 0, 0x3F, 0xFF, 0xFF, 0x00,
			1800, 1800, 0, 0x0, SM5007_ops, 500, sm5703_buck_output_list, 0x00, 0x00, 0x00),
	SM5007_REG_PS(PS4, 0x40, 0, 0x40, 0xFF, 0xFF, 0x00,
			3000, 3000, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x00, 0x00, 0x00),
	SM5007_REG_PS(PS5, 0x41, 0, 0x41, 0xFF, 0xFF, 0x00,
			2800, 2800, 0, 0, SM5007_ops, 500, sm5703_ldo_output_list, 0x00, 0x00, 0x00),
};

static inline struct SM5007_regulator *find_regulator_info(int id)
{
	struct SM5007_regulator *ri;
	int i;

	for (i = 0; i < ARRAY_SIZE(SM5007_regulator); i++) {
		ri = &SM5007_regulator[i];
		if (ri->desc.id == id)
			return ri;
	}
	return NULL;
}

static int SM5007_regulator_preinit(struct device *parent,
		struct SM5007_regulator *ri,
		struct sm5007_regulator_platform_data *SM5007_pdata)
{
	int ret = 0;

	if (!SM5007_pdata->init_apply)
		return 0;

	if (SM5007_pdata->init_uV >= 0) {
		ret = __SM5007_set_voltage(parent, ri,
				SM5007_pdata->init_uV,
				SM5007_pdata->init_uV, 0);
		if (ret < 0) {
			dev_err(ri->dev, "Not able to initialize voltage %d "
					"for rail %d err %d\n", SM5007_pdata->init_uV,
					ri->desc.id, ret);
			return ret;
		}
	}
	//dong : Need to modify below enable squence due to bit number.
	if (SM5007_pdata->init_enable)
		ret = sm5007_set_bits(parent, ri->reg_en_reg,
				(1 << ri->en_bit));
	else
		ret = sm5007_clr_bits(parent, ri->reg_en_reg,
				(1 << ri->en_bit));
	if (ret < 0)
		dev_err(ri->dev, "Not able to %s rail %d err %d\n",
				(SM5007_pdata->init_enable) ? "enable" : "disable",
				ri->desc.id, ret);

	return ret;
}

static void SM5007_regulaort_init(struct regulator_dev *rdev, struct sm5007_regulator_platform_data *pdata)
{

	switch(pdata->lpm) {
		case LPM_IGNORE:
			break;
		case LPM_LDO_ATTACH_TO_STM:
			SM5007_set_lpm_pwrstm_function_enable(rdev);
			break;
		case LPM_LDO_FORCE_INTO_LPM:
			SM5007_set_lpm_function_enable(rdev);
			break;
		case LPM_LDO_NORMAL:
			SM5007_set_lpm_function_disable(rdev);
			break;
		case LPM_BUCK_AUTO:
			SM5007_set_buck_auto_function_enable(rdev);
			break;
		case LPM_BUCK_PWM:
			SM5007_set_buck_pwm_function_enable(rdev);
			break;
		default :
			printk("%s : error lpm(%d)\n", __func__, pdata->lpm);
	}
}

static struct regulator_config SM5007_regulator_config;

static int SM5007_regulator_probe(struct platform_device *pdev)
{
	struct SM5007_regulator *ri = NULL;
	struct regulator_dev *rdev;
	struct sm5007_regulator_platform_data *tps_pdata;
	int id = pdev->id;
	int err;

	ri = find_regulator_info(id);
	if (ri == NULL) {
		dev_err(&pdev->dev, "invalid regulator ID specified\n");
		return -EINVAL;
	}
	tps_pdata = pdev->dev.platform_data;
	ri->dev = &pdev->dev;
	SM5007_suspend_status = 0;

	err = SM5007_regulator_preinit(pdev->dev.parent, ri, tps_pdata);
	if (err) {
		dev_err(&pdev->dev, "Fail in pre-initialisation\n");
		return err;
	}

	SM5007_regulator_config.dev = &pdev->dev;
	SM5007_regulator_config.init_data = &tps_pdata->regulator;
	SM5007_regulator_config.driver_data = ri;

	//	rdev = regulator_register(&ri->desc, &pdev->dev,
	//				&tps_pdata->regulator, ri);
	rdev = regulator_register(&ri->desc, &SM5007_regulator_config);
	if (IS_ERR_OR_NULL(rdev)) {
		dev_err(&pdev->dev, "failed to register regulator %s\n",
				ri->desc.name);
		return PTR_ERR(rdev);
	}
	//lpm mode init function
	sm5007_set_bits(pdev->dev.parent, SM5007_CNTL2 , 0x02); //PWRSTM_EN : Enable

	SM5007_regulaort_init(rdev, tps_pdata);
	platform_set_drvdata(pdev, rdev);

	if (rdev && tps_pdata->init_enable && ri->desc.ops->is_enabled(rdev)) {
		rdev->use_count++;
	}

	return 0;
}

static int SM5007_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *rdev = platform_get_drvdata(pdev);

	regulator_unregister(rdev);
	return 0;
}

static struct platform_driver SM5007_regulator_driver = {
	.driver	= {
		.name	= "sm5007-regulator",
		.owner	= THIS_MODULE,
	},
	.probe		= SM5007_regulator_probe,
	.remove		= SM5007_regulator_remove,
};

static int __init SM5007_regulator_init(void)
{
	return platform_driver_register(&SM5007_regulator_driver);
}
subsys_initcall(SM5007_regulator_init);

static void __exit SM5007_regulator_exit(void)
{
	platform_driver_unregister(&SM5007_regulator_driver);
}
module_exit(SM5007_regulator_exit);

MODULE_DESCRIPTION("SILICONMITUS SM5007 regulator driver");
MODULE_ALIAS("platform:sm5007-regulator");
MODULE_LICENSE("GPL");
