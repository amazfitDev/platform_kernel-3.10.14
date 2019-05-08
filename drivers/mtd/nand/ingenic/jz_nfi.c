/*
 *  Copyright (C) cli <chen.li@ingenic.com>
 *  Nand Falsh Interface controller driver
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

#define NAND_NFCM_DLYF_MASK		(0x7ff << 21)	 /* Extra delay time before CLE goes to high */
#define NAND_NFCM_DLYF(n)		((n) << 21)	 /* Extra delay time before CLE goes to high */
#define NAND_NFCM_DLYB_MASK		(0x7ff << 10)	 /* Extra delay time after CLE goes to low */
#define NAND_NFCM_DLYB(n)		((n) << 10)	 /* Extra delay time after CLE goes to low */
#define NAND_NFCM_BUSY			(1 << 8)	 /* wait busy start */
#define NAND_NFCM_CMD_MASK		(0xff << 0)	 /* nand flash command */
#define NAND_NFCM_CMD(n)		((n) << 0)	 /* nand flash command */

#define NAND_NFCR_CSEN_MASK		(0x3f << 16)
#define NAND_NFCR_INIT			(0x1 << 1)		 /* inital NFI, the bit is wrote 1 only,when soft reset */
#define NAND_NFCR_EN			(0x1 << 0)		 /* NFI enable */
#define NAND_NFCR_BUSWIDTH_MASK		(0x1 << 5)		 /* the buswidth of nand interface */
#define NAND_NFCR_BUSWIDTH(n)		((n) << 5)		 /* the buswidth of nand interface */
#define NAND_NFCR_SEL_MASK		(0x7 << 2)		 /* nand flash interface select */
#define NAND_NFCR_SEL(n)		((n) << 2)		 /* nand flash interface select */
#define NAND_NFCR_EMPTY			(0x1 << 11)		 /* All command/address/write data/read data process done */
#define NAND_NFCR_CSMOD			(0x1 << 15)		 /* if the bit is 1, we should be select chip with NFCS when the nand*/
#define NAND_NFCR_IDLE			(0x1 << 6)		 /* NFI is idle;when write->read,*/

/* NAND	flash timing0 */
#define NAND_NFIT0_SWE_BIT	16 /* Number of clock cycles+1(HCLK) to set up the WE; min ~ max -> 0x0 ~ 0xfffe */
#define NAND_NFIT0_SWE_MASK	(0xFFFF << NAND_NFIT0_SWE_BIT)
#define NAND_NFIT0_SWE(n)	((n) << NAND_NFIT0_SWE_BIT)
#define NAND_NFIT0_WWE_BIT	0 /* Define the minimum number of HCLK clock cycles to assert WE;min~max -> 0x1~0xfffe */
#define NAND_NFIT0_WWE_MASK	(0xFFFF << NAND_NFIT0_WWE_BIT)
#define NAND_NFIT0_WWE(n)	((n) << NAND_NFIT0_WWE_BIT)

/* NAND	flash timing1 */
#define NAND_NFIT1_HWE_BIT	16 /* Number of clock cycles+1(HCLK) to hold the WE; min ~ max -> 0x0 ~ 0xfffe */
#define NAND_NFIT1_HWE_MASK	(0xFFFF << NAND_NFIT1_HWE_BIT)
#define NAND_NFIT1_HWE(n)	((n) << NAND_NFIT1_HWE_BIT)
#define NAND_NFIT1_SRE_BIT	0 /* Number of clock cycles+1(HCLK) to set up the RE; min~max -> 0x0~0xfffe */
#define NAND_NFIT1_SRE_MASK	(0xFFFF << NAND_NFIT1_SRE_BIT)
#define NAND_NFIT1_SRE(n)	((n) << NAND_NFIT1_SRE_BIT)

