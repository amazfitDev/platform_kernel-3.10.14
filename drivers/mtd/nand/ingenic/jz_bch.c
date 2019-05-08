/*
 *  Copyright (C) cli <chen.li@ingenic.com>
 *  BCH driver
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
#include <linux/types.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/err.h>
#include "jz_bch.h"

#define CNT_PARITY  28
#define CNT_ERRROPT 64

struct bch_regs {
	volatile u32 bhcr;
	volatile u32 bhcsr;
	volatile u32 bhccr;
	volatile u32 bhcnt;
	volatile u32 bhdr;
	volatile u32 bhpar[CNT_PARITY];
	volatile u32 bherr[CNT_ERRROPT];
	volatile u32 bhint;
	volatile u32 bhintes;
	volatile u32 bhintec;
	volatile u32 bhinte;
	volatile u32 bhto;
};

#define BCH_BHCR_BSEL_SHIFT             4
#define BCH_BHCR_BSEL_MASK              (0x7f << BCH_BHCR_BSEL_SHIFT)
#define BCH_BHCR_ENCE                   BIT(2)
#define BCH_BHCR_INIT                   BIT(1)
#define BCH_BHCR_BCHE                   BIT(0)

#define BCH_BHCNT_PARITYSIZE_SHIFT      16
#define BCH_BHCNT_PARITYSIZE_MASK       (0x7f << BCH_BHCNT_PARITYSIZE_SHIFT)
#define BCH_BHCNT_BLOCKSIZE_SHIFT       0
#define BCH_BHCNT_BLOCKSIZE_MASK        (0x7ff << BCH_BHCNT_BLOCKSIZE_SHIFT)

#define BCH_BHERR_MASK_SHIFT            16
#define BCH_BHERR_MASK_MASK             (0xffff << BCH_BHERR_MASK_SHIFT)
#define BCH_BHERR_INDEX_SHIFT           0
#define BCH_BHERR_INDEX_MASK            (0x7ff << BCH_BHERR_INDEX_SHIFT)

#define BCH_BHINT_ERRC_SHIFT            24
#define BCH_BHINT_ERRC_MASK             (0x7f << BCH_BHINT_ERRC_SHIFT)
#define BCH_BHINT_TERRC_SHIFT           16
#define BCH_BHINT_TERRC_MASK            (0x7f << BCH_BHINT_TERRC_SHIFT)
#define BCH_BHINT_DECF                  BIT(3)
#define BCH_BHINT_ENCF                  BIT(2)
#define BCH_BHINT_UNCOR                 BIT(1)
#define BCH_BHINT_ERR                   BIT(0)

#define BCH_TIMEOUT                     100

struct jz_bch {
	struct clk *clk;
	struct clk *clk_gate;
	struct bch_regs __iomem *regs;
};

static void jz_bch_init(struct jz_bch *bch,
			    struct jz_bch_params *params, bool encode)
{
	uint32_t reg;

	/* Clear interrupt status. */
	writel(readl(&bch->regs->bhint), &bch->regs->bhint);

	/* Set up BCH count register. */
	reg = params->size << BCH_BHCNT_BLOCKSIZE_SHIFT;
	reg |= params->bytes << BCH_BHCNT_PARITYSIZE_SHIFT;
	writel(reg, &bch->regs->bhcnt);

	/* Initialise and enable BCH. */
	reg = BCH_BHCR_BCHE | BCH_BHCR_INIT;
	reg |= params->strength << BCH_BHCR_BSEL_SHIFT;
	if (encode)
		reg |= BCH_BHCR_ENCE;
	writel(reg, &bch->regs->bhcr);
}

static void jz_bch_disable(struct jz_bch *bch)
{
	writel(readl(&bch->regs->bhint), &bch->regs->bhint);
	writel(BCH_BHCR_BCHE, &bch->regs->bhccr);
}

