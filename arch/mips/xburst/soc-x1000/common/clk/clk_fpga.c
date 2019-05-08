/*
 * JZSOC Clock and Power Manager
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/proc_fs.h>

#include <soc/base.h>
#include <soc/extal.h>

#include "clk.h"

static int clk_srcs_size;
static int cgu_set_rate(struct clk *clk, unsigned long rate){
	if(strncmp(clk->name,"cgu_msc",7))
		if(clk){
			clk->rate = rate;
		}
	return clk->rate;
}
static int cgu_set_parent(struct clk *clk, struct clk *parent){
	if(clk&&parent)
		clk->parent = parent;
	return 0;
}

static struct clk_ops clk_cgu_ops = {
	.set_rate = cgu_set_rate,
	.set_parent = cgu_set_parent,
};


void __init init_ext_pll(struct clk *clk_srcs)
{
	int i;

	clk_srcs[CLK_ID_EXT0].rate = JZ_EXTAL_RTC;
	clk_srcs[CLK_ID_EXT0].flags |= CLK_FLG_ENABLE;
	clk_srcs[CLK_ID_EXT1].rate = JZ_EXTAL;
	clk_srcs[CLK_ID_EXT1].flags |= CLK_FLG_ENABLE;

	for(i=0; i<clk_srcs_size; i++) {
		if(! (clk_srcs[i].flags & CLK_FLG_PLL))
			continue;

		clk_srcs[i].flags |= CLK_FLG_ENABLE;
		clk_srcs[i].parent = &clk_srcs[CLK_ID_EXT1];
	}

	clk_srcs[CLK_ID_SCLKA].flags |= CLK_FLG_ENABLE;

	clk_srcs[CLK_ID_SCLKA].rate = 24000000;
	clk_srcs[CLK_ID_APLL].rate = 24000000;
	clk_srcs[CLK_ID_MPLL].rate = 800000000;
}

void __init init_cpccr_clk(struct clk *clk_srcs)
{
	int i;
	for(i=0; i<clk_srcs_size; i++) {
		if(! (clk_srcs[i].flags & CLK_FLG_CPCCR))
			continue;

		clk_srcs[i].rate = 24000000;
		clk_srcs[i].flags |= CLK_FLG_ENABLE;
	}
}

void __init init_cgu_clk(struct clk *clk_srcs)
{
	int i;

	for(i=0; i<clk_srcs_size; i++) {
		if(! (clk_srcs[i].flags & CLK_FLG_CGU))
			continue;
		if(clk_srcs[i].flags & CLK_FLG_PARENT) {
			int id = CLK_PARENT(clk_srcs[i].flags);
			clk_srcs[i].parent = &clk_srcs[id];
		}
		if(clk_srcs[i].parent)
			clk_srcs[i].rate = clk_srcs[i].parent->rate;
		clk_srcs[i].ops = &clk_cgu_ops;
	}
}

void __init init_gate_clk(struct clk *clk_srcs)
{
	int i;
	for(i=0; i<clk_srcs_size; i++) {
		if(! (clk_srcs[i].flags & CLK_FLG_GATE))
			continue;

		clk_srcs[i].flags |= CLK_FLG_ENABLE;

		if(!clk_srcs[i].rate && clk_srcs[i].parent)
			clk_srcs[i].rate = clk_srcs[i].parent->rate;
	}
}

int __init init_all_clk(void)
{
	int i;
	struct clk *clk_srcs = get_clk_from_id(0);
	clk_srcs_size = get_clk_sources_size();

	init_ext_pll(clk_srcs);
	init_cpccr_clk(clk_srcs);
	init_cgu_clk(clk_srcs);
	init_gate_clk(clk_srcs);
	for(i = 0; i < clk_srcs_size; i++) {
		if(clk_srcs[i].rate)
			continue;
		if(clk_srcs[i].flags & CLK_FLG_ENABLE)
			clk_srcs[i].count = 1;
		if(clk_srcs[i].flags & CLK_FLG_PARENT) {
			int id = CLK_PARENT(clk_srcs[i].flags);
			clk_srcs[i].parent = &clk_srcs[id];
		}
		if(! clk_srcs[i].parent) {
			clk_srcs[i].parent = &clk_srcs[CLK_ID_EXT0];
			printk(KERN_DEBUG "[CLK] %s no parent.\n",clk_srcs[i].name);
		}
		clk_srcs[i].rate = clk_srcs[i].parent->rate;
	}
	return 0;
}
arch_initcall(init_all_clk);

static int clk_gate_ctrl(struct clk *clk, int enable)
{
	return 0;
}

struct clk *clk_get(struct device *dev, const char *id)
{
	int i;
	struct clk *retval = NULL;
	struct clk *clk_srcs = get_clk_from_id(0);
	struct clk *parent_clk = NULL;
	for(i = 0; i < get_clk_sources_size(); i++) {
		if(id && clk_srcs[i].name && !strcmp(id,clk_srcs[i].name)) {
			if(clk_srcs[i].flags & CLK_FLG_NOALLOC)
				return &clk_srcs[i];
			retval = kzalloc(sizeof(struct clk),GFP_KERNEL);
			if(!retval)
				return ERR_PTR(-ENODEV);
			memcpy(retval,&clk_srcs[i],sizeof(struct clk));
			retval->source = &clk_srcs[i];
			if(CLK_FLG_RELATIVE & clk_srcs[i].flags)
			{
				parent_clk = get_clk_from_id(CLK_RELATIVE(clk_srcs[i].flags));
				parent_clk->child = NULL;
			}
			retval->count = 0;
			return retval;
		}
	}
	return ERR_PTR(-EINVAL);
}
EXPORT_SYMBOL(clk_get);

int clk_enable(struct clk *clk)
{
	*(volatile unsigned int*)(0xB0000020) = 0;
	return 0;
	if(!clk)
		return -EINVAL;

	clk->count++;

	if(clk->flags & CLK_FLG_ENABLE)
		return 0;

	clk_enable(clk->parent);

	if(clk->flags & CLK_FLG_GATE)
		clk_gate_ctrl(clk,1);

	if(clk->ops && clk->ops->enable)
		clk->ops->enable(clk,1);

	clk->count = 1;
	clk->flags |= CLK_FLG_ENABLE;

	return 0;
}
EXPORT_SYMBOL(clk_enable);

int clk_is_enabled(struct clk *clk)
{
	return !!(clk->flags & CLK_FLG_ENABLE);

}
EXPORT_SYMBOL(clk_is_enabled);

void clk_disable(struct clk *clk)
{
	if(!clk)
		return;

	if(clk->count > 1) {
		clk->count--;
		return;
	}

	if(clk->flags & CLK_FLG_GATE)
		clk_gate_ctrl(clk,0);

	if(clk->ops && clk->ops->enable)
		clk->ops->enable(clk,0);

	clk->count = 0;
	clk->flags &= ~CLK_FLG_ENABLE;

	clk_disable(clk->parent);
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	return clk? clk->rate: 0;
}
EXPORT_SYMBOL(clk_get_rate);

void clk_put(struct clk *clk)
{
	if(clk)
		clk->flags &= ~CLK_FLG_NOALLOC;
	return;
}
EXPORT_SYMBOL(clk_put);


/*
 * The remaining APIs are optional for machine class support.
 */
long clk_round_rate(struct clk *clk, unsigned long rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	if (!clk || !clk->ops || !clk->ops->set_rate)
		return -EINVAL;
	clk->rate = clk->ops->set_rate(clk, rate);

	return 0;
}
EXPORT_SYMBOL(clk_set_rate);

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int err;

	if (!clk || !clk->ops || !clk->ops->set_parent)
		return -EINVAL;

	err = clk->ops->set_parent(clk, parent);

	return err;
}
EXPORT_SYMBOL(clk_set_parent);

struct clk *clk_get_parent(struct clk *clk)
{
	return clk? clk->parent: NULL;
}
EXPORT_SYMBOL(clk_get_parent);
