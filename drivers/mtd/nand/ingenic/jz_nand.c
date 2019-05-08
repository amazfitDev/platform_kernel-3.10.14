/*
 *  Copyright (C) cli <chen.li@ingenic.com>
 *  NAND layer driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand_bch.h>
#include <linux/bch.h>
#include <linux/gpio.h>
#include <linux/platform_data/xburst_nand.h>
#include "jz_bch.h"
#include "jz_nand.h"

#define DRVNAME "jz-nand"
#define MAX_RB_DELAY_US             50

struct jz_nand {
	struct mtd_info mtd;
	struct nand_chip chip;
	struct nand_ecclayout ecclayout;
	int selected;						/* current chip select */
	int wp_gpio;						/* write protect gpio */
	int rb_gpio;						/* ready busy gpio */
	struct platform_device *pdev;
	struct platform_device *hw_ecc_pdev;			/*ecc sub device*/
	struct device *hwecc;
	struct platform_device *nandc_pdev;			/*nand controller sub device*/
	struct device *nandc;
	int twb;
};

static inline struct jz_nand *to_jz_nand(struct mtd_info *mtd)
{
	return container_of(mtd, struct jz_nand, mtd);
}

static void jz_nand_enable_wp(struct jz_nand *nand, int enable)
{
	if (!gpio_is_valid(nand->wp_gpio))
		return;
	if (enable)
		gpio_set_value_cansleep(nand->wp_gpio, 1);
	else
		gpio_set_value_cansleep(nand->wp_gpio, 0);
}

static int jz_nand_dev_is_ready(struct mtd_info *mtd)
{
	struct jz_nand *nand = to_jz_nand(mtd);
	BUG_ON(nand->selected < 0);
	if (nand->rb_gpio != -1) {
		if (nand->twb > 100) ndelay(nand->twb-100);
		return !!gpio_get_value(nand->rb_gpio);
	}
	return jz_nandc_dev_is_ready(nand->nandc, &nand->chip);
}

void jz_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct jz_nand *nand = to_jz_nand(mtd);
	jz_nandc_select_chip(nand->nandc, &nand->chip, chip);
	nand->selected = chip;
}

void jz_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct jz_nand *nand = to_jz_nand(mtd);
	BUG_ON(nand->selected < 0);
	jz_nandc_cmd_ctrl(nand->nandc, &nand->chip, cmd, ctrl);
}

static void jz_nand_ecc_hwctl(struct mtd_info *mtd, int mode) {}

static int jz_nand_ecc_calculate(struct mtd_info *mtd,
		const uint8_t *dat, uint8_t *ecc_code)
{
	struct jz_nand *nand = to_jz_nand(mtd);
	struct jz_bch_params params;

	if (nand->chip.state == FL_READING)
		return 0;
	params.size = nand->chip.ecc.size;
	params.bytes = nand->chip.ecc.bytes;
	params.strength = nand->chip.ecc.strength;
	return jz_bch_calculate(nand->hwecc, &params, dat, ecc_code);
}

static int jz_nand_ecc_correct(struct mtd_info *mtd, uint8_t *dat,
		uint8_t *read_ecc, uint8_t *calc_ecc)
{
	struct jz_nand *nand = to_jz_nand(mtd);
	struct jz_bch_params params;

	params.size = nand->chip.ecc.size;
	params.bytes = nand->chip.ecc.bytes;
	params.strength = nand->chip.ecc.strength;
	return jz_bch_correct(nand->hwecc, &params, dat, read_ecc);
}

static int jz_nand_init_params(struct jz_nand_params *nand_param_pri, struct xburst_nand_chip_platform_data *pdata)
{
	nand_param_pri->tALS = pdata->tALS;
	nand_param_pri->tALH = pdata->tALH;
	nand_param_pri->tRP = pdata->tRP;
	nand_param_pri->tWP = pdata->tWP;
	nand_param_pri->tRHW = pdata->tRHW;
	nand_param_pri->tWHR = pdata->tWHR;
	nand_param_pri->tWHR2 = pdata->tWHR2;
	nand_param_pri->tRR = pdata->tRR;
	nand_param_pri->tWB = pdata->tWB;
	nand_param_pri->tADL = pdata->tADL;
	nand_param_pri->tCWAW = pdata->tCWAW;
	nand_param_pri->tCS = pdata->tCS;
	nand_param_pri->tCLH = pdata->tCLH;
	nand_param_pri->tWH = pdata->tWH;
	nand_param_pri->tCH = pdata->tCH;
	nand_param_pri->tDH = pdata->tDH;
	nand_param_pri->tREH = pdata->tREH;
	nand_param_pri->buswidth = pdata->options & NAND_BUSWIDTH_16 ? 16 : 8;
	return 0;
}