static void jz_bch_write_data(struct jz_bch *bch, const void *buf,
				  size_t size)
{
	size_t align32 = ((4 - ((size_t )buf & 0x3)) & 0x3);		/*Note: size always bigger than 4 byte*/
	size_t size32 = (size - align32) / sizeof(uint32_t);
	size_t size8 = (size - align32) & (sizeof(uint32_t) - 1);
	uint32_t *src32;
	uint8_t *src8;

	src8 = (uint8_t *)buf;
	while (align32--)
		writeb(*src8++, &bch->regs->bhdr);

	src32 = (uint32_t *)src8;
	while (size32--)
		writel(*src32++, &bch->regs->bhdr);

	src8 = (uint8_t *)src32;
	while (size8--)
		writeb(*src8++, &bch->regs->bhdr);
}

static void jz_bch_read_parity(struct jz_bch *bch, void *buf,
		size_t size)
{
	size_t size32 = size / sizeof(uint32_t);
	size_t size8 = size & (sizeof(uint32_t) - 1);
	uint32_t *dest32;
	uint8_t *dest8;
	uint32_t val;
	int i = 0;

	dest32 = (uint32_t *)buf;
	while (size32--) {
		*dest32++ = readl(&bch->regs->bhpar[i]);
		i++;
	}

	dest8 = (uint8_t *)dest32;
	val = readl(&bch->regs->bhpar[i]);
	switch (size8) {
	case 3:
		dest8[2] = (val >> 16) & 0xff;
	case 2:
		dest8[1] = (val >> 8) & 0xff;
	case 1:
		dest8[0] = val & 0xff;
		break;
	}
}

static bool jz_bch_wait_complete(struct jz_bch *bch, unsigned int irq,
		uint32_t *status)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(BCH_TIMEOUT);
	uint32_t reg = 0;

	/*
	 * While we could use use interrupts here and sleep until the operation
	 * completes, the controller works fairly quickly (usually a few
	 * microseconds), so the overhead of sleeping until we get an interrupt
	 * actually quite noticably decreases performance.
	 */
	while (!((reg = readl(&bch->regs->bhint)) & irq) &&
			time_before(jiffies, timeout))
		cond_resched();

	if (likely(reg & irq)) {
		if (status) *status = reg;
		writel(reg, &bch->regs->bhint);
		return true;
	}
	printk("%lx %lx reg = %x\n", jiffies, timeout, reg);
	return false;
}

/**
 * jz_bch_calculate() - calculate ECC for a data buffer
 * @dev: BCH device.
 * @params: BCH parameters.
 * @buf: input buffer with raw data.
 * @ecc_code: output buffer with ECC.
 *
 * Return: 0 on success, -ETIMEDOUT if timed out while waiting for BCH
 * controller.
 */
int jz_bch_calculate(struct device *dev, struct jz_bch_params *params,
			 const uint8_t *buf, uint8_t *ecc_code)
{
	struct jz_bch *bch = dev_get_drvdata(dev);
	int ret = 0;

	jz_bch_init(bch, params, true);
	jz_bch_write_data(bch, buf, params->size);

	if (jz_bch_wait_complete(bch, BCH_BHINT_ENCF, NULL)) {
		jz_bch_read_parity(bch, ecc_code, params->bytes);
	} else {
		dev_err(dev, "timed out while calculating ECC\n");
		ret = -ETIMEDOUT;
	}

	jz_bch_disable(bch);
	return ret;
}
EXPORT_SYMBOL(jz_bch_calculate);

/**
 * jz_bch_correct() - detect and correct bit errors
 * @dev: BCH device.
 * @params: BCH parameters.
 * @buf: raw data read from the chip.
 * @ecc_code: ECC read from the chip.
 *
 * Given the raw data and the ECC read from the NAND device, detects and
 * corrects errors in the data.
 *
 * Return: the number of bit errors corrected, or -1 if there are too many
 * errors to correct or we timed out waiting for the controller.
 */