/* NAND	flash timing2 */
#define NAND_NFIT2_WRE_BIT	16 /* Define the minimum number of HCLK clock cycles to assert RE;min~max -> 0x1~0xfffe */
#define NAND_NFIT2_WRE_MASK	(0xFFFF << NAND_NFIT2_WRE_BIT)
#define NAND_NFIT2_WRE(n)	((n) << NAND_NFIT2_WRE_BIT)
#define NAND_NFIT2_HRE_BIT	0 /* Number of clock cycles+1(HCLK) to hold the RE; min ~ max -> 0x0 ~ 0xfffe */
#define NAND_NFIT2_HRE_MASK	(0xFFFF << NAND_NFIT2_HRE_BIT)
#define NAND_NFIT2_HRE(n)	((n) << NAND_NFIT2_HRE_BIT)

/* NAND	flash timing3 */
#define NAND_NFIT3_SCS_BIT	16 /* Nand chip select setup time; min~max -> 0x1~0xfffe */
#define NAND_NFIT3_SCS_MASK	(0xFFFF << NAND_NFIT3_SCS_BIT)
#define NAND_NFIT3_SCS(n)	((n) << NAND_NFIT3_SCS_BIT)
#define NAND_NFIT3_WCS_BIT	0 /* Nand chip select wait time; min~max -> 0x1~0xfffe */
#define NAND_NFIT3_WCS_MASK	(0xFFFF << NAND_NFIT3_WCS_BIT)
#define NAND_NFIT3_WCS(n)	((n) << NAND_NFIT3_WCS_BIT)


/* NAND	flash timing4 */
#define NAND_NFIT4_BUSY_BIT	16 /* Ready to RE high time; min~max -> 0x0~0xfffe */
#define NAND_NFIT4_BUSY_MASK	(0xFFFF << NAND_NFIT4_BUSY_BIT)
#define NAND_NFIT4_BUSY(n)	((n) << NAND_NFIT4_BUSY_BIT)
#define NAND_NFIT4_EDO_BIT	0 /* Nand flash EDO mode delay for reading data; min~max -> 0x1~0xfffe */
#define NAND_NFIT4_EDO_MASK	(0xFFFF << NAND_NFIT4_EDO_BIT)
#define NAND_NFIT4_EDO(n)	((n) << NAND_NFIT4_EDO_BIT)

struct nfi_regs {
	volatile uint32_t res0;
	volatile uint32_t res1;
	volatile uint32_t nfcs;
	volatile uint32_t nfbc;
	volatile uint32_t nfcr;
	volatile uint32_t pncr;
	volatile uint32_t pndr;
	volatile uint32_t bcnt;
	volatile uint32_t nfdl;
	volatile uint32_t nfi_t0;
	volatile uint32_t nfi_t1;
	volatile uint32_t nfi_t2;
	volatile uint32_t nfi_t3;
	volatile uint32_t nfi_t4;
	volatile uint32_t nfi_tg0;
	volatile uint32_t nfi_tg1;
	volatile uint32_t nfi_of0;
	volatile uint32_t nfi_of1;
	volatile uint32_t nfbs0;
	volatile uint32_t nfbs1;
	volatile uint32_t nfrb;
};

struct nfi_chips {
	int bank;
#define OFFSET_DATA     0x00000000
#define OFFSET_CMD      0x00400000
#define OFFSET_ADDR     0x00800000
	void* __iomem io_base;
};

struct jz_nfi {
	struct platform_device *pdev;
#define MAX_CHIP_NUM 3
	int selected;
	struct nfi_chips chips[MAX_CHIP_NUM];
	struct nfi_regs *regs;
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
	struct jz_nfi* nfi = dev_get_drvdata(dev);
	struct nfi_chips *nfi_chips = NULL;

	BUG_ON(chip >= nfi->max_chip_num);
	if (chip != -1) {
		if (unlikely(nfi->selected != -1 &&
					nfi->selected != chip)) {
			writel(0, &nfi->regs->nfcs);
		}
		nfi->selected = chip;
		nfi_chips = &nfi->chips[nfi->selected];
		writel(nfi_chips->bank,  &nfi->regs->nfcs);
		nand_chip->IO_ADDR_R = nfi_chips->io_base + OFFSET_DATA;
	} else {
		nfi->selected = chip;
		writel(0, &nfi->regs->nfcs);
		nand_chip->IO_ADDR_R  = 0;
	}