static int jz_nand_init_ecc(struct jz_nand *nand, struct xburst_nand_chip_platform_data *pdata)
{
	int start, i;

	nand->chip.ecc.size = pdata->ecc_size;
	nand->chip.ecc.strength = pdata->ecc_strength;
	nand->chip.ecc.bytes =  nand->chip.ecc.strength * 14 / 8;
	nand->chip.ecc.mode = pdata->ecc_mode;

#ifdef CONFIG_MTD_NAND_JZBCH
	if (pdata->ecc_mode == NAND_ECC_HW ||
			pdata->ecc_mode == NAND_ECC_HW_OOB_FIRST ||
			pdata->ecc_mode == NAND_ECC_HW_SYNDROME) {
		/*bch sub device register*/
		nand->hw_ecc_pdev = platform_device_register_resndata(NULL, "jz-bch", 0 ,
				nand->pdev->resource, nand->pdev->num_resources,
				NULL, 0);
		if (IS_ERR_OR_NULL(nand->hw_ecc_pdev))
			return -ENOMEM;
		if (nand->hw_ecc_pdev->dev.driver == NULL)	/*driver probe failed*/
			return -ENODEV;
		nand->chip.ecc.calculate = jz_nand_ecc_calculate;
		nand->chip.ecc.correct   = jz_nand_ecc_correct;
		nand->chip.ecc.hwctl	 = jz_nand_ecc_hwctl;
		nand->hwecc = &nand->hw_ecc_pdev->dev;
	}
#else
	nand->chip.ecc.mode      = NAND_ECC_SOFT_BCH;
#endif
	/* Generate ECC layout. ECC codes are right aligned in the OOB area. */
	nand->ecclayout.eccbytes = nand->mtd.writesize / nand->chip.ecc.size * nand->chip.ecc.bytes;
	start = nand->mtd.oobsize - nand->ecclayout.eccbytes;
	for (i = 0; i < nand->ecclayout.eccbytes; i++)
		nand->ecclayout.eccpos[i] = start + i;
	nand->ecclayout.oobfree->offset = nand->chip.badblockpos + 2;
	nand->ecclayout.oobfree->length = nand->mtd.oobsize - nand->ecclayout.eccbytes - 2;
	nand->chip.ecc.layout = &nand->ecclayout;
	return 0;
}

static int jz_mtd_partitions_init(struct mtd_partition **partitions, struct xburst_nand_chip_platform_data *pdata)
{
	char *part_name_base = NULL;
	int i, nr_partitions = pdata->nr_partitions;

	*partitions = kzalloc(nr_partitions * (sizeof(struct mtd_partition) + 32*sizeof(char)),
			GFP_KERNEL);
	if (!(*partitions)) return -ENOMEM;

	memcpy(*partitions, pdata->partitions, sizeof(struct mtd_partition) * nr_partitions);
	part_name_base = (char *)(*partitions) + nr_partitions * sizeof(struct mtd_partition);

	for (i = 0; i < nr_partitions; i++) {
		(*partitions)[i].name = part_name_base + i * 32 * sizeof(char);
		strcpy((*partitions)[i].name, pdata->partitions[i].name);
		printk("%s %llx:%llx\n", (*partitions)[i].name, (*partitions)[i].offset, (*partitions)[i].size);
	}
	return nr_partitions;
}

