/*
 *  Copyright (C) cli <chen.li@ingenic.com>
 *  External Nand Memory Controller driver
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
#include "jz_nand.h"


#define NEMC_SMCRn(n)           (0x14 + (((n) - 1) * 4))
#define NEMC_NFCSR              0x50

/* NEMC NAND Flash Control & Status Register (NFCSR) */
#define NEMC_NFCSR_NFCEn(n) (1 << (((n - 1) * 2) + 1))
#define NEMC_NFCSR_NFEn(n)  (1 << (((n - 1) * 2) + 0))

#define NEMC_SMCR_SMT_BIT   0
#define NEMC_SMCR_BL_BIT    1
#define NEMC_SMCR_BW_BIT    6   /* bus width, 0: 8-bit 1: 16-bit */
#define NEMC_SMCR_TAS_BIT   8
#define NEMC_SMCR_TAH_BIT   12
#define NEMC_SMCR_TBP_BIT   16
#define NEMC_SMCR_TAW_BIT   20
#define NEMC_SMCR_STRV_BIT  24

#define NEMC_SMCR_TAS_MASK  0xf
#define NEMC_SMCR_TAH_MASK  0xf
#define NEMC_SMCR_TBP_MASK  0xf
#define NEMC_SMCR_TAW_MASK  0xf
#define NEMC_SMCR_STRV_MASK 0x3f

struct nemc_chips {
	int bank;
#define OFFSET_DATA     0x00000000
#define OFFSET_CMD      0x00400000
#define OFFSET_ADDR     0x00800000
	void* __iomem io_base;
};

struct jz_nemc {
	struct platform_device *pdev;
#define MAX_CHIP_NUM 3
	int selected;
	struct nemc_chips chips[MAX_CHIP_NUM];
	void * __iomem base;
	struct clk *clk;
	int max_chip_num;
};

int jz_nandc_dev_is_ready(struct device *dev, struct nand_chip *chip)
{
	udelay(chip->chip_delay);
	return 1;
}
EXPORT_SYMBOL(jz_nandc_dev_is_ready);

int jz_nandc_select_chip(struct device *dev, struct nand_chip *nand_chip, int chip)
{
	struct jz_nemc *nemc = dev_get_drvdata(dev);
	struct nemc_chips *nemc_chips = NULL;

	BUG_ON(chip >= nemc->max_chip_num);
	if (chip == -1 && nemc->selected >= 0) {
		uint32_t nfcsr = readl(nemc->base + NEMC_NFCSR);
		nemc_chips = &nemc->chips[nemc->selected];
		nfcsr &= ~NEMC_NFCSR_NFCEn(nemc_chips->bank);
		writel(nfcsr, nemc->base + NEMC_NFCSR);
	}
	nemc->selected = chip;
	nemc_chips = &nemc->chips[nemc->selected];
	nand_chip->IO_ADDR_R = nemc_chips->io_base + OFFSET_DATA;
	return 0;
}
EXPORT_SYMBOL(jz_nandc_select_chip);

void jz_nandc_cmd_ctrl(struct device *dev, struct nand_chip *nand_chip, int cmd, unsigned int ctrl)
{
	struct jz_nemc *nemc = dev_get_drvdata(dev);
	struct nemc_chips *nemc_chips = NULL;

	BUG_ON(nemc->selected < 0);
	nemc_chips = &nemc->chips[nemc->selected];

	if (ctrl & NAND_CTRL_CHANGE) {
		uint32_t nfcsr = readl(nemc->base + NEMC_NFCSR);

		if (ctrl & NAND_ALE)
			nand_chip->IO_ADDR_W = nemc_chips->io_base + OFFSET_ADDR;
		else if (ctrl & NAND_CLE)
			nand_chip->IO_ADDR_W = nemc_chips->io_base + OFFSET_CMD;
		else
			nand_chip->IO_ADDR_W = nemc_chips->io_base + OFFSET_DATA;

		if (ctrl & NAND_NCE)
			nfcsr |= NEMC_NFCSR_NFCEn(nemc_chips->bank);
		else
			nfcsr &= ~NEMC_NFCSR_NFCEn(nemc_chips->bank);

		writel(nfcsr, nemc->base + NEMC_NFCSR);
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, nand_chip->IO_ADDR_W);
}
EXPORT_SYMBOL(jz_nandc_cmd_ctrl);

