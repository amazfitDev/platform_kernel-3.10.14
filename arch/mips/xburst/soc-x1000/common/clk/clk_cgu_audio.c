#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <soc/cache.h>
#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include <soc/ddr.h>
#include "clk.h"

static int audio_div_apll[64];
static int audio_div_mpll[64];
static DEFINE_SPINLOCK(cpm_cgu_lock);
struct clk_selectors {
	unsigned int route[4];
};
enum {
	SELECTOR_AUDIO = 0,
};
const struct clk_selectors audio_selector[] = {
#define CLK(X)  CLK_ID_##X
/*
 *         bit31,bit30
 *          0   , 0       EXT1
 *          0   , 1       APLL
 *          1   , 0       EXT1
 *          1   , 1       MPLL
 */
	[SELECTOR_AUDIO].route = {CLK(EXT1),CLK(SCLKA),CLK(EXT1),CLK(MPLL)},
#undef CLK
};

struct cgu_clk {
	int off,en,maskm,bitm,maskn,bitn,maskd,bitd,sel,cache;
};
static struct cgu_clk cgu_clks[] = {
	[CGU_AUDIO_I2S] = 	{ CPM_I2SCDR, 1<<29, 0x1f << 13, 13, 0x1fff, 0, SELECTOR_AUDIO},
	[CGU_AUDIO_I2S1] = 	{ CPM_I2SCDR1, -1, -1, -1, -1, -1, -1},
	[CGU_AUDIO_PCM] = 	{ CPM_PCMCDR, 1<<29, 0x1f << 13, 13, 0x1fff, 0, SELECTOR_AUDIO},
	[CGU_AUDIO_PCM1] = 	{ CPM_PCMCDR1, -1, -1, -1, -1, -1, -1},
};

static unsigned long cgu_audio_get_rate(struct clk *clk)
{
	unsigned long m,n,d;
	unsigned long flags;
	int no = CLK_CGU_AUDIO_NO(clk->flags);

	if(clk->parent == get_clk_from_id(CLK_ID_EXT1))
		return clk->parent->rate;

	spin_lock_irqsave(&cpm_cgu_lock,flags);
	m = cpm_inl(cgu_clks[no].off);
	n = m&cgu_clks[no].maskn;
	m &= cgu_clks[no].maskm;
	if(no == CGU_AUDIO_I2S){
		d = inl(I2S_PRI_DIV);
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return (clk->parent->rate*m)/(n*((d&0x3f) + 1)*(64));
	}else if (no == CGU_AUDIO_PCM){
		d = inl(PCM_PRI_DIV);
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return (clk->parent->rate*m)/(n*(((d&0x1f<<6)>>6)+1)*8);
	}
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return -EINVAL;
}
static int cgu_audio_enable(struct clk *clk,int on)
{
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	int reg_val;
	unsigned long flags;
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	if(on){
		reg_val = cpm_inl(cgu_clks[no].off);
		if(reg_val & (cgu_clks[no].en))
			goto cgu_enable_finish;
		if(!cgu_clks[no].cache)
			printk("must set rate before enable\n");
		cpm_outl(cgu_clks[no].cache, cgu_clks[no].off);
		cpm_outl(cgu_clks[no].cache | cgu_clks[no].en, cgu_clks[no].off);
		cgu_clks[no].cache = 0;
	}else{
		reg_val = cpm_inl(cgu_clks[no].off);
		reg_val &= ~cgu_clks[no].en;
		cpm_outl(reg_val,cgu_clks[no].off);
	}
cgu_enable_finish:
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return 0;
}