int jz_bch_correct(struct device *dev, struct jz_bch_params *params,
		       uint8_t *buf, uint8_t *ecc_code)
{
	struct jz_bch *bch = dev_get_drvdata(dev);
	uint32_t reg, mask, index;
	int i, ret, count;

	jz_bch_init(bch, params, false);
	jz_bch_write_data(bch, buf, params->size);
	jz_bch_write_data(bch, ecc_code, params->bytes);

	if (!jz_bch_wait_complete(bch, BCH_BHINT_DECF, &reg)) {
		dev_err(dev, "timed out while correcting data\n");
		ret = -1;
		goto out;
	}

	if (reg & BCH_BHINT_UNCOR) {
		dev_warn(dev, "uncorrectable ECC error\n");
		ret = -1;
		goto out;
	}

	/* Correct any detected errors. */
	if (reg & BCH_BHINT_ERR) {
		count = (reg & BCH_BHINT_ERRC_MASK) >> BCH_BHINT_ERRC_SHIFT;
		ret = (reg & BCH_BHINT_TERRC_MASK) >> BCH_BHINT_TERRC_SHIFT;
		for (i = 0; i < count; i++) {
			reg = readl(&bch->regs->bherr[i]);
			mask = (reg & BCH_BHERR_MASK_MASK) >>
				BCH_BHERR_MASK_SHIFT;
			index = (reg & BCH_BHERR_INDEX_MASK) >>
				BCH_BHERR_INDEX_SHIFT;
			buf[(index * 2) + 0] ^= mask;
			buf[(index * 2) + 1] ^= mask >> 8;
		}
	} else {
		ret = 0;
	}
out:
	jz_bch_disable(bch);
	return ret;
}
EXPORT_SYMBOL(jz_bch_correct);

static int jz_bch_probe(struct platform_device *pdev)
{
	struct jz_bch *bch = NULL;
	struct resource *res = NULL;

	bch = devm_kzalloc(&pdev->dev, sizeof(struct jz_bch), GFP_KERNEL);
	if (!bch)
		return -ENOMEM;
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "bch");
	if (!res)
		return -EIO;
	res = devm_request_mem_region(&pdev->dev, res->start, resource_size(res), "bch");
	if (!res)
		return -ENOMEM;
	bch->regs = devm_ioremap_nocache(&pdev->dev, res->start, resource_size(res));
	if (!bch->regs)
		return -ENOMEM;
	bch->clk = clk_get(&pdev->dev, "cgu_bch");
	bch->clk_gate = clk_get(&pdev->dev, "bch");
	if (IS_ERR_OR_NULL(bch->clk) || IS_ERR_OR_NULL(bch->clk_gate)) {
		return -ENODEV;
	}

	clk_enable(bch->clk_gate);
	clk_set_rate(bch->clk, 200 * 1000 * 1000);
	clk_enable(bch->clk);

	platform_set_drvdata(pdev, bch);

	dev_info(&pdev->dev, "Successfully registered bch driver\n");
	return 0;
}

static int jz_bch_remove(struct platform_device *pdev)
{
	struct jz_bch *bch = platform_get_drvdata(pdev);

	clk_put(bch->clk);
	clk_disable(bch->clk);
	clk_put(bch->clk_gate);
	clk_disable(bch->clk_gate);
	return 0;
}

static struct platform_driver jz_bch_driver = {
	.probe = jz_bch_probe,
	.remove = jz_bch_remove,
	.driver = {
		.name = "jz-bch",
		.owner = THIS_MODULE,
	},
};

static int jz_bch_plat_init(void)
{
	return platform_driver_register(&jz_bch_driver);
}
module_init(jz_bch_plat_init);

static void jz_bch_plat_exit(void)
{
	platform_driver_unregister(&jz_bch_driver);
}
module_exit(jz_bch_plat_exit);

MODULE_DESCRIPTION("Ingenic JZ BCH error correction driver");
MODULE_AUTHOR("cli<chen.li@ingenic.com>");
MODULE_LICENSE("GPL");