static int jz_nand_probe(struct platform_device *pdev)
{
	struct jz_nand *nand = NULL;
	struct xburst_nand_chip_platform_data *pdata =
						pdev->dev.platform_data;
	struct jz_nand_params *nand_param_pri = NULL;	/*driver private nand param*/
	struct nand_flash_dev *nand_table_ids = NULL;
	struct mtd_partition *partitions = NULL;
	int nr_partitions = 0;
	int ret = 0;

	nand = (struct jz_nand *)devm_kzalloc(&pdev->dev,
			sizeof(struct jz_nand), GFP_KERNEL);
	if (!nand)
		return -ENOMEM;
	nand->pdev = pdev;
	nand->twb = pdata->tWB;

	/*rb&wp init*/
	nand->rb_gpio = pdata->rb_gpio;
	if (gpio_is_valid(nand->rb_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev, nand->rb_gpio, GPIOF_DIR_IN, "nand-bank0-rb");
		if (ret)
			return ret;
	} else
		nand->rb_gpio = -1;

	nand->wp_gpio = pdata->wp_gpio;
	if (gpio_is_valid(nand->wp_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev, nand->wp_gpio, GPIOF_INIT_LOW|GPIOF_DIR_OUT,
				"nand-band0-wp");
		if (ret)
			return ret;
	} else
		nand->wp_gpio = -1;

	/*nandc device register*/
	nand_param_pri = devm_kzalloc(&pdev->dev, sizeof(struct jz_nand_params), GFP_KERNEL);
	if (!nand_param_pri)
		return -ENOMEM;
	jz_nand_init_params(nand_param_pri, pdata);
	nand->nandc_pdev = platform_device_register_resndata(&pdev->dev,
#if defined(CONFIG_MTD_NAND_JZNEMC)
			"jz-nemc", 0,
#else
			"jz-nfi", 0,
#endif
			pdev->resource, pdev->num_resources,
			nand_param_pri, sizeof(struct jz_nand_params));
	if (IS_ERR_OR_NULL(nand->nandc_pdev))
		return -ENOMEM;
	if (!nand->nandc_pdev->dev.driver)
		return -ENODEV;
	nand->nandc = &nand->nandc_pdev->dev;

	nand->mtd.priv = &nand->chip;
	nand->mtd.name = dev_name(&pdev->dev);
	nand->mtd.owner = THIS_MODULE;
	nand->chip.chip_delay  = pdata->chip_delay ? pdata->chip_delay : MAX_RB_DELAY_US;
	nand->chip.dev_ready = jz_nand_dev_is_ready;
	nand->chip.select_chip = jz_nand_select_chip;
	nand->chip.cmd_ctrl = jz_nand_cmd_ctrl;
	nand->chip.bbt_options = pdata->bbt_options;
	nand->chip.options = pdata->options;
	if (pdata->init_size)
		nand->chip.init_size = pdata->init_size;

	/*nand scan*/
	if (pdata->has_ids)
		nand_table_ids = pdata->nand_flash_ids;
	if (nand_scan_ident(&nand->mtd, 1, nand_flash_ids)) {
		ret = -ENXIO;
		goto err_nand_scan_1;
	}
	if (jz_nand_init_ecc(nand, pdata)) {
		ret = -ENOMEM;
		goto err_nand_scan_1;
	}
	if (nand_scan_tail(&nand->mtd)) {
		ret = -ENXIO;
		goto err_nand_scan_2;
	}

	/*dump nand info*/
	dump_nand_info(&nand->mtd, &nand->chip);

	/*mtd register*/
	nr_partitions = jz_mtd_partitions_init(&partitions, pdata);
	if (nr_partitions < 0) {
		ret = -ENOMEM;
		goto err_nand_scan_2;
	}

	ret = mtd_device_parse_register(&nand->mtd, NULL, NULL,
			partitions, nr_partitions);

	kfree(partitions);
	devm_kfree(&pdev->dev, nand_param_pri);

	if (!ret) {
		platform_set_drvdata(pdev, nand);
		dev_info(&pdev->dev, "Successfully registered NAND auto interface driver.\n");
		return 0;
	}

	nand_release(&nand->mtd);
err_nand_scan_2:
	if (nand->hw_ecc_pdev)
		platform_device_unregister(nand->hw_ecc_pdev);
err_nand_scan_1:
	platform_device_unregister(nand->nandc_pdev);
	return ret;
}

static int jz_nand_remove(struct platform_device *pdev)
{
	struct jz_nand *nand = platform_get_drvdata(pdev);

	nand_release(&nand->mtd);
	jz_nand_enable_wp(nand, 1);
	platform_device_unregister(nand->nandc_pdev);
	if (nand->hw_ecc_pdev)
		platform_device_unregister(nand->hw_ecc_pdev);
	return 0;
}

static struct platform_driver jz_nand_driver = {
	.probe = jz_nand_probe,
	.remove = jz_nand_remove,
	.driver = {
		.name = "jz-nand",
		.owner = THIS_MODULE,
	},
};

static int __init jz_nand_init(void)
{
	return platform_driver_register(&jz_nand_driver);
}
module_init(jz_nand_init);

static void __exit jz_nand_exit(void)
{
	platform_driver_unregister(&jz_nand_driver);
}
module_exit(jz_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cli <chen.li@ingenic.com>");
MODULE_DESCRIPTION("NAND layer driver for ingenic SoC");