int jz_nand_chip_init(struct jz_nemc *nemc, struct jz_nand_params *param)
{
	struct clk *clk = devm_clk_get(&nemc->pdev->dev, "h2clk");
	int valume, smcr, cycle, nfcsr;
	unsigned long h2clk;

	if (IS_ERR_OR_NULL(clk))
		return -EIO;

	h2clk = clk_get_rate(clk);
	cycle = 1000000000/(h2clk/1000);

	/* NEMC.TAS */
	valume = (param->tALS * 1000 + cycle - 1) / cycle;
	valume -= (valume > 1) ? 1 : 0;
	smcr = (valume & NEMC_SMCR_TAS_MASK) << NEMC_SMCR_TAS_BIT;
	/* NEMC.TAH */
	valume = (param->tALH * 1000 + cycle -1) / cycle;
	valume -= (valume > 1) ? 1 : 0;
	smcr |= (valume & NEMC_SMCR_TAH_MASK) << NEMC_SMCR_TAH_BIT;
	/* NEMC.TBP */
	valume = (param->tWP * 1000 + cycle - 1) / cycle;
	smcr |= (valume & NEMC_SMCR_TBP_MASK) << NEMC_SMCR_TBP_BIT;
	/* NEMC.TAW */
	valume = (param->tRP * 1000 + cycle -1) / cycle;
	smcr |= (valume & NEMC_SMCR_TAW_MASK) << NEMC_SMCR_TAW_BIT;
	/* NEMC.STRV */
	valume = (param->tRHW * 1000 + cycle - 1) / cycle;

	writel(smcr, nemc->base+NEMC_SMCRn(1));

	nfcsr = readl(nemc->base + NEMC_NFCSR);
	nfcsr |= NEMC_NFCSR_NFEn(1);
	writel(nfcsr, nemc->base + NEMC_NFCSR);

	nemc->clk = devm_clk_get(&nemc->pdev->dev "nemc");
	clk_enable(nemc->clk);
	return 0;
}

void jz_nand_chip_deinit(struct jz_nemc *nemc)
{
	clk_disable(nemc->clk);
}

static int jz_nemc_probe(struct platform_device *pdev)
{
	struct jz_nemc *nemc;
	struct jz_nand_params *nand_param = pdev->dev.platform_data;
	struct resource *res;
	int i;

	nemc = (struct jz_nemc *)devm_kzalloc(&pdev->dev,
			sizeof(struct jz_nemc), GFP_KERNEL);
	if (!nemc)
		return -ENOMEM;
	nemc->pdev = pdev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM , "nemc");
	if (!res)
		return -ENOMEM;
	res = devm_request_mem_region(&pdev->dev, res->start,
			resource_size(res), "nemc");
	if (!res)
		return -ENOMEM;
	nemc->base = devm_ioremap_nocache(&pdev->dev, res->start, resource_size(res));
	if(!nemc->base)
		return -ENOMEM;
	nemc->selected = -1;
	for (i = 0; i < MAX_CHIP_NUM; i++) {
		char name[20];
		sprintf(name, "nemc-cs%d", i + 1);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM , name);
		if (!res)
			break;
		res = devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res), name);
		if (!res)
			return -ENOMEM;
		nemc->chips[i].io_base = devm_ioremap_nocache(&pdev->dev,
				res->start, resource_size(res));
		if (!nemc->chips[i].io_base)
			return -ENOMEM;
		nemc->chips[i].bank = i + 1;
		nemc->max_chip_num++;
	}

	if (nemc->max_chip_num == 0)
		return -ENOMEM;

	if (jz_nand_chip_init(nemc, nand_param))
		return -EIO;

	platform_set_drvdata(pdev, nemc);

	dev_info(&pdev->dev, "Successfully registered External Nand Memory Controller driver\n");
	return 0;
}

static int jz_nemc_remove(struct platform_device *pdev)
{
	struct jz_nemc *nemc = platform_get_drvdata(pdev);
	jz_nand_chip_deinit(nemc);
	return 0;
}

static struct platform_driver jz_nemc_driver = {
	.probe = jz_nemc_probe,
	.remove = jz_nemc_remove,
	.driver = {
		.name = "jz-nemc",
		.owner = THIS_MODULE,
	},
}
;
static int __init jz_nemc_init(void)
{
	return platform_driver_register(&jz_nemc_driver);
}
module_init(jz_nemc_init);

static void __exit jz_nemc_exit(void)
{
	platform_driver_unregister(&jz_nemc_driver);
}
module_exit(jz_nemc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cli <chen.li@ingenic.com>");
MODULE_DESCRIPTION("Ingenic External Nand Memory Controller");
