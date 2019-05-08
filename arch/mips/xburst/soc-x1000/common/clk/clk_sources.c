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
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>

#include <soc/cpm.h>
#include <soc/base.h>
#include <soc/extal.h>
#include "clk.h"
/*
 *     31 ... 24   GATE_ID or CPCCR_ID or CGU_ID or PLL_ID or CGU_ID.
 *     23 ... 16   PARENR_ID or RELATIVE_ID.
 *     16 ... 0    some FLG.
 */

static struct clk clk_srcs[] = {
#define GATE(x)  (((x)<<24) | CLK_FLG_GATE)
#define CPCCR(x) (((x)<<24) | CLK_FLG_CPCCR)
#define CGU(no)  (((no)<<24) | CLK_FLG_CGU)
#define CGU_AUDIO(no)  (((no)<<24) | CLK_FLG_CGU_AUDIO)
#define PLL(no)  (((no)<<24) | CLK_FLG_PLL)
#define PARENT(P)  (((CLK_ID_##P)<<16) | CLK_FLG_PARENT)
#define RELATIVE(P)  (((CLK_ID_##P)<<16) | CLK_FLG_RELATIVE)
#define DEF_CLK(N,FLAG)						\
	[CLK_ID_##N] = { .name = CLK_NAME_##N, .flags = FLAG, }

	DEF_CLK(EXT0,  		CLK_FLG_NOALLOC),
	DEF_CLK(EXT1,  		CLK_FLG_NOALLOC),
	DEF_CLK(OTGPHY,         CLK_FLG_NOALLOC),

	DEF_CLK(APLL,  		PLL(CPM_CPAPCR)),
	DEF_CLK(MPLL,  		PLL(CPM_CPMPCR)),

	DEF_CLK(SCLKA,		CPCCR(SCLKA)),
	DEF_CLK(CCLK,  		CPCCR(CDIV)),
	DEF_CLK(L2CLK,  	CPCCR(L2CDIV)),
	DEF_CLK(H0CLK,  	CPCCR(H0DIV)),
	DEF_CLK(H2CLK, 		CPCCR(H2DIV)),
	DEF_CLK(PCLK, 		CPCCR(PDIV)),

	DEF_CLK(NEMC,  		GATE(0) | PARENT(H2CLK)),
	DEF_CLK(EFUSE,  	GATE(1) | PARENT(H2CLK)),
	DEF_CLK(SFC,   		GATE(2) | PARENT(CGU_SFC)),
	DEF_CLK(OTG,   		GATE(3)),
	DEF_CLK(MSC0,  		GATE(4) | PARENT(PCLK)),
	DEF_CLK(MSC1,  		GATE(5) | PARENT(PCLK)),
	DEF_CLK(SCC,  		GATE(6) | PARENT(PCLK)),
	DEF_CLK(I2C0,  		GATE(7) | PARENT(PCLK)),
	DEF_CLK(I2C1,  		GATE(8) | PARENT(PCLK)),
	DEF_CLK(I2C2,  		GATE(9) | PARENT(PCLK)),
	DEF_CLK(I2C3,  		GATE(10) | PARENT(PCLK)),
	DEF_CLK(AIC,  		GATE(11)),
	DEF_CLK(VPU,  		GATE(12) | PARENT(LCD)),
	DEF_CLK(SADC,  		GATE(13)),
	DEF_CLK(UART0, 		GATE(14) | PARENT(EXT1)),
	DEF_CLK(UART1, 		GATE(15) | PARENT(EXT1)),
	DEF_CLK(UART2, 		GATE(16) | PARENT(EXT1)),
	DEF_CLK(DMIC,  		GATE(17)),
	DEF_CLK(TCU,  		GATE(18)),
	DEF_CLK(SSI,  		GATE(19)),
	DEF_CLK(SYS_OST,	GATE(20)),
	DEF_CLK(PDMA,  		GATE(21)),
	DEF_CLK(CIM,  		GATE(22) | PARENT(LCD)),
	DEF_CLK(LCD,  		GATE(23)),
	DEF_CLK(AES,  		GATE(24)),
	DEF_CLK(MAC,  		GATE(25)),
	DEF_CLK(PCM,  		GATE(26)),
	DEF_CLK(RTC,  		GATE(27)),
	DEF_CLK(APB0,  		GATE(28)),
	DEF_CLK(AHB0,  		GATE(29)),
	DEF_CLK(CPU,  		GATE(30)),
	DEF_CLK(DDR,  		GATE(31)),

	DEF_CLK(CGU_MSC_MUX,  	CGU(CGU_MSC_MUX)),
	DEF_CLK(CGU_PCM,	CGU_AUDIO(CGU_AUDIO_PCM)),
	DEF_CLK(CGU_CIM,	CGU(CGU_CIM)),
	DEF_CLK(CGU_SFC,	CGU(CGU_SFC)),
	DEF_CLK(CGU_USB,	CGU(CGU_USB)),
	DEF_CLK(CGU_MSC1,	CGU(CGU_MSC1)| PARENT(CGU_MSC_MUX)),
	DEF_CLK(CGU_MSC0,	CGU(CGU_MSC0)| PARENT(CGU_MSC_MUX)),
	DEF_CLK(CGU_LCD,	CGU(CGU_LCD)),
	DEF_CLK(CGU_I2S,	CGU_AUDIO(CGU_AUDIO_I2S)),
	DEF_CLK(CGU_MACPHY,	CGU(CGU_MACPHY)),
	DEF_CLK(CGU_DDR,	CGU(CGU_DDR)),
#undef GATE
#undef CPCCR
#undef CGU
#undef CGU_AUDIO
#undef PARENT
#undef DEF_CLK
#undef RELATIVE
};
int get_clk_sources_size(void){
	return ARRAY_SIZE(clk_srcs);
}
struct clk *get_clk_from_id(int clk_id)
{
	return &clk_srcs[clk_id];
}
int get_clk_id(struct clk *clk)
{
	return (clk - &clk_srcs[0]);
}

int dump_out_clk(char* str,DUMP_CALLBACK dump_callback)
{
	int i;
	int len = 0;
//	len += dump_callback(str + len,"ID NAME       FRE        stat       count     parent\n");
	for(i = 0; i < ARRAY_SIZE(clk_srcs); i++) {
		if (clk_srcs[i].name == NULL) {
			//len += dump_callback(str + len,"--------------------------------------------------------\n");
		} else {
			unsigned int mhz = clk_srcs[i].rate / 10000;
			len += dump_callback(str + len,"%2d %-10s %4d.%02dMHz %3sable   %d %s\n",i,clk_srcs[i].name
				    , mhz/100, mhz%100
				    , clk_srcs[i].flags & CLK_FLG_ENABLE? "en": "dis"
				    , clk_srcs[i].count
				    , clk_srcs[i].parent? clk_srcs[i].parent->name: "root");
		}
	}
	len += dump_callback(str + len,"CLKGR\t: %08x\n",cpm_inl(CPM_CLKGR));
	len += dump_callback(str + len,"LCR1\t: %08x\n",cpm_inl(CPM_LCR));
	return len;
}
static int dump_str(char *s, const char *fmt, ...)
{
	va_list args;
	int r;
	s = s;
	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);
	return r;
}
void dump_clk(void)
{
	dump_out_clk(0,dump_str);
}
