#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/bsearch.h>

#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>

#include "clk.h"

static DEFINE_SPINLOCK(cpm_pll_lock);

static unsigned long pll_get_rate(struct clk *clk) {
	unsigned int offset;
	unsigned long cpxpcr;
	unsigned long m,n,od;
	unsigned long rate;
	unsigned long flags;

	spin_lock_irqsave(&cpm_pll_lock,flags);
	if(clk->CLK_ID  == CLK_ID_APLL)
		offset = 8;
	else if(clk->CLK_ID == CLK_ID_MPLL)
		offset = 7;
	else
		offset = 0;

	cpxpcr = cpm_inl(CLK_PLL_NO(clk->flags));
	if(cpxpcr >> offset & 1){
		clk->flags |= CLK_FLG_ENABLE;
		m = ((cpxpcr >> 24) & 0x7f) + 1;
		n = ((cpxpcr >> 18) & 0x1f) + 1;
		od = ((cpxpcr >> 16) & 0x3);
		od = 1 << od;
		rate = clk->parent->rate * m / n / od;
	}else  {
		clk->flags &= ~(CLK_FLG_ENABLE);
		rate = 0;
	}
	spin_unlock_irqrestore(&cpm_pll_lock,flags);
	return rate;
}
static struct clk_ops clk_pll_ops = {
	.get_rate = pll_get_rate,
	.set_rate = NULL,
};
void __init init_ext_pll(struct clk *clk)
{
	switch (get_clk_id(clk)) {
	case CLK_ID_EXT0:
		clk->rate = JZ_EXTAL_RTC;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	case CLK_ID_EXT1:
		clk->rate = JZ_EXTAL;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	case CLK_ID_OTGPHY:
		clk->rate = 48 * 1000 * 1000;
		clk->flags |= CLK_FLG_ENABLE;
		break;
	default:
		clk->parent = get_clk_from_id(CLK_ID_EXT1);
		clk->rate = pll_get_rate(clk);
		clk->ops = &clk_pll_ops;
		break;
	}
}
