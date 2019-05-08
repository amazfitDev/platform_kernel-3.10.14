/* arch/mips/xburst/soc-m200/common/irq.c
 * Interrupt controller driver for Ingenic's M200 SoC.
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 *	http://www.ingenic.com
 * Author: Huang LiHong <lhhuang@ingenic.cn>
 * Modified By: Sun Jiwei <jiwei.sun@ingenic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>

#include <asm/irq_cpu.h>

#include <soc/base.h>
#include <soc/irq.h>

#include <smp_cp0.h>

#define TRACE_IRQ        1
#define PART_OFF	0x20

#define ISR_OFF		(0x00)
#define IMR_OFF		(0x04)
#define IMSR_OFF	(0x08)
#define IMCR_OFF	(0x0c)
#define IPR_OFF		(0x10)

static void __iomem *intc_base;
static unsigned long intc_saved[2];
static unsigned long intc_wakeup[2];

extern void __enable_irq(struct irq_desc *desc, unsigned int irq, bool resume);

static void intc_irq_ctrl(struct irq_data *data, int msk, int wkup)
{
	int intc = (int)irq_data_get_irq_chip_data(data);
	void *base = intc_base + PART_OFF * (intc/32);

	if (msk == 1)
		writel(BIT(intc%32), base + IMSR_OFF);
	else if (msk == 0)
		writel(BIT(intc%32), base + IMCR_OFF);

	if (wkup == 1)
		intc_wakeup[intc / 32] |= 1 << (intc % 32);
	else if (wkup == 0)
		intc_wakeup[intc / 32] &= ~(1 << (intc % 32));
}

static void intc_irq_unmask(struct irq_data *data)
{
	intc_irq_ctrl(data, 0, -1);
}

static void intc_irq_mask(struct irq_data *data)
{
	intc_irq_ctrl(data, 1, -1);
}

static int intc_irq_set_wake(struct irq_data *data, unsigned int on)
{
	intc_irq_ctrl(data, -1, !!on);
	return 0;
}

static struct irq_chip jzintc_chip = {
	.name 		= "jz-intc",
	.irq_mask	= intc_irq_mask,
	.irq_mask_ack 	= intc_irq_mask,
	.irq_unmask 	= intc_irq_unmask,
	.irq_set_wake 	= intc_irq_set_wake,
};

void __init arch_init_irq(void)
{
	int i;

	clear_c0_status(0xff04); /* clear ERL */
	set_c0_status(0x0400);   /* set IP2 */

	/* Set up MIPS CPU irq */
	mips_cpu_irq_init();

	/* Set up INTC irq */
	intc_base = ioremap(INTC_IOBASE, 0xfff);

	writel(0xffffffff, intc_base + IMSR_OFF);
	writel(0xffffffff, intc_base + PART_OFF + IMSR_OFF);
	for (i = IRQ_INTC_BASE; i < IRQ_INTC_BASE + INTC_NR_IRQS; i++) {
		irq_set_chip_data(i, (void *)(i - IRQ_INTC_BASE));
		irq_set_chip_and_handler(i, &jzintc_chip, handle_level_irq);
	}

	/* enable cpu interrupt mask */
	set_c0_status(IE_IRQ0 | IE_IRQ1);

	return;
}

static void intc_irq_dispatch(void)
{
	unsigned long ipr[2];

	ipr[0] = readl(intc_base + IPR_OFF);

	ipr[1] = readl(intc_base + PART_OFF + IPR_OFF);

	if (ipr[0]) {
		do_IRQ(ffs(ipr[0]) - 1 + IRQ_INTC_BASE);
	}

	if (ipr[1]) {
		do_IRQ(ffs(ipr[1]) + 31 + IRQ_INTC_BASE);
	}
}

asmlinkage void plat_irq_dispatch(void)
{
	unsigned int cause = read_c0_cause();
	unsigned int pending = cause & read_c0_status() & ST0_IM;

	if(pending & CAUSEF_IP2)
		intc_irq_dispatch();
	cause = read_c0_cause();
	pending = cause & read_c0_status() & ST0_IM;
}

void arch_suspend_disable_irqs(void)
{
	int i,j,irq;
	struct irq_desc *desc;

	local_irq_disable();

	intc_saved[0] = readl(intc_base + IMR_OFF);
	intc_saved[1] = readl(intc_base + PART_OFF + IMR_OFF);

	writel(0xffffffff & ~intc_wakeup[0], intc_base + IMSR_OFF);
	writel(0xffffffff & ~intc_wakeup[1], intc_base + PART_OFF + IMSR_OFF);

	for(j=0;j<2;j++) {
		for(i=0;i<32;i++) {
			if(intc_wakeup[j] & (0x1<<i)) {
				irq = i + IRQ_INTC_BASE + 32*j;
				desc = irq_to_desc(irq);
				__enable_irq(desc, irq, true);
			}
		}
	}
}

void arch_suspend_enable_irqs(void)
{
	writel(0xffffffff & ~intc_saved[0], intc_base + IMCR_OFF);
	writel(0xffffffff & ~intc_saved[1], intc_base + PART_OFF + IMCR_OFF);
	local_irq_enable();
}