static int get_div_val(int max1,int max2,int machval, int* res1, int* res2){
	int tmp1=0,tmp2=0;
	for (tmp1=1; tmp1<max1; tmp1++)
		for(tmp2=1; tmp2<max2; tmp2++)
			if(tmp1*tmp2 == machval)
				break;
	if(tmp1*tmp2 != machval){
		printk("can't find mach wal\n");
		return -1;
	}
	*res1 = tmp1;
	*res2 = tmp2;
	return 0;
}
static int cgu_audio_calculate_set_rate(struct clk* clk, unsigned long rate, unsigned int pid){
	int i,m,n,d,sync,tmp_val,d_max,sync_max;
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	int n_max = cgu_clks[no].maskn >> cgu_clks[no].bitn;
	int *audio_div;
	unsigned long flags;
	if(pid == CLK_ID_MPLL){
		audio_div = (int*)audio_div_mpll;
	}
	else if(pid == CLK_ID_SCLKA)
		audio_div = (int*)audio_div_apll;
	else
		return 0;

	for(i=0;i<50;i+=3){
		if(audio_div[i] == rate)
			break;
	}
	if(i >= 50){
		printk("cgu aduio set rate err!\n");
		return -EINVAL;
	}else{
		m = audio_div[i+1];
		if(no == CGU_AUDIO_I2S){
#ifdef CONFIG_SND_ASOC_JZ_AIC_SPDIF_V13
			m*=2;
#endif
			d_max = 0x1ff;
			tmp_val = audio_div[i+2]/64;
			if(tmp_val > n_max){
				if(get_div_val(n_max,d_max,tmp_val,&n,&d))
					goto calculate_err;
			}else{
				n = tmp_val;
				d = 1;
			}
			spin_lock_irqsave(&cpm_cgu_lock,flags);
			tmp_val = cpm_inl(cgu_clks[no].off)&(~(cgu_clks[no].maskm|cgu_clks[no].maskn));
			tmp_val |= (m<<cgu_clks[no].bitm)|(n<<cgu_clks[no].bitn);
			if(tmp_val&cgu_clks[no].en){
				cpm_outl(tmp_val,cgu_clks[no].off);
			}else{
				cgu_clks[no].cache = tmp_val;
			}
			writel(d-1,(volatile unsigned int*)I2S_PRI_DIV);
			spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		}else if (no == CGU_AUDIO_PCM){
			tmp_val = audio_div[i+2]/(8);
			d_max = 0x7f;
			if(tmp_val > n_max){
				if(get_div_val(n_max,d_max,tmp_val,&n,&d))
					goto calculate_err;
				if(d > 0x3f){
					tmp_val = d;
					d_max = 0x3f, sync_max = 0x1f;
					if(get_div_val(d_max,sync_max,tmp_val,&d,&sync))
						goto calculate_err;
				}else{
					sync = 1;
				}
			}else{
				n = tmp_val;
				d = 1;
				sync = 1;
			}
			spin_lock_irqsave(&cpm_cgu_lock,flags);
			tmp_val = cpm_inl(cgu_clks[no].off)&(~(cgu_clks[no].maskm|cgu_clks[no].maskn));
			tmp_val |= (m<<cgu_clks[no].bitm)|(n<<cgu_clks[no].bitn);
			if(tmp_val&cgu_clks[no].en){
				cpm_outl(tmp_val,cgu_clks[no].off);
			}else{
				cgu_clks[no].cache = tmp_val;
			}
			writel(((d-1)|(sync-1)<<6),(volatile unsigned int*)PCM_PRI_DIV);
			spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		}
	}
	clk->rate = rate;
	return 0;
calculate_err:
		printk("audio div Calculate err!\n");
	return -EINVAL;
}

static struct clk* cgu_audio_get_parent(struct clk *clk)
{
	unsigned int no,cgu,idx,pidx;
	unsigned long flags;
	struct clk* pclk;

	spin_lock_irqsave(&cpm_cgu_lock,flags);
	no = CLK_CGU_AUDIO_NO(clk->flags);
	cgu = cpm_inl(cgu_clks[no].off);
	idx = cgu >> 30;
	pidx = audio_selector[cgu_clks[no].sel].route[idx];
	if (pidx == CLK_ID_STOP || pidx == CLK_ID_INVALID){
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return NULL;
	}
	pclk = get_clk_from_id(pidx);
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);

	return pclk;
}