	return 0;
}
EXPORT_SYMBOL(jz_nandc_select_chip);

void jz_nandc_cmd_ctrl(struct device *dev, struct nand_chip *nand_chip, int cmd, unsigned int ctrl)
{
	struct jz_nfi* nfi = dev_get_drvdata(dev);
	struct nfi_chips *nfi_chips = NULL;

	BUG_ON(nfi->selected < 0);
	nfi_chips = &nfi->chips[nfi->selected];
	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_ALE)
			nand_chip->IO_ADDR_W = nfi_chips->io_base + OFFSET_ADDR;
		else if (ctrl & NAND_CLE)
			nand_chip->IO_ADDR_W = nfi_chips->io_base + OFFSET_CMD;
		else
			nand_chip->IO_ADDR_W = nfi_chips->io_base + OFFSET_DATA;
	}
	if (cmd != NAND_CMD_NONE)
		writeb(cmd, nand_chip->IO_ADDR_W);
}
EXPORT_SYMBOL(jz_nandc_cmd_ctrl);

int jz_nand_chip_init(struct jz_nfi *nfi, struct jz_nand_params *param)
{
	struct clk *clk = devm_clk_get(&nfi->pdev->dev, "h2clk");
	unsigned int cycle, nfcr, lbit, hbit;
	unsigned long h2clk;

	nfi->clk = devm_clk_get(&nfi->pdev->dev, "nfi");
	if (IS_ERR_OR_NULL(clk) || IS_ERR_OR_NULL(nfi->clk))
		return -EIO;
	clk_enable(nfi->clk);

	h2clk = clk_get_rate(clk);
	cycle = 1000000000/(h2clk/1000);
	printk("h2clk = %ld cycle %d\n", h2clk, cycle);

	/*enable nfi*/
	nfcr = readl(&nfi->regs->nfcr);
	nfcr &= ~(NAND_NFCR_CSEN_MASK);
	nfcr |= NAND_NFCR_CSMOD | NAND_NFCR_INIT | NAND_NFCR_EN;
	writel(nfcr, &nfi->regs->nfcr);
	ndelay(100);

	/*select buswidth and mode(common nand)*/
	do {
		nfcr = readl(&nfi->regs->nfcr);
	} while(!(nfcr & NAND_NFCR_EMPTY));
	nfcr &= ~(NAND_NFCR_SEL_MASK | NAND_NFCR_BUSWIDTH_MASK);
	nfcr |=  NAND_NFCR_BUSWIDTH((param->buswidth == 16)) | NAND_NFCR_SEL(0);
	writel(nfcr, &nfi->regs->nfcr);

	printk("nfcr %x\n", readl(&nfi->regs->nfcr));

	hbit = ((param->tWH - param->tDH) * 1000) / cycle;
	lbit = ((param->tWP) * 1000 + cycle - 1) / cycle;
	writel((NAND_NFIT0_SWE(hbit) | NAND_NFIT0_WWE(lbit)), &nfi->regs->nfi_t0);

	hbit = ((param->tDH) * 1000) / cycle;
	lbit = ((param->tREH - param->tDH ) * 1000 + cycle - 1) / cycle;
	writel((NAND_NFIT1_HWE(hbit) | NAND_NFIT1_SRE(lbit)), &nfi->regs->nfi_t1);

	hbit = ((param->tRP) * 1000 + cycle - 1) / cycle;
	lbit = ((param->tDH) * 1000) / cycle;
	writel((NAND_NFIT2_WRE(hbit) | NAND_NFIT2_HRE(lbit)), &nfi->regs->nfi_t2);

	hbit = ((param->tCS) * 1000 + cycle - 1) / cycle;
	lbit = ((param->tCH) * 1000 + cycle - 1) / cycle;
	writel((NAND_NFIT3_SCS(hbit) | NAND_NFIT3_WCS(lbit)), &nfi->regs->nfi_t3);

	hbit = ((param->tRR) * 1000 + cycle - 1) / cycle;
	lbit = (1 * 1000 + cycle - 1) / cycle ;
	writel((NAND_NFIT4_BUSY(hbit) | NAND_NFIT4_EDO(lbit)), &nfi->regs->nfi_t4);

	printk("%s time0 = 0x%08x, time1 = 0x%08x \n time2 = 0x%08x, time3 = 0x%08x, time4 = 0x%08x\n",
			__func__, *(volatile unsigned int *)0xb3410028,*(volatile unsigned int *)0xb341002c,
			*(volatile unsigned int *)0xb3410030, *(volatile unsigned int *)0xb3410034,
			*(volatile unsigned int *)0xb3410038);

	return 0;
}

