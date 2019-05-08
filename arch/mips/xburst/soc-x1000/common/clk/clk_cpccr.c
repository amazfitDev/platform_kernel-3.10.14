#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/cpufreq.h>
#include <linux/bsearch.h>

#include <asm/cacheops.h>
#include <soc/cpm.h>
#include <soc/cache.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <jz_notifier.h>
#include "clk.h"
#define USE_PLL

struct cpccr_clk {
	short off,sel,ce;
};
static struct cpccr_clk cpccr_clks[] = {
#define CPCCR_CLK(N,O,D,E)			\
	[N] = { .off = O, .sel = D, .ce = E}
	CPCCR_CLK(CDIV, 0, 28,22),
	CPCCR_CLK(L2CDIV, 4, 28,22),
	CPCCR_CLK(H0DIV, 8, 26,21),
	CPCCR_CLK(H2DIV, 12, 24,20),
	CPCCR_CLK(PDIV, 16, 24,20),
	CPCCR_CLK(SCLKA,-1, -1,30),
#undef CPCCR_CLK
};
static unsigned int cpccr_selector[4] = {0,CLK_ID_SCLKA,CLK_ID_MPLL,0};

static unsigned long cpccr_get_rate(struct clk *clk)
{
	int sel;
	unsigned long cpccr = cpm_inl(CPM_CPCCR);
	unsigned int rate;
	int v;
	if(CLK_CPCCR_NO(clk->flags) == SCLKA)
	{
		int clka_sel[4] = {0,CLK_ID_EXT1,CLK_ID_APLL,0};
		sel = cpm_inl(CPM_CPCCR) >> 30;
		if(clka_sel[sel] == 0) {
			rate = 0;
			clk->flags &= ~CLK_FLG_ENABLE;
		}else {
			clk->parent = get_clk_from_id(clka_sel[sel]);
			rate = clk->parent->rate;
			clk->flags |= CLK_FLG_ENABLE;
		}
	}else{
		v = (cpccr >> cpccr_clks[CLK_CPCCR_NO(clk->flags)].off) & 0xf;
		sel = (cpccr >> (cpccr_clks[CLK_CPCCR_NO(clk->flags)].sel)) & 0x3;
		rate = get_clk_from_id(cpccr_selector[sel])->rate;
		rate = rate / (v + 1);
	}
	return rate;
}
static struct clk_ops clk_cpccr_ops = {
	.get_rate = cpccr_get_rate,
	.set_rate = NULL,
};

void __init init_cpccr_clk(struct clk *clk)
{
	int sel;	//check
	unsigned long cpccr = cpm_inl(CPM_CPCCR);
	if(CLK_CPCCR_NO(clk->flags) != SCLKA) {
		sel = (cpccr >> cpccr_clks[CLK_CPCCR_NO(clk->flags)].sel) & 0x3;
		if(cpccr_selector[sel] != 0) {
			clk->parent = get_clk_from_id(cpccr_selector[sel]);
			clk->flags |= CLK_FLG_ENABLE;
		}else {
			clk->parent = NULL;
			clk->flags &= ~CLK_FLG_ENABLE;
		}
	}
	clk->rate = cpccr_get_rate(clk);
	clk->ops = &clk_cpccr_ops;
}
