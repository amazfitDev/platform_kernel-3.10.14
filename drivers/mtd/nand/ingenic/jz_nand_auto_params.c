/*
 *  Copyright (C) cli <chen.li@ingenic.com>
 *  NAND auto detect timing driver (With the ingenic burner used)
 *
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
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
#include "jz_nand_param.h"
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

static int jz_nand_init_params(struct jz_nand_params *nand_param_pri, nand_flash_param *nand_param)
{
	nand_timing_param *timing = &nand_param->timing;

	nand_param_pri->tALS = timing->tALS;
	nand_param_pri->tALH = timing->tALH;
	nand_param_pri->tRP = timing->tRP;
	nand_param_pri->tWP = timing->tWP;
	nand_param_pri->tRHW = timing->tRHW;
	nand_param_pri->tWHR = timing->tWHR;
	nand_param_pri->tWHR2 = timing->tWHR2;
	nand_param_pri->tRR = timing->tRR;
	nand_param_pri->tWB = timing->tWB;
	nand_param_pri->tADL = timing->tADL;
	nand_param_pri->tCWAW = timing->tCWAW;
	nand_param_pri->tCS = timing->tCS;
	nand_param_pri->tCLH = timing->tCLH;
	nand_param_pri->tWH = timing->tWH;
	nand_param_pri->tCH = timing->tCH;
	nand_param_pri->tDH = timing->tDH;
	nand_param_pri->tREH = timing->tREH;
	nand_param_pri->buswidth = nand_param->buswidth;
	return 0;
}

static int jz_nand_init_ecc(struct jz_nand *nand, int ecc_strength)
{
	int start, i;

	nand->chip.ecc.size = (nand->mtd.writesize) < 1024 ? nand->mtd.writesize : 1024;
	nand->chip.ecc.strength = ecc_strength;
	nand->chip.ecc.bytes = nand->chip.ecc.strength * 14 / 8;

#ifdef CONFIG_MTD_NAND_JZBCH
	nand->hw_ecc_pdev = platform_device_register_resndata(NULL, "jz-bch",  0,
			nand->pdev->resource, nand->pdev->num_resources,
			NULL, 0);
	if (IS_ERR_OR_NULL(nand->hw_ecc_pdev))
		return -ENOMEM;
	if (nand->hw_ecc_pdev->dev.driver == NULL)	/*driver probe failed*/
		return -ENODEV;
	nand->chip.ecc.mode      = NAND_ECC_HW_OOB_FIRST;
	nand->chip.ecc.calculate = jz_nand_ecc_calculate;
	nand->chip.ecc.correct   = jz_nand_ecc_correct;
	nand->chip.ecc.hwctl	 = jz_nand_ecc_hwctl;
	nand->hwecc = &nand->hw_ecc_pdev->dev;
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

static int jz_mtd_partitions_init(struct mtd_partition **partitions, MTDPartitionInfo *pinfo)
{
	MTDNandppt *ndppt = pinfo->ndppt;
	char *part_name_base = NULL;
	int i, nr_partitions = 0;

	for (i = 0; i < MAX_PART_NUM && ndppt[i].size != 0; i++, nr_partitions++);

	*partitions = kzalloc(nr_partitions * (sizeof(struct mtd_partition) + 32*sizeof(char)),
			GFP_KERNEL);
	if (!(*partitions)) return -ENOMEM;

	part_name_base = (char *)(*partitions) + nr_partitions * sizeof(struct mtd_partition);

	for (i = 0; i < nr_partitions; i++) {
		(*partitions)[i].name = part_name_base + 32 * i;
		strcpy((*partitions)[i].name, ndppt[i].name);

		(*partitions)[i].offset = ndppt[i].offset * 1024 * 1024LL ;

		if (ndppt[i].size != -1)
			(*partitions)[i].size = ndppt[i].size * 1024 * 1024LL;
		else
			(*partitions)[i].size = MTDPART_SIZ_FULL;

		printk("%s(offset:%llx, size:%llx)\n", (*partitions)[i].name,
				(*partitions)[i].offset,
				(*partitions)[i].size);
	}
	return nr_partitions;
}

static int jz_nand_probe(struct platform_device *pdev)
{
	struct jz_nand *nand = NULL;
	struct mtd_nand_params *mtd_nand_params = (struct mtd_nand_params *)0xb3422000; /*uboot load the nand params here*/
	MTDPartitionInfo *pinfo = &mtd_nand_params->pinfo;
	nand_flash_param *nand_param = &mtd_nand_params->nand_params;
	struct nand_flash_dev nand_flash_table[2];		/* nand ids */
	char nand_flash_name[32];				/* nand name */
	struct jz_nand_params *nand_param_pri = NULL;	/*driver private nand param*/
	struct mtd_partition *partitions = NULL;
	int nr_partitions = 0;
	int ret = 0;

	nand = (struct jz_nand *)devm_kzalloc(&pdev->dev,
			sizeof(struct jz_nand), GFP_KERNEL);
	if (!nand)
		return -ENOMEM;
	nand->pdev = pdev;
	nand->twb = nand_param->timing.tWB;

	/*rb&wp init*/
	nand->rb_gpio = pinfo->rb_gpio[0];
	if (gpio_is_valid(nand->rb_gpio)) {
		ret = devm_gpio_request_one(&pdev->dev, nand->rb_gpio, GPIOF_DIR_IN, "nand-bank0-rb");
		if (ret)
			return ret;
	} else
		nand->rb_gpio = -1;

	nand->wp_gpio = pinfo->gpio_wp;
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
	jz_nand_init_params(nand_param_pri, nand_param);
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
	nand->chip.chip_delay = MAX_RB_DELAY_US;
	nand->chip.dev_ready = jz_nand_dev_is_ready;
	nand->chip.select_chip = jz_nand_select_chip;
	nand->chip.cmd_ctrl = jz_nand_cmd_ctrl;
	nand->chip.bbt_options = NAND_BBT_USE_FLASH;
	nand->chip.options = NAND_NO_SUBPAGE_WRITE;

	/*prepare nand flash table from nand param*/
	memset(nand_flash_table, 0, 2 * sizeof(struct nand_flash_dev));
	nand_flash_table[0].name = nand_flash_name;
	strcpy(nand_flash_table[0].name, nand_param->name);
	nand_flash_table[0].dev_id = nand_param->id & 0xff;
	nand_flash_table[0].pagesize = nand_param->pagesize;
	nand_flash_table[0].erasesize = nand_param->blocksize;
	nand_flash_table[0].chipsize = (nand_param->blocksize/1024)*
		nand_param->totalblocks/1024;   /*Mbytes*/
	nand_flash_table[0].chipsize = 1 << (fls(nand_flash_table[0].chipsize) - 1); /*Align*/
	if (nand_param->buswidth == 16)
		nand_flash_table[0].options = NAND_BUSWIDTH_16;

	/*nand scan*/
	if (nand_scan_ident(&nand->mtd, 1, nand_flash_table)) {
		ret = -ENXIO;
		goto err_nand_scan_1;
	}

	nand->mtd.oobsize = nand_param->oobsize;	/*FIXME we reset it here*/
	if (jz_nand_init_ecc(nand, nand_param->eccbit)) {
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
	nr_partitions = jz_mtd_partitions_init(&partitions, pinfo);
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