void jz_nand_chip_deinit(struct jz_nfi *nfi)
{
	unsigned int nfcr;
	clk_disable(nfi->clk);
	nfi->clk = NULL;
	nfcr = readl(&nfi->regs->nfcr);
	nfcr &= ~(NAND_NFCR_EN);
	writel(nfcr, &nfi->regs->nfcr);
}

static int jz_nfi_probe(struct platform_device *pdev)
{
	struct jz_nfi *nfi = NULL;
	struct resource *res;
	struct jz_nand_params *nand_param = pdev->dev.platform_data;
	int i;

	nfi = devm_kzalloc(&pdev->dev, sizeof(struct jz_nfi), GFP_KERNEL);
	if (!nfi)
		return -ENOMEM;
	nfi->pdev = pdev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM , "nfi");
	if (!res)
		return -ENOMEM;
	res = devm_request_mem_region(&pdev->dev, res->start,
			resource_size(res), "nfi");
	if (!res)
		return -ENOMEM;
	nfi->regs = devm_ioremap_nocache(&pdev->dev, res->start, resource_size(res));
	if(!nfi->regs)
		return -ENOMEM;
	nfi->selected = -1;

	for (i = 0; i < MAX_CHIP_NUM; i++) {
		char name[20];
		sprintf(name, "nfi_cs%d", i + 1);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
		if (!res)
			break;
		res = devm_request_mem_region(&pdev->dev, res->start,
				resource_size(res), name);
		if (!res)
			break;
		nfi->chips[i].io_base = devm_ioremap_nocache(&pdev->dev,
				res->start, resource_size(res));
		if (!nfi->chips[i].io_base)
			break;
		nfi->chips[i].bank = i + 1;
		nfi->max_chip_num++;
	}

	if (nfi->max_chip_num == 0)
		return -ENOMEM;

	if (jz_nand_chip_init(nfi, nand_param))
		return -EIO;

	platform_set_drvdata(pdev, nfi);

	dev_info(&pdev->dev, "Successfully registered NAND Flash Interface driver.\n");
	return 0;
}

static int jz_nfi_remove(struct platform_device *pdev)
{
	struct jz_nfi *nfi = platform_get_drvdata(pdev);
	jz_nand_chip_deinit(nfi);
	return 0;
}

static struct platform_driver jz_nfi_driver = {
	.probe = jz_nfi_probe,
	.remove = jz_nfi_remove,
	.driver = {
		.name = "jz-nfi",
		.owner = THIS_MODULE,
	},
}
;
static int __init jz_nfi_init(void)
{
	return platform_driver_register(&jz_nfi_driver);
}
module_init(jz_nfi_init);

static void __exit jz_nfi_exit(void)
{
	platform_driver_unregister(&jz_nfi_driver);
}
module_exit(jz_nfi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cli <chen.li@ingenic.com>");
MODULE_DESCRIPTION("Ingenic Nand Flash Interface driver");