static int cgu_audio_set_parent(struct clk *clk, struct clk *parent)
{
	int tmp_val,i;
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	unsigned long flags;
	for(i = 0;i < 4;i++) {
		if(audio_selector[cgu_clks[no].sel].route[i] == get_clk_id(parent)){
			break;
		}
	}

	if(i >= 4)
		return -EINVAL;
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	if(get_clk_id(parent) != CLK_ID_EXT1){
		tmp_val = cpm_inl(cgu_clks[no].off)&(~(3<<30));
		tmp_val |= i<<30;
		cpm_outl(tmp_val,cgu_clks[no].off);
	}else{
		tmp_val = cpm_inl(cgu_clks[no].off)&(~(3<<30|0x3fffff));
		tmp_val |= i<<30|1<<13|1;
		cpm_outl(tmp_val,cgu_clks[no].off);
	}
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);

	return 0;
}



static int cgu_audio_set_rate(struct clk *clk, unsigned long rate)
{
	int tmp_val;
	unsigned long flags;
	int no = CLK_CGU_AUDIO_NO(clk->flags);
	if(rate == 24000000){
		cgu_audio_set_parent(clk,get_clk_from_id(CLK_ID_EXT1));
		clk->parent = get_clk_from_id(CLK_ID_EXT1);
		clk->rate = rate;
		spin_lock_irqsave(&cpm_cgu_lock,flags);
		tmp_val = cpm_inl(cgu_clks[no].off);
		tmp_val &= ~0x3fffff;
		tmp_val |= 1<<13|1;
		if(tmp_val&cgu_clks[no].en)
			cpm_outl(tmp_val,cgu_clks[no].off);
		else
			cgu_clks[no].cache = tmp_val;
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
		return 0;
	}else{
		cgu_audio_calculate_set_rate(clk,rate,CLK_ID_MPLL);
		if(get_clk_id(clk->parent) == CLK_ID_EXT1)
			cgu_audio_set_parent(clk,get_clk_from_id(CLK_ID_MPLL));
		clk->parent = get_clk_from_id(CLK_ID_MPLL);
	}
	return 0;
}


static int cgu_audio_is_enabled(struct clk *clk) {
	int no,state;
	unsigned long flags;
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	no = CLK_CGU_AUDIO_NO(clk->flags);
	state = (cpm_inl(cgu_clks[no].off) & cgu_clks[no].en);
	spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	return state;
}

static struct clk_ops clk_cgu_audio_ops = {
	.enable	= cgu_audio_enable,
	.get_rate = cgu_audio_get_rate,
	.set_rate = cgu_audio_set_rate,
	.get_parent = cgu_audio_get_parent,
	.set_parent = cgu_audio_set_parent,
};
void __init init_cgu_audio_clk(struct clk *clk)
{
	int no,id,tmp_val;
	unsigned long flags;
	memcpy(audio_div_apll,(void*)(0xf4000000),256);
	memcpy(audio_div_mpll,(void*)(0xf4000000)+256,256);

	if (clk->flags & CLK_FLG_PARENT) {
		id = CLK_PARENT(clk->flags);
		clk->parent = get_clk_from_id(id);
	} else {
		clk->parent = cgu_audio_get_parent(clk);
	}
	no = CLK_CGU_AUDIO_NO(clk->flags);
	cgu_clks[no].cache = 0;
	if(cgu_audio_is_enabled(clk)) {
		clk->flags |= CLK_FLG_ENABLE;
	}
	clk->rate = cgu_audio_get_rate(clk);
	spin_lock_irqsave(&cpm_cgu_lock,flags);
	tmp_val = cpm_inl(cgu_clks[no].off);
	tmp_val &= ~0x3fffff;
	tmp_val |= 1<<13|1;
	if((tmp_val&cgu_clks[no].en)&&(clk->rate == 24000000))
		cpm_outl(tmp_val,cgu_clks[no].off);
	else
		cgu_clks[no].cache = tmp_val;
		spin_unlock_irqrestore(&cpm_cgu_lock,flags);
	clk->ops = &clk_cgu_audio_ops;
}
